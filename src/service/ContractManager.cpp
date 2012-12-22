#include <map>
#include <string>
#include <vector>

#include <zmq.hpp>
#include <boost/unordered_map.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>

#include "zmq/Subscriber.hpp"
#include "zmq/ZmqUtils.hpp"

#include "ZmqProtoBuffer.hpp"

#include "platform/contract_symbol.hpp"
#include "service/ContractManager.hpp"

using std::string;
using std::vector;
using ::zmq::context_t;
using ::zmq::socket_t;
using ::zmq::error_t;

using atp::zmq::Subscriber;

namespace p = proto::ib;


namespace atp {
namespace service {


class ContractManager::implementation : public Subscriber::Strategy
{

 public:

  explicit implementation(const string& cm_endpoint,
                          const string& cm_messages_endpoint,
                          const vector<string>& filters,
                          context_t* context) :
      cm_endpoint_(cm_endpoint),
      cm_messages_endpoint_(cm_messages_endpoint),
      filters_(filters),
      context_(context),
      contract_details_subscriber_(NULL)
  {
    if (context_ == NULL) {
      context_ = new context_t(1);
    }
    // Connect to Contract Manager (CM) socket (outbound)
    cm_socket_ = new socket_t(*context_, ZMQ_PUSH);
    cm_socket_->connect(cm_endpoint.c_str());

    CONTRACT_MANAGER_LOGGER << "Connected to " << cm_endpoint_;

    // Add subscriptions
    filters_.push_back(CONTRACT_DETAILS_RESPONSE_.GetTypeName());
    filters_.push_back(CONTRACT_DETAILS_END_.GetTypeName());

    // Start inbound subscriber for Responses coming from CM
    contract_details_subscriber_.reset(new Subscriber(
        cm_messages_endpoint_,
        filters_, *this, context_));

    CONTRACT_MANAGER_LOGGER << "ContractManager ready.";

  }

  // implements Strategy
  virtual bool check_message(socket_t& socket)
  {
    try {
      string messageKeyFrame;
      bool more = atp::zmq::receive(socket, &messageKeyFrame);

      if (more) {

        if (messageKeyFrame == CONTRACT_DETAILS_RESPONSE_.GetTypeName()) {

          p::ContractDetailsResponse resp;
          bool received = atp::receive<p::ContractDetailsResponse>(
              socket, resp);

          if (received) {
            // Now check to see if there's a pending order status for this
            boost::shared_lock<boost::shared_mutex> lock(mutex_);

            string key(resp.details().symbol());
            if (contractDetails_.find(key) == contractDetails_.end()) {
              contractDetails_[key] = resp.details();

              CONTRACT_MANAGER_LOGGER << "Updated " << key;

            } else {
              CONTRACT_MANAGER_WARNING
                  << "Received another contract detail for "
                  << key
                  << ", updating.";
              contractDetails_[key] = resp.details();
            }
          }

        } else if (messageKeyFrame == CONTRACT_DETAILS_END_.GetTypeName()) {
          p::ContractDetailsEnd *status = new p::ContractDetailsEnd();
          bool received = atp::receive<p::ContractDetailsEnd>(socket, *status);

          if (received) {

            // Now check to see if there's a pending order status for this
            boost::shared_lock<boost::shared_mutex> lock(mutex_);

            RequestId key(status->request_id());
            if (pendingRequests_.find(key) != pendingRequests_.end()) {

              AsyncContractDetailsEnd s = pendingRequests_[key];

              CONTRACT_MANAGER_LOGGER << "Received final end for request: "
                                      << messageKeyFrame << ",reqId="
                                      << key << ','
                                      << &s;

              s->set_response(status);
            }
          }
        }
      }
    } catch (::zmq::error_t e) {
      CONTRACT_MANAGER_ERROR << "Error: " << e.what();
    }
    return true;
  }

  const AsyncContractDetailsEnd
  requestContractDetails(const RequestId& id, const p::Contract& contract)
  {
    async_response<p::ContractDetailsEnd>* response = NULL;

    if (cm_socket_ != NULL) {

      p::RequestContractDetails req;

      req.mutable_contract()->CopyFrom(contract);
      req.mutable_contract()->set_id(0); // to be filled by response.
      req.set_request_id(id);

      size_t sent = atp::send(*cm_socket_, now_micros(), now_micros(), req);

      CONTRACT_MANAGER_LOGGER << "Sent for contract details: "
                              << req.GetTypeName()
                              << " (" << sent << ")";

      response = new async_response<p::ContractDetailsEnd>();
    }

    AsyncContractDetailsEnd status(response);
    boost::unique_lock<boost::shared_mutex> lock(mutex_);
    pendingRequests_[id] = status;
    return status;
  }

