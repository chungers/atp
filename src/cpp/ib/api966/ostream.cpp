
#include <boost/date_time/posix_time/posix_time.hpp>

#include "ostream.hpp"



using boost::posix_time::ptime;


std::ostream& operator<<(std::ostream& os, const Contract& c)
{
  os << "contract="
     << "conId:" << c.conId
     << ";symbol:" << c.symbol
     << ";secType:" << c.secType
     << ";right:" << c.right
     << ";strike:" << c.strike
     << ";currency:" << c.currency
     << ";multiplier:" << c.multiplier
     << ";expiry:" << c.expiry
     << ";localSymbol:" << c.localSymbol
     << "}";
    return os;
}


std::ostream& operator<<(std::ostream& os, const Order& o)
{
  os << "order="
     << "orderId:" << o.orderId
     << ";clientId:" << o.clientId
     << ";permId:" << o.permId
     << ";orderRef:" << o.orderRef
     << ";action:" << o.action
     << ";totalQuantity:" << o.totalQuantity
     << ";orderType:" << o.orderType
     << ";lmtPrice:" << o.lmtPrice
     << ";auxPrice:" << o.auxPrice
     << ";outsideRth:" << o.outsideRth
     << ";tif:" << o.tif
     << ";allOrNone:" << o.allOrNone
     << ";minQty:" << o.minQty
      ;
  return os;
}

std::ostream& operator<<(std::ostream& out, const OrderState& v)
{
  out << "orderState="
      << "status:" << v.status
      << ";initMargin:" << v.initMargin
      << ";maintMagrin:" << v.maintMargin
      << ";equityWithLoan:" << v.equityWithLoan
      << ";commission:" << v.commission
      << ";minCommission:" << v.minCommission
      << ";maxCommission:" << v.maxCommission
      << ";commissionCurrency:" << v.commissionCurrency
      << ";warningText:" << v.warningText
      ;
  return out;
}

std::ostream& operator<<(std::ostream& out, const Execution& v)
{
  out << "execution="
      << "execId:" << v.execId
      << ";time:" << v.time
      << ";acctNumber:" << v.acctNumber
      << ";exchange:" << v.exchange
      << ";side:" << v.side
      << ";shares:" << v.shares
      << ";price:" << v.price
      << ";permId:" << v.permId
      << ";clientId:" << v.clientId
      << ";orderId:" << v.orderId
      << ";liquidation:" << v.liquidation
      << ";cumQty:" << v.cumQty
      << ";avgPrice:" << v.avgPrice
      << ";orderRef:" << v.orderRef
      ;
  return out;
}
