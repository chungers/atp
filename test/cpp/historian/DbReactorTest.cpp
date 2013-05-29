
#include <stdio.h>
#include <string>
#include <iostream>
#include <vector>

#include <gtest/gtest.h>
#include <glog/logging.h>

#include "utils.hpp"
#include "zmq/ZmqUtils.hpp"

#include "common/time_utils.hpp"
#include "historian/DbReactorStrategy.hpp"


using boost::posix_time::ptime;
using historian::Db;
using proto::ib::MarketData;
using proto::ib::MarketDepth;
using proto::historian::SessionLog;

using zmq::context_t;
using zmq::error_t;
using zmq::message_t;
using zmq::socket_t;

using std::string;

TEST(DbReactorTest, ZmqProtoTest)
{
  const string est("2012-02-14 04:30:34.567899");
  ptime t;
  atp::common::parse(est, &t);

  MarketData d;

  d.set_timestamp(atp::common::as_micros(t));
  d.set_symbol("AAPL.STK");
  d.set_event("ASK");
  d.mutable_value()->set_type(proto::common::Value_Type_DOUBLE);
  d.mutable_value()->set_double_value(500.0);
  d.set_contract_id(9999);

  {
    string buffer;
    EXPECT_TRUE(d.SerializeToString(&buffer));

    message_t frame(buffer.size());
    memcpy(frame.data(), buffer.data(), buffer.size());

    string buffer2;
    buffer2.assign(static_cast<char*>(frame.data()), frame.size());
    EXPECT_EQ(buffer, buffer2);

    MarketData d2;
    EXPECT_TRUE(d2.ParseFromString(buffer2));
  }
  try {

    context_t context(1);
    socket_t sender_socket(context, ZMQ_PUSH);
    socket_t receiver_socket(context, ZMQ_PULL);

    const string& ep(atp::zmq::EndPoint::ipc("__dbreactortest"));

    receiver_socket.bind(ep.c_str());
    sender_socket.connect(ep.c_str());

    // send a bunch to fill the buffer
    for (int i=0; i < 10; ++i) {
      d.mutable_value()->set_double_value(100. * i);

      string sendBuffer;
      EXPECT_TRUE(d.SerializeToString(&sendBuffer));
      size_t sent = atp::zmq::send_copy(sender_socket, sendBuffer);
      EXPECT_EQ(sent, sendBuffer.size());
    }


    for (int i=0; i < 10; ++i) {
      string receiveBuffer;
      bool more = atp::zmq::receive(receiver_socket, &receiveBuffer);
      EXPECT_FALSE(more);

      MarketData m;
      EXPECT_TRUE(m.ParseFromString(receiveBuffer));

      EXPECT_EQ(100. * i, m.value().double_value());
    }

  } catch (zmq::error_t e) {
    LOG(FATAL) << "Exception: " << e.what();
  }
}
