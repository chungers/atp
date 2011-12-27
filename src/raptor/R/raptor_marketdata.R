
options(digits.secs=6)

# Given a list contract details (e.g. list(c1, c2, c3, ...)
# extracts the topics based on the contract information such as
# the security type, strike, right, and expiry, etc.
marketdata.get_topics <- function(listOfContractDetails) {
  sapply(listOfContractDetails,
         # contract is a member of contractDetail
         function(cDetail) {
           ifelse(cDetail$contract$sectype == 'STK' ||
                  cDetail$contract$sectype == 'IND',

                  paste(sep='.',
                        cDetail$contract$symbol,
                        cDetail$contract$sectype),
                  paste(sep='.',
                        cDetail$contract$symbol,
                        cDetail$contract$sectype,
                        cDetail$contract$strike,
                        cDetail$contract$right,
                        cDetail$contract$expiry))
               })
}

marketdata.default_handler <- function(source, t, topic, event, value) {
  utc <- as.POSIXct(t, origin='1970-01-01', tz='GMT');
  nyc <- as.POSIXlt(utc, 'America/New_York');
  message(paste(nyc, topic, event, value, sep=' '));
  return(TRUE)
}


marketdata.subscribe <- function(endpoint, contractDetails,
                                 handler = marketdata.default_handler,
                                 varz = 9999) {
  # contractDetails is a list of contractDetails from IBroker
  # reqContractDetails() call.
  topics <- marketdata.get_topics(contractDetails)

  options(digits.secs=6)
  result <- .Call("firehose_subscribe_marketdata",
                  endpoint, topics, handler, varz,
                  PACKAGE = "raptor")
  result
}

marketdata.cancel_marketdata <- function(handle, contracts) {
  # contract is a list.
  result <- .Call("firehose_unsubscribe_marketdata",
                  handle, contracts,
                  PACKAGE = "raptor");
  result
}
