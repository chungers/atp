source("utils.R")

library(raptor)

load('firehose_contracts.RData')
stopifnot(is.vector(symbols))

zmq <- raptor.zmq.connect(addr='tcp://69.164.211.61:6666', type='ZMQ_REQ')
result <- fh_cancel_marketData(zmq=zmq, symbols=symbols)
raptor.zmq.disconnect(zmq)

