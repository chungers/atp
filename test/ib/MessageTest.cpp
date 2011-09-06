#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <glog/logging.h>
#include <gtest/gtest.h>
#include <zmq.hpp>

#include "utils.hpp"
#include "common.hpp"
#include "ib/IBAPIFields.hpp"
#include "ib/IBAPIValues.hpp"
#include "ib/Message.hpp"

using FIX::FieldMap;

class MarketDataRequest : public IBAPI::Message
{
 public:
  MarketDataRequest() : Message("MarketDataRequest")
  {
  }

  FIELD_SET(*this, IBAPI::Symbol);
  FIELD_SET(*this, IBAPI::SecurityID);
  FIELD_SET(*this, IBAPI::SecurityType);
  FIELD_SET(*this, IBAPI::PutOrCall);
  FIELD_SET(*this, IBAPI::StrikePrice);
  FIELD_SET(*this, IBAPI::MaturityDay);
  FIELD_SET(*this, IBAPI::MaturityMonthYear);
};

TEST(MessageTest, ApiTest)
{
  LOG(INFO) << "Current TimeMicros = " << now_micros() << std::endl;

  MarketDataRequest request;

  request.setField(IBAPI::SecurityType("OPT"));
  request.setField(IBAPI::PutOrCall(IBAPI::PutOrCall_PUT));
  request.setField(IBAPI::Symbol("AAPL"));

  for (FIX::FieldMap::iterator itr = request.begin();
       itr != request.end();
       ++itr) {
    LOG(INFO)
        << "{" << itr->second.getValue() << "," <<
        itr->second.getLength() << "/" << itr->second.getTotal()
        << "}" <<
        "[" << itr->second.getField() << ":"
        << "\"" << itr->second.getString() << "\"]" << std::endl;
  }

  IBAPI::PutOrCall putOrCall;
  request.getField(putOrCall);
  EXPECT_EQ(IBAPI::PutOrCall_PUT, putOrCall);

  // Get header information
  const IBAPI::Header& header = request.getHeader();
  IBAPI::MsgType msgType;
  header.getField(msgType);

  EXPECT_EQ(msgType.getString(), "MarketDataRequest");

  const IBAPI::Trailer& trailer = request.getTrailer();
  IBAPI::Ext_SendingTimeMicros sendingTimeMicros;
  trailer.getField(sendingTimeMicros);

  LOG(INFO) << "Timestamp = " << sendingTimeMicros.getString() << std::endl;
}

struct SocketReader : NoCopyAndAssign {
  virtual void operator()(zmq::socket_t& socket) = 0;
};

class Responder
{
 public:
  Responder(const string& addr, SocketReader& reader) :
      context_(1),
      socket_(context_, ZMQ_REP),
      running_(false),
      reader_(reader) {

    socket_.bind(addr.c_str());
    // start thread
    thread_ = boost::shared_ptr<boost::thread>(new boost::thread(
        boost::bind(&Responder::processMessages, this)));
    running_ = true;

    LOG(INFO) << "Started responder. " << std::endl;
  }

  zmq::context_t& context()
  {
    return context_;
  }

  void stop()
  {
    boost::unique_lock<boost::mutex> lock(mutex_);
    running_ = false;
    thread_->join();
  }

 private:
  void processMessages()
  {
    while (running_) {
      reader_(socket_);
    }
  }

 private:
  zmq::context_t context_;
  zmq::socket_t socket_;
  bool running_;
  boost::shared_ptr<boost::thread> thread_;
  boost::mutex mutex_;
  SocketReader& reader_;
};

void free_func(void* mem, void* mem2)
{
  LOG(INFO) << "free_func called" << std::endl;
}

void send(const FIX::FieldBase& f, zmq::socket_t& socket, int sendMore)
{
  LOG(INFO) << "Sending [" << f.getValue() << "]" << std::endl;
  const std::string& fv = f.getString();
  const char* buff = fv.c_str();
  size_t size = fv.length();
  // Force zero copy by providing a mem free function that does
  // nothing (ownership of buffer still belongs to the Message)
  zmq::message_t frame((void*)(buff), size, free_func);
  socket.send(frame, sendMore);
}

void send(const FIX::FieldMap& map, zmq::socket_t& socket, int sendMore)
{
  for (FIX::FieldMap::iterator itr = map.begin();
       itr != map.end();
       ++itr) {
    send(itr->second, socket, sendMore);
  }
}

static bool receive(zmq::socket_t & socket, std::string* output) {
  zmq::message_t message;
  socket.recv(&message);
  output->assign(static_cast<char*>(message.data()), message.size());
  int64_t more;           //  Multipart detection
  size_t more_size = sizeof (more);
  socket.getsockopt(ZMQ_RCVMORE, &more, &more_size);
  LOG(INFO) << '"' << *output << '/' << more << '"' << std::endl;
  return more;
}

TEST(MessageTest, ZmqSendTest)
{
  LOG(INFO) << "Current TimeMicros = " << now_micros() << std::endl;

  struct TestReader : SocketReader
  {
    void operator()(zmq::socket_t& socket)
    {
      std::string buff;
      bool more = receive(socket, &buff);
      while (more) {
        more = receive(socket, &buff);
      }
    }
  } testReader;

  const std::string& addr = "inproc://ZmqSendTest";

  Responder responder(addr, testReader);

  // For inproc endpoint, we need to use a shared context. Otherwise, the
  // program will crash.
  zmq::socket_t client(responder.context(), ZMQ_REQ);
  client.connect(addr.c_str());

  MarketDataRequest request;
  request.setField(IBAPI::SecurityType(IBAPI::SecurityType_COMMON_STOCK));
  request.setField(IBAPI::Symbol("GOOG"));

  const IBAPI::Header& header = request.getHeader();
  send(header, client, ZMQ_SNDMORE);
  send(request, client, ZMQ_SNDMORE);
  const IBAPI::Trailer& trailer = request.getTrailer();
  IBAPI::Ext_SendingTimeMicros sendingTimeMicros;
  trailer.getField(sendingTimeMicros);
  send(sendingTimeMicros, client, 0); // END

  responder.stop();
}

