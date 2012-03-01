
library(raptor)
options(digits.secs=6)

dbFile <- '/Volumes/backup1/data/test2.ldb'

db <- raptor.historian.open(dbFile)

nycTime <- function(t) {
  utc <- as.POSIXct(t, origin='1970-01-01', tz='GMT');
  as.POSIXlt(utc, 'America/New_York');
}

t <- c()
e <- c()
v <- c()

# callback for each observation -- this is optional and SLOW.
h <- function(symbol, ts, event, value) {
  nyt <- nycTime(ts)

  t <<- c(t, as.numeric(ts))
  e <<- c(e, as.character(event))
  v <<- c(v, as.numeric(value))

  message(paste(symbol, nyt, event, value, sep=','))
  return(TRUE)
}


start <- '2012-02-29 09:30:00'
end <- '2012-02-29 16:00:00'

mkdata <- function(db, symbol, event,
                   qstart=start, qend=end) {
  raw <- raptor.historian.ib_marketdata(db, symbol, event, qstart, qend)
  library(xts)
  data <- as.xts(raw$value,
                 order.by=nycTime(raw$utc_t),
                 tzone='America/New_York')
  return(data)
}


dev.new(); p1 <- dev.cur()
dev.new(); p2 <- dev.cur()

aapl <- mkdata(db, 'AAPL.STK', 'LAST')
spx <- mkdata(db, 'SPX.IND', 'LAST')

# locf - last observation carried forward.
# this fills in the NA at different sampling instants
aapl_spx <- na.locf(merge(aapl, spx), fromLast=TRUE)
dev.set(p1)
plot.zoo(aapl_spx)

gld <- mkdata(db, 'GLD.STK', 'LAST')
gdx <- mkdata(db, 'GDX.STK', 'LAST')

# locf - last observation carried forward.
# this fills in the NA at different sampling instants
gld_gdx <- na.locf(merge(gld, gdx), fromLast=TRUE)
dev.set(p2)
plot.zoo(gld_gdx)


#raptor.historian.close(db)

