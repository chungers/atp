#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <glog/logging.h>
#include <gtest/gtest.h>
#include <zmq.hpp>
#include <stdlib.h>

#include "utils.hpp"
#include "common.hpp"
#include "ib/IBAPIFields.hpp"
#include "ib/IBAPIValues.hpp"
#include "ib/Message.hpp"

using FIX::FieldMap;

class MarketDataRequest : public IBAPI::Message
{
 public:
  MarketDataRequest() : Message("MarketDataRequest", "ib.v964")
  {
  }

  FIELD_SET(*this, FIX::Symbol);
  FIELD_SET(*this, FIX::SecurityID);
  FIELD_SET(*this, FIX::SecurityType);
  FIELD_SET(*this, FIX::PutOrCall);
  FIELD_SET(*this, FIX::StrikePrice);
  FIELD_SET(*this, FIX::RoutingID);
  FIELD_SET(*this, FIX::MaturityDate);
};

TEST(MessageTest, ApiTest)
{
  LOG(INFO) << "Current TimeMicros = " << now_micros() ;

  MarketDataRequest request;

  // Using set(X) as the type-safe way (instead of setField())
  request.set(FIX::SecurityType(FIX::SecurityType_OPTION));
  request.set(FIX::PutOrCall(FIX::PutOrCall_PUT));

  FIX::Symbol aapl("AAPL");
  request.set(aapl);
  request.set(FIX::StrikePrice(425.50));  // Not a valid strike but for testing

  // See FieldTypes.h
  FIX::LocalDate expiration(17, 11, 2011);
  //request.set(FIX::MaturityDate(expiration));


  // This will not compile -- field is not defined.
  //request.set(IBAPI::SecurityExchange("SMART"));


  for (FIX::FieldMap::iterator itr = request.begin();
       itr != request.end();
       ++itr) {
    LOG(INFO)
        << "{" << itr->second.getValue() << "," <<
        itr->second.getLength() << "/" << itr->second.getTotal()
        << "}" <<
        "[" << itr->second.getField() << ":"
        << "\"" << itr->second.getString() << "\"]" ;
  }

  FIX::PutOrCall putOrCall;
  request.get(putOrCall);
  EXPECT_EQ(FIX::PutOrCall_PUT, putOrCall);

  FIX::StrikePrice strike;
  request.get(strike);
  EXPECT_EQ("425.5", strike.getString());
  EXPECT_EQ(425.5, strike);

  // Get header information
  const IBAPI::Header& header = request.getHeader();
  FIX::MsgType msgType;
  header.get(msgType);

  EXPECT_EQ(msgType.getString(), "MarketDataRequest");

  const IBAPI::Trailer& trailer = request.getTrailer();
  IBAPI::Ext_SendingTimeMicros sendingTimeMicros;
  trailer.get(sendingTimeMicros);

  LOG(INFO) << "Timestamp = " << sendingTimeMicros.getString() ;
}

TEST(MessageTest, CopyTest)
{
  LOG(INFO) << "Current TimeMicros = " << now_micros() ;

  MarketDataRequest request;

  request.set(FIX::SecurityType(FIX::SecurityType_COMMON_STOCK));
  request.set(FIX::Symbol("SPY"));
  request.set(FIX::RoutingID(IBAPI::RoutingID_DEFAULT));

  for (FIX::FieldMap::iterator itr = request.begin();
       itr != request.end();
       ++itr) {
    LOG(INFO)
        << "{" << itr->second.getValue() << "," <<
        itr->second.getLength() << "/" << itr->second.getTotal()
        << "}" <<
        "[" << itr->second.getField() << ":"
        << "\"" << itr->second.getString() << "\"]" ;
  }

  FIX::RoutingID routingId;
  request.get(routingId);
  EXPECT_EQ("SMART", routingId.getString());

  // Create a copy
  MarketDataRequest copy(request);
  FIX::RoutingID copyRoutingId;
  copy.get(copyRoutingId);
  EXPECT_EQ(routingId.getString(), copyRoutingId.getString());

  const IBAPI::Trailer& trailer = request.getTrailer();
  IBAPI::Ext_SendingTimeMicros sendingTimeMicros;
  trailer.get(sendingTimeMicros);

  const IBAPI::Trailer& trailer2 = copy.getTrailer();
  IBAPI::Ext_SendingTimeMicros sendingTimeMicros2;
  trailer2.get(sendingTimeMicros2);
  EXPECT_EQ(sendingTimeMicros.getString(), sendingTimeMicros2.getString());
}


struct SocketReader : NoCopyAndAssign {
  virtual bool operator()(zmq::socket_t& socket) = 0;
};

class Reactor
{
 public:
  Reactor(const string& addr, SocketReader& reader) :
      context_(1),
      socket_(context_, ZMQ_REP),
      running_(false),
      reader_(reader) {

    socket_.bind(addr.c_str());
    // start thread
    thread_ = boost::shared_ptr<boost::thread>(new boost::thread(
        boost::bind(&Reactor::processMessages, this)));
    running_ = true;

    LOG(INFO) << "Started reactor. " ;
  }

