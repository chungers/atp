source("include/FirehoseClient.class.R")
source("config.R")

# load the contract database
load(CONFIG$contractDb)

# if invoked with R --vanilla --slave --args local
# then use the localhost address.
env <- commandArgs()[5]
if (is.na(env)) {
  env <- 'remote'
}

ep <- CONFIG$firehose[[env]]
message(env, ', endpoint = ', ep)

fh <- new.FirehoseClient(cdb, ep)

requestMarketData(fh, stockSymbols(cdb))
requestMarketData(fh, CONFIG$options)
requestMarketDepth(fh, CONFIG$book)

marketDataRequested <- c(stockSymbols(cdb), CONFIG$options)
marketDepthRequested <- CONFIG$book

save(marketDataRequested, marketDepthRequested,
     file='marketdata_requests.RData')


