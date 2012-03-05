#ifndef HISTORIAN_DB_REACTOR_H_
#define HISTORIAN_DB_REACTOR_H_

#include <boost/shared_ptr.hpp>

#include <zmq.hpp>
#include "zmq/Reactor.hpp"
#include "zmq/ZmqUtils.hpp"

#include "historian/Db.hpp"

// ZMQ Reactor for client-server Db queries.

using std::string;
using atp::zmq::Reactor;
using zmq::context_t;
using zmq::socket_t;
namespace historian {

class DbReactorStrategy : public Reactor::Strategy
{
 public:
  DbReactorStrategy(const string& leveldbFile);

  ~DbReactorStrategy();

  bool respond(socket_t& socket);

  int socketType();

  /// Open the database.  Returns false if db cannot be opened.
  bool OpenDb();

 private:
  boost::shared_ptr<historian::Db> db_;
  boost::shared_ptr<context_t> context_;
};

} // historian

#endif //HISTORIAN_DB_REACTOR_H_
