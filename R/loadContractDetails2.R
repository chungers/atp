source("utils2.R")

# load contract details for all the symbols and store them in a RData file
# as database of contracts for market data requests later on.
indexes <- c(
'INDU',
'SPX')

symbols <- c(
'AAPL',
'AKAM',
'AMZN',
'APC',
'BAC',
'BIDU',
'BMC',
'C',
'CAT',
'CMG',
'CRM',
'CROX',
'DECK',
'DDM',
'DXD',
'ERX',
'ERY',
'FAS',
'FAZ',
'FFIV',
'GOOG',
'IBM',
'INTC',
'ISRG',
'JPM',
'GLD',
'GLL',
'GRPN',
'GS',
'LNKD',
'MS',
'MSFT',
'NFLX',
'OPEN',
'ORCL',
'PCLN',
'QID',
'QLD',
'REW',
'RIMM',
'RTH',
'SMN',
'SOHU',
'SPY',
'URE',
'VMW',
'WFM',
'WYNN',
'XLE',
'XLV',
"ZNGA")

library(IBrokers)

ibg <- twsConnect(host="localhost", port=4001, clientId=9999)

message("Loading contractDetails for symbols, count = ", length(symbols))

stkContractDetails <- fh_load_stk_contract_details(ibg, symbols)
optContractDetails <- fh_load_opt_contract_details(ibg, symbols)

contractDetails <- sapply(symbols,
                          function(s) {
                            v <- list()
                            v[[s]] <- list(STK=stkContractDetails[[s]],
                                           OPT=optContractDetails[[s]])
                            return(v)
                          })


indexDetails <- fh_load_ind_contract_details(ibg, indexes)

contractDetails <- c(contractDetails, indexDetails)

twsDisconnect(ibg)
rm(ibg)

message("Found contractDetails, count = ", length(contractDetails))

save.image('firehose_contracts2.RData')




