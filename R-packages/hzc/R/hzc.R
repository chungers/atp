
# Assumes a local ssh tunnel is open per
# https://github.com/lab616/hub/wiki/Historian:---the-Market-Data-Backend
#
hzc.connect <- function(address = 'tcp://127.0.0.1:1111',
                        callbackAddress = 'tcp://127.0.0.1:1112') {
  handle <- .Call("hzc_connect",
                  as.character(address),
                  as.character(callbackAddress),
                  PACKAGE = "hzc");
  handle
}

hzc.close <- function(handle) {
  result <- .Call("hzc_close",
                  handle,
                  PACKAGE = "hzc");
  rm(handle)
  result
}

hzc.query <- function(handle, symbol, event,
                      rStart, rStop,
                      callback = NULL, est = TRUE) {
  result <- .Call("hzc_query",
                  handle,
                  as.character(symbol), as.character(event),
                  as.character(rStart), as.character(rStop), callback, est,
                  PACKAGE = "hzc");
  result
}
