

raptor.firehose.req_marketdata <- function(handle, contract) {
  # contract is a list.
  result <- .Call("raptor_firehose_marketdata",
            handle,
            lapply(contract, function(f) { as.character(f) }),
            PACKAGE = "raptor");
  result
}

raptor.firehose.cancel_marketdata <- function(handle, contract) {
  # contract is a list.
  result <- .Call("raptor_firehose_cancel_marketdata",
            handle,
            lapply(contract, function(f) { as.character(f) }),
            PACKAGE = "raptor");
  result
}
