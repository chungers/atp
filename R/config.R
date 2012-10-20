CONFIG <- list()

# name of the contract database
CONFIG$contractDb <- 'ContractDb.RData'

CONFIG$firehose <- list(remote = 'tcp://69.164.211.61:6666',
                        local = 'tcp://127.0.0.1:6666')

CONFIG$gatewayPort <- 4002

CONFIG$hzHost <- '50.116.57.10'
CONFIG$hzPort <- 1111

CONFIG$indexes <- c(
'INDU',
'SPX',
'VIX')

CONFIG$symbols <- c(
'AAPL',
'AMZN',
'CAT',
'CMG',
'FAS',
'FAZ',
'FB',
'FFIV',
'GOOG',
'IBM',
'INTC',
'JPM',
'GDX',
'GLD',
'GS',
'LNKD',
'MS',
'MSFT',
'NFLX',
'ORCL',
'PCLN',
'SPY',
'WFM',
'WYNN',
'XLE',
'XLV',
"ZNGA")

CONFIG$book <- c('AAPL.STK', 'GOOG.STK', 'FB.STK')

source("options.R")
expiry <- options.nextFriday()
CONFIG$options <- c(
                    options.buildOptionStrikes('AAPL', expiry, 8),
                    options.buildOptionStrikes('GOOG', expiry, 8),
                    options.buildOptionStrikes('SPY', expiry, 8)
                    )
