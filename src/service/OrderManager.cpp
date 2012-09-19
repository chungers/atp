#include <map>
#include <string>
#include <vector>

#include <boost/thread.hpp>

#include "zmq/Subscriber.hpp"
#include "zmq/ZmqUtils.hpp"

#include "ZmqProtoBuffer.hpp"
#include "OrderManager.hpp"

using std::string;
using std::map;
using std::vector;
using ::zmq::context_t;
using ::zmq::socket_t;
using ::zmq::error_t;

using atp::zmq::Subscriber;

namespace p = proto::ib;


namespace atp {
namespace service {


class OrderManager::implementation : public Subscriber::Strategy
{

 public:

  typedef boost::uint64_t SubmittedOrderId;

  explicit implementation(const string& em_endpoint,
                          const string& em_messages_endpoint,
                          const vector<string>& filters,
                          context_t* context) :
      em_endpoint_(em_endpoint),
      em_messages_endpoint_(em_messages_endpoint),
      filters_(filters),
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

    // Add one subscription specifically for order status
    filters_.push_back(ORDER_STATUS_MESSAGE_.GetTypeName());

    // Start inbound subscriber for OrderStatus coming from EM
    order_status_subscriber_.reset(new Subscriber(em_messages_endpoint_,
                                                  filters_, *this, context_));

    ORDER_MANAGER_LOGGER << "OrderManager ready.";

  }

  // implements Strategy
  virtual bool check_message(socket_t& socket)
  {
    try {
      string messageKeyFrame;
      bool more = atp::zmq::receive(socket, &messageKeyFrame);

      if (more) {

        if (messageKeyFrame == ORDER_STATUS_MESSAGE_.GetTypeName()) {
          p::OrderStatus* status = new p::OrderStatus();
          bool received = atp::receive<p::OrderStatus>(socket, *status);

          if (received) {
            // Now check to see if there's a pending order status for this
            boost::lock_guard<boost::mutex> lock(mutex_);

            SubmittedOrderId key(status->order_id());
            if (pendingOrders_.find(key) != pendingOrders_.end()) {

              AsyncOrderStatus s = pendingOrders_[key];

              LOG(INFO) << "Received status for pending order: "
                        << messageKeyFrame << ",orderId="
                        << key
                        << ",status=" << status->status()
                        << ",filled=" << status->filled()
                        << &s;

              s->set_response(status);
            }
          }
        }
      }
    } catch (error_t e) {
      ORDER_MANAGER_ERROR << "Error: " << e.what();
    }
    return true;
  }

  template <typename P, typename K>
  const AsyncOrderStatus send(P& proto, const K& key)
  {
    AsyncResponse<p::OrderStatus>* response = NULL;
    if (em_socket_ != NULL) {
      size_t sent = atp::send(*em_socket_, now_micros(), now_micros(), proto);

      ORDER_MANAGER_LOGGER << "Sent " << proto.GetTypeName()
                           << " (" << sent << ")";

      response = new AsyncResponse<p::OrderStatus>();
    }

    AsyncOrderStatus status(response);

    boost::lock_guard<boost::mutex> lock(mutex_);
    pendingOrders_[key] = status;

    return status;
  }

 private:

  const string em_endpoint_;
  const string em_messages_endpoint_;

  vector<string> filters_;

  context_t* context_;
  socket_t* em_socket_;

  boost::scoped_ptr<Subscriber> order_status_subscriber_;

  p::OrderStatus ORDER_STATUS_MESSAGE_;

  boost::mutex mutex_;
  map<SubmittedOrderId, AsyncOrderStatus> pendingOrders_;
};


OrderManager::OrderManager(const string& em_endpoint,
                           const string& em_messages_endpoint,
                           const vector<string>& filters,
                           context_t* context) :
    impl_(new implementation(em_endpoint, em_messages_endpoint,
                             filters, context))
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



