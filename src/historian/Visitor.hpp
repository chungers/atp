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

} // namespace historian

#endif //HISTORIAN_VISITOR_H_
