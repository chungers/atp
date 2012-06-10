

library(ib)
library(foreach)
library(IBrokers)

ibg <- ibgConnect()
orderId <-  as.numeric(reqIds(ibg)) + 1100

message('next oder id ', orderId)

load('ContractDb.RData')


appl <- find(cdb, 'AAPL.STK')$contract

message('Connecting to em')

em <- new.em()

limitOrder <- twsOrder(orderId)
limitOrder$lmtPrice <- 300.50

message('sending limit order', limitOrder$orderId)

orderSend(em, appl, limitOrder)

orderId <- orderId + 1
marketOrder <- twsOrder(orderId)
marketOrder$orderType <- 'MKT'

message('sending market order')

orderSend(em, appl, marketOrder)

