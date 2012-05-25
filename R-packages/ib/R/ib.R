
#
new.ib <- function(apiAddress = 'tcp://127.0.0.1:6666') {

  options(digits.secs=6)
  Sys.setenv(TZ='America/New_York')

  this <- new.env()
  class(this) <- 'ib'

  this$.apiAddress = apiAddress

  this$.apiConnection <- .Call("api_connect", this$.apiAddress, PACKAGE='ib')

  nycTime <- function(t) {
    utc <- as.POSIXct(t, origin='1970-01-01', tz='GMT');
    as.POSIXlt(utc, 'America/New_York');
  }

  # Request marketdata
  this$requestMarketdata <- function(contract, tick_types="", snapshot=FALSE) {
    raw <- .Call("api_request_marketdata",
                 this$.apiConnection, contract, tick_types, snapshot,
                 PACKAGE="ib")
    return(raw)
  }
  environment(this$requestMarketdata) <- as.environment(this)

  # Cancel marketdata
  this$cancelMarketdata <- function(contract) {
    raw <- .Call("api_cancel_marketdata",
                 this$.apiConnection, contract,
                 PACKAGE="ib")
    return(raw)
  }
  environment(this$cancelMarketdata) <- as.environment(this)

  return(this)
}


# S3 function mapping

# RequestMarketdata
requestMarketdata <- function(x, ...)
  UseMethod('requestMarketdata')
requestMarketdata.ib <- function(x, ...)
  x$requestMarketdata(...)

# CancelMarketdata
cancelMarketdata <- function(x, ...)
  UseMethod('cancelMarketdata')
cancelMarketdata.ib <- function(x, ...)
  x$cancelMarketdata(...)


