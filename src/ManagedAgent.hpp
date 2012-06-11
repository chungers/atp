#ifndef ATP_MANAGED_AGENT_H_
#define ATP_MANAGED_AGENT_H_

#include <string>
#include <sstream>

#include <boost/bind.hpp>
#include <boost/function.hpp>

#include "log_levels.h"
#include "varz/varz.hpp"
#include "varz/VarzServer.hpp"
#include "zmq/ZmqUtils.hpp"


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
               const string& eventEndpoint,
               int varzPort) :
      id_(id), adminEndpoint_(adminEndpoint),
      varzPort_(varzPort), varz_(varzPort, 1),
      running_(false),
      eventSocketPtr_(NULL),
      eventContextPtr_(NULL),
      eventEndpoint_(eventEndpoint)
  {
    VARZ_managed_id = id;
    varz_.start();
    running_ = true;
    MANAGED_AGENT_LOGGER << "Started varz server at port " << varzPort;
  }

  ~ManagedAgent()
  {
    stop();
    // Now stop the event context
    if (eventSocketPtr_ != NULL) {
      delete eventSocketPtr_;
    }
    if (eventContextPtr_ != NULL) {
      delete eventContextPtr_;
    }
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

        // Subscribe to admin messages from coordinator
        socketPtr->connect(adminEndpoint_.c_str());
        socketPtr->setsockopt(ZMQ_SUBSCRIBE,
                              id_.c_str(), id_.length());

        // Notify start up of the managed agent through the event socket
        ::zmq::context_t* context = getEventSocketContext();
        if (context == NULL) {
          eventContextPtr_ = new ::zmq::context_t(1);
          context = eventContextPtr_;
        }
        eventSocketPtr_ = new ::zmq::socket_t(*context, ZMQ_PUSH);
        eventSocketPtr_->connect(eventEndpoint_.c_str());

        reportEventReady();

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

  virtual ::zmq::context_t* getEventSocketContext()
  {
    return NULL;
  }

 private:
  void reportEventReady()
  {
    if (eventSocketPtr_ != NULL) {
      const string verb = "UP";
      atp::zmq::send_copy(*eventSocketPtr_, verb, true);
      atp::zmq::send_copy(*eventSocketPtr_, id_, true);

      std::ostringstream oss;
      oss << "http://localhost:" << varzPort_ << "/varz";
      atp::zmq::send_copy(*eventSocketPtr_, oss.str(), false);
    }
  }

 private:
  string id_;
  string adminEndpoint_;
  HandlerMap adminHandlers_;
  int varzPort_;
  atp::varz::VarzServer varz_;
  bool running_;
  ::zmq::socket_t* eventSocketPtr_;
  ::zmq::context_t* eventContextPtr_;
  string eventEndpoint_;
};


} // namespace atp


#endif //ATP_MANAGED_AGENT_H_
