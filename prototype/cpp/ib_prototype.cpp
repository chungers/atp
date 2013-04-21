
#include <string>
#include <math.h>
#include <vector>

#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/ref.hpp>
#include <boost/thread.hpp>


#include <gflags/gflags.h>
#include <gtest/gtest.h>
#include <glog/logging.h>

#include "zmq/Reactor.hpp"
#include "zmq/ZmqUtils.hpp"

#include <gtest/gtest.h>
#include <glog/logging.h>

#include "utils.hpp"
#include "main/em.hpp"

#include "ib/api966/ApiMessages.hpp"
#include "ib/api966/ostream.hpp"


using std::string;

namespace api = IBAPI::V966;


const string FH_ENDPOINT("tcp://127.0.0.1:6666");

TEST(IbPrototype, RequestContractDetails)
{
  ::zmq::context_t context(1);
  ::zmq::socket_t socket(context, ZMQ_PUSH);
  socket.connect(FH_ENDPOINT.c_str());

  api::RequestContractDetails req;

  int reqId = 100;
  req.proto().mutable_contract()->set_id(0); // to be filled by response.
  req.proto().mutable_contract()->set_symbol("AAPL");
  req.proto().mutable_contract()->set_local_symbol("AAPL");

  req.proto().set_message_id(reqId);

  EXPECT_GT(req.send(socket, req.proto().message_id()), 0);
}


