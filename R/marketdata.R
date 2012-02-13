
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

  this <- new.env()

  this$id <- id
  this$endpoint <- endpoint
  this$adminSocket <- 'tcp://127.0.0.1:4444'
  this$eventSocket <- 'tcp://127.0.0.1:4445'
  this$varz <- varz
  this$subscription <- contractDetails
  this$tradingEnd <- NULL
  this$sharedMemory <- sharedMemory

  this$.Data <- list()
  this$.Latencies <- list()
  this$.Count <- 0
  this$.hour <- 0

  # This for shared memory implementation
  this$.Shared <- list()
  this$.Desc <- list()

  this$nycTime <- function(t) {
    utc <- as.POSIXct(t, origin='1970-01-01', tz='GMT');
    as.POSIXlt(utc, 'America/New_York');
  }

  # Non-shared memory implementation - data set is local.
  this$handler <- function(topic, t, evt, val, delay) {

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
    if (is.null(tradingEnd)) {
      tradingEnd <<- ISOdatetime(lt$year + 1900, lt$mon + 1, lt$mday,
                                 16, 0, 0, tz='America/New_York')
    }
    return(lt < tradingEnd)
  }

  # Shared memory implementation - file-backed + shared across R sessions.
  this$handler_shm <- function(topic, t, evt, val, delay) {

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
      fn <- paste(topic, lt$year + 1900, lt$mon + 1, lt$mday, evt, sep='_')
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
    if (is.null(tradingEnd)) {
      tradingEnd <<- ISOdatetime(lt$year + 1900, lt$mon + 1, lt$mday,
                                 16, 0, 0, tz='America/New_York')
    }
    return(lt < tradingEnd)
  }

  this$start <- function() {
    if (sharedMemory) {
      marketdata.start(subscriber, handler_shm)
    } else {
      marketdata.start(subscriber, handler)
    }
    gc()
  }

  # start up of this class
  this$subscriber <-
    marketdata.newSubscriber(id, this$adminSocket, this$eventSocket,
                             endpoint, this$varz)

  marketdata.subscribe(this$subscriber, this$subscription)

  # ensure proper environment for the member functions
  environment(this$handler) <- as.environment(this)
  environment(this$handler_shm) <- as.environment(this)
  environment(this$start) <- as.environment(this)

  class(this) <- 'MarketDataSubscriber'

  return(this)
}

# S3 mechanism for mapping functions as object methods
# see help(UseMethod)
# invocation - start(subscriber)
start <- function(x, ...) UseMethod('start')
start.MarketDataSubscriber <- function(x, ...) x$start()



