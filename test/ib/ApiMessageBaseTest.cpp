//#include <boost/bind.hpp>
//#include <boost/shared_ptr.hpp>
//#include <boost/thread.hpp>
#include <glog/logging.h>
#include <gtest/gtest.h>
//#include <zmq.hpp>
//#include <stdlib.h>

//#include <quickfix/FixFields.h>
//#include <quickfix/FixValues.h>

//#include "utils.hpp"
//#include "common.hpp"
//#include "ib/IBAPIFields.hpp"
//#include "ib/IBAPIValues.hpp"
#include "ib/ApiMessageBase.hpp"
#//include "ib/Message.hpp"
#include "zmq/Reactor.hpp"

using FIX::FieldMap;
using atp::zmq::Reactor;
using ib::internal::ApiMessageBase;
using ib::internal::ZmqMessage;
using ib::internal::ZmqMessage;


class TestMarketDataRequest : public ApiMessageBase
{
 public:
  TestMarketDataRequest() : ApiMessageBase("MarketDataRequest", "ib.v964")
  {
  }

  FIELD_SET(*this, FIX::Symbol);
  FIELD_SET(*this, FIX::PutOrCall);
  FIELD_SET(*this, FIX::StrikePrice);
  FIELD_SET(*this, FIX::MaturityDate);

  bool callApi(ib::internal::EClientPtr eclient)
  {
    return false;
  }
};

struct ReceiveOneMessage : Reactor::Strategy
{
  ReceiveOneMessage() : done(false) {}

  int socketType() { return ZMQ_PULL; }
  bool respond(zmq::socket_t& socket)
  {
    LOG(INFO) << "Starting to receive.";
    done = zmqMessage.receive(socket);
    LOG(INFO) << "Done receive: " << done;
    return done;
  }
  ZmqMessage zmqMessage;
  bool done;
};


static TestMarketDataRequest* newRequest(const std::string& symbol,
                                         int putOrCall,
                                         double strike)
{
  TestMarketDataRequest *req = new TestMarketDataRequest();
  req->set(FIX::Symbol(symbol));
  req->set(FIX::PutOrCall(putOrCall));
  req->set(FIX::StrikePrice(strike));

  for (FIX::FieldMap::iterator itr = req->begin();
       itr != req->end();
       ++itr) {
    LOG(INFO)
        << "{" << itr->second.getValue() << "," <<
        itr->second.getLength() << "/" << itr->second.getTotal()
        << "}" <<
        "[" << itr->second.getField() << ":"
        << "\"" << itr->second.getString() << "\"]" ;
  }
  return req;
}

TEST(ApiMessageBaseTest, BuildMessageTest)
{
  LOG(INFO) << "Current TimeMicros = " << now_micros() ;

  TestMarketDataRequest *req = newRequest("SPY", FIX::PutOrCall_PUT, 120.);
  TestMarketDataRequest& request = *req;

  EXPECT_EQ(3, request.totalFields());

  FIX::PutOrCall putOrCall;
  request.get(putOrCall);
  EXPECT_EQ(FIX::PutOrCall_PUT, putOrCall);

  FIX::StrikePrice strike;
  request.get(strike);
  EXPECT_EQ("120", strike.getString());
  EXPECT_EQ(120., strike);

  // Get header information
  const IBAPI::Header& header = request.getHeader();
  FIX::MsgType msgType;
  header.get(msgType);

  EXPECT_EQ("MarketDataRequest", msgType.getString());

  const IBAPI::Trailer& trailer = request.getTrailer();
  IBAPI::Ext_SendingTimeMicros sendingTimeMicros;
  trailer.get(sendingTimeMicros);

  LOG(INFO) << "Timestamp = " << sendingTimeMicros.getString();

  // Create a copy
  TestMarketDataRequest copy(request);

  const IBAPI::Trailer& trailer2 = copy.getTrailer();
  IBAPI::Ext_SendingTimeMicros sendingTimeMicros2;
  trailer2.get(sendingTimeMicros2);
  EXPECT_EQ(sendingTimeMicros.getString(), sendingTimeMicros2.getString());

  delete req;
}

TEST(ApiMessageBaseTest, SendMessageTest)
{
  // Set up the reactor
  const std::string& addr =
      "ipc://_zmq.ApiMessageBaseTest_sendMessageTest.in";
  ReceiveOneMessage strategy;
  Reactor reactor(addr, strategy);

  LOG(INFO) << "Client connecting.";

  // Now set up client to send
  zmq::context_t context(1);
  zmq::socket_t client(context, ZMQ_PUSH);
  client.connect(addr.c_str());

  LOG(INFO) << "Client connected.";

  TestMarketDataRequest *req = newRequest("SPY", FIX::PutOrCall_PUT, 120.);
  EXPECT_FALSE(req->isEmpty());



  req->send(client);

  LOG(INFO) << "Message sent";

  while (!strategy.done) { sleep(1); }

  EXPECT_EQ(3, strategy.zmqMessage.totalFields());

  LOG(INFO) << "Test finished.";
}
