#ifndef ATP_ORDER_MANAGER_H_
#define ATP_ORDER_MANAGER_H_

#include <string>
#include <vector>
#include <zmq.hpp>

#include <boost/scoped_ptr.hpp>

#include "proto/ib.pb.h"

#include "AsyncResponse.hpp"


using std::string;
using std::vector;
using zmq::context_t;

using proto::ib::CancelOrder;
using proto::ib::MarketOrder;
using proto::ib::LimitOrder;
using proto::ib::OrderStatus;
using proto::ib::StopLimitOrder;


namespace atp {

typedef boost::shared_ptr< AsyncResponse<OrderStatus> > AsyncOrderStatus;

class OrderManager
{
 public:
  OrderManager(const string& em_endpoint,
               const vector<string>& filters,
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


} // atp


#endif //ATP_ORDER_MANAGER_H_
