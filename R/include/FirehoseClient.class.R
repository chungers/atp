new.FirehoseClient <- function(contractDb,
                               connector_endpoint = "tcp://127.0.0.1:6666") {

  # make sure contractDb is an instance of ContractDb
  if (class(contractDb) != 'ContractDb') {
    message('contractDb must be an instance of ContractDb: ',
            class(contractDb))
    return(FALSE)
  }

  library(ib)
  library(foreach)

  this <- new.env()
  class(this) <- 'FirehoseClient'

  this$.contractDb <- contractDb$.Data
  this$.contractKeys <- names(this$.contractDb)
  this$.connector_endpoint <- connector_endpoint


  # connect to zmq
  this$.fh <- new.fh(this$.connector_endpoint)

  #
  # Request market data
  # Takes a vector of topics or regular expressions and request market data
  # for each.
  #
  this$requestMarketData <- function(symbols, tick_types="", snapshot=FALSE) {
    # symbols can be a list of regular expressions:
    # "APC.OPT.201205.*\\.8[0-9].C" matches all 2012 May 80 and 85 calls.
    matches <- foreach(regex = symbols, .combine='c') %do% {
      this$.contractKeys[grep(regex, this$.contractKeys)]
    }
    result <- foreach(s = matches, .combine="c") %do% {
      ret <- list()
      ret[[s]] <- requestMarketdata(this$.fh, this$.contractDb[[s]]$contract,
                                    tick_types, snapshot)

      message('Requested market data ', s, ' ', ret)
      return(ret)
    }
    return(result)
  }
  environment(this$requestMarketData) <- as.environment(this)


  #
  # Cancel market data
  # Given a vector of regex or topics, cancel market data for each
  #
  this$cancelMarketData <- function(symbols) {
    # symbols can be a list of regular expressions:
    # "APC.OPT.201205.*\\.8[0-9].C" matches all 2012 May 80 and 85 calls.
    matches <- foreach(regex = symbols, .combine='c') %do% {
      this$.contractKeys[grep(regex, this$.contractKeys)]
    }

    result <- foreach(s = matches, .combine="c") %do% {
      ret <- list()
      ret[[s]] <- cancelMarketdata(this$.fh, this$.contractDb[[s]]$contract)

      message('Canceled market data ', s, ' ', ret)
      return(ret)
    }
    return(result)
  }
  environment(this$cancelMarketData) <- as.environment(this)

  #
  # Request market depth
  # Takes a vector of topics or regular expressions and request market depth
  # for each.
  #
  this$requestMarketDepth <- function(symbols, rows=20) {
    # symbols can be a list of regular expressions:
    # "APC.OPT.201205.*\\.8[0-9].C" matches all 2012 May 80 and 85 calls.
    matches <- foreach(regex = symbols, .combine='c') %do% {
      this$.contractKeys[grep(regex, this$.contractKeys)]
    }
    result <- foreach(s = matches, .combine="c") %do% {
      ret <- list()
      ret[[s]] <- requestMarketdepth(this$.fh, this$.contractDb[[s]]$contract)
      message('Requested market depth ', s, ' ', ret)
      return(ret)
    }
    return(result)
  }
  environment(this$requestMarketDepth) <- as.environment(this)


  #
  # Cancel market depth
  # Given a vector of regex or topics, cancel market depth for each
  #
  this$cancelMarketDepth <- function(symbols) {
    # symbols can be a list of regular expressions:
    # "APC.OPT.201205.*\\.8[0-9].C" matches all 2012 May 80 and 85 calls.
    matches <- foreach(regex = symbols, .combine='c') %do% {
      this$.contractKeys[grep(regex, this$.contractKeys)]
    }

    result <- foreach(s = matches, .combine="c") %do% {
      ret <- list()
      ret[[s]] <- cancelMarketdepth(this$.fh, this$.contractDb[[s]]$contract)
      message('Canceled market depth ', s, ' ', ret)
      return(ret)
    }
    return(result)
  }
  environment(this$cancelMarketDepth) <- as.environment(this)


  return(this)
}

# S3 function mapping
requestMarketData <- function(x, ...) UseMethod('requestMarketData')
requestMarketData.FirehoseClient <-
  function(x, ...) x$requestMarketData(...)

cancelMarketData <- function(x,  ...) UseMethod('cancelMarketData')
cancelMarketData.FirehoseClient <-
  function(x,  ...) x$cancelMarketData(...)

requestMarketDepth <- function(x,  ...) UseMethod('requestMarketDepth')
requestMarketDepth.FirehoseClient <-
  function(x,  ...) x$requestMarketDepth(...)

cancelMarketDepth <- function(x,  ...) UseMethod('cancelMarketDepth')
cancelMarketDepth.FirehoseClient <-
  function(x,  ...) x$cancelMarketDepth(...)

