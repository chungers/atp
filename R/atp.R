


atp.load <- function(file="") {
# Loads CSV file processed by LogReader
# Returns data frame
	colNames <- c("timestamp_utc", "symbol", "event", "value")
	colTypes <- c("character", "character", "character", "numeric")
	options(digits.sec=6)
	
	# xts cannot support mix data types, so use data.frame
	df <- read.table(file, sep=",", header=TRUE,
					col.names = colNames, colClasses = colTypes)
	df
}

atp.extract <- function(dframe, symbl, evnt) {
# Extracts events of 'event' for 'symbol' from xts data.
# @param dframe - dataFrame from LogReader.loadCsv
# @param symbol - the symbol (e.g. 'GOOG')
# @param event - the event (e.g. 'LAST')
# @return data frame

	# simple subsetting by the symbol and event from raw data.
	# dframe[dframe$symbol==symbol & dframe$event == event, ]
	
	# check for column names
	if (sum(c("symbol", "event") %in% colnames(dframe)) == 2) {
		subset(dframe, symbol==symbl & event == evnt)
	} else {
		stop("No columns named symbol and/or event.")
		traceback()
	}
}

atp.as.xts <- function(df, name = "value") {
	reqColumns = c(name, "timestamp_utc")
	if (sum(reqColumns %in% colnames(df)) != 2) {
		stop("No columns: ", reqColumns)
		traceback()
	}
	xts <- as.xts(df[name], order.by=as.POSIXlt(df$timestamp_utc, tz="UTC"))
	colnames(xts) <- c(name)
	xts
}

atp.tradingHoursForDay <- function(dateString = "", tz="America/New_York") {
# Returns a vector of the start and end POSIXct of the trading day.
# @param dateString - date string e.g. '2011-10-04'
# @param tz - timezone e.g. 'America/New_York' (default)
# @return [ start, end ] 	
	tdate <- if (dateString == "") Sys.Date() else as.Date(dateString)
	tstart <- as.POSIXct(paste(tdate, " 09:30:00"), tz)
	tstop <- as.POSIXct(paste(tdate, " 15:59:59"), tz)
	c(tstart, tstop)
}

atp.tradingHours <- paste(format(atp.tradingHoursForDay(), 
					"T%H:%M:%S", tz="UTC"), collapse='/')
					
atp.rth <- function(data) {
# Filters data (dataframe or xts) by regular trading hours
# @param data - univariate data frame or xts (e.g. all ASK or all BID)
# @return xts filtered by Regular Trading Hours	
	if (ncol(data) > 2) {
		warning("Input data has extra columns: ", 
			paste(colnames(data), collapse=","))
	}
	xts <- if ("xts" %in% class(data)) data else atp.as.xts(data)
	tHours <- atp.tradingHoursForDay()
	filter <- paste(format(tHours, "T%H:%M:%S", tz="UTC"), collapse='/')
	xts[filter]
}

atp.data <- function(df, symbol, event, rth=TRUE) {
# Filters the dataframe by symbol and event.	
# @param df - The data frame
# @param symbol - The symbol to filter (e.g AAPL)
# @param event - The event to filter (e.g. ASK_SIZE)
# @param rth - Regular Trading Hour
# @return xts data object -- can be passed into atp.ohlc()

	df <- atp.extract(df, symbol, event)
	xts <- atp.as.xts(df)
	colnames(xts) <- paste(c(symbol, event), collapse=".")
	atp.rth(xts)
}

atp.ohlc <- function(data, on="secs", multiple=1) {
	# get the prefix from column name like 'AAPL.ASK':
	prefix <- colnames(data)[1]
	cols <- c(
		paste(c(prefix, "Open"), collapse="."),
		paste(c(prefix, "High"), collapse="."),
		paste(c(prefix, "Low"), collapse="."),
		paste(c(prefix, "Close"), collapse="."),
		paste(c(prefix, "Count"), collapse=".")
		)
	 
	xts <- if ("xts" %in% class(data)) data else atp.as.xts(data)
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

atp.process_output <- function(fname) {
  
  library(quantmod)

  colNames <- c('timestamp', 'id', 'label', 'v1', 'v2', 'v3', 'v4')
  colTypes <- c('character', 'character', 'character', 'numeric', 'numeric', 'numeric', 'numeric')
  df <- read.table(fname, sep=',', fill=TRUE, col.names=colNames, colClasses=colTypes)
  last <- subset(df, df$label=='last')
  last.xts <- as.xts(df$v1, order.by=as.POSIXlt(df$timestamp, tz="Americas/New_York"))
  ohlc <- subset(df, df$label=='ohlc')
  open.xts <- as.xts(ohlc$v1, order.by=as.POSIXlt(ohlc$timestamp, tz="Americas/New_York"))
  high.xts <- as.xts(ohlc$v2, order.by=as.POSIXlt(ohlc$timestamp, tz="Americas/New_York"))
  low.xts <- as.xts(ohlc$v3, order.by=as.POSIXlt(ohlc$timestamp, tz="Americas/New_York"))
  close.xts <- as.xts(ohlc$v4, order.by=as.POSIXlt(ohlc$timestamp, tz="Americas/New_York"))
  cols <- c('Open', 'High', 'Low', 'Close')
  ohlc.xts <- cbind(open.xts, high.xts, low.xts, close.xts)
  colnames(ohlc.xts) <- cols
 
  chartSeries(ohlc.xts)
  
  list(last=last.xts, ohlc=ohlc.xts)
}
