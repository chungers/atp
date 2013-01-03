/// Simple main to set up market data subscription

#include <iostream>
#include <vector>
#include <signal.h>
#include <gflags/gflags.h>
#include <glog/logging.h>

#include <boost/algorithm/string.hpp>

#include "ZmqProtoBuffer.hpp"
#include "common/time_utils.hpp"
#include "platform/contract_symbol.hpp"
#include "proto/ib.pb.h"
#include "service/ContractManager.hpp"
#include "main/global_defaults.hpp"



DEFINE_string(fhEp, atp::global::FH_CONTROLLER_ENDPOINT,
              "Firehose (fh) reactor endpoint");
DEFINE_string(cmEp, atp::global::CM_CONTROLLER_ENDPOINT,
              "ContractManager (cm) reactor endpoint");
DEFINE_string(cmPublishEp, atp::global::CM_OUTBOUND_ENDPOINT,
              "ContractManager (cm) publish outbound endpoint");
DEFINE_bool(unsubscribe, false,
            "True to unsubscribe");
DEFINE_string(symbols, "",
              "Comma-delimited list of stocks to subscribe.");
DEFINE_bool(optionChain, false,
            "True to request option chain for each stock symbol");
DEFINE_int64(requestTimeoutMillis, 10000,
             "Timeout for requesting contract details in milliseconds.");
DEFINE_bool(subscribe, true,
            "True to start subscription via FH");
DEFINE_bool(depth, false,
            "True to request depth in addition to marketdata");

void OnTerminate(int param)
{
  LOG(INFO) << "===================== SHUTTING DOWN =======================";
  LOG(INFO) << "Bye.";
  exit(1);
}

using std::string;
using std::vector;
namespace platform = atp::platform;
namespace service = atp::service;
namespace p = proto::ib;


int main(int argc, char** argv)
{
  google::SetUsageMessage("Data subscription setup");
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

  // Start the contract manager:
  service::ContractManager cm_client(FLAGS_cmEp, FLAGS_cmPublishEp);

  LOG(INFO) << "ContractManager started.";

  // Connect to Firehose
  ::zmq::context_t ctx(1);
  ::zmq::socket_t fh_socket(ctx, ZMQ_PUSH);

  try {

    LOG(INFO) << "Connecting to fh @ " << FLAGS_fhEp;
    fh_socket.connect(FLAGS_fhEp.c_str());

  } catch (::zmq::error_t e) {

    LOG(FATAL) << "Cannot connect to FH: " << e.what();

  }

  // For each symbol in the list
  // 1. Check with the ContractManager via find() to see
  // if there's a contract in storage.
  // 2. If yes, then make a marketdata subscription request.
  // If not, then request contract details from ContractManager
  // which will talk to the CM gateway.

  // symbols are in the form of the security key, e.g. APPL.STK
  vector<string> keys;
  boost::split(keys, FLAGS_symbols, boost::is_any_of(","));

  int request = 0;
  for (vector<string>::const_iterator itr = keys.begin();
       itr != keys.end(); ++itr, ++request) {

    p::Contract contract;
    if (cm_client.findContract(*itr, &contract)) {

      if (FLAGS_unsubscribe) {
        // Now cancel a subscription:
        p::CancelMarketData req;
        req.mutable_contract()->CopyFrom(contract);

        size_t sent = atp::send<p::CancelMarketData>(fh_socket, req);
        if (sent > 0) {
          LOG(INFO) << "Canceled marketdata for " << *itr;
        }

      } else if (FLAGS_subscribe) {
        // Now make a subscription:
        p::RequestMarketData req;
        req.mutable_contract()->CopyFrom(contract);

        size_t sent = atp::send<p::RequestMarketData>(fh_socket, req);
        if (sent > 0) {
          LOG(INFO) << "Requested marketdata for " << *itr;
        }
      }

    } else {

      // Don't have this symbol.  Request it from the gateway
      LOG(INFO) << "No contract found for " << *itr << " querying cm.";

      // Symbol looks like AAPL.OPT.20121216.500.C
      string sym;
      p::Contract::Type sec_type;
      p::Contract::Right right;
      double strike;
      int year, month, day;

      if (!platform::parse_encoded_symbol(
              *itr,
              &sym, &sec_type, &strike, &right, &year, &month, &day)) {
        LOG(ERROR) << "Bad symbol: " << *itr << ", skipping.";
        continue;
      }

      service::AsyncContractDetailsEnd res, chain;
      switch (sec_type) {
        case p::Contract::STOCK:
          res = cm_client.requestStockContractDetails(request, sym);
          if (FLAGS_optionChain) {
            chain = cm_client.requestOptionChain(++request, sym);

            chain->get(FLAGS_requestTimeoutMillis);
            if (!chain->is_ready()) {
              LOG(WARNING) << "Option chain taking too long: " << sym;
            }
          }
          break;
        case p::Contract::OPTION:
          res = cm_client.requestOptionContractDetails(
              request, sym,
              boost::gregorian::date(year, month, day), strike, right);
          break;
        case p::Contract::INDEX:
          res = cm_client.requestIndexContractDetails(request, sym);
          break;
      }

      res->get(FLAGS_requestTimeoutMillis);
      if (!res->is_ready()) {
        LOG(WARNING) << "Getting contract detail timed out: " << *itr;
      } else {
        p::Contract c;
        if (cm_client.findContract(*itr, &c)) {
          LOG(INFO) << "Found contract detail for " << *itr;

          if (FLAGS_unsubscribe) {
            // Now cancel a subscription:
            p::CancelMarketData req;
            req.mutable_contract()->CopyFrom(c);

            size_t sent = atp::send<p::CancelMarketData>(fh_socket, req);
            if (sent > 0) {
              LOG(INFO) << "Canceled marketdata for " << *itr;
            }

          } else if (FLAGS_subscribe) {
            // Now make a subscription:
            p::RequestMarketData req;
            req.mutable_contract()->CopyFrom(c);

            size_t sent = atp::send<p::RequestMarketData>(fh_socket, req);
            if (sent > 0) {
              LOG(INFO) << "Requested marketdata for " << *itr;

              // also for market depth
              if (FLAGS_depth) {
                p::RequestMarketDepth depth;
                depth.mutable_contract()->CopyFrom(c);
                sent = atp::send<p::RequestMarketDepth>(fh_socket, depth);
                if (sent > 0) {
                  LOG(INFO) << "Requested marketdepth for " << *itr;
                } else {
                  LOG(ERROR) << "Failed to request depth for " << *itr;
                }
              }
            } else {
              LOG(ERROR) << "Failed to request marketdata for " << *itr;
            }
          }
        } else {
          LOG(INFO) << "No contract information after requesting "
                    << "contract details.  "
                    << "Contract definition not known to IB: "
                    << *itr;
        }
      }
    }
  }
}
