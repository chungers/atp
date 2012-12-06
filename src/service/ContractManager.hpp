#ifndef ATP_SERVICE_CONTRACT_MANAGER_H_
#define ATP_SERVICE_CONTRACT_MANAGER_H_

#include <string>
#include <vector>
#include <zmq.hpp>

#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/scoped_ptr.hpp>

#include "proto/ib.pb.h"

#include "common.hpp"
#include "common/async_response.hpp"


using std::string;
using std::vector;
using zmq::context_t;

namespace p = proto::ib;

using atp::common::async_response;

namespace atp {
namespace service {

namespace p = proto::ib;

typedef boost::shared_ptr< async_response<p::ContractDetailsEnd> >
AsyncContractDetailsEnd;

static const vector<string> ALL_CONTRACT_EVENTS;

class ContractManager : NoCopyAndAssign
{
 public:

  typedef int RequestId;
  typedef boost::gregorian::date Date;

  const static p::Contract::Right PutOption;
  const static p::Contract::Right CallOption;


  ContractManager(const string& em_endpoint,  // receives orders
                  const string& em_messages_endpoint, // order status
                  const vector<string>& filters = ALL_CONTRACT_EVENTS,
                  context_t* context = NULL);

  ~ContractManager();

  /// Symbol here is the simple case like 'AAPL'
  const AsyncContractDetailsEnd
  requestStockContractDetails(const RequestId& id, const std::string& symbol);

  /// Symbol here is the simple case like 'AAPL'
  const AsyncContractDetailsEnd
  requestOptionContractDetails(const RequestId& id, const std::string& symbol,
                               const Date& expiry,
                               const double strike,
                               const p::Contract::Right& putOrCall);

  /// Symbol here is the simple case like 'AAPL'
  const AsyncContractDetailsEnd
  requestOptionChain(const RequestId& id, const std::string& symbol);

  /// Symbol here is the simple case like 'AAPL'
  const AsyncContractDetailsEnd
  requestIndex(const RequestId& id, const std::string& symbol);

  /// Key is something like 'AAPL.STK'
  bool findContract(const std::string& key, p::Contract* contract) const;

  /// for Option
  bool findOptionContract(const std::string& symbol,
                          const Date& expiry,
                          const double strike,
                          const p::Contract::Right& putOrCall,
                          p::Contract* contract) const;

 private:

  class implementation;
  boost::scoped_ptr<implementation> impl_;

};



} // service
} // atp


#endif //ATP_SERVICE_CONTRACT_MANAGER_H_
