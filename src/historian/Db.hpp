#ifndef HISTORIAN_DB_H_
#define HISTORIAN_DB_H_

#include <string>

#include <boost/scoped_ptr.hpp>

#include "historian/Visitor.hpp"


namespace historian {

using proto::historian::QueryByRange;
using proto::historian::QueryBySymbol;

class Db
{
 public:

  Db(const std::string& file);
  ~Db();

 public:
  bool Open();

  template <typename T> bool Write(const T& value, bool overwrite = true);

  int Query(const std::string& start, const std::string& stop,
            Visitor* visit);
  int Query(const QueryByRange& query, Visitor* visit);
  int Query(const QueryBySymbol& query, Visitor* visit);


 private:
  class implementation;
  boost::scoped_ptr<implementation> impl_;
};

} // namespace historian

#endif //HISTORIAN_DB_H_
