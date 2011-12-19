
firehose.req_marketdata <- function(handle, contract) {
  # contract is a list.
  result <- .Call("firehose_marketdata",
            handle,
            lapply(contract, function(f) { as.character(f) }),
            PACKAGE = "raptor");
  result
}

firehose.cancel_marketdata <- function(handle, contract) {
  # contract is a list.
  result <- .Call("firehose_cancel_marketdata",
            handle,
            lapply(contract, function(f) { as.character(f) }),
            PACKAGE = "raptor");
  result
}
