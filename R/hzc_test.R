
library(raptor)
options(digits.secs=6)

nycTime <- function(t) {
  utc <- as.POSIXct(t, origin='1970-01-01', tz='GMT');
  as.POSIXlt(utc, 'America/New_York');
}

# callback for each observation -- this is optional and SLOW.
h <- function(symbol, ts, event, value) {
  nyt <- nycTime(ts)

  t <<- c(t, as.numeric(ts))
  e <<- c(e, as.character(event))
  v <<- c(v, as.numeric(value))

  message(paste(symbol, nyt, event, value, sep=','))
  return(TRUE)
}

mkdata <- function(client, symbol, event, qStart, qStop) {
  raw <- raptor.hzc.query(client, symbol, event, qStart, qStop)
  library(xts)
  data <- as.xts(raw$value,
                 order.by=nycTime(raw$utc_t),
                 tzone='America/New_York')
  return(data)
}


# set up local ssh tunnel:
# ssh jenkins@stage.lab616.com -R 1112:localhost:1112 -L 1111:localhost:1111

hzAddress <- "tcp://127.0.0.1:1111";
cbAddress <- "tcp://127.0.0.1:1113";

client <- raptor.hzc.connect(hzAddress, cbAddress);

t <- c()
e <- c()
v <- c()


qStart <- '2012-04-13 09:30:00'
qStop <- '2012-04-13 16:00:00'

system.time(aapl <- mkdata(client, 'AAPL.STK', 'LAST', qStart, qStop))
spx <- mkdata(client, 'SPX.IND', 'LAST', qStart, qStop)
gld <- mkdata(client, 'GLD.STK', 'LAST', qStart, qStop)
gdx <- mkdata(client, 'GDX.STK', 'LAST', qStart, qStop)

# locf - last observation carried forward.
# this fills in the NA at different sampling instants
aapl_spx <- na.locf(merge(aapl, spx), fromLast=TRUE)

# locf - last observation carried forward.
# this fills in the NA at different sampling instants
gld_gdx <- na.locf(merge(gld, gdx), fromLast=TRUE)

# Plot them!
dev.new(); p1 <- dev.cur()
dev.new(); p2 <- dev.cur()

dev.set(p1)
plot.zoo(aapl_spx)
dev.set(p2)
plot.zoo(gld_gdx)

#raptor.hzc.close(client)

