source("utils.R")

library(raptor)

load('firehose_contracts.RData')

#zmq <- raptor.zmq.connect(addr='tcp://127.0.0.1:6666', type='ZMQ_REQ')
zmq <- raptor.zmq.connect(addr='tcp://69.164.211.61:6666', type='ZMQ_REQ')
# request market data for all symbols
stopifnot(is.vector(symbols))
result <- fh_marketData(zmq=zmq, symbols=symbols)
raptor.zmq.disconnect(zmq)


