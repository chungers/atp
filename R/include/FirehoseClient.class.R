new.FirehoseClient <- function(contractDb,
                               connector_endpoint = NULL) {
  # make sure contractDb is an instance of ContractDb
  if (class(contractDb) != 'ContractDb') {
    message('contractDb must be an instance of ContractDb: ',
            class(contractDb))
    return(FALSE)
  }

  library(raptor)
  library(foreach)

  this <- new.env()
  class(this) <- 'FirehoseClient'

  this$.contractDb <- contractDb$.Data
  this$.contractKeys <- names(this$.contractDb)
  this$.connector_endpoint <- ifelse(!is.null(connector_endpoint),
                                     connector_endpoint,
                                     "tcp://127.0.0.1:6666")

  # connect to zmq
  this$.zmq <- raptor.zmq.connect(this$.connector_endpoint, type='ZMQ_REQ')

  #
  # Request market data
  # Takes a vector of topics or regular expressions and request market data
  # for each.
  #
  this$requestMarketData <- function(symbols) {
    # symbols can be a list of regular expressions:
    # "APC.OPT.201205.*\\.8[0-9].C" matches all 2012 May 80 and 85 calls.
    matches <- foreach(regex = symbols, .combine='c') %do% {
      this$.contractKeys[grep(regex, this$.contractKeys)]
    }
    result <- foreach(s = matches, .combine="c") %do% {
      ret <- list()
      ret[[s]] <- firehose.req_marketdata(this$.zmq,
                                          this$.contractDb[[s]]$contract)
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
      ret[[s]] <- firehose.cancel_marketdata(this$.zmq,
                                             this$.contractDb[[s]]$contract)
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
  this$requestMarketDepth <- function(symbols) {
    # symbols can be a list of regular expressions:
    # "APC.OPT.201205.*\\.8[0-9].C" matches all 2012 May 80 and 85 calls.
    matches <- foreach(regex = symbols, .combine='c') %do% {
      this$.contractKeys[grep(regex, this$.contractKeys)]
    }
    result <- foreach(s = matches, .combine="c") %do% {
      ret <- list()
      ret[[s]] <- firehose.req_marketdepth(this$.zmq,
                                           this$.contractDb[[s]]$contract)
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
      ret[[s]] <- firehose.cancel_marketdepth(this$.zmq,
                                              this$.contractDb[[s]]$contract)
      message('Canceled market depth ', s, ' ', ret)
      return(ret)
    }
    return(result)
  }
  environment(this$cancelMarketDepth) <- as.environment(this)

  #
  # Request market ohlc
  # Takes a vector of topics or regular expressions and request market ohlc
  # for each.
  #
  this$requestMarketOhlc <- function(symbols) {
    # symbols can be a list of regular expressions:
    # "APC.OPT.201205.*\\.8[0-9].C" matches all 2012 May 80 and 85 calls.
    matches <- foreach(regex = symbols, .combine='c') %do% {
      this$.contractKeys[grep(regex, this$.contractKeys)]
    }
    result <- foreach(s = matches, .combine="c") %do% {
      ret <- list()
      ret[[s]] <- firehose.req_marketohlc(this$.zmq,
                                           this$.contractDb[[s]]$contract)
      message('Requested market ohlc ', s, ' ', ret)
      return(ret)
    }
    return(result)
  }
  environment(this$requestMarketOhlc) <- as.environment(this)


  #
  # Cancel market ohlc
  # Given a vector of regex or topics, cancel market ohlc for each
  #
  this$cancelMarketOhlc <- function(symbols) {
    # symbols can be a list of regular expressions:
    # "APC.OPT.201205.*\\.8[0-9].C" matches all 2012 May 80 and 85 calls.
    matches <- foreach(regex = symbols, .combine='c') %do% {
      this$.contractKeys[grep(regex, this$.contractKeys)]
    }

    result <- foreach(s = matches, .combine="c") %do% {
      ret <- list()
      ret[[s]] <- firehose.cancel_marketohlc(this$.zmq,
                                              this$.contractDb[[s]]$contract)
      message('Canceled market ohlc ', s, ' ', ret)
      return(ret)
    }
    return(result)
  }
  environment(this$cancelMarketOhlc) <- as.environment(this)

  return(this)
}

# S3 function mapping
requestMarketData <- function(x, symbols, ...) UseMethod('requestMarketData')
requestMarketData.FirehoseClient <-
  function(x, symbols, ...) x$requestMarketData(symbols)

cancelMarketData <- function(x, symbols, ...) UseMethod('cancelMarketData')
cancelMarketData.FirehoseClient <-
  function(x, symbols, ...) x$cancelMarketData(symbols)

requestMarketDepth <- function(x, symbols, ...) UseMethod('requestMarketDepth')
requestMarketDepth.FirehoseClient <-
  function(x, symbols, ...) x$requestMarketDepth(symbols)

cancelMarketDepth <- function(x, symbols, ...) UseMethod('cancelMarketDepth')
cancelMarketDepth.FirehoseClient <-
  function(x, symbols, ...) x$cancelMarketDepth(symbols)

requestMarketOhlc <- function(x, symbols, ...) UseMethod('requestMarketOhlc')
requestMarketOhlc.FirehoseClient <-
  function(x, symbols, ...) x$requestMarketOhlc(symbols)

cancelMarketOhlc <- function(x, symbols, ...) UseMethod('cancelMarketOhlc')
cancelMarketOhlc.FirehoseClient <-
  function(x, symbols, ...) x$cancelMarketOhlc(symbols)

