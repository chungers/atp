source("utils.R")

# load contract details for all the symbols and store them in a RData file
# as database of contracts for market data requests later on.
indexes <- c(
'INDU',
'SPX')

# first three will get the market depth (IB limit)
symbols <- c(
'AAPL',
'GOOG',
'SPY',
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

contractDetails <- fh_load_stk_contract_details(ibg, symbols)
indexDetails <- fh_load_ind_contract_details(ibg, indexes)

contractDetails <- c(contractDetails, indexDetails)

twsDisconnect(ibg)
rm(ibg)

message("Found contractDetails, count = ", length(contractDetails))

save.image('firehose_contracts.RData')




