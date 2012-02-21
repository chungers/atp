CONFIG <- list()

# name of the contract database
CONFIG$contractDb <- 'ContractDb.RData'

CONFIG$firehose <- list(remote = 'tcp://69.164.211.61:6666',
                        local = 'tcp://127.0.0.1:6666')

CONFIG$gatewayPort <- 4002


CONFIG$indexes <- c(
'INDU',
'SPX',
'VIX')

CONFIG$symbols <- c(
'AAPL',
'AKAM',
'AMZN',
'APA',
'APC',
'BAC',
'BIDU',
'BXP',
'C',
'CAT',
'CMG',
'CROX',
'DECK',
'FAS',
'FAZ',
'FFIV',
'GMCR',
'GOOG',
'IBM',
'INTC',
'ISRG',
'JPM',
'GDX',
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
'RIMM',
'RTH',
'SMN',
'SOHU',
'SPY',
'VNO',
'WFM',
'WYNN',
'XLE',
'XLV',
"ZNGA")

CONFIG$book <- c('AAPL.STK', 'GOOG.STK', 'BIDU.STK')

source("options.R")
expiry <- options.nextFriday()
CONFIG$options <- c(
                    options.buildOptionStrikes('AAPL', expiry, 8),
                    options.buildOptionStrikes('AMZN', expiry, 6),
                    options.buildOptionStrikes('BIDU', expiry, 6),
                    options.buildOptionStrikes('GOOG', expiry, 8),
                    options.buildOptionStrikes('SPY', expiry, 6)
                    )
