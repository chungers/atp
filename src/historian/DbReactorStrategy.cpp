
#include <iostream>
#include <boost/lexical_cast.hpp>
#include <boost/scoped_ptr.hpp>
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
using boost::shared_ptr;
using boost::uint64_t;
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
  DbVisitor(socket_t& socket, uint64_t responseId) :
      socket_(socket), responseId_(boost::lexical_cast<string>(responseId)) {}
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
      size_t sent = atp::zmq::send_copy(socket_, responseId_, true);
      sent += atp::zmq::send_copy(socket_, recordProto, false);
      return sent;
    } catch (zmq::error_t e) {
      HISTORIAN_REACTOR_ERROR << "Exception while sending: " << e.what();
    }
    return 0;
  }

 private:
  socket_t& socket_;
  string responseId_;
};


DbReactorStrategy::DbReactorStrategy(const shared_ptr<historian::Db>& db) :
    db_(db), context_(new context_t(1))
{
  HISTORIAN_REACTOR_LOGGER << "Started on " << db_->GetDbPath();
}

DbReactorStrategy::~DbReactorStrategy()
{
}

bool replyDataReady(socket_t& socket, uint64_t responseId)
{
  // send two frames.
  try {
    atp::zmq::send_copy(socket, boost::lexical_cast<string>(200), true);
    atp::zmq::send_copy(socket, boost::lexical_cast<string>(responseId), false);
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
inline int handleQuery(const uint64_t responseId,
                       const boost::shared_ptr<Db>& db,
                       const Q& q, socket_t& socket)
{
  DbVisitor visitor(socket, responseId);
  return db->Query(q, &visitor);
}

template int handleQuery<QueryByRange>(const uint64_t responseId,
                                       const boost::shared_ptr<Db>&,
                                       const QueryByRange&, socket_t&);

template int handleQuery<QueryBySymbol>(const uint64_t responseId,
                                        const boost::shared_ptr<Db>&,
                                        const QueryBySymbol&, socket_t&);


class CallbackStreamer
{
 public:
  CallbackStreamer(uint64_t responseId,
                   const boost::shared_ptr<historian::Db>& db,
                   const Query& query,
                   const boost::shared_ptr<zmq::context_t>& context) :
      responseId_(responseId),
      db_(db),
      query_(query),
      connected_(false),
      context_(context)
  {
  }

  ~CallbackStreamer() {}

  /// Stream the data back to the callback socket.
  size_t operator()()
  {
    try {
      callback_.reset(new socket_t(*context_, ZMQ_PUSH));
      callback_->connect(query_.callback().c_str());
      connected_ = true;
      HISTORIAN_REACTOR_DEBUG << "Connected to callback.";
    } catch (zmq::error_t e) {
      HISTORIAN_REACTOR_ERROR << "Error trying to connect callback: "
                              << query_.callback() << ", " << e.what();
    }

    if (!connected_) return 0;
    size_t count = 0;
    uint64_t start = now_micros();

    string reqId = boost::lexical_cast<string>(responseId_);

    Query_Type type = query_.type();
    switch (type) {

      using namespace proto::historian;


      case Query_Type_QUERY_BY_RANGE :
        count = handleQuery<QueryByRange>
            (responseId_, db_, query_.query_by_range(), *callback_);
        break;


      case Query_Type_QUERY_BY_SYMBOL :
        count = handleQuery<QueryBySymbol>
            (responseId_, db_, query_.query_by_symbol(), *callback_);
        break;

    }
    uint64_t elapsed = now_micros() - start;

    // finally send a terminating frame
    HISTORIAN_REACTOR_DEBUG << "Finished query: " << count << " records in "
                            << static_cast<double>(elapsed) / 1000000.
                            << " seconds."
                            << " Sending complete command.";

    // Send two frames:
    atp::zmq::send_copy(*callback_, reqId, true);
    atp::zmq::send_copy(*callback_, boost::lexical_cast<string>(200), false);
    return count;
  }

 private:
  uint64_t responseId_;
  boost::shared_ptr<historian::Db> db_;
  Query query_;
  bool connected_;
  boost::shared_ptr<zmq::context_t> context_;
  boost::scoped_ptr<socket_t> callback_;
};

bool DbReactorStrategy::respond(socket_t& socket)
{
  try {

    int more = 1;
    string frame1;
    if (more) more = atp::zmq::receive(socket, &frame1);
    while (more) {
      string buffer;
      more = atp::zmq::receive(socket, &buffer);
      HISTORIAN_REACTOR_ERROR << "Extra frames: " << buffer;
    }

    // Process the query:
    Query query;
    if (!query.ParseFromString(frame1)) {
      LOG(ERROR) << "Cannot parse query: " << frame1;
      return false;
    }

    // Assign responseId:  make this small to reduce bytes transferred.
    uint64_t responseId = now_micros() % 1000000LL;
    if (replyDataReady(socket, responseId)) {

      CallbackStreamer doCallback(responseId, db_, query, context_);
      doCallback();
    }

  } catch (zmq::error_t e) {
    HISTORIAN_REACTOR_ERROR << "Exception from socket: " << e.what();
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
