
library(IBrokers)
library(raptor)

options(digits.secs=6)

# convenience functions
getVar <- function(n) { get(n, .GlobalEnv) }

setVar <- function(n, v) { assign(n, v, .GlobalEnv) }

nycTime <- function(t) {
  utc <- as.POSIXct(t, origin='1970-01-01', tz='GMT');
  as.POSIXlt(utc, 'America/New_York');
}


# if invoked with R --vanilla --slave --args local
# then use the localhost address.
fh <- commandArgs()[5]
ep <- ifelse(!is.na(fh) && fh == 'local',
             'tcp://127.0.0.1:7777',
             'tcp://69.164.211.61:7777')

load('firehose_contracts.RData')

message('endpoint = ', ep)

subscriber <- marketdata.newSubscriber(ep)

marketdata.subscribe(subscriber,
                     list(contractDetails$AAPL));

# initial state
count <- 0
DATA <- list()
latencies <- c()
#x11()

tradingEnd <- ISOdatetime(2011, 12, 29, 16, 0, 0, tz='America/New_York')

marketdata.start(subscriber, function(topic, t, evt, val, delay) {

  lt <- nycTime(t)
  sample <- xts(as.numeric(val), lt, tzone='America/New_York')
  if (is.null(DATA[[topic]])) {
    DATA[[topic]] <- list()
  }
  if (is.null(DATA[[topic]][[evt]])) {
    DATA[[topic]][[evt]] <- sample
    names(DATA[[topic]][[evt]]) <- c(paste(topic,evt, sep=':'))
  } else {
    DATA[[topic]][[evt]] <- c(DATA[[topic]][[evt]], sample)
  }
  setVar('DATA', DATA)

  message(paste(count, topic, lt, evt, val, delay, sep=' '))

  setVar('latencies', c(latencies, delay))
  setVar('count', count + 1)

  #hist(latencies)
  return(lt < tradingEnd)
})

save.image('2011_12_29_AAPL.RData')
