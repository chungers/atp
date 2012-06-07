
#
new.em <- function(apiAddress = 'tcp://127.0.0.1:6666') {

  options(digits.secs=6)
  Sys.setenv(TZ='America/New_York')

  this <- new.env()
  class(this) <- 'em'

  this$.apiAddress = apiAddress

  this$.apiConnection <- .Call("api_connect", this$.apiAddress, PACKAGE='ib')

  nycTime <- function(t) {
    utc <- as.POSIXct(t, origin='1970-01-01', tz='GMT');
    as.POSIXlt(utc, 'America/New_York');
  }

  # Methods

  return(this)
}


# S3 function mapping


