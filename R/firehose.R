source("env.R")
source("loadContractDetails.R")

# load contract details for all the symbols and store them in a RData file
# as database of contracts for market data requests later on.
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
'GOOG',
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

ibg <- twsConnect(host="localhost", port=5001, clientId=9999)

message("Loading contractDetails for symbols, count = ", length(symbols))

contractDetails <- fh_load_stk_contract_details(ibg, symbols)

message("Found contractDetails, count = ", length(contractDetails))

save.image('firehose_contracts.RData')

twsDisconnect(ibg)



