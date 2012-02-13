
CONFIG <- list()

CONFIG$indexes <- c(
'INDU',
'SPX')

CONFIG$symbols <- c(
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
'GMCR',
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
'MGM',
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

library(foreach)
source("options.R")

CONFIG$book <- c('AAPL.STK', 'GOOG.STK', 'BIDU.STK')

expiry <- 20120210

CONFIG$options <- c(
                    options.buildOptionStrikes('AAPL', expiry, 8),
                    options.buildOptionStrikes('AMZN', expiry, 8),
                    options.buildOptionStrikes('BIDU', expiry, 8),
                    options.buildOptionStrikes('SPY', expiry, 8)
                    options.buildOptionStrikes('ZNGA', expiry, 8)
                    )

# name of the contract database
CONFIG$contractDb <- 'ContractDb.RData'

CONFIG$firehose <- list(remote = 'tcp://69.164.211.61:6666',
                        local = 'tcp://127.0.0.1:6666')

CONFIG$gatewayPort <- 4002
