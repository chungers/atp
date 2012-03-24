options.nextFriday <- function(t = Sys.Date(), asString = TRUE) {
# Given the date, return the next friday.
  if (!require(lubridate)) {
    install.packages('lubridate')
    library(lubridate)
  }
  offset <- as.numeric(6 - wday(t))  # 6 is friday
  offset <- ifelse(offset < 0, 7 + offset, offset)
  nextFriday <- t + offset
  return(ifelse(asString, format(nextFriday, "%Y%m%d"), nextFriday))
}

options.StrikesAboutPrice <- function(price, strikes) {
# price is the mid-point (ATM)
# return vector of n strikes about this price

  increment <- ifelse(price > 50, 5, 1)
  k <- seq(0, ceiling(strikes/2))
  increment * c(floor(price/increment) - rev(k), ceiling(price/increment) + k)
}

options.getStrikesFromPrevClosing <- function(symbol, strikes) {
# Given a symbol, determine a set of strikes based on
# the last day's closing price.

  library(quantmod)
  # load the data first
  data <- getSymbols(symbol, src='google', auto.assign=FALSE)

  # Get the last closing price
  lastDay <- tail(data, 1)
  lastClose <- as.numeric(Cl(lastDay))
  increment <- ifelse(lastClose > 50, 5, 1)

  message('symbol = ', symbol,
          ', lastDay = ', index(lastDay),
          ', lastClose = ', lastClose,
          ', increment = ', increment)

  options.StrikesAboutPrice(lastClose, strikes);
}

options.buildOptionStrikes <- function(symbol, expiry,
                                       num_strikes = 8) {
# Builds a list of option symbols -- note that this is a matching
# expression where calls and puts will match
# e.g. AAPL.OPT.20120210.450
  library(foreach)
  option_strikes <- options.getStrikesFromPrevClosing(symbol, num_strikes)
  option_symbols <- foreach(strike = option_strikes, .combine='c') %do% {
    paste(symbol, 'OPT', expiry, strike, sep='.')
  }
  return(option_symbols)
}


