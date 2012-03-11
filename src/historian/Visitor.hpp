#ifndef HISTORIAN_VISITOR_H_
#define HISTORIAN_VISITOR_H_

#include <string>

#include "proto/historian.pb.h"

namespace historian {

using proto::historian::Record;

class Visitor
{
 public:
  virtual ~Visitor() {}

  // Visit a record of given type.  Returns true to continue, false to stop.
  virtual bool operator()(const Record& record) = 0;
};


struct DefaultVisitor : public Visitor
{
  virtual ~DefaultVisitor() {}

  virtual bool operator()(const Record& record)
  { return true; }
};


} // namespace historian

#endif //HISTORIAN_VISITOR_H_
