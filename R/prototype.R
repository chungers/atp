
# Working with bigmemory
setwd('/tmp')
d <- dget('/tmp/AAPL.STK_LAST.desc')
last.bm <- attach.big.matrix(d)
last <- xts(last.bm[1:1000,2], order.by=as.POSIXlt(last.bm[1:1000,1], tz='GMT', origin='1970-01-01'), tz='America/New_York')

shm_xts <- function(dir, f) {
  desc <- dget(paste(dir, f, sep='/'))
  m <- attach.big.matrix(desc, path=dir)
  rows <- m[1,1]
  xts(m[2:rows+1,2], tz='America/New_York', 
      order.by=as.POSIXlt(m[2:rows+1,1], tz='GMT', origin='1970-01-01'))
}
