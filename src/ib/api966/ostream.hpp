#ifndef IBAPI_OSTREAM_H_
#define IBAPI_OSTREAM_H_

#include <iostream>

#include <Shared/Execution.h>
#include <Shared/Order.h>
#include <Shared/OrderState.h>


std::ostream& operator<<(std::ostream& out, const Order& v);

std::ostream& operator<<(std::ostream& out, const OrderState& v);

std::ostream& operator<<(std::ostream& out, const Execution& v);



#endif //IBAPI_OSTREAM_H_
