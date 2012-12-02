#ifndef ATP_SERVICE_CONTRACT_MANAGER_H_
#define ATP_SERVICE_CONTRACT_MANAGER_H_

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

using proto::ib::Contract;
using proto::ib::ContractDetailsEnd;

using atp::common::async_response;

namespace atp {
namespace service {


typedef boost::shared_ptr< async_response<ContractDetailsEnd> >
AsyncContractDetailsEnd;

static const vector<string> ALL_CONTRACT_EVENTS;

class ContractManager : NoCopyAndAssign
{
 public:

  typedef int RequestId;

  ContractManager(const string& em_endpoint,  // receives orders
               const string& em_messages_endpoint, // order status
               const vector<string>& filters = ALL_CONTRACT_EVENTS,
               context_t* context = NULL);

  ~ContractManager();

  /// Symbol here is the simple case like 'AAPL'
  const AsyncContractDetailsEnd
  requestContractDetails(const RequestId& id, const std::string& symbol);

  /// Key is something like 'AAPL.STK'
  bool findContract(const std::string& key, Contract* contract) const;

 private:

  class implementation;
  boost::scoped_ptr<implementation> impl_;

};

} // service
} // atp


#endif //ATP_SERVICE_CONTRACT_MANAGER_H_
