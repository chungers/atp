
#include "marshall.hpp"


// std::ostream& operator<<(std::ostream& os, const proto::ib::OrderStatus& o)
// {
//   os << "OrderStatus="
//      << "timestamp:" << o.timestamp()
//      << ",message_id:" << o.message_id()
//      << ",order_id:" << o.order_id()
//      << ",status:" << o.status()
//      << ",filled:" << o.filled()
//      << ",remaining:" << o.remaining()
//      << ",avg_fill_price:" << o.avg_fill_price()
//      << ",last_fill_price:" << o.last_fill_price()
//      << ",client_id:" << o.client_id()
//      << ",why_held:" << o.why_held()

//   return os;
// }

std::ostream& operator<<(std::ostream& os, const Order& o)
{
  os << "order="
     << "orderId:" << o.orderId
     << ",clientId:" << o.clientId
     << ",permId:" << o.permId
     << ",orderRef:" << o.orderRef
     << ",action:" << o.action
     << ",totalQuantity:" << o.totalQuantity
     << ",orderType:" << o.orderType
     << ",lmtPrice:" << o.lmtPrice
     << ",auxPrice:" << o.auxPrice
     << ",outsideRth:" << o.outsideRth
     << ",tif:" << o.tif
     << ",allOrNone:" << o.allOrNone
     << ",minQty:" << o.minQty;

  return os;
}

bool operator<<(Order& o, const proto::ib::Order& p)
{
  // check required:
  if (!p.IsInitialized()) {
    return false;
  }

  o.orderId = p.id();
  o.orderRef = p.ref();

  switch (p.action()) {
    case proto::ib::Order::BUY:
      o.action = "BUY";
      break;
    case proto::ib::Order::SELL:
      o.action = "SELL";
      break;
    case proto::ib::Order::SSHORT:
      o.action = "SSHORT";
      break;
  }

  o.totalQuantity = p.quantity();

  if (p.has_all_or_none()) {
    o.allOrNone = p.all_or_none();
  }

  if (p.has_min_quantity()) {
    o.minQty = p.min_quantity();
  }

  if (p.has_outside_rth()) {
    o.outsideRth = p.outside_rth();
  }

  switch (p.time_in_force()) {
    case proto::ib::Order::DAY:
      o.tif = "DAY";
      break;
    case proto::ib::Order::GTC:
      o.tif = "GTC";
      break;
    case proto::ib::Order::IOC:
      o.tif = "IOC";
      break;
  }
  return true;
}

bool operator<<(proto::ib::Order& p, const Order& o)
{
  p.Clear();

  p.set_id(o.orderId);
  p.set_ref(o.orderRef);

  if (o.action == "BUY") {
    p.set_action(proto::ib::Order::BUY);
  } else if (o.action == "SELL") {
    p.set_action(proto::ib::Order::SELL);
  } else if (o.action == "SSHORT") {
    p.set_action(proto::ib::Order::SSHORT);
  }

  p.set_quantity(o.totalQuantity);
  p.set_all_or_none(o.allOrNone);
  p.set_min_quantity(o.minQty);

  p.set_outside_rth(o.outsideRth);

  if (o.tif == "DAY") {
    p.set_time_in_force(proto::ib::Order::DAY);
  } else if (o.tif == "GTC") {
    p.set_time_in_force(proto::ib::Order::GTC);
  } else if (o.tif == "IOC") {
    p.set_time_in_force(proto::ib::Order::IOC);
  }

  return true;
}




bool operator<<(Order& o, const proto::ib::MarketOrder& p)
{
  return (o << p.base());
}

bool operator<<(proto::ib::MarketOrder& p, const Order& o)
{
  p.Clear();
  return ((*p.mutable_base()) << o);
}

bool operator<<(Order& o, const proto::ib::LimitOrder& p)
{
  if (o << p.base()) {
    o.lmtPrice = p.limit_price().amount();
    return true;
  }
  return false;
}

bool operator<<(proto::ib::LimitOrder& p, const Order& o)
{
  p.Clear();
  if ((*p.mutable_base()) << o) {
    p.mutable_limit_price()->set_amount(o.lmtPrice);
    return true;
  }
  return false;
}

bool operator<<(Order& o, const proto::ib::StopLimitOrder& p)
{
  if (o << p.base()) {
    o.lmtPrice = p.limit_price().amount();
    o.auxPrice = p.stop_price().amount();
    return true;
  }
  return false;
}

bool operator<<(proto::ib::StopLimitOrder& p, const Order& o)
{
  p.Clear();
  if ((*p.mutable_base()) << o) {
    p.mutable_limit_price()->set_amount(o.lmtPrice);
    p.mutable_stop_price()->set_amount(o.auxPrice);
    return true;
  }
  return false;
}
