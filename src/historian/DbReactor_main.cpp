

#include <signal.h>
#include <stdio.h>
#include <zmq.hpp>

#include <gflags/gflags.h>
#include <glog/logging.h>

#include "log_levels.h"
#include "varz/varz.hpp"
#include "zmq/Reactor.hpp"

#include "historian/DbReactorStrategy.hpp"


void OnTerminate(int param)
{
  LOG(INFO) << "===================== SHUTTING DOWN =======================";
  LOG(INFO) << "Bye.";
  exit(1);
}


DEFINE_string(ep, "tcp://127.0.0.1:1111", "Reactor port");
DEFINE_string(leveldb, "", "Leveldb");


////////////////////////////////////////////////////////
//
// MAIN
//
int main(int argc, char** argv)
{
  google::SetUsageMessage("ZMQ Reactor for historian db");
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);
  atp::varz::Varz::initialize();

  // Signal handler: Ctrl-C
  signal(SIGINT, OnTerminate);
  // Signal handler: kill -TERM pid
  void (*terminate)(int);
  terminate = signal(SIGTERM, OnTerminate);
  if (terminate == SIG_IGN) {
    signal(SIGTERM, SIG_IGN);
  }

  historian::DbReactorStrategy strategy(FLAGS_leveldb);
  if (!strategy.OpenDb()) {
    LOG(FATAL) << "Cannot open db: " << FLAGS_leveldb;
  }

  // continue
  LOG(INFO) << "Starting context.";
  zmq::context_t context(1);

  atp::zmq::Reactor reactor(FLAGS_ep, strategy, &context);
  reactor.block();
}
