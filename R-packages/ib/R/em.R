
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

  scrub <- function(o) {
    o$orderId <- as.numeric(o$orderId)
    o$lmtPrice <- as.numeric(o$lmtPrice)
    o$auxPrice <- as.numeric(o$auxPrice)
    o$totalQuantity <- as.numeric(o$totalQuantity)
    o$minQty <- as.character(o$minQty)
    o$outsideRTH <- as.numeric(o$outsideRTH)
    o$allOrNone <- as.numeric(o$allOrNone)
    return(o)
  }

  # Methods

  # orderSend
  this$orderSend <- function(contract, order) {
    .Call("api_place_order",
          this$.apiConnection, scrub(order),
          contract, PACKAGE="ib")
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



