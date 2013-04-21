
# Assumes a local ssh tunnel is open per
# https://github.com/lab616/hub/wiki/Historian:---the-Market-Data-Backend
#
raptor.hzc.connect <- function(address = 'tcp://127.0.0.1:1111',
                               callbackAddress = 'tcp://127.0.0.1:1112') {
  handle <- .Call("raptor_hzc_connect",
                  as.character(address),
                  as.character(callbackAddress),
                  PACKAGE = "raptor");
  handle
}

raptor.hzc.close <- function(handle) {
  result <- .Call("raptor_hzc_close",
                  handle,
                  PACKAGE = "raptor");
  rm(handle)
  result
}

raptor.hzc.query <- function(handle, symbol, event,
                             rStart, rStop,
                             callback = NULL, est = TRUE) {
  result <- .Call("raptor_hzc_query",
                  handle,
                  as.character(symbol), as.character(event),
                  as.character(rStart), as.character(rStop), callback, est,
                  PACKAGE = "raptor");
  result
}
