#ifndef HISTORIAN_VISITOR_H_
#define HISTORIAN_VISITOR_H_

#include <string>

#include "proto/ib.pb.h"
#include "proto/historian.pb.h"

namespace historian {

using proto::ib::MarketData;
using proto::ib::MarketDepth;
using proto::historian::SessionLog;

class Visitor
{
 public:
  virtual ~Visitor() {}

  // Visit a record of given type.  Returns true to continue, false to stop.
  virtual bool operator()(const SessionLog& log) = 0;
  virtual bool operator()(const MarketData& data) = 0;
  virtual bool operator()(const MarketDepth& data) = 0;
};


struct DefaultVisitor : public Visitor
{
  virtual ~DefaultVisitor() {}

  virtual bool operator()(const SessionLog& log)
  { return true; }

  virtual bool operator()(const MarketData& data)
  { return true; }

  virtual bool operator()(const MarketDepth& data)
  { return true; }

};

template <typename Q>
struct FilterVisitor : public Visitor
{
  FilterVisitor(Visitor* visit, const Q& query) :
      visit_(visit), query_(query) {}

  virtual bool operator()(const SessionLog& log)
  {
    return (*visit_)(log);
  }

  virtual bool operator()(const MarketData& data)
  {
    if (query_.has_depth_only() && query_.depth_only()) {
      return true; // Skip
    }
    if (query_.has_filter()) {
      if (data.event() == query_.filter()) {
        return (*visit_)(data);
      } else {
        return true; // skip
      }
    }
    return (*visit_)(data);
  }

  virtual bool operator()(const MarketDepth& data)
  {
    return (*visit_)(data);
  }


  Visitor* visit_;
  const Q& query_;
};


} // namespace historian

#endif //HISTORIAN_VISITOR_H_
