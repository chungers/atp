source("utils.R")

library(raptor)

load('firehose_contracts.RData')

# request market data for all symbols
stopifnot(is.vector(symbols))

# if invoked with R --vanilla --slave --args local
# then use the localhost address.
fh <- commandArgs()[5]
ep <- ifelse(!is.na(fh) && fh == 'local',
             'tcp://127.0.0.1:6666',
             'tcp://69.164.211.61:6666')

message('endpoint = ', ep)

zmq <- raptor.zmq.connect(addr=ep, type='ZMQ_REQ')
result <- fh_marketData(zmq=zmq, symbols=symbols)
result <- fh_marketData(zmq=zmq, symbols=indexes)

# market depth: IB allows only three at a time.
book <- c('AAPL', 'GOOG', 'SPY')

sapply(book, function(symbol) {
  firehose.req_marketdepth(zmq, contractDetails[[symbol]]$contract)
})

raptor.zmq.disconnect(zmq)


