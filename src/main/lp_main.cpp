/// Simple main to read a log file and publish

#include <iostream>
#include <signal.h>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <zmq.hpp>

#include "service/LogReader.hpp"
#include "service/LogReaderVisitor.hpp"

#include "main/global_defaults.hpp"
#include "zmq/ZmqUtils.hpp"


DEFINE_string(pub_host, atp::global::LP_HOST,
              "Market data publish host");
DEFINE_int32(pub_port, atp::global::LP_OUTBOUND_PORT,
              "Market data publish port");
DEFINE_string(logfile, "",
              "Name of the logfile.");
DEFINE_bool(rth, true,
            "Regular trading hours.");
DEFINE_bool(est, true,
            "Eastern timezone.");
DEFINE_bool(send_stop, true,
            "Sends stop event at end of logfile.");

void OnTerminate(int param)
{
  LOG(INFO) << "===================== SHUTTING DOWN =======================";
  LOG(INFO) << "Bye.";
  exit(1);
}

using std::string;
using std::vector;

namespace service = atp::log_reader;

int main(int argc, char** argv)
{
  google::SetUsageMessage("Log data publisher");
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);

  // Signal handler: Ctrl-C
  signal(SIGINT, OnTerminate);
  // Signal handler: kill -TERM pid
  void (*terminate)(int);
  terminate = signal(SIGTERM, OnTerminate);
  if (terminate == SIG_IGN) {
    signal(SIGTERM, SIG_IGN);
  }

  // Start the log reader:
  service::LogReader reader(FLAGS_logfile, FLAGS_rth, FLAGS_est);

  string pub_endpoint = atp::zmq::EndPoint::tcp(FLAGS_pub_port, FLAGS_pub_host);
  LOG(INFO) << "Starting publish endpoint at " << pub_endpoint;

  ::zmq::context_t context(1);
  ::zmq::socket_t pub_socket(context, ZMQ_PUB);
  pub_socket.bind(pub_endpoint.c_str());

  LOG(INFO) << "Bound publisher socket at " << pub_endpoint;

  service::visitor::MarketDataDispatcher p1(&pub_socket);
  service::visitor::MarketDepthDispatcher p2(&pub_socket);

  service::LogReader::marketdata_visitor_t m1 = p1;
  service::LogReader::marketdepth_visitor_t m2 = p2;

  LOG(INFO) << "Start scanning the log file " << FLAGS_logfile;

  size_t processed = reader.Process(m1, m2);
  LOG(INFO) << "processed " << processed;

  if (FLAGS_send_stop) {
    atp::zmq::send_copy(pub_socket, atp::log_reader::DATA_END, true);
    atp::zmq::send_copy(pub_socket, atp::log_reader::DATA_END, false);
  }
  pub_socket.close();
}