  zmq::context_t& context()
  {
    return context_;
  }

  void stop()
  {
    boost::unique_lock<boost::mutex> lock(mutex_);
    running_ = false;
    socket_.close();
    thread_->join();
  }

 private:
 void processMessages()
  {
    while (running_ && reader_(socket_)) {
    }
  }

 private:
  zmq::context_t context_;
  zmq::socket_t socket_;
  bool running_;
  boost::shared_ptr<boost::thread> thread_;
  SocketReader& reader_;
  boost::mutex mutex_;
};

void free_func(void* mem, void* mem2)
{
  LOG(INFO) << "free_func called" ;
}

void send(const FIX::FieldBase& f, zmq::socket_t& socket, int sendMore)
{
  LOG(INFO) << "Sending [" << f.getValue() << "]" ;
  const std::string& fv = f.getValue();
  const char* buff = fv.c_str();
  // FIX field pads an extra '\001' byte.  In the case of zmq, we don't
  // send this extra byte.  (see http://goo.gl/EBDDi#L113)
  size_t size = fv.size() - 1;
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
  LOG(INFO) << "unpacking " << message.data() << " size = " << message.size()
            ;

  output->assign(static_cast<char*>(message.data()), message.size());

  int64_t more;           //  Multipart detection
  size_t more_size = sizeof (more);
  socket.getsockopt(ZMQ_RCVMORE, &more, &more_size);
  return more;
}

static bool parseField(const std::string& buff, IBAPI::Message& message)
{
  // parse the message string
  size_t pos = buff.find('=');
  if (pos != std::string::npos) {
    int code = atoi(buff.substr(0, pos).c_str());
    std::string value = buff.substr(pos+1);
    LOG(INFO) << "Code: " << code
              << ", Value: " << value
              << " (" << value.length() << ")"
              ;
    switch (code) {
      case FIX::FIELD::MsgType:
      case FIX::FIELD::BeginString:
      case FIX::FIELD::SendingTime:
        message.getHeader().setField(code, value);
        break;
      case FIX::FIELD::Ext_SendingTimeMicros:
      case FIX::FIELD::Ext_OrigSendingTimeMicros:
        message.getTrailer().setField(code, value);
      default:
        message.setField(code, value);
    }
    return true;
  }
  return false;
}

TEST(MessageTest, ZmqSendTest)
{
  LOG(INFO) << "Current TimeMicros = " << now_micros() ;

  struct TestReader : SocketReader
  {
    bool operator()(zmq::socket_t& socket)
    {
      IBAPI::Message message;
      std::string buff;
      bool status = false;
      try {
        while (receive(socket, &buff)) {
          parseField(buff, message);
        }
        // Last line
        parseField(buff, message);
        boost::lock_guard<boost::mutex> lock(mutex);
        messages.push_back(message);
        hasReceivedMessage.notify_one();

        LOG(INFO) << "Got the whole message" ;
        status = true;
      } catch (zmq::error_t e) {
        LOG(WARNING) << "Got exception " << e.what() ;
        status = false;
      }
      return status;
    }

    std::vector<IBAPI::Message> messages;
    boost::mutex mutex;
    boost::condition_variable hasReceivedMessage;
  } testReader;

  const std::string& addr = "inproc://ZmqSendTest";

  Reactor reactor(addr, testReader);

  // For inproc endpoint, we need to use a shared context. Otherwise, the
  // program will crash.
  zmq::socket_t client(reactor.context(), ZMQ_REQ);
  client.connect(addr.c_str());

  MarketDataRequest request;
  request.set(FIX::SecurityType(FIX::SecurityType_COMMON_STOCK));
  request.set(FIX::Symbol("GOOG"));

  // Sending out message
  const IBAPI::Header& header = request.getHeader();
  send(header, client, ZMQ_SNDMORE);
  send(request, client, ZMQ_SNDMORE);
  const IBAPI::Trailer& trailer = request.getTrailer();
  IBAPI::Ext_SendingTimeMicros sendingTimeMicros;
  trailer.get(sendingTimeMicros);
  send(sendingTimeMicros, client, 0); // END

  // Waiting for the other side to receive it.
  boost::unique_lock<boost::mutex> lock(testReader.mutex);
  testReader.hasReceivedMessage.wait(lock);

  IBAPI::Message message = testReader.messages.front();
  for (FIX::FieldMap::iterator itr = message.begin();
       itr != message.end();
       ++itr) {
    LOG(INFO) << "Got message field " << itr->second.getString() ;
  }

  FIX::MsgType msgType1;
  request.getHeader().get(msgType1);
  FIX::MsgType msgType2;
  message.getHeader().get(msgType2);

  EXPECT_EQ("MarketDataRequest", msgType1.getString());
  EXPECT_EQ(msgType1.getString(), msgType2.getString());


  LOG(INFO) << "Len = " << msgType1.getString().length()
            << ", " << msgType2.getString().length() ;

  FIX::Symbol symbol;
  message.getField(symbol);
  EXPECT_EQ(symbol.getString(), "GOOG");

  LOG(INFO) << "Stopping reactor." ;
  reactor.stop();
}
