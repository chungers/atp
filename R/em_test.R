

library(ib)
library(foreach)
library(IBrokers)

load('ContractDb.RData')

appl <- find(cdb, 'AAPL.STK')$contract

limitOrder <- twsOrder(1000)
limitOrder$lmtPrice <- 500.50

marketOrder <- twsOrder(1001)
marketOrder$orderType <- 'MKT'

em <- new.em()
marketOrder$orderId

orderSend(em, appl, marketOrder)

