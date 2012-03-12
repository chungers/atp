
#include <iostream>
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
using std::ostream;
using zmq::context_t;
using zmq::socket_t;
using atp::zmq::Reactor;

using proto::ib::MarketData;
using proto::ib::MarketDepth;
using proto::historian::Record;
using proto::historian::Type;
using proto::historian::SessionLog;
using proto::historian::Query;
using proto::historian::Query_Type;
using proto::historian::QueryByRange;
using proto::historian::QueryBySymbol;


class DbVisitor : public historian::Visitor
{
 public:
  DbVisitor(socket_t& socket) : socket_(socket) {}
  ~DbVisitor() {}

  bool operator()(const Record& record)
  {
    return send(record) > 0;
  }


 private:

  size_t send(const Record& record)
  {
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

ostream& operator<<(ostream& out, const QueryByRange& q)
{
  out << "QueryByRange<"
      << q.type() << ">["
      << q.first() << ", " << q.last() << ")"
      << (q.has_index() ?
          ", index=" + q.index() : "");
  return out;
}

ostream& operator<<(ostream& out, const QueryBySymbol& q)
{
  out << "QueryBySymbol<"
      << q.type() << ">@"
      << q.symbol()
      << "[" << q.utc_first_micros()
      << ", " << q.utc_last_micros()
      << ")"
      << (q.has_index() ?
          ", index=" + q.index() : "");
  return out;
}

template <typename Q>
inline int handleQuery(const boost::shared_ptr<Db> db,
                       const Q& q,
                       socket_t& socket, string* message)
{
  DbVisitor visitor(socket);
  return db->Query(q, &visitor);
}

template int handleQuery<QueryByRange>(const boost::shared_ptr<Db>,
                                       const QueryByRange&,
                                       socket_t&, string*);
template int handleQuery<QueryBySymbol>(const boost::shared_ptr<Db>,
                                       const QueryBySymbol&,
                                       socket_t&, string*);



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
      } else {
        LOG(ERROR) << "Cannot parse query: " << frame1;
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
            if ((count = handleQuery<QueryByRange>(
                    db_, query.query_by_range(),
                    *callback, &error)) < 0) {
              replyError(*callback, error);
            }
            break;
          case Query_Type_QUERY_BY_SYMBOL :
            if ((count = handleQuery<QueryBySymbol>(
                    db_, query.query_by_symbol(),
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
  return db_->Open();
}

} // historian
