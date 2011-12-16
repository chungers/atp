library(IBrokers)
library(raptor)

davidc415 <- twsConnect(clientId=4444, port=5001)
goog <- twsSTK(symbol='GOOG')
goog_details <- reqContractDetails(davidc415, goog)
goog <- goog_details[[1]]$contract

znga <- twsSTK(symbol='ZNGA')
znga_details <- reqContractDetails(davidc415, znga)
znga <- znga_details[[1]]$contract

aapl <- twsSTK(symbol='AAPL')
aapl_details <- reqContractDetails(davidc415, aapl)
aapl <- aapl_details[[1]]$contract

lnkd <- twsSTK(symbol='LNKD')
lnkd_details <- reqContractDetails(davidc415, lnkd)
linkd <- lnkd_details[[1]]$contract


zmq <- raptor.zmq.connect(addr='tcp://69.164.211.61:6666', type='ZMQ_REQ')

raptor.firehose.req_marketdata(zmq, aapl)
raptor.firehose.req_marketdata(zmq, goog)
raptor.firehose.req_marketdata(zmq, lnkd)
raptor.firehose.req_marketdata(zmq, znga)




