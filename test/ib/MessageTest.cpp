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
  MarketDataRequest() : Message("IBAPI964", "MarketDataRequest")
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
  virtual bool operator()(zmq::socket_t& socket) = 0;
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
  LOG(INFO) << "free_func called" << std::endl;
}

void send(const FIX::FieldBase& f, zmq::socket_t& socket, int sendMore)
{
  LOG(INFO) << "Sending [" << f.getValue() << "]" << std::endl;
  const std::string& fv = f.getValue();
  const char* buff = fv.c_str();
  size_t size = fv.size() - 1;  // Don't send the \0 in the C string
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
            << std::endl;

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
              << std::endl;
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
  LOG(INFO) << "Current TimeMicros = " << now_micros() << std::endl;

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

        LOG(INFO) << "Got the whole message" << std::endl;
        status = true;
      } catch (zmq::error_t e) {
        LOG(WARNING) << "Got exception " << e.what() << std::endl;
        status = false;
      }
      return status;
    }

    std::vector<IBAPI::Message> messages;
    boost::mutex mutex;
    boost::condition_variable hasReceivedMessage;
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

  // Sending out message
  const IBAPI::Header& header = request.getHeader();
  send(header, client, ZMQ_SNDMORE);
  send(request, client, ZMQ_SNDMORE);
  const IBAPI::Trailer& trailer = request.getTrailer();
  IBAPI::Ext_SendingTimeMicros sendingTimeMicros;
  trailer.getField(sendingTimeMicros);
  send(sendingTimeMicros, client, 0); // END

  // Waiting for the other side to receive it.
  boost::unique_lock<boost::mutex> lock(testReader.mutex);
  testReader.hasReceivedMessage.wait(lock);

  IBAPI::Message message = testReader.messages.front();
  for (FIX::FieldMap::iterator itr = message.begin();
       itr != message.end();
       ++itr) {
    LOG(INFO) << "Got message field " << itr->second.getString() << std::endl;
  }

  IBAPI::MsgType msgType1;
  request.getHeader().getField(msgType1);
  IBAPI::MsgType msgType2;
  message.getHeader().getField(msgType2);

  EXPECT_EQ(msgType1.getString(), "MarketDataRequest");
  EXPECT_EQ(msgType1.getString(), msgType2.getString());


  LOG(INFO) << "Len = " << msgType1.getString().length()
            << ", " << msgType2.getString().length() << std::endl;

  IBAPI::Symbol symbol;
  message.getField(symbol);
  EXPECT_EQ(symbol.getString(), "GOOG");

  LOG(INFO) << "Stopping responder." << std::endl;
  responder.stop();
}

