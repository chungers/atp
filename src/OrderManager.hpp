#ifndef ATP_ORDER_MANAGER_H_
#define ATP_ORDER_MANAGER_H_

#include <string>
#include <vector>
#include <zmq.hpp>

#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>

#include "proto/ib.pb.h"


using std::string;
using std::vector;
using zmq::context_t;

using proto::ib::CancelOrder;
using proto::ib::MarketOrder;
using proto::ib::LimitOrder;
using proto::ib::OrderStatus;
using proto::ib::StopLimitOrder;


namespace atp {

template <typename T>
class AsyncResponse
{
 public:
  const T& get() const;  // blocks until received async
  const bool is_ready() const;

 private:
  boost::condition_variable received_;
};


class OrderManager
{
 public:
  OrderManager(const string& em_endpoint,
               const vector<string>& filters,
               context_t* context = NULL);

  ~OrderManager();

  typedef boost::shared_ptr< AsyncResponse< OrderStatus > > AsyncOrderStatus;

  const AsyncOrderStatus& send(MarketOrder& order);
  const AsyncOrderStatus& send(LimtOrder& order);
  const AsyncOrderStatus& send(StopLimitOrder& order);
  const AsyncOrderStatus& send(CancelOrder& order);

};


} // atp


#endif //ATP_ORDER_MANAGER_H_
