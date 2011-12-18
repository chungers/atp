
#include <string>
#include <vector>
#include <signal.h>
#include <stdlib.h>
#include <gflags/gflags.h>
#include <glog/logging.h>

#include <boost/algorithm/string.hpp>

#include "utils.hpp"
#include "zmq/Publisher.hpp"
#include "zmq/Reactor.hpp"
#include "zmq/ZmqUtils.hpp"

#define DEBUG_LOG VLOG(20)

using namespace std;
using atp::zmq::Publisher;
using atp::zmq::Reactor;

static const string& NOT_SET = "___not_set___";

DEFINE_bool(server, false, "Run as server.");
DEFINE_bool(echo, false, "True to have server echo message received.");
DEFINE_bool(pubsub, false, "True to use pub/sub");
DEFINE_string(endpoint, "tcp://127.0.0.1:5555", "Endpoint");
DEFINE_string(publishEndpoint, "tcp://127.0.0.1:7777", "Publish endpoint");
DEFINE_string(message, NOT_SET, "Comma-delimited key=value pairs.");
DEFINE_string(subscription, "", "Subscription");


struct NoEchoStrategy : Reactor::Strategy
{
  int socketType() { return ZMQ_PULL; }
  bool respond(zmq::socket_t& socket)
  {
    std::string buff;
    bool status = false;
    int seq = 0;
    try {
      DEBUG_LOG << "RECEIVE =====================================" << std::endl;
      while (1) {
        int more = atp::zmq::receive(socket, &buff);
        DEBUG_LOG << "Part[" << seq++ << "]:" << buff << ", more = " << more;
        if (more == 0) break;
      }
      DEBUG_LOG << "frames = " << seq;
      status = true;
    } catch (zmq::error_t e) {
      LOG(WARNING) << "Got exception: " << e.what() << std::endl;
      status = false;
    }
    return status;
  }
};

typedef std::vector<std::string> Message;
typedef std::vector<std::string>::iterator MessageFrames;

struct EchoStrategy : Reactor::Strategy
{
  int socketType() { return ZMQ_REP; }
  bool respond(zmq::socket_t& socket)
  {
    Message message;
    std::string buff;
    bool status = false;
    int seq = 0;
    try {
      DEBUG_LOG << "RECEIVE =====================================" << std::endl;
      while (1) {
        int more = atp::zmq::receive(socket, &buff);
        DEBUG_LOG << "Part[" << seq++ << "]:" << buff << ", more = " << more;
        message.push_back(buff);
        if (more == 0) break;
      }

      DEBUG_LOG << "ECHO ========================================" << std::endl;
      // Now send everything back:
      MessageFrames f = message.begin();
      size_t frames = message.size();
      for (; f != message.end(); ++f) {
        bool more = --seq > 0;
        DEBUG_LOG
            << "Reply[" << (frames-seq) << "]:" << *f << " more = " << more;
        size_t sent = atp::zmq::send_copy(socket , *f, more);
        DEBUG_LOG << " size=" << sent << std::endl;
      }

      status = true;
    } catch (zmq::error_t e) {
      LOG(WARNING) << "Got exception: " << e.what() << std::endl;
      status = false;
    }
    return status;
  }
};


void OnTerminate(int param)
{
  DEBUG_LOG << "===================== SHUTTING DOWN =======================";
  DEBUG_LOG << "Bye.";
  exit(1);
}

/**
 * Simple test server / client that uses the Reactor where client can
 * send comma-delimited key=value pairs as a single dataframe message.
 */
int main(int argc, char** argv)
{
  google::SetUsageMessage("ZMQ Reactor server / client.");
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);

  // Signal handler: Ctrl-C
  signal(SIGINT, OnTerminate);
  // Signal handler: kill -TERM pid
  void (*terminate)(int);
  terminate = signal(SIGTERM, OnTerminate);
  if (terminate == SIG_IGN) {
    DEBUG_LOG << "********** RESETTING SIGNAL SIGTERM";
    signal(SIGTERM, SIG_IGN);
  }

  DEBUG_LOG << "Starting context." << std::endl;

  zmq::context_t context(1);

  if (FLAGS_server) {
    // =========================== SERVER ==========================
    if (FLAGS_pubsub) {

      DEBUG_LOG << "Publisher starting up at "
                << "inbound @" << FLAGS_endpoint
                << ",publish @" << FLAGS_publishEndpoint;

      Publisher publisher(FLAGS_endpoint, FLAGS_publishEndpoint,
                          &context);

      publisher.block();

    } else {

      // Start as a server listening at given endpoint.
      if (FLAGS_echo) {
        EchoStrategy strategy;
        Reactor reactor(FLAGS_endpoint, strategy);
        DEBUG_LOG << "Server started in ECHO mode." << std::endl;
        reactor.block();
      } else {
        NoEchoStrategy strategy;
        Reactor reactor(FLAGS_endpoint, strategy);
        reactor.block();
      }

    }
  } else {
    // =========================== CLIENT ==========================

    // Start as a client sending message to the endpoint.
    int socketType = ZMQ_PUSH;
    if (FLAGS_echo) {
      socketType = ZMQ_REQ;
      DEBUG_LOG << "Socket type = ZMQ_REQ";
    }
    // Special case -- if no message flag specified, then it's a subscriber
    if (FLAGS_message == NOT_SET && FLAGS_pubsub) {
      socketType = ZMQ_SUB;
      DEBUG_LOG << "Socket type = ZMQ_SUB";
    }

    zmq::socket_t client(context, socketType);
    const string& ep = (socketType == ZMQ_SUB) ?
        FLAGS_publishEndpoint : FLAGS_endpoint;

    client.connect(ep.c_str());
    DEBUG_LOG << "Client connected to " << ep ;

    // add subscription
    if (socketType == ZMQ_SUB) {
      DEBUG_LOG << "Client subscribing to " << FLAGS_subscription;
      client.setsockopt(ZMQ_SUBSCRIBE,
                        FLAGS_subscription.c_str(),
                        FLAGS_subscription.length());
    }

    if (socketType != ZMQ_SUB) {
      Message message;
      boost::split(message, FLAGS_message, boost::is_any_of(","));

      MessageFrames f = message.begin();
      int total = message.size();
      int seq = 0;
      DEBUG_LOG << "SEND ==========================================";
      for (; f != message.end(); ++f, ++seq) {
        bool more = seq != total - 1;
        DEBUG_LOG << "Send[" << seq << "]:" << *f << " more = " << more;
        size_t sent = atp::zmq::send_copy(client , *f, more);
        DEBUG_LOG << " size=" << sent << std::endl;
      }
    }
    if (FLAGS_echo || socketType == ZMQ_SUB) {
      std::string buff;
      int seq = 0;

      if (socketType == ZMQ_SUB) {
        while (1) {
          DEBUG_LOG << "SUBSCRIBE =====================================";
          seq = 0;
          std::ostringstream oss;
          while (1) {
            int more = atp::zmq::receive(client, &buff);
            DEBUG_LOG << "Part[" << seq++ << "]:"
                      << buff << ", more = " << more;
            oss << " " << buff;
            if (more == 0) break;
          }
          LOG(INFO) << oss.str() << " " << now_micros();
        }
      } else {
        DEBUG_LOG << "RECEIVE =====================================";
        while (1) {
          int more = atp::zmq::receive(client, &buff);
          DEBUG_LOG << "Part[" << seq++ << "]:" << buff << ", more = " << more;
          if (more == 0) break;
        }
      }
    }

    DEBUG_LOG << "Client finished. Exiting." << std::endl;
  }
}
