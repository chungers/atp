library(IBrokers)
library(raptor)

load('firehose.RData')

zmq <- raptor.zmq.connect(addr='tcp://69.164.211.61:6666', type='ZMQ_REQ')

raptor.firehose.req_marketdata(zmq, aapl)
raptor.firehose.req_marketdata(zmq, goog)
raptor.firehose.req_marketdata(zmq, lnkd)
raptor.firehose.req_marketdata(zmq, znga)





