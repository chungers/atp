#ifndef IBAPI_V966_API_MESSAGES_H_
#define IBAPI_V966_API_MESSAGES_H_

// api966
#include "proto/ib.pb.h"

#include "ib/ProtoBufferApiMessage.hpp"
#include "ib/TickerMap.hpp"

#include "marshall.hpp"

namespace IBAPI {
namespace V966 {

using ib::internal::ProtoBufferApiMessage;
using ib::internal::EClientPtr;

namespace p = proto::ib;


/**
 * RequestMarketData
 */
class RequestMarketData : public ProtoBufferApiMessage<p::RequestMarketData>
{
 protected:
  virtual bool callApi(const p::RequestMarketData& proto, EClientPtr eclient)
  {
    long requestId = proto.contract().id();
    Contract c;
    if (proto.contract() >> c) {

      // register the contract
      long tickerId = ib::internal::TickerMap::registerContract(c);
      if (tickerId > 0) {
        eclient->reqMktData(requestId, c,
                            proto.tick_types(),
                            proto.snapshot());
        return true;
      }
    }
    return false;
  }
};


/**
 * CancelMarketData
 */
class CancelMarketData : public ProtoBufferApiMessage<p::CancelMarketData>
{
 protected:
  virtual bool callApi(const p::CancelMarketData& proto, EClientPtr eclient)
  {
    eclient->cancelMktData(proto.contract().id());
    return true;
  }
};


/**
 * RequestMarketDepth
 */
class RequestMarketDepth : public ProtoBufferApiMessage<p::RequestMarketDepth>
{
 protected:
  virtual bool callApi(const p::RequestMarketDepth& proto, EClientPtr eclient)
  {
    long requestId = proto.contract().id();
    Contract c;
    if (proto.contract() >> c) {

      // register the contract
      long tickerId = ib::internal::TickerMap::registerContract(c);
      if (tickerId > 0) {
        eclient->reqMktDepth(requestId, c, proto.rows());
        return true;
      }
    }
    return false;
  }
};


/**
 * CancelMarketDepth
 */
class CancelMarketDepth : public ProtoBufferApiMessage<p::CancelMarketDepth>
{
 protected:
  virtual bool callApi(const p::CancelMarketDepth& proto, EClientPtr eclient)
  {
    eclient->cancelMktDepth(proto.contract().id());
    return true;
  }
};


/**
 * CancelOrder
 */
class CancelOrder : public ProtoBufferApiMessage<p::CancelOrder>
{
 protected:
  virtual bool callApi(const p::CancelOrder& proto, EClientPtr eclient)
  {
    eclient->cancelOrder(proto.order_id());
    return true;
  }
};


/**
 * MarketOrder
 */
class MarketOrder : public ProtoBufferApiMessage<p::MarketOrder>
{
 protected:
  virtual bool callApi(const p::MarketOrder& proto, EClientPtr eclient)
  {
    Contract contract;
    Order order;
    if (proto >> order && proto.base().contract() >> contract) {
      order.orderType = "MKT";
      eclient->placeOrder(proto.base().id(), contract, order);
      return true;
    }
    return false;
  }
};


/**
 * LimitOrder
 */
class LimitOrder : public ProtoBufferApiMessage<p::LimitOrder>
{
 protected:
  virtual bool callApi(const p::LimitOrder& proto, EClientPtr eclient)
  {
    Contract contract;
    Order order;
    if (proto >> order && proto.base().contract() >> contract) {
      order.orderType = "LMT";
      eclient->placeOrder(proto.base().id(), contract, order);
      return true;
    }
    return false;
  }
};


/**
 * StopLimitOrder
 */
class StopLimitOrder : public ProtoBufferApiMessage<p::StopLimitOrder>
{
 protected:
  virtual bool callApi(const p::StopLimitOrder& proto, EClientPtr eclient)
  {
    Contract contract;
    Order order;
    if (proto >> order && proto.base().contract() >> contract) {
      order.orderType = "STPLMT";
      eclient->placeOrder(proto.base().id(), contract, order);
      return true;
    }
    return false;
  }
};

} // V966
} // IBAPI


#endif //IBAPI_V966_API_MESSAGES_H_
