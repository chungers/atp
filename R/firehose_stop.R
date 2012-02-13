source("include/FirehoseClient.class.R")
source("config.R")
load("marketdata_requests.RData")
load(CONFIG$contractDb)


# if invoked with R --vanilla --slave --args local then use the localhost address.
env <- commandArgs()[5]
if (is.na(env)) {
  env <- 'remote'
}
ep <- CONFIG$firehose[[env]]
message(env, ', endpoint = ', ep)

fh <- new.FirehoseClient(cdb, ep)

cancelMarketData(fh, marketDataRequested)
cancelMarketDepth(fh, marketDepthRequested)

