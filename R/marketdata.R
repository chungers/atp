
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

marketdata.subscribe(subscriber, list(contractDetails$AAPL))

getVar <- function(n) { get(n, .GlobalEnv) }
setVar <- function(n, v) { assign(n, v, .GlobalEnv) }

count <- 0
max <- 100000000
latencies <- c()
x11()

marketdata.start(subscriber, function(topic, t, event, value, delay) {

  message(paste(count, topic, t, event, value, delay, sep=' '))

  setVar('latencies', c(latencies, delay))
  setVar('count', count + 1)

  hist(latencies)
  return(count < max)
})
