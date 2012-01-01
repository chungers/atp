#ifndef ATP_MANAGED_AGENT_H_
#define ATP_MANAGED_AGENT_H_

#include <string>
#include <zmq.hpp>

#include <boost/bind.hpp>
#include <boost/function.hpp>

#include "log_levels.h"
#include "varz/varz.hpp"
#include "varz/VarzServer.hpp"

DEFINE_VARZ_string(managed_id, "", "");


namespace atp {

using namespace std;

typedef boost::function< bool(const string&, const string&) > AdminHandler;
typedef map< string, AdminHandler > HandlerMap;


/// Base class for a managed agent which
/// 1. Listens as a pubsub client on an admin socket for admin messages
///    addressed to the agent (with the agent's id as the topic).
/// 2. Broadcasts identity and varz information to a known port (PUSH)
///    so that all agents running on a server can be discovered.
class ManagedAgent
{
 public:
  ManagedAgent(const string& id, const string& adminEndpoint,
               int varzPort) :
      id_(id), adminEndpoint_(adminEndpoint),
      varz_(varzPort, 1), running_(false)
  {
    VARZ_managed_id = id;
    varz_.start();
    running_ = true;
    MANAGED_AGENT_LOGGER << "Started varz server at port " << varzPort;
  }

  ~ManagedAgent()
  {
    stop();
  }

  /// Must be called explicitly to set up the managed agent.
  bool initialize()
  {
    bool ready = false;

    ::zmq::socket_t* socketPtr = getAdminSocket();
    if (socketPtr != NULL) {
      // connect to the admin socket and register self as the topic to
      // listen to.
      try {

        socketPtr->connect(adminEndpoint_.c_str());
        socketPtr->setsockopt(ZMQ_SUBSCRIBE,
                              id_.c_str(), id_.length());

        ready = true;

      } catch (::zmq::error_t e) {

        MANAGED_AGENT_ERROR << "Error: " << e.what();

      }
    }

    return ready;
  }

  virtual bool stop()
  {
    if (running_) {
      varz_.stop();
      running_ = false;
    }
    return running_;
  }

  const string& getId()
  {
    return id_;
  }

  const string& getAdminEndpoint()
  {
    return adminEndpoint_;
  }

  bool isAdminMessage(const string& topic,
                      const string& verb, const string& data)
  {
    return topic == id_;
  }

  bool handleAdminMessage(const string& verb, const string& data)
  {
    bool response = true;
    if (adminHandlers_.find(verb) != adminHandlers_.end()) {
      // This is an admin message.
      MANAGED_AGENT_LOGGER << "Admin message: " << verb << " " << data;

      response = adminHandlers_[verb](verb, data);
    }
    return response;
  }

 protected:

  void registerHandler(const string& verb, AdminHandler handler)
  {
    adminHandlers_[verb] = handler;
  }

  virtual ::zmq::socket_t* getAdminSocket()
  {
    return NULL;
  }

 private:
  string id_;
  string adminEndpoint_;
  HandlerMap adminHandlers_;
  atp::varz::VarzServer varz_;
  bool running_;
};


} // namespace atp


#endif //ATP_MANAGED_AGENT_H_
