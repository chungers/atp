
library(raptor)
options(digits.secs=6)

dbFile <- '/tmp/mytest1'

db <- raptor.historian.open(dbFile)

nycTime <- function(t) {
  utc <- as.POSIXct(t, origin='1970-01-01', tz='GMT');
  as.POSIXlt(utc, 'America/New_York');
}

h <- function(symbol, ts, event, value) {
  t <- nycTime(ts)
  message(paste(symbol, t, event, value, sep=','))
  return(TRUE)
}

symbol <- 'AAPL.STK'
start <- '2012-02-24 09:30:00'
end <- '2012-02-24 16:00:00'

message('Fetching data')

raptor.historian.ib_marketdata(db, symbol, start, end, h)

raptor.historian.close(db)