  const AsyncContractDetailsEnd
  requestStockContractDetails(const RequestId& id, const std::string& symbol)
  {
    async_response<p::ContractDetailsEnd>* response = NULL;

    if (cm_socket_ != NULL) {

      p::RequestContractDetails req;

      req.mutable_contract()->set_id(0); // to be filled by response.
      req.mutable_contract()->set_symbol(symbol);
      req.mutable_contract()->set_local_symbol(symbol);
      req.mutable_contract()->set_type(p::Contract::STOCK);
      req.set_request_id(id);

      size_t sent = atp::send(*cm_socket_, now_micros(), now_micros(), req);

      CONTRACT_MANAGER_LOGGER << "Sent for stock contract " << symbol << ' '
                              << req.GetTypeName()
                              << " (" << sent << ")";

      response = new async_response<p::ContractDetailsEnd>();
    }

    AsyncContractDetailsEnd status(response);
    boost::unique_lock<boost::shared_mutex> lock(mutex_);
    pendingRequests_[id] = status;
    return status;
  }

  /// Look up contract by key (e.g. AAPL.STK), returns true if found.
  bool findContract(const std::string& key, p::Contract* contract) const
  {
    if (contractDetails_.find(key) != contractDetails_.end()) {
      contract->CopyFrom(contractDetails_.find(key)->second.summary());
      return true;
    }
    return false;
  }


 private:

  const string cm_endpoint_;
  const string cm_messages_endpoint_;

  vector<string> filters_;

  context_t* context_;
  socket_t* cm_socket_;

  boost::scoped_ptr<Subscriber> contract_details_subscriber_;

  p::ContractDetailsResponse CONTRACT_DETAILS_RESPONSE_;
  p::ContractDetailsEnd CONTRACT_DETAILS_END_;

  boost::shared_mutex mutex_;
  boost::unordered_map<RequestId, AsyncContractDetailsEnd> pendingRequests_;

  boost::unordered_map<std::string, p::ContractDetails> contractDetails_;
};

const p::Contract::Right ContractManager::PutOption = p::Contract::PUT;
const p::Contract::Right ContractManager::CallOption = p::Contract::CALL;



ContractManager::ContractManager(const string& cm_endpoint,
                           const string& cm_messages_endpoint,
                           const vector<string>& filters,
                           context_t* context) :
    impl_(new implementation(cm_endpoint, cm_messages_endpoint,
                             filters, context))
{
}

ContractManager::~ContractManager()
{
}

const AsyncContractDetailsEnd
ContractManager::requestStockContractDetails(const RequestId& id,
                                             const std::string& symbol)
{
  p::Contract contract;
  contract.set_symbol(symbol);
  contract.set_local_symbol(symbol);
  contract.set_type(p::Contract::STOCK);
  return impl_->requestContractDetails(id, contract);
}

const AsyncContractDetailsEnd
ContractManager::requestOptionContractDetails(
    const RequestId& id,
    const std::string& symbol,
    const Date& expiry,
    const double strike,
    const p::Contract::Right& putOrCall)

{
  p::Contract contract;
  contract.set_symbol(symbol);
  contract.set_local_symbol(symbol);
  contract.set_type(p::Contract::OPTION);
  contract.set_right(putOrCall);
  contract.mutable_strike()->set_amount(strike);
  contract.mutable_expiry()->set_year(expiry.year());
  contract.mutable_expiry()->set_month(expiry.month());
  contract.mutable_expiry()->set_day(expiry.day());

  return impl_->requestContractDetails(id, contract);
}

const AsyncContractDetailsEnd
ContractManager::requestOptionChain(const RequestId& id,
                                    const std::string& symbol)
{
  p::Contract contract;
  contract.set_symbol(symbol);
  contract.set_type(p::Contract::OPTION);
  return impl_->requestContractDetails(id, contract);
}

const AsyncContractDetailsEnd
ContractManager::requestIndexContractDetails(const RequestId& id,
                                             const std::string& symbol)
{
  p::Contract contract;
  contract.set_symbol(symbol);
  contract.set_local_symbol(symbol);
  contract.set_type(p::Contract::INDEX);
  return impl_->requestContractDetails(id, contract);

}

bool ContractManager::findContract(const std::string& key,
                                   p::Contract* contract) const
{
  return impl_->findContract(key, contract);
}

bool ContractManager::findOptionContract(
    const std::string& symbol,
    const Date& expiry,
    const double strike,
    const p::Contract::Right& putOrCall,
    p::Contract* contract) const
{
  std::string key;
  if (atp::platform::format_option_contract_key(
          symbol,
          strike,
          putOrCall,
          expiry.year(), expiry.month(), expiry.day(), &key)) {
    return impl_->findContract(key, contract);
  }
  return false;
}


} // service
} // atp



