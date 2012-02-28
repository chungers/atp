
library(raptor)
options(digits.secs=6)

dbFile <- '/tmp/mytest1'

db <- raptor.historian.open(dbFile)

nycTime <- function(t) {
  utc <- as.POSIXct(t, origin='1970-01-01', tz='GMT');
  as.POSIXlt(utc, 'America/New_York');
}

t <- c()
e <- c()
v <- c()

h <- function(symbol, ts, event, value) {
  nyt <- nycTime(ts)

  t <<- c(t, as.numeric(ts))
  e <<- c(e, as.character(event))
  v <<- c(v, as.numeric(value))

  message(paste(symbol, nyt, event, value, sep=','))
  return(TRUE)
}

symbol <- 'AAPL.STK'
start <- '2012-02-24 09:30:00'
end <- '2012-02-24 16:00:00'

message('Fetching data')

raw <- raptor.historian.ib_marketdata(db, symbol, 'LAST', start, end)

message('Loaded')

library(xts)

last.xts <- as.xts(raw$value,
                   order.by=nycTime(raw$utc_t),
                   tzone='America/New_York')

head(last.xts, 10)

#raptor.historian.close(db)

