#ifndef HISTORIAN_DB_REACTOR_CLIENT_H_
#define HISTORIAN_DB_REACTOR_CLIENT_H_

#include <boost/scoped_ptr.hpp>
#include <zmq.hpp>

#include "historian/constants.hpp"
#include "historian/Visitor.hpp"

namespace historian {


using std::string;
using zmq::context_t;
using zmq::socket_t;
using proto::historian::QueryByRange;
using proto::historian::QueryBySymbol;


class DbReactorClient
{
 public:
  DbReactorClient(const string& addr, const string& cbAddr,
                  context_t* context);
  ~DbReactorClient();

  bool connect();

  int Query(const QueryByRange& query, Visitor* visitor);

  int Query(const QueryBySymbol& query, Visitor* visitor);

 private:
  string endpoint_;
  string callbackEndpoint_;
  boost::scoped_ptr<socket_t> socket_;
  boost::scoped_ptr<socket_t> callbackSocket_;
};

} // historian

#endif //HISTORIAN_DB_REACTOR_CLIENT_H_
