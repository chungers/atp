
source('marketdata.R') # loads the definition of functions for marketdata client

# if invoked with R --vanilla --slave --args local
# then use the localhost address.
fh <- commandArgs()[5]
fh <- 'local'
ep <- ifelse(!is.na(fh) && fh == 'local',
             'tcp://127.0.0.1:7777',
             'tcp://69.164.211.61:7777')

load('firehose_contracts.RData')

message('endpoint = ', ep)

subscriber <- new_marketDataSubscriber('test-subscriber', ep, 18001,
                                       list(contractDetails$AAPL),
                                       sharedMemory=TRUE)
start(subscriber)
