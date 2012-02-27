
raptor.historian.open <- function(db) {
  handle <- .Call("raptor_historian_open",
                  as.character(db),
                  PACKAGE = "raptor");
  handle
}

raptor.historian.close <- function(handle) {
  result <- .Call("raptor_historian_close",
                  handle,
                  PACKAGE = "raptor");
  rm(handle)
  result
}

raptor.historian.ib_marketdata <- function(handle, symbol,
                                           rStart, rStop, callback,
                                           est = TRUE) {
  result <- .Call("raptor_historian_ib_marketdata",
                  handle,
                  as.character(symbol),
                  as.character(rStart), as.character(rStop), callback, est,
                  PACKAGE = "raptor");
  result
}

raptor.historian.sessionlogs <- function(handle, symbol) {
  result <- .Call("raptor_historian_sessionlogs",
                  handle,
                  as.character(symbol),
                  PACKAGE = "raptor");
  result
}
