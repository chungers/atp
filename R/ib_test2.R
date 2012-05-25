source('config.R')
load(CONFIG$contractDb)

aapl <- find(cdb, 'AAPL.STK')$contract

library('ib')
ib <- new.ib()
ib

r <- cancelMarketdata(ib, aapl)

