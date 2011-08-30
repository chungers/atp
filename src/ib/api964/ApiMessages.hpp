#ifndef IBAPI_API_MESSAGES_H_
#define IBAPI_API_MESSAGES_H_

#include "ib/ib_common.hpp"
#include "ib/Message.hpp"

namespace IBAPI {


class MarketDataRequest : public IBAPI::Message
{
 public:

  MarketDataRequest(const SecurityType& type,
                    const std::string& symbol,
                    const Exchange& exchange,
                    const Currency& currency) :
      securityType(type),
      symbol(symbol),
      exchange(exchange),
      currency(currency)
  {
  }

  const SecurityType securityType;
  const std::string& symbol;
  const Exchange exchange;
  const Currency currency;
};



class NextOrderIdMessage : public IBAPI::Message {
 public:
  NextOrderIdMessage(unsigned int nextOrderId) {}
};


}



#endif // IBAPI_API_MESSAGES_H_
