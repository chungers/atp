
#include <boost/lexical_cast.hpp>
#include <boost/shared_ptr.hpp>
#include <zmq.hpp>

#include "log_levels.h"
#include "utils.hpp"
#include "zmq/ZmqUtils.hpp"

#include "historian/DbReactorStrategy.hpp"
#include "historian/Visitor.hpp"


namespace historian {

using std::string;
using zmq::context_t;
using zmq::socket_t;
using atp::zmq::Reactor;

using proto::ib::MarketData;
using proto::ib::MarketDepth;
using proto::historian::Record;
using proto::historian::Record_Type;
using proto::historian::SessionLog;
using proto::historian::Query;
using proto::historian::Query_Type;
using proto::historian::QueryByRange;
using proto::historian::QueryBySymbol;
using proto::historian::QuerySessionLogs;

template <typename Q>
class DbVisitor : public historian::Visitor
{
 public:
  DbVisitor(const Q& query, socket_t& socket) :
      query_(query), socket_(socket) {}
  ~DbVisitor() {}

  bool operator()(const SessionLog& log)
  {
    return send(log) > 0;
  }

  bool operator()(const MarketData& data)
  {
    if (query_.has_filter() && data.event() == query_.filter()) {
      return send(data) > 0;
    }
    return send(data) > 0;
  }

  bool operator()(const MarketDepth& data)
  {
    return send(data) > 0;
  }

 private:

  void set(Record* record, const MarketData& data)
  {
    record->set_type(proto::historian::Record_Type_IB_MARKET_DATA);
    record->mutable_ib_marketdata()->CopyFrom(data);
  }
  void set(Record* record, const MarketDepth& data)
  {
    record->set_type(proto::historian::Record_Type_IB_MARKET_DEPTH);
    record->mutable_ib_marketdepth()->CopyFrom(data);
  }
  void set(Record* record, const SessionLog& data)
  {
    record->set_type(proto::historian::Record_Type_SESSION_LOG);
    record->mutable_session_log()->CopyFrom(data);
  }

  template <typename T> size_t send(const T& message)
  {
    Record record;
    set(&record, message);

    string recordProto;
    if (!record.SerializeToString(&recordProto)) {
      HISTORIAN_REACTOR_ERROR << "Error serializing " << &record;
      return false;
    }

    try {

      size_t sent = atp::zmq::send_copy(socket_, recordProto, false);
      return sent;
    } catch (zmq::error_t e) {
      HISTORIAN_REACTOR_ERROR << "Exception while sending: " << e.what();
    }
    return 0;
  }

 private:
  const Q& query_;
  socket_t& socket_;
};


DbReactorStrategy::DbReactorStrategy(const string& leveldbFile) :
    db_(new Db(leveldbFile)),
    context_(new context_t(1))
{
  HISTORIAN_REACTOR_LOGGER << "Started on " << leveldbFile;
}

DbReactorStrategy::~DbReactorStrategy()
{
}

void replyError(socket_t& socket, const string& message)
{
  // send two frames.
  HISTORIAN_REACTOR_ERROR << "Error: " << message;
  try {
    atp::zmq::send_copy(socket, boost::lexical_cast<string>(500), true);
    atp::zmq::send_copy(socket, message, true);
  } catch (zmq::error_t e) {
    HISTORIAN_REACTOR_ERROR << "Exception while reply with error: " << e.what();
  }
}

bool replyDataReady(socket_t& socket)
{
  // send two frames.
  try {
    atp::zmq::send_copy(socket, boost::lexical_cast<string>(200), true);
    atp::zmq::send_copy(socket, "OK", false);

    HISTORIAN_REACTOR_DEBUG << "Sent data ready.";

    return true;
  } catch (zmq::error_t e) {
    HISTORIAN_REACTOR_ERROR << "Exception while reply with ready: " << e.what();
  }
  return false;
}

int handleQueryByRange(const boost::shared_ptr<Db> db,
                          const QueryByRange& q,
                          socket_t& socket, string* message)
{
  HISTORIAN_REACTOR_DEBUG << "QueryByRange ["
                          << q.first() << ", " << q.last() << ")";

  DbVisitor<QueryByRange> visitor(q, socket);
  return db->query(q, &visitor);
}

int handleQueryBySymbol(const boost::shared_ptr<Db> db,
                           const QueryBySymbol& q,
                           socket_t& socket, string* message)
{
  HISTORIAN_REACTOR_DEBUG << "QueryBySymbol @"
                          << q.symbol()
                          << "[" << q.utc_first_micros()
                          << ", " << q.utc_last_micros()
                          << ")";
  DbVisitor<QueryBySymbol> visitor(q, socket);
  return db->query(q, &visitor);
  return 0;
}

int handleQuerySessionLogs(const boost::shared_ptr<Db> db,
                              const QuerySessionLogs& q,
                              socket_t& socket, string* message)
{
  HISTORIAN_REACTOR_DEBUG << "QuerySessionLogs @"
                          << q.symbol();
  return 0;
}

bool DbReactorStrategy::respond(socket_t& socket)
{
  try {
    while (1) {
      int more = 1;
      string frame1;
      if (more) more = atp::zmq::receive(socket, &frame1);
      while (more) {
        string buffer;
        more = atp::zmq::receive(socket, &buffer);
        HISTORIAN_REACTOR_ERROR << "Extra frames: " << buffer;
      }

      // Now process the input request:
      boost::scoped_ptr<socket_t> callback;

      Query query;
      if (query.ParseFromString(frame1)) {

        HISTORIAN_REACTOR_DEBUG << "Callback = " << query.callback();

        if (replyDataReady(socket)) {

          callback.reset(new socket_t(*context_, ZMQ_PUSH));
          try {
            callback->connect(query.callback().c_str());

            HISTORIAN_REACTOR_DEBUG << "Connected to callback.";

          } catch (zmq::error_t e) {
            HISTORIAN_REACTOR_ERROR << "Error trying to connect callback: "
                                    << query.callback() << ", " << e.what();
          }
        }
      }

      // TODO need to start a thread.
      int count = 0;
      boost::uint64_t start = now_micros();

      if (callback.get() != NULL) {
        Query_Type type = query.type();
        string error;

        switch (type) {

          using namespace proto::historian;

          case Query_Type_QUERY_BY_RANGE :
            if ((count = handleQueryByRange(db_, query.query_by_range(),
                                            *callback, &error)) < 0) {
              replyError(*callback, error);
            }
            break;
          case Query_Type_QUERY_BY_SYMBOL :
            if ((count = handleQueryBySymbol(db_, query.query_by_symbol(),
                                             *callback, &error)) < 0) {
              replyError(*callback, error);
            }
            break;
          case Query_Type_QUERY_SESSION_LOGS :
            if ((count = handleQuerySessionLogs(db_, query.query_session_logs(),
                                                *callback, &error)) < 0) {
              replyError(*callback, error);
            }
            break;
        }

        boost::uint64_t elapsed = now_micros() - start;

        // finally send a terminating frame
        HISTORIAN_REACTOR_DEBUG << "Finished query: " << count << " records in "
                                << static_cast<double>(elapsed) / 1000000.
                                << " seconds."
                                << " Sending complete command.";
        string empty("COMPLETED");
        atp::zmq::send_copy(*callback, empty, false);
      }

    }
  } catch (zmq::error_t e) {

  }
  return true;
}

int DbReactorStrategy::socketType()
{
  return ZMQ_REP;
}

bool DbReactorStrategy::OpenDb()
{
  return db_->open();
}

} // historian
