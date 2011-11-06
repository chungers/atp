
raptor.zmq.connect <- function(addr, type) {
    handle <- .Call("raptor_zmq_connect",
        as.character(addr), as.character(type),
        PACKAGE = "raptor");
    handle
}

raptor.zmq.send <- function(handle, message) {
    .Call("raptor_zmq_send", handle, message, PACKAGE = "raptor");
}

