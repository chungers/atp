
# Contract database
new.ContractDb <- function() {

  library(IBrokers)
  library(foreach)
  library(raptor)

  this <- new.env()
  class(this) <- 'ContractDb'

  # connection object to IB gateway
  this$.ibg <- NULL
  this$.Data <- NULL
  this$.Symbols <- NULL
  this$.Indexes <- NULL

  #
  # Connect to IB gateway
  #
  this$connect <- function(host='localhost', port=4001, clientId=9999) {
    if (!is.null(this$.ibg)) {
      twsDisconnect(this$.ibg)
      this$.ibg <- NULL
    } else {
      this$.ibg <- twsConnect(host=host, port=port, clientId=clientId)
    }
  }
  environment(this$connect) <- as.environment(this)

  # private:
  fh_stk_contract_detail <- function(ibg, symbol) {
    stopifnot(isConnected(ibg))
    message('Getting contract details for ', symbol)
    x <- reqContractDetails(ibg, twsSTK(symbol=symbol))[[1]]

    s<-as.character(x$contract$symbol)
    t<-as.character(x$contract$sectype)
    key <- paste(s, t, sep='.')
    kv <- list()
    kv[[key]] <- x
    return(kv)
  }

  # private:
  fh_ind_contract_detail <- function(ibg, symbol) {
    stopifnot(isConnected(ibg))
    message('Getting contract details for ', symbol)
    x <- reqContractDetails(ibg, twsIND(symbol=symbol))[[1]]

    s<-as.character(x$contract$symbol)
    t<-as.character(x$contract$sectype)
    key <- paste(s, t, sep='.')
    kv <- list()
    kv[[key]] <- x
    return(kv)
  }

  # private:
  fh_opt_contract_detail <- function(ibg, symbol) {
    stopifnot(isConnected(ibg))

    q <- twsOPT(symbol)
    # Fix bug with IBrokers -- local must be empty. use symbol instead
    q$symbol <- symbol
    q$local <- ''

    # contract details is a list, get the first one.
    message('Getting option chain for ', symbol)

    l <- reqContractDetails(ibg, q)

    opt <- list()
    lapply(l, function(x) {
      s<-as.character(x$contract$symbol)
      t<-as.character(x$contract$sectype)
      k<-as.character(x$contract$strike)
      r<-as.character(x$contract$right)
      e<-as.character(x$contract$expiry)
      key <- paste(s, t, e, k, r, sep='.')
      opt[[key]] <<- x
    })
    return(opt)
  }

  #
  # Load stock contracts
  #
  this$loadStockContractDetails <- function(symbols) {
    list <- foreach(s = symbols, .combine="c") %do% {
      fh_stk_contract_detail(this$.ibg, s)
    }
    this$.Data <- c(this$.Data, list)
    this$.Symbols <- unique(c(this$.Symbols, symbols))
    length(list)
  }
  environment(this$loadStockContractDetails) <- as.environment(this)

  #
  # Load index contracts
  #
  this$loadIndexContractDetails <- function(symbols) {
    list <- foreach(s = symbols, .combine="c") %do% {
      fh_ind_contract_detail(this$.ibg, s)
    }
    this$.Data <- c(this$.Data, list)
    this$.Indexes <- unique(c(this$.Indexes, symbols))
    length(list)
  }
  environment(this$loadIndexContractDetails) <- as.environment(this)

  #
  # Load option contracts
  #
  this$loadOptionContractDetails <- function(symbols) {
    list <- foreach(s = symbols, .combine="c") %do% {
      fh_opt_contract_detail(this$.ibg, s)
    }
    this$.Data <- c(this$.Data, list)
    this$.Symbols <- unique(c(this$.Symbols, symbols))
    length(list)
  }
  environment(this$loadOptionContractDetails) <- as.environment(this)

  #
  # Load stocks + options
  #
  this$load <- function(symbols, index=FALSE) {
    if (index) {
      this$loadIndexContractDetails(symbols)
    } else {
      c(this$loadStockContractDetails(symbols),
        this$loadOptionContractDetails(symbols))
    }
  }
  environment(this$load) <- as.environment(this)

  #
  # Disconnect
  #
  this$disconnect <- function() {
    if (!is.null(this$.ibg)) {
      twsDisconnect(this$.ibg)
      this$.ibg <- NULL
    }
  }
  environment(this$disconnect) <- as.environment(this)

  #
  # Get the stock symbols
  #
  this$stockSymbols <- function() {
    stocks <- foreach (s = this$.Symbols, .combine='c') %do% {
      paste(s, 'STK', sep='.')
    }
    indexes <- foreach (s = this$.Indexes, .combine='c') %do% {
      paste(s, 'IND', sep='.')
    }
    c(stocks, indexes)
  }
  environment(this$stockSymbols) <- as.environment(this)

  return(this)
}

# S3 function mapping
connect <- function(x, h, p, c, ...) UseMethod('connect')
connect.ContractDb <-
  function(x, host='localhost', port=4001, clientId=9999, ...)
  x$connect(host, port, clientId)

disconnect <- function(x, ...) UseMethod('disconnect')
disconnect.ContractDb <- function(x, ...) x$disconnect()

stockSymbols <- function(x, ...) UseMethod('stockSymbols')
stockSymbols.ContractDb <- function(x, ...) x$stockSymbols()

loadStockContractDetails <-
  function(x, s, ...) UseMethod('loadStockContractDetails')
loadStockContractDetails.ContractDb <-
  function(x, s, ...) x$loadStockContractDetails(s)

loadIndexContractDetails <-
  function(x, s, ...) UseMethod('loadIndexContractDetails')
loadIndexContractDetails.ContractDb <-
  function(x, s, ...) x$loadIndexContractDetails(s)

loadOptionContractDetails <-
  function(x, s, ...) UseMethod('loadOptionContractDetails')
loadOptionContractDetails.ContractDb <-
  function(x, s, ...) x$loadOptionContractDetails(s)

load <- function(x, s, index, ...) UseMethod('load')
load.ContractDb <- function(x, s, index = FALSE, ...) x$load(s, index)
