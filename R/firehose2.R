library(IBrokers)
library(raptor)

load('firehose_contracts.RData')

zmq <- raptor.zmq.connect(addr='tcp://127.0.0.1:6666', type='ZMQ_REQ')

raptor.firehose.req_marketdata(zmq, contractDetails$GOOG$contract)
raptor.firehose.req_marketdata(zmq, contractDetails$AAPL$contract)
raptor.firehose.req_marketdata(zmq, contractDetails$BMC$contract)
raptor.firehose.req_marketdata(zmq, contractDetails$IBM$contract)
raptor.firehose.req_marketdata(zmq, contractDetails$MSFT$contract)
raptor.firehose.req_marketdata(zmq, contractDetails$LNKD$contract)
raptor.firehose.req_marketdata(zmq, contractDetails$GRPN$contract)
raptor.firehose.req_marketdata(zmq, contractDetails$ZNGA$contract)
raptor.firehose.req_marketdata(zmq, contractDetails$RIMM$contract)
raptor.firehose.req_marketdata(zmq, contractDetails$RTH$contract)
raptor.firehose.req_marketdata(zmq, contractDetails$OPEN$contract)
raptor.firehose.req_marketdata(zmq, contractDetails$NFLX$contract)
raptor.firehose.req_marketdata(zmq, contractDetails$BIDU$contract)




