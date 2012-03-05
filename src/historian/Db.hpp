#ifndef HISTORIAN_DB_H_
#define HISTORIAN_DB_H_

#include <string>

#include <boost/date_time/posix_time/posix_time.hpp>

#include "proto/ib.pb.h"
#include "proto/historian.pb.h"

#include "historian/Visitor.hpp"


namespace historian {

using boost::posix_time::ptime;
using proto::ib::MarketData;
using proto::ib::MarketDepth;
using proto::historian::SessionLog;

using proto::historian::QueryByRange;
using proto::historian::QueryBySymbol;
using proto::historian::QuerySessionLogs;

class Db
{
 public:

  Db(const std::string& file);
  ~Db();

  bool open();

  bool write(const MarketData& value, bool overwrite = true);
  bool write(const MarketDepth& value, bool overwrite = true);
  bool write(const SessionLog& value, bool overwrite = true);

  int query(const QueryByRange& query, Visitor* visit);
  int query(const std::string& start, const std::string& stop,
            Visitor* visit);

  int query(const QueryBySymbol& query, Visitor* visit);
  int query(const std::string& symbol, const ptime& startUtc,
            const ptime& stopUtc,
            Visitor* visit);


 private:

  class implementation;

  boost::scoped_ptr<implementation> impl_;
};

} // namespace historian

#endif //HISTORIAN_DB_H_
