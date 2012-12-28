#ifndef ATP_SERVICE_ORDER_MANAGER_H_
#define ATP_SERVICE_ORDER_MANAGER_H_

#include <string>
#include <vector>
#include <zmq.hpp>

#include <boost/scoped_ptr.hpp>

#include "proto/ib.pb.h"

#include "common.hpp"
#include "common/async_response.hpp"


using std::string;
using std::vector;
using zmq::context_t;

using proto::ib::CancelOrder;
using proto::ib::MarketOrder;
using proto::ib::LimitOrder;
using proto::ib::OrderStatus;
using proto::ib::StopLimitOrder;

using atp::common::async_response;

namespace atp {
namespace service {


typedef boost::shared_ptr< async_response<OrderStatus> > AsyncOrderStatus;

class OrderManager : NoCopyAndAssign
{
 public:

  OrderManager(const string& em_endpoint,  // receives orders
               const string& em_messages_endpoint, // order status
               context_t* context = NULL);

  ~OrderManager();


  const AsyncOrderStatus send(CancelOrder& order);
  const AsyncOrderStatus send(MarketOrder& order);
  const AsyncOrderStatus send(LimitOrder& order);
  const AsyncOrderStatus send(StopLimitOrder& order);

 private:

  class implementation;
  boost::scoped_ptr<implementation> impl_;

};

} // service
} // atp


#endif //ATP_SERVICE_ORDER_MANAGER_H_
