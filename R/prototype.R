
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

ohlc_from_xts <- function(xts, on="mins", multiple=1) {
  # get the prefix from column name like 'AAPL.ASK':
	prefix <- colnames(data)[1]
	cols <- c(
		paste(c(prefix, "Open"), collapse="."),
		paste(c(prefix, "High"), collapse="."),
		paste(c(prefix, "Low"), collapse="."),
		paste(c(prefix, "Close"), collapse="."),
		paste(c(prefix, "Count"), collapse=".")
		)
	# end of the intervals specified: (end of seconds, for example)
	eps <- endpoints(xts, on)
	# by multiples of 2, 4, 5 seconds/minutes, etc.  seq.int = internal form of seq()
	eps <- eps[seq.int(1L, length(eps), multiple)];
	
	# the following throws an error…
	# apparently it's failing with functions that takes extra args (e.g. head, tail)
	#m <- period.apply(xts, INDEX=eps, FUN=function(x){c(head(x,1), max(x), min(x), tail(x,1), length(x))})
	
	hi <- period.apply(xts, INDEX=eps, FUN=max)
	lo <- period.apply(xts, INDEX=eps, FUN=min)
	op <- period.apply(xts, INDEX=eps, FUN=head, 1)
	cl <- period.apply(xts, INDEX=eps, FUN=tail, 1)
	ct <- period.apply(xts, INDEX=eps, FUN=length)
	m <- cbind(op, hi, lo, cl, ct)
	
	colnames(m) <- cols
	m
}