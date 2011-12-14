

raptor.firehose.req_marketdata <- function(handle, contract) {
  # contract is a list.
  result <- .Call("raptor_firehose_req_marketdata",
            handle,
            names(contract),
            lapply(contract, function(f) { as.character(f) }),
            PACKAGE = "raptor");
  result
}

