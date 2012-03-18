options(digits.secs=6)

marketdata.get_topics <- function(listOfContractDetails) {
  # Given a list contract details (e.g. list(c1, c2, c3, ...)
  # extracts the topics based on the contract information such as
  # the security type, strike, right, and expiry, etc.
  sapply(listOfContractDetails,
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

marketdata.default_handler <- function(topic, t, event, value) {
  utc <- as.POSIXct(t, origin='1970-01-01', tz='GMT');
  nyc <- as.POSIXlt(utc, 'America/New_York');
  message(paste(nyc, topic, event, value, sep=' '));
  return(TRUE)
}

marketdata.newSubscriber <- function(id, adminEndpoint,
                                     endpoint, varz = 18000,
                                     eventEndpoint) {
  options(digits.secs=6)
  result <- .Call("marketdata_create_subscriber",
                  id, adminEndpoint, endpoint, varz, eventEndpoint,
                  PACKAGE = "raptor")
  return(result)
}

marketdata.start <- function(subscriber,
                             handler = marketdata.default_handler) {
  result <- .Call("marketdata_start_subscriber",
                  subscriber, handler,
                  PACKAGE = "raptor");
  return(result)
}

marketdata.stop <- function(subscriber) {
  result <- .Call("marketdata_stop_subscriber",
                  subscriber,
                  PACKAGE = "raptor");
  return(result)
}

marketdata.subscribe <- function(subscriber, contractDetails) {
  # contractDetails is a list of contractDetails from IBroker
  # reqContractDetails() call.
  topics <- marketdata.get_topics(contractDetails)

  options(digits.secs=6)
  result <- .Call("marketdata_subscribe",
                  subscriber, topics,
                  PACKAGE = "raptor")
  return(result)
}

marketdata.unsubscribe <- function(subscriber, contractDetails) {
  # contractDetails is a list of contractDetails from IBroker
  # reqContractDetails() call.
  topics <- marketdata.get_topics(contractDetails)

  result <- .Call("marketdata_unsubscribe",
                  subscriber, topics,
                  PACKAGE = "raptor");
  return(result)
}

