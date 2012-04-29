
# Assumes a local ssh tunnel is open per
# https://github.com/lab616/hub/wiki/Historian:---the-Market-Data-Backend
#
new.hzc <- function(address = 'tcp://127.0.0.1:1111',
                    callbackAddress = 'tcp://127.0.0.1:1112') {

  library(xts)
  options(digits.secs=6)

  this <- new.env()
  class(this) <- 'hzc'

  this$.historianAddress <- as.character(address)
  this$.callbackAddress <- as.character(callbackAddress)

  # connect
  this$.connection <- .Call("hzc_connect",
                            this$.historianAddress,
                            this$.callbackAddress, PACKAGE='hzc')

  nycTime <- function(t) {
    utc <- as.POSIXct(t, origin='1970-01-01', tz='GMT');
    as.POSIXlt(utc, 'America/New_York');
  }

  # retrieve marketdata
  this$mktdata <- function(symbol, event, qStart, qStop,
                           callback=NULL, est=TRUE) {
    raw <- .Call("hzc_query",
                 this$.connection,
                 as.character(symbol), as.character(event),
                 as.character(qStart), as.character(qStop),
                 callback, est,
                 PACKAGE="hzc")

    as.xts(raw$value, order.by=nycTime(raw$utc_t),
           tzone='America/New_York')
  }
  environment(this$mktdata) <- as.environment(this)

  return(this)
}

# S3 function mapping
mktdata <- function(x, symbol, event, qStart, qStop,
                    callback = NULL, est = TRUE)
  UseMethod('mktdata')

mktdata.hzc <- function(x, symbol, event, qStart, qStop,
                        callback = NULL, est = TRUE)
  x$mktdata(symbol, event, qStart, qStop, callback, est)

