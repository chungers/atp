source("utils.R")

library(raptor)

load('firehose_contracts.RData')
stopifnot(is.vector(symbols))

# if invoked with R --vanilla --slave --args local then use the localhost address.
fh <- commandArgs()[5]
ep <- ifelse(!is.na(fh) && fh == 'local', 'tcp://127.0.0.1:6666', 'tcp://69.164.211.61:6666')

message('endpoint = ', ep)

zmq <- raptor.zmq.connect(addr=ep, type='ZMQ_REQ')
result <- fh_cancel_marketData(zmq=zmq, symbols=symbols)
result <- fh_cancel_marketData(zmq=zmq, symbols=indexes)
raptor.zmq.disconnect(zmq)

