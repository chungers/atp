
library(IBrokers)
library(raptor)

options(digits.secs=6)

# Using environment / closure to properly encapsulate the
# marketdata subscriber and its data, state.
# See http://www.lemnica.com/esotericR/Introducing-Closures/
new_marketDataSubscriber <- function(id, endpoint, varz = 18000,
                                     contractDetails) {

  state <- new.env()

  state$id <- id
  state$endpoint <- endpoint
  state$adminSocket <- 'tcp://127.0.0.1:4444'
  state$eventSocket <- 'tcp://127.0.0.1:4445'
  state$varz <- varz
  state$subscription <- contractDetails
  state$tradingEnd <- ISOdatetime(2011, 12, 30, 12, 0, 0, tz='America/New_York')

  state$.Data <- list()
  state$.Latencies <- list()
  state$.Count <- 0

  state$nycTime <- function(t) {
    utc <- as.POSIXct(t, origin='1970-01-01', tz='GMT');
    as.POSIXlt(utc, 'America/New_York');
  }

  state$subscriber <-
    marketdata.newSubscriber(id, state$adminSocket, state$eventSocket,
                             endpoint, state$varz)

  marketdata.subscribe(state$subscriber, state$subscription)

  state$handler <- function(topic, t, evt, val, delay) {

    lt <- nycTime(t)

    sample <- xts(as.numeric(val), lt, tzone='America/New_York')

    if (is.null(.Data[[topic]])) {
      .Data[[topic]] <<- list()
    }

    if (is.null(.Data[[topic]][[evt]])) {
      .Data[[topic]][[evt]] <<- sample
      names(.Data[[topic]][[evt]]) <<- c(paste(topic,evt, sep=':'))
    } else {
      .Data[[topic]][[evt]] <<- c(.Data[[topic]][[evt]], sample)
    }

    message(paste(.Count, topic, lt, evt, val, delay, sep=' '))

    .Latencies <<- c(.Latencies, delay)
    .Count <<- .Count+1
    return(lt < tradingEnd)
  }

  state$start <- function() {
    marketdata.start(subscriber, handler)
    gc()
  }

  environment(state$handler) <- as.environment(state)
  environment(state$start) <- as.environment(state)

  class(state) <- 'MarketDataSubscriber'

  return(state)
}

# S3 mechanism for mapping functions as object methods
# see help(UseMethod)
# invocation - start(subscriber)
start <- function(x, ...) UseMethod('start')
start.MarketDataSubscriber <- function(x, ...) x$start()



