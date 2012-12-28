#include <map>
#include <string>
#include <vector>

#include <zmq.hpp>
#include <boost/bind.hpp>
#include <boost/unordered_map.hpp>
#include <boost/thread.hpp>

#include "platform/message_processor.hpp"

#include "ZmqProtoBuffer.hpp"
#include "service/OrderManager.hpp"

using std::string;
using std::vector;
using ::zmq::context_t;
using ::zmq::socket_t;
using ::zmq::error_t;

namespace platform = atp::platform;
namespace p = proto::ib;


namespace atp {
namespace service {


class OrderManager::implementation
{

 public:

  typedef boost::uint64_t SubmittedOrderId;

  explicit implementation(const string& em_endpoint,  // input to em
                          const string& em_messages_endpoint, // output from em
                          context_t* context) :
      em_endpoint_(em_endpoint),
      em_messages_endpoint_(em_messages_endpoint),
      context_(context),
      order_status_subscriber_(NULL)
  {
    if (context_ == NULL) {
      context_ = new context_t(1);
    }
    // Connect to Execution Manager socket (outbound)
    em_socket_ = new socket_t(*context_, ZMQ_PUSH);
    em_socket_->connect(em_endpoint.c_str());

    ORDER_MANAGER_LOGGER << "Connected to " << em_endpoint_;

    // Start inbound subscriber for OrderStatus coming from EM

    // order status
    p::OrderStatus orderStatus;
    handlers_.register_handler(
        orderStatus.GetTypeName(),
        boost::bind(&implementation::handleOrderStatus,
                    this, _1, _2));
    handlers_.register_handler(
        "stop",
        boost::bind(&implementation::handleStop,
                    this, _1, _2));
    order_status_subscriber_.reset(
        new platform::message_processor(em_messages_endpoint_, handlers_));

    ORDER_MANAGER_LOGGER << "OrderManager ready.";

  }

  bool handleOrderStatus(const string& topic, const string& message)
  {
    p::OrderStatus *status = new p::OrderStatus();

    bool received = status->ParseFromString(message);
    if (received) {
      // Now check to see if there's a pending order status for this
      boost::shared_lock<boost::shared_mutex> lock(mutex_);

      SubmittedOrderId key(status->order_id());
      if (pendingOrders_.find(key) != pendingOrders_.end()) {

        AsyncOrderStatus s = pendingOrders_[key];

        LOG(INFO) << "Received status for pending order: "
                  << topic << ",orderId="
                  << key
                  << ",status=" << status->status()
                  << ",filled=" << status->filled()
                  << &s;

        s->set_response(status);
      }
    }
    return true;
  }

  bool handleStop(const string& topic, const string& message)
  {
    ORDER_MANAGER_LOGGER << "Stopping on " << topic << ',' << message;
    return false;
  }

  template <typename P, typename K>
  const AsyncOrderStatus send(P& proto, const K& key)
  {
    async_response<p::OrderStatus>* response = NULL;
    if (em_socket_ != NULL) {
      size_t sent = atp::send(*em_socket_, now_micros(), now_micros(), proto);

      ORDER_MANAGER_LOGGER << "Sent " << proto.GetTypeName()
                           << " (" << sent << ")";

      response = new async_response<p::OrderStatus>();
    }

    AsyncOrderStatus status(response);
    boost::unique_lock<boost::shared_mutex> lock(mutex_);
    pendingOrders_[key] = status;
    return status;
  }

 private:

  const string em_endpoint_;
  const string em_messages_endpoint_;

  context_t* context_;
  socket_t* em_socket_;

  platform::message_processor::protobuf_handlers_map handlers_;
  boost::scoped_ptr<platform::message_processor> order_status_subscriber_;

  boost::shared_mutex mutex_;
  boost::unordered_map<SubmittedOrderId, AsyncOrderStatus> pendingOrders_;
};


OrderManager::OrderManager(const string& em_endpoint,
                           const string& em_messages_endpoint,
                           context_t* context) :
    impl_(new implementation(em_endpoint, em_messages_endpoint, context))
{
}

OrderManager::~OrderManager()
{
}

const AsyncOrderStatus OrderManager::send(p::CancelOrder& order)
{
  return impl_->send<p::CancelOrder>(order, order.order_id());
}

const AsyncOrderStatus OrderManager::send(p::MarketOrder& order)
{
  return impl_->send<p::MarketOrder>(order, order.order().id());
}

const AsyncOrderStatus OrderManager::send(p::LimitOrder& order)
{
  return impl_->send<p::LimitOrder>(order, order.order().id());
}

const AsyncOrderStatus OrderManager::send(p::StopLimitOrder& order)
{
  return impl_->send<p::StopLimitOrder>(order, order.order().id());
}



} // service
} // atp



