source("include/FirehoseClient.class.R")

# common configs
CONFIG <- list()
CONFIG$contractDb <- 'ContractDb.RData'
CONFIG$firehose <- list(remote = 'tcp://69.164.211.61:6666',
                        local = 'tcp://127.0.0.1:6666')


load(CONFIG$contractDb)

env <- 'local'
ep <- CONFIG$firehose[[env]]
message(env, ', endpoint = ', ep)

fh <- new.FirehoseClient(cdb, ep)

l <- c('AAPL.STK', 'GOOG.STK')
requestMarketData(fh, l)
requestMarketDepth(fh, l)

