
#include <boost/lexical_cast.hpp>
#include <boost/scoped_ptr.hpp>

#include "log_levels.h"
#include "utils.hpp"
#include "zmq/ZmqUtils.hpp"
#include "proto/ib.pb.h"

#include "historian/DbReactorClient.hpp"

using std::string;
using boost::uint64_t;

using zmq::context_t;
using zmq::socket_t;

using proto::ib::MarketData;
using proto::ib::MarketDepth;
using proto::historian::SessionLog;
using proto::historian::Record;
using proto::historian::Type;
using proto::historian::Query;
using proto::historian::Query_Type;
using proto::historian::QueryByRange;
using proto::historian::QueryBySymbol;


namespace historian {

namespace internal {

inline void set(Query* query, const QueryByRange& q)
{
  query->set_type(proto::historian::Query_Type_QUERY_BY_RANGE);
  query->mutable_query_by_range()->CopyFrom(q);
}

inline void set(Query* query, const QueryBySymbol& q)
{
  query->set_type(proto::historian::Query_Type_QUERY_BY_SYMBOL);
  query->mutable_query_by_symbol()->CopyFrom(q);
}

template <typename T>
size_t send(Query_Type type, const T& q,
            const boost::scoped_ptr<socket_t>& socket, const string& callback)
{
  Query envelope;
  envelope.set_callback(callback);
  set(&envelope, q);
  string envelopeProto;
  if (!envelope.IsInitialized() ||
      !envelope.SerializeToString(&envelopeProto)) {
    HISTORIAN_REACTOR_ERROR << "Cannot serialize " << type;
    return 0;
  }

  try {

    size_t sent = atp::zmq::send_copy(*socket, envelopeProto, false);
    return sent;

  } catch (zmq::error_t e) {
    HISTORIAN_REACTOR_ERROR << "Error sending request: " << e.what();
  }
  return 0;
}
} // internal



DbReactorClient::DbReactorClient(const string& addr, const string& cbAddr,
                                 context_t* context) :
    endpoint_(addr),
    callbackEndpoint_(cbAddr),
    socket_(new socket_t(*context, ZMQ_REQ)),
    callbackSocket_(new socket_t(*context, ZMQ_PULL))
{
  HISTORIAN_REACTOR_DEBUG << "Server socket=" << socket_.get()
                          << ", Callback socket=" << callbackSocket_.get();
}


DbReactorClient::~DbReactorClient()
{
}

bool DbReactorClient::connect()
{
  // Start the callback
  try {

    HISTORIAN_REACTOR_DEBUG << "Callback listening at " << callbackEndpoint_
                            << " on " << callbackSocket_.get();
    callbackSocket_->bind(callbackEndpoint_.c_str());
  } catch (zmq::error_t e) {
    HISTORIAN_REACTOR_ERROR << "Error while binding for callback: " << e.what();
    return false;
  }

  // Now connect to server
  try {

    HISTORIAN_REACTOR_DEBUG << "Connecting to server at " << endpoint_
                            << " on " << socket_.get();
    socket_->connect(endpoint_.c_str());
    return true;
  } catch (zmq::error_t e) {
    HISTORIAN_REACTOR_ERROR << "Error while connecting to server: " << e.what();
    return false;
  }
}

uint64_t processQueryResponse(const boost::scoped_ptr<socket_t>& socket,
                              string* error)
{
  string frame1, frame2;

  try {
    size_t more = atp::zmq::receive(*socket, &frame1);
    if (more) more = atp::zmq::receive(*socket, &frame2);

    HISTORIAN_REACTOR_DEBUG << "Received from server: "
                            << frame1 << "," << frame2;

    int status = boost::lexical_cast<int>(frame1);
    uint64_t responseId = boost::lexical_cast<uint64_t>(frame2);

    if (status == 200) {
      LOG(INFO) << "Response id " << responseId;
      return responseId;
    } else {
      *error = frame2;
      return 0;
    }

  } catch (zmq::error_t e) {
    HISTORIAN_REACTOR_ERROR << "Exception from socket: " << e.what();
  }
  return 0;
}

int processCallback(const uint64_t& responseId,
                    const boost::scoped_ptr<socket_t>& socket, Visitor* visit)
{
  int count = 0;
  while (true) {

    // frame1 = responseId, frame2 = proto
    string frame1, frame2;

    try {

      int more = atp::zmq::receive(*socket, &frame1);
      if (more) more = atp::zmq::receive(*socket, &frame2);

      uint64_t receivedId = boost::lexical_cast<uint64_t>(frame1);
      if (responseId != receivedId) {
        HISTORIAN_REACTOR_ERROR << "Bad response id " << receivedId;
        return 0; // TODO - need t implement multiplexing.
      }

    } catch (zmq::error_t e) {
      HISTORIAN_REACTOR_ERROR << "Error receiving: " << e.what();
      break;
    }

    // Deserialize to Record;
    Record record;
    if (record.ParseFromString(frame2)) {
      if ((*visit)(record)) {
        count++;
      }
    } else if (200 == boost::lexical_cast<int>(frame2)) {
      LOG(INFO) << "Response stream " << responseId << " completed.";
      break;
    } else {
      HISTORIAN_REACTOR_DEBUG << "Not a Record. Received: " << frame1;
      break;
    }
  }
  return count;
}

int DbReactorClient::Query(const QueryByRange& query, Visitor* visitor)
{
  using namespace historian::internal;
  if (send(proto::historian::Query_Type_QUERY_BY_RANGE, query, socket_,
           callbackEndpoint_) > 0) {
    // now wait for the reply and check to see if we should listen on
    // the callback socket.
    string message;
    uint64_t responseId = processQueryResponse(socket_, &message);
    if (responseId > 0) {
      return processCallback(responseId, callbackSocket_, visitor);
    } else {
      HISTORIAN_REACTOR_ERROR << "Error from server: " << message;
    }
  }
  return 0;
}

int DbReactorClient::Query(const QueryBySymbol& query, Visitor* visitor)
{
  using namespace historian::internal;
  if (send(proto::historian::Query_Type_QUERY_BY_SYMBOL, query, socket_,
           callbackEndpoint_) > 0) {
    // now wait for the reply and check to see if we should listen on
    // the callback socket.
    string message;
    uint64_t responseId = processQueryResponse(socket_, &message);
    if (responseId > 0) {
      return processCallback(responseId, callbackSocket_, visitor);
    } else {
      HISTORIAN_REACTOR_ERROR << "Error from server: " << message;
    }
  }
  return 0;
}
} // historian



