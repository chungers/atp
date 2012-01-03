
library(IBrokers)
library(bigmemory)
library(raptor)

options(digits.secs=6)

# Using environment / closure to properly encapsulate the
# marketdata subscriber and its data, state.
# See http://www.lemnica.com/esotericR/Introducing-Closures/
new_marketDataSubscriber <- function(id, endpoint, varz = 18000,
                                     contractDetails,
                                     sharedMemory = TRUE) {

  state <- new.env()

  state$id <- id
  state$endpoint <- endpoint
  state$adminSocket <- 'tcp://127.0.0.1:4444'
  state$eventSocket <- 'tcp://127.0.0.1:4445'
  state$varz <- varz
  state$subscription <- contractDetails
  state$tradingEnd <- ISOdatetime(2011, 12, 30, 12, 0, 0, tz='America/New_York')
  state$sharedMemory <- sharedMemory

  state$.Data <- list()
  state$.Latencies <- list()
  state$.Count <- 0
  state$.hour <- 0

  # State for shared memory implementation
  state$.Shared <- list()
  state$.Desc <- list()

  state$nycTime <- function(t) {
    utc <- as.POSIXct(t, origin='1970-01-01', tz='GMT');
    as.POSIXlt(utc, 'America/New_York');
  }

  # Non-shared memory implementation - data set is local.
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

    if (lt$hour > .hour) {
      message(paste(.Count, topic, lt, evt, val, delay, sep=' '))
      .hour <<- lt$hour
    }

    .Latencies <<- c(.Latencies, delay)
    .Count <<- .Count+1
    return(lt < tradingEnd)
  }

  # Shared memory implementation - file-backed + shared across R sessions.
  state$handler_shm <- function(topic, t, evt, val, delay) {

    lt <- nycTime(t)

    # store as a M*2 matrix of doubles
    # We store the number of observations in the first row of the matrix
    # because we have to preallocate a fixed sized matrix but the actual
    # samples may be well below that size.
    if (is.null(.Shared[[topic]])) {
      .Shared[[topic]] <<- list()
      .Desc[[topic]] <<- list()
    }
    if (is.null(.Shared[[topic]][[evt]])) {
      dir <- '/tmp'
      fn <- paste(topic, evt, sep='_')
      .Shared[[topic]][[evt]] <<-
        filebacked.big.matrix(1000000, 2, backingpath=dir, backingfile=fn)
      .Desc[[topic]][[evt]] <<- paste(paste(dir, fn, sep='/'), 'desc', sep='.')

      # Initialize the first row with the current row count of the matrix
      .Shared[[topic]][[evt]][1, ] <<- c(0, 0)

    } else {
      rows <- .Shared[[topic]][[evt]][1,1]
      .Shared[[topic]][[evt]][rows + 2, ] <<- c(t, val)
      .Shared[[topic]][[evt]][1, ] <<- c(rows + 1, rows + 1)
      message(paste(.Count, topic, lt, evt, val, delay, sep=' '))
    }


    if (lt$hour > .hour) {
      message(paste(.Count, topic, lt, evt, val, delay, sep=' '))
      .hour <<- lt$hour
    }

    .Latencies <<- c(.Latencies, delay)
    .Count <<- .Count+1
    return(lt < tradingEnd)
  }

  state$start <- function() {
    if (sharedMemory) {
      marketdata.start(subscriber, handler_shm)
    } else {
      marketdata.start(subscriber, handler)
    }
    gc()
  }

  # start up of this class
  state$subscriber <-
    marketdata.newSubscriber(id, state$adminSocket, state$eventSocket,
                             endpoint, state$varz)

  marketdata.subscribe(state$subscriber, state$subscription)

  # ensure proper environment for the member functions
  environment(state$handler) <- as.environment(state)
  environment(state$handler_shm) <- as.environment(state)
  environment(state$start) <- as.environment(state)

  class(state) <- 'MarketDataSubscriber'

  return(state)
}

# S3 mechanism for mapping functions as object methods
# see help(UseMethod)
# invocation - start(subscriber)
start <- function(x, ...) UseMethod('start')
start.MarketDataSubscriber <- function(x, ...) x$start()



