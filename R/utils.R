library(IBrokers)
library(foreach)
library(raptor)


fh_stk_contract_detail <- function(ibg, symbol) {
  stopifnot(isConnected(ibg))
  
  # contract details is a list, get the first one.
  message('Getting contract details for ', symbol)
  cd <- reqContractDetails(ibg, twsSTK(symbol=symbol))[[1]]

  message('Found details = ', cd)
  # combine into a map where keys are the symbols and values are the contractDetails
  key <- cd$contract$symbol
  kv <- list()
  kv[[key]] <- cd
  return(kv)
}

fh_ind_contract_detail <- function(ibg, symbol) {
  stopifnot(isConnected(ibg))
  
  # contract details is a list, get the first one.
  message('Getting contract details for ', symbol)
  cd <- reqContractDetails(ibg, twsIND(symbol=symbol))[[1]]

  message('Found details = ', cd)
  # combine into a map where keys are the symbols and values are the contractDetails
  key <- cd$contract$symbol
  kv <- list()
  kv[[key]] <- cd
  return(kv)
}

# Load contract details from IB gateway.  Returns a list of contractDetails
# keyed by the symbol name.
#
# > l <- fh_load_stk_contract_details(ibg, c('aapl', 'goog'))
# > l
# $AAPL
# List of 18
#  $ version       : chr "6"
#  $ contract      :List of 16
#   ..$ conId          : chr "265598"
#   ..$ symbol         : chr "AAPL"
#   ..$ sectype        : chr "STK"
#   ..$ exch           : chr "SMART"
#   ..$ primary        : chr "NASDAQ"
#   ..$ expiry         : chr ""
#   ..$ strike         : chr "0"
#   ..$ currency       : chr "USD"
#   ..$ right          : chr ""
#   ..$ local          : chr "AAPL"
#   ..$ multiplier     : chr ""
#   ..$ combo_legs_desc: chr ""
#   ..$ comboleg       : chr ""
#   ..$ include_expired: chr ""
#   ..$ secIdType      : chr ""
#   ..$ secId          : chr ""
#   ..- attr(*, "class")= chr "twsContract"
#  $ marketName    : chr "NMS"
#  $ tradingClass  : chr "NMS"
#  $ conId         : chr "265598"
#  $ minTick       : chr "0.01"
#  $ orderTypes    : chr [1:50] "ACTIVETIM" "ADJUST" "ALERT" "ALGO" ...
#  $ validExchanges: chr [1:20] "SMART" "ARCA" "BATS" "BEX" ...
#  $ priceMagnifier: chr "1"
#  $ underConId    : chr "0"
#  $ longName      : chr "APPLE INC"
#  $ contractMonth : chr ""
#  $ industry      : chr "Technology"
#  $ category      : chr "Computers"
#  $ subcategory   : chr "Computers"
#  $ timeZoneId    : chr "EST"
#  $ tradingHours  : chr "20111217:CLOSED;20111219:0400-2000"
#  $ liquidHours   : chr "20111217:CLOSED;20111219:0930-1600"
# 
# $GOOG
# List of 18
#  $ version       : chr "6"
#  $ contract      :List of 16
#   ..$ conId          : chr "30351181"
#   ..$ symbol         : chr "GOOG"
#   ..$ sectype        : chr "STK"
#   ..$ exch           : chr "SMART"
#   ..$ primary        : chr "NASDAQ"
#   ..$ expiry         : chr ""
#   ..$ strike         : chr "0"
#   ..$ currency       : chr "USD"
#   ..$ right          : chr ""
#   ..$ local          : chr "GOOG"
#   ..$ multiplier     : chr ""
#   ..$ combo_legs_desc: chr ""
#   ..$ comboleg       : chr ""
#   ..$ include_expired: chr ""
#   ..$ secIdType      : chr ""
#   ..$ secId          : chr ""
#   ..- attr(*, "class")= chr "twsContract"
#  $ marketName    : chr "NMS"
#  $ tradingClass  : chr "NMS"
#  $ conId         : chr "30351181"
#  $ minTick       : chr "0.01"
#  $ orderTypes    : chr [1:50] "ACTIVETIM" "ADJUST" "ALERT" "ALGO" ...
#  $ validExchanges: chr [1:20] "SMART" "ARCA" "BATS" "BEX" ...
#  $ priceMagnifier: chr "1"
#  $ underConId    : chr "0"
#  $ longName      : chr "GOOGLE INC-CL A"
#  $ contractMonth : chr ""
#  $ industry      : chr "Communications"
#  $ category      : chr "Internet"
#  $ subcategory   : chr "Web Portals/ISP"
#  $ timeZoneId    : chr "EST"
#  $ tradingHours  : chr "20111217:CLOSED;20111219:0400-2000"
#  $ liquidHours   : chr "20111217:CLOSED;20111219:0930-1600"
fh_load_stk_contract_details <- function(ibg, symbols) {
  list <- foreach(s = symbols, .combine="c") %do% fh_stk_contract_detail(ibg, s)
  return(list)
}

fh_load_ind_contract_details <- function(ibg, symbols) {
  list <- foreach(s = symbols, .combine="c") %do% fh_ind_contract_detail(ibg, s)
  return(list)
}

fh_marketData <- function(zmq, symbols) {
  result <- foreach(s = symbols, .combine="c") %do% {
    ret <- list()
    ret[[s]] <- firehose.req_marketdata(zmq, contractDetails[[s]]$contract)
    message('Requested market data', ret)
    return(ret)
  }
  return(result)
}

fh_cancel_marketData <- function(zmq, symbols) {
  result <- foreach(s = symbols, .combine="c") %do% {
    ret <- list()
    ret[[s]] <- firehose.cancel_marketdata(zmq, contractDetails[[s]]$contract)
    message('Canceled market data', ret)
    return(ret)
  }
  return(result)
}





