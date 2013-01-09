#include <map>
#include <string>
#include <vector>

#include <zmq.hpp>
#include <boost/bind.hpp>
#include <boost/unordered_map.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>

#include "platform/message_processor.hpp"

#include "ZmqProtoBuffer.hpp"

#include "platform/contract_symbol.hpp"
#include "service/ContractManager.hpp"

using std::string;
using std::vector;
using ::zmq::context_t;
using ::zmq::socket_t;
using ::zmq::error_t;

namespace platform = atp::platform;
namespace p = proto::ib;


namespace atp {
namespace service {


class ContractManager::implementation
{

 public:

  explicit implementation(const string& cm_endpoint,
                          const string& cm_messages_endpoint,
                          context_t* context) :
      cm_endpoint_(cm_endpoint),
      cm_messages_endpoint_(cm_messages_endpoint),
      context_(context),
      own_context_(false),
      contract_details_subscriber_(NULL)
  {
    if (context_ == NULL) {
      context_ = new context_t(1);
      own_context_ = true;
    }
    // Connect to Contract Manager (CM) socket (outbound)
    cm_socket_ = new socket_t(*context_, ZMQ_PUSH);
    cm_socket_->connect(cm_endpoint.c_str());

    CONTRACT_MANAGER_LOGGER << "Connected to " << cm_endpoint_;

    // Start inbound subscriber for Responses coming from CM

    // contractDetailsResponse
    p::ContractDetailsResponse contractDetailsResponse;
    handlers_.register_handler(
        contractDetailsResponse.GetTypeName(),
        boost::bind(&implementation::handleContractDetailsResponse,
                    this, _1, _2));

    // contractDetailsEnd
    p::ContractDetailsEnd contractDetailsEnd;
    handlers_.register_handler(
        contractDetailsEnd.GetTypeName(),
        boost::bind(&implementation::handleContractDetailsEnd,
                    this, _1, _2));

    // stop
    handlers_.register_handler(
        "stop",
        boost::bind(&implementation::handleStop,
                    this, _1, _2));
    contract_details_subscriber_.reset(
        new platform::message_processor(cm_messages_endpoint_, handlers_));

    CONTRACT_MANAGER_LOGGER << "ContractManager ready.";
  }

  bool handleContractDetailsResponse(const string& topic, const string& message)
  {
    p::ContractDetailsResponse resp;
    bool received = resp.ParseFromString(message);
    if (received) {
      // Now check to see if there's a pending order status for this
      boost::shared_lock<boost::shared_mutex> lock(mutex_);

      string key(resp.details().symbol());
      if (contractDetails_.find(key) == contractDetails_.end()) {

        CONTRACT_MANAGER_LOGGER << "Received: " << key;

      } else {
        CONTRACT_MANAGER_WARNING
            << "Received another contract detail for "
            << key;
      }

      // we need to make sure the contract's currency is USD;
      if (resp.details().summary().strike().currency() ==
          proto::common::Money::USD) {
        contractDetails_[key] = resp.details();
        CONTRACT_MANAGER_LOGGER << "Updated " << key;
      } else {
        CONTRACT_MANAGER_LOGGER << "Non-USD contract ignored: " << key;
      }

    }
    return true;
  }

  bool handleContractDetailsEnd(const string& topic, const string& message)
  {
    p::ContractDetailsEnd *status = new p::ContractDetailsEnd();
    bool received = status->ParseFromString(message);
    if (received) {
      // Now check to see if there's a pending order status for this
      boost::shared_lock<boost::shared_mutex> lock(mutex_);

      RequestId key(status->request_id());
      if (pendingRequests_.find(key) != pendingRequests_.end()) {

        AsyncContractDetailsEnd s = pendingRequests_[key];

        CONTRACT_MANAGER_LOGGER << "Received final end for request: "
                                << topic << ",reqId="
                                << key << ','
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
  bool own_context_;

  platform::message_processor::protobuf_handlers_map handlers_;
  boost::scoped_ptr<platform::message_processor> contract_details_subscriber_;

  boost::shared_mutex mutex_;
  boost::unordered_map<RequestId, AsyncContractDetailsEnd> pendingRequests_;

  boost::unordered_map<std::string, p::ContractDetails> contractDetails_;
};

const p::Contract::Right ContractManager::PutOption = p::Contract::PUT;
const p::Contract::Right ContractManager::CallOption = p::Contract::CALL;

ContractManager::ContractManager(const string& cm_endpoint,
                           const string& cm_messages_endpoint,
                           context_t* context) :
    impl_(new implementation(cm_endpoint, cm_messages_endpoint, context))
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

  // Specify a 0 strike so that we can set the currency properly.
  contract.mutable_strike()->set_amount(0.);
  contract.mutable_strike()->set_currency(proto::common::Money::USD);

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
  contract.mutable_strike()->set_currency(proto::common::Money::USD);
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



