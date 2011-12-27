
library(IBrokers)
library(raptor)

options(digits.secs=6)

load('firehose_contracts.RData')

endpoint <- 'tcp://127.0.0.1:7777'

marketdata.subscribe(endpoint,
                     list(contractDetails$AAPL,
                          contractDetails$GOOG))
