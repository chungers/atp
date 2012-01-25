
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

library(foreach)

CONFIG$book <- c('AAPL.STK', 'GOOG.STK', 'BIDU.STK')

expiry <- 20120127

aapl_options <- foreach(strike = c(405, 410, 415, 420, 425, 430, 435),
                        .combine='c') %do% {
  paste('AAPL.OPT', expiry, strike, sep='.')
}

amzn_options <- foreach(strike = c(175, 180, 185, 190, 195, 200, 205),
                        .combine='c') %do% {
  paste('AMZN.OPT', expiry, strike, sep='.')
}

goog_options <- foreach(strike = c(565, 570, 575, 580, 585, 590, 595, 600),
                        .combine='c') %do% {
  paste('GOOG.OPT', expiry, strike, sep='.')
}

bidu_options <- foreach(strike = c(110, 115, 120, 125, 130, 135, 140),
                        .combine='c') %do% {
  paste('BIDU.OPT', expiry, strike, sep='.')
}

spy_options <- foreach(strike = c(120, 125, 130, 135, 140),
                        .combine='c') %do% {
  paste('SPY.OPT', expiry, strike, sep='.')
}

CONFIG$options <- c(aapl_options, amzn_options, goog_options, bidu_options, spy_options)

# name of the contract database
CONFIG$contractDb <- 'ContractDb.RData'

CONFIG$firehose <- list(remote = 'tcp://69.164.211.61:6666',
                        local = 'tcp://127.0.0.1:6666')
