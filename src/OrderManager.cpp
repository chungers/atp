

#include "OrderManager.hpp"

using std::string;
using std::vector;
using zmq::context_t;
using zmq::socket_t;

namespace p = proto::ib;


namespace atp {

class OrderManager::implementation
{

 public:
  implementation(const string& em_endpoint,
                 const vector<string>& filters,
                 context_t* context) :
      em_endpoint_(em_endpoint),
      filters_(filters),
      context_(context)
  {
    if (context_ == NULL) {
      context_ = new context_t(1);
    }
    em_socket_ = new socket_t(*context_, ZMQ_PUSH);
    em_socket_->connect(em_endpoint.c_str());
  }


  template <typename P>
  const AsyncOrderStatus send(P& proto)
  {
    return AsyncOrderStatus(new AsyncResponse<p::OrderStatus>());
  }

 private:
  const string& em_endpoint_;
  const vector<string>& filters_;

  context_t* context_;
  socket_t* em_socket_;

};


OrderManager::OrderManager(const string& em_endpoint,
                           const vector<string>& filters,
                           context_t* context) :
    impl_(new implementation(em_endpoint, filters, context))
{
}

OrderManager::~OrderManager()
{
}

const AsyncOrderStatus OrderManager::send(p::CancelOrder& order)
{
  return impl_->send<p::CancelOrder>(order);
}

const AsyncOrderStatus OrderManager::send(p::MarketOrder& order)
{
  return impl_->send<p::MarketOrder>(order);
}

const AsyncOrderStatus OrderManager::send(p::LimitOrder& order)
{
  return impl_->send<p::LimitOrder>(order);
}

const AsyncOrderStatus OrderManager::send(p::StopLimitOrder& order)
{
  return impl_->send<p::StopLimitOrder>(order);
}




} // atp



