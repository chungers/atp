

raptor.as.ticker_id <- function(symbol) {
    .Call("raptor_to_ticker_id", as.character(symbol), PACKAGE = "raptor");
}

raptor.as.symbol <- function(tickerId) {
    .Call("raptor_to_symbol", as.numeric(tickerId), PACKAGE = "raptor");
}

