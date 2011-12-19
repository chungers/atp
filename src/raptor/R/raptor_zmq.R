
raptor.zmq.connect <- function(addr, type) {
  handle <- .Call("raptor_zmq_connect",
                  as.character(addr), as.character(type),
                  PACKAGE = "raptor");
  handle
}

raptor.zmq.disconnect <- function(handle) {
  result <- .Call("raptor_zmq_disconnect",
                  handle,
                  PACKAGE = "raptor");
  rm(handle)
  result
}

raptor.zmq.send <- function(handle, message) {
  # Check the input mode and peform conversion if necessary.
  result <- switch (
      mode(message),
      character =
      .Call("raptor_zmq_send",
            handle,
            message,
            PACKAGE = "raptor"),
      logical =
      .Call("raptor_zmq_send",
            handle,
            as.character(message),
            PACKAGE = "raptor"),
      numeric =
      .Call("raptor_zmq_send",
            handle,
            as.character(message),
            PACKAGE = "raptor"),
      list =
      .Call("raptor_zmq_send_kv",
            handle,
            names(message),
            lapply(message, function(f) { as.character(f) }),
            PACKAGE = "raptor"),
      'Not supported');

  result
}

