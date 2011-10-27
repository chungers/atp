
#include <string>
#include <vector>
#include <signal.h>
#include <stdlib.h>
#include <gflags/gflags.h>
#include <glog/logging.h>

#include <boost/algorithm/string.hpp>

#include "zmq/Reactor.hpp"
#include "zmq/ZmqUtils.hpp"


using namespace std;


DEFINE_bool(server, false, "Run as server.");
DEFINE_bool(echo, false, "True to have server echo message received.");
DEFINE_string(endpoint, "tcp://127.0.0.1:5555", "Endpoint");
DEFINE_string(message, "k1=hello,k2=world", "Comma-delimited key=value pairs.");

struct NoEchoStrategy : atp::zmq::Reactor::Strategy
{
  int socketType() { return ZMQ_PULL; }
  bool respond(zmq::socket_t& socket)
  {
    std::string buff;
    bool status = false;
    int seq = 0;
    try {
      LOG(INFO) << "RECEIVE =====================================" << std::endl;
      while (1) {
        int more = atp::zmq::receive(socket, &buff);
        LOG(INFO) << "Part[" << seq++ << "]:" << buff << ", more = " << more;
        if (more == 0) break;
      }
      LOG(INFO) << "frames = " << seq;
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

struct EchoStrategy : atp::zmq::Reactor::Strategy
{
  int socketType() { return ZMQ_REP; }
  bool respond(zmq::socket_t& socket)
  {
    Message message;
    std::string buff;
    bool status = false;
    int seq = 0;
    try {
      LOG(INFO) << "RECEIVE =====================================" << std::endl;
      while (1) {
        int more = atp::zmq::receive(socket, &buff);
        LOG(INFO) << "Part[" << seq++ << "]:" << buff << ", more = " << more;
        message.push_back(buff);
        if (more == 0) break;
      }

      LOG(INFO) << "ECHO ========================================" << std::endl;
      // Now send everything back:
      MessageFrames f = message.begin();
      size_t frames = message.size();
      for (; f != message.end(); ++f, --seq) {
        bool more = true; //seq != 0;
        LOG(INFO)
            << "Reply[" << (frames-seq) << "]:" << *f << " more = " << more;
        size_t sent = atp::zmq::send_copy(socket , *f, more);
        LOG(INFO) << " size=" << sent << std::endl;
      }
      atp::zmq::send_copy(socket ,"", false);

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
  LOG(INFO) << "===================== SHUTTING DOWN =======================";
  LOG(INFO) << "Bye.";
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
    LOG(INFO) << "********** RESETTING SIGNAL SIGTERM";
    signal(SIGTERM, SIG_IGN);
  }

  LOG(INFO) << "Starting context." << std::endl;

  zmq::context_t context(1);

  if (FLAGS_server) {
    // Start as a server listening at given endpoint.
    if (FLAGS_echo) {
      EchoStrategy strategy;
      atp::zmq::Reactor reactor(FLAGS_endpoint, strategy);
      LOG(INFO) << "Server started in ECHO mode." << std::endl;
      reactor.block();
    } else {
      NoEchoStrategy strategy;
      atp::zmq::Reactor reactor(FLAGS_endpoint, strategy);
      reactor.block();
    }

  } else {
    // Start as a client sending message to the endpoint.
    zmq::socket_t client(context, (FLAGS_echo) ? ZMQ_REQ : ZMQ_PUSH);
    client.connect(FLAGS_endpoint.c_str());
    LOG(INFO) << "Client connected." << std::endl;

    Message message;
    boost::split(message, FLAGS_message, boost::is_any_of(","));

    MessageFrames f = message.begin();
    int total = message.size();
    int seq = 0;
    LOG(INFO) << "SEND ==========================================" << std::endl;
    for (; f != message.end(); ++f, ++seq) {
      bool more = seq != total - 1;
      LOG(INFO) << "Send[" << seq << "]:" << *f << " more = " << more;
      size_t sent = atp::zmq::send_copy(client , *f, more);
      LOG(INFO) << " size=" << sent << std::endl;
    }

    if (FLAGS_echo) {
      LOG(INFO) << "RECEIVE =====================================" << std::endl;
      std::string buff;
      seq = 0;
      while (1) {
        int more = atp::zmq::receive(client, &buff);
        LOG(INFO) << "Part[" << seq++ << "]:" << buff << ", more = " << more;
        if (more == 0) break;
      }
    }

    LOG(INFO) << "Client finished. Exiting." << std::endl;
  }
}
