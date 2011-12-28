
library(IBrokers)
library(raptor)

options(digits.secs=6)


# if invoked with R --vanilla --slave --args local
# then use the localhost address.
fh <- commandArgs()[5]
ep <- ifelse(!is.na(fh) && fh == 'local',
             'tcp://127.0.0.1:7777',
             'tcp://69.164.211.61:7777')

load('firehose_contracts.RData')

message('endpoint = ', ep)

subscriber <- marketdata.newSubscriber(ep)

marketdata.subscribe(subscriber, list(contractDetails$AAPL,
                                      contractDetails$GOOG))

marketdata.start(subscriber)
