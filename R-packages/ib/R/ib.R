
#
new.ib <- function(fhAddress = 'tcp://127.0.0.1:6666',
                   emAddress = 'tcp://127.0.0.1:6667') {

  options(digits.secs=6)
  Sys.setenv(TZ='America/New_York')

  this <- new.env()
  class(this) <- 'ib'

  this$.fhAddress = fhAddress
  this$.emAddress = emAddress

  # Get fh (firehose) client
  this$fh <- function() {
    new.fh(this$fhAddress)
  }
  environment(this$fh) <- as.environment(this)

  # Get em (execution manager) client
  this$em <- function() {
    new.em(this$fhAddress)
  }
  environment(this$fh) <- as.environment(this)

  return(this)
}


# S3 function mapping

# Get fh client
fh <- function(x, ...)
  UseMethod('fh')
fh.ib <- function(x, ...)
  x$fh(...)

# Get em client
em <- function(x, ...)
  UseMethod('em')
em.ib <- function(x, ...)
  x$em(...)

