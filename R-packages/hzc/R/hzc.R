
# https://github.com/lab616/hub/wiki/Historian:---the-Market-Data-Backend
#
new.hzc <- function(cbPort = 1112,
                    hzLocalPort = 1111,
                    hzPort = 1111,
                    hzHost = 'prod1.lab616.com',
                    hzUser = 'jenkins',
                    startTunnel = TRUE) {

  stopifnot(require('xts', quietly=TRUE))
  options(digits.secs=6)
  Sys.setenv(TZ='America/New_York')

  this <- new.env()
  class(this) <- 'hzc'

  this$.cbPort = cbPort
  this$.hzPort = hzPort
  this$.hzLocalPort = hzLocalPort
  this$.historianAddress <- paste('tcp://127.0.0.1', hzPort,
                                  sep=':')
  this$.callbackAddress <- paste('tcp://127.0.0.1', cbPort,
                                 sep=':')

  if (startTunnel) {
    # The tunnel runs as a child process of the R session.
    # When R session exits, the tunnel will be closed.
    cmd <- paste('ssh ', hzUser, '@', hzHost,
                 ' -R ', cbPort, ':localhost:', cbPort,
                 ' -L ', hzLocalPort, ':localhost:', hzPort,
                 ' -f -N', sep='')
    message('Starting tunnel: ', cmd)
    system(cmd, wait=FALSE)
  }

  # connect
  this$.connection <- .Call("hzc_connect",
                            this$.historianAddress,
                            this$.callbackAddress, PACKAGE='hzc')

  nycTime <- function(t) {
    utc <- as.POSIXct(t, origin='1970-01-01', tz='GMT');
    as.POSIXlt(utc, 'America/New_York');
  }

  # retrieve marketdata
  this$mktdata <- function(symbol, event, qStart, qStop,
                           callback=NULL, est=TRUE) {
    raw <- .Call("hzc_query",
                 this$.connection,
                 as.character(symbol), as.character(event),
                 as.character(qStart), as.character(qStop),
                 callback, est,
                 PACKAGE="hzc")

    xts <- as.xts(raw$value, order.by=nycTime(raw$utc_t),
                  tzone='America/New_York')
    colnames(xts) <- c(paste(symbol, event, sep='.'))
    return(xts)
  }
  environment(this$mktdata) <- as.environment(this)


  killProcess <- function(port) {
    cmd <- paste('lsof -i TCP@localhost:', port, ' -F p', sep='')

    # don't kill the parent process which is the R session
    # find child process
    p <- system(cmd, intern=TRUE)
    p <- max(as.numeric(sub('p', '', p)))

    if (p > 0) {
      cmd <- paste('kill -9 ', p, sep=' ')
      message(cmd)
      system(cmd, wait=FALSE)
    }
  }

  this$stopTunnel <- function() {
    killProcess(this$.hzLocalPort)
  }
  environment(this$stopTunnel) <- as.environment(this)

  return(this)
}


# S3 function mapping
stopTunnel <- function(x) UseMethod('stopTunnel')
stopTunnel.hzc <- function(x) x$stopTunnel()

mktdata <- function(x, symbol, event, qStart, qStop,
                    callback = NULL, est = TRUE)
  UseMethod('mktdata')
mktdata.hzc <- function(x, symbol, event, qStart, qStop,
                        callback = NULL, est = TRUE)
  x$mktdata(symbol, event, qStart, qStop, callback, est)

# BID
bid <- function(x, symbol, qStart, qStop,
                callback = NULL, est = TRUE)
  UseMethod('bid')
bid.hzc <- function(x, symbol, qStart, qStop,
                    callback = NULL, est = TRUE)
  x$mktdata(symbol, 'BID', qStart, qStop, callback, est)

# ASK
ask <- function(x, symbol, qStart, qStop,
                callback = NULL, est = TRUE)
  UseMethod('ask')
ask.hzc <- function(x, symbol, qStart, qStop,
                    callback = NULL, est = TRUE)
  x$mktdata(symbol, 'ASK', qStart, qStop, callback, est)

# LAST
lastTrade <- function(x, symbol, qStart, qStop,
                 callback = NULL, est = TRUE)
  UseMethod('lastTrade')
lastTrade.hzc <- function(x, symbol, qStart, qStop,
                     callback = NULL, est = TRUE)
  x$mktdata(symbol, 'LAST', qStart, qStop, callback, est)

# BID_SIZE
bidSize <- function(x, symbol, qStart, qStop,
                    callback = NULL, est = TRUE)
  UseMethod('bidSize')
bidSize.hzc <- function(x, symbol, qStart, qStop,
                        callback = NULL, est = TRUE)
  x$mktdata(symbol, 'BID_SIZE', qStart, qStop, callback, est)

# ASK_SIZE
askSize <- function(x, symbol, qStart, qStop,
                callback = NULL, est = TRUE)
  UseMethod('askSize')
askSize.hzc <- function(x, symbol, qStart, qStop,
                    callback = NULL, est = TRUE)
  x$mktdata(symbol, 'ASK_SIZE', qStart, qStop, callback, est)

# LAST_SIZE
lastSize <- function(x, symbol, qStart, qStop,
                     callback = NULL, est = TRUE)
  UseMethod('lastSize')
lastSize.hzc <- function(x, symbol, qStart, qStop,
                         callback = NULL, est = TRUE)
  x$mktdata(symbol, 'LAST_SIZE', qStart, qStop, callback, est)


