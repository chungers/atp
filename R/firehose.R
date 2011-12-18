library(IBrokers)
library(raptor)

davidc415 <- twsConnect(clientId=4444, port=4001)
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

save.image('firehose.RData')




