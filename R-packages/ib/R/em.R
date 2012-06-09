
#
new.em <- function(apiAddress = 'tcp://127.0.0.1:6667') {

  options(digits.secs=6)
  Sys.setenv(TZ='America/New_York')

  this <- new.env()
  class(this) <- 'em'

  this$.apiAddress = apiAddress

  this$.apiConnection <- .Call("api_connect", this$.apiAddress, PACKAGE='ib')

  nycTime <- function(t) {
    utc <- as.POSIXct(t, origin='1970-01-01', tz='GMT');
    as.POSIXlt(utc, 'America/New_York');
  }

  # Methods

  # orderSend
  this$orderSend <- function(contract, order) {
    .Call("api_place_order",
          this$.apiConnection, order, contract, PACKAGE="ib")
  }
  environment(this$orderSend) <- as.environment(this)

  # orderCancel
  this$orderCancel <- function(orderId) {
    .Call("api_cancel_order",
          this$.apiConnection, orderId, PACKAGE="ib")
  }
  environment(this$orderCancel) <- as.environment(this)
  return(this)
}

# S3 function mapping

# orderSend
orderSend <- function(x, ...)
  UseMethod('orderSend')
orderSend.em <- function(x, ...)
  x$orderSend(...)

# orderCancel
orderCancel <- function(x, ...)
  UseMethod('orderCancel')
orderCancel.em <- function(x, ...)
  x$orderCancel(...)



