#ifndef ATP_MANAGED_AGENT_H_
#define ATP_MANAGED_AGENT_H_

#include <string>
#include <zmq.hpp>

#include <boost/bind.hpp>
#include <boost/function.hpp>

#include "varz/varz.hpp"

DEFINE_VARZ_string(managed_id, "", "");


namespace atp {


using namespace std;

typedef boost::function< bool(const string&, const string&) > AdminHandler;
typedef map< string, AdminHandler > HandlerMap;


class ManagedAgent
{
 public:
  ManagedAgent(const string& id, const string& adminEndpoint) :
      id_(id), adminEndpoint_(adminEndpoint)
  {
    VARZ_managed_id = id;
  }

  ~ManagedAgent()
  {
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

      }
    }

    return ready;
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
      LOG(INFO) << "Admin message: " << verb << " " << data;

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

};


} // namespace atp


#endif //ATP_MANAGED_AGENT_H_
