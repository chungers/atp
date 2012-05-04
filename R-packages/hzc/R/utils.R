
# xts = xts object
# m = multiple in units (mins, hours)
as.ohlc <- function(xts, m=1, on='mins') {
  # get the prefix from column name like 'AAPL.ASK':
  prefix <- colnames(xts)[1]
  cols <- c(
            paste(c(prefix, "Open"), collapse="."),
            paste(c(prefix, "High"), collapse="."),
            paste(c(prefix, "Low"), collapse="."),
            paste(c(prefix, "Close"), collapse=".")
            )
  # end of the intervals specified: (end of seconds, for example)
  eps <- endpoints(xts, on)
  # by multiples of 2, 4, 5 seconds/minutes, etc.
  # seq.int = internal form of seq()
  if (m > 1) {
    eps <- eps[seq.int(1L, length(eps), m)];
  }

  hi <- period.apply(xts, INDEX=eps, FUN=max)
  lo <- period.apply(xts, INDEX=eps, FUN=min)
  op <- period.apply(xts, INDEX=eps, FUN=head, 1)
  cl <- period.apply(xts, INDEX=eps, FUN=tail, 1)

  m <- merge(op, hi, lo, cl)

  colnames(m) <- cols
  return(m)
}
