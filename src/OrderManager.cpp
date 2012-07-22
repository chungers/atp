
#include "zmq/Subscriber.hpp"
#include "zmq/ZmqUtils.hpp"

#include "ZmqProtoBuffer.hpp"
#include "OrderManager.hpp"

using std::string;
using std::vector;
using ::zmq::context_t;
using ::zmq::socket_t;

using atp::zmq::Subscriber;

namespace p = proto::ib;


namespace atp {

class OrderManager::implementation : public Subscriber::Strategy
{

 public:

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
    filters_.push_back(ORDER_STATUS_MESSAGE_.key());

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

        if (messageKeyFrame == ORDER_STATUS_MESSAGE_.key()) {
          p::OrderStatus status;
          bool received = atp::receive<p::OrderStatus>(socket, status);


          LOG(ERROR) << "Received: " << messageKeyFrame << ","
                     << status.order_id();

        }
      }
    } catch (error_t e) {
      ORDER_MANAGER_ERROR << "Error: " << e.what();
    }
    return false;
  }

  template <typename P>
  const AsyncOrderStatus send(P& proto)
  {
    AsyncResponse<p::OrderStatus>* response = NULL;
    if (em_socket_ != NULL) {
      size_t sent = atp::send(*em_socket_, now_micros(), now_micros(), proto);

      ORDER_MANAGER_LOGGER << "Sent " << proto.key() << " (" << sent << ")";

      response = new AsyncResponse<p::OrderStatus>();
    }
    return AsyncOrderStatus(response);
  }

 private:
  const string em_endpoint_;
  const string em_messages_endpoint_;

  vector<string> filters_;

  context_t* context_;
  socket_t* em_socket_;

  boost::scoped_ptr<Subscriber> order_status_subscriber_;

  p::OrderStatus ORDER_STATUS_MESSAGE_;
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



