#ifndef IBAPI_FIELDS_H_
#define IBAPI_FIELDS_H_

//#include <quickfix/Field.h>
//#include <quickfix/FieldMap.h>
#include <quickfix/FixFields.h>
#include <quickfix/FixValues.h>


/// See FixFieldNumbers.h in QuickFIX.
/// Extension to include microsecond timestamps
namespace FIX {
namespace FIELD {

const int Ext_SendingTimeMicros = 100000;
const int Ext_OrigSendingTimeMicros = 100001;

} // namespace FIELD
} // namespace FIX


using namespace FIX;

/// Similar to FixFields.h, this defines the
/// valid fields for the IBAPI messages.
/// Where possbile, existing FIX fields are used.
namespace IBAPI {

DEFINE_STRING(Ext_SendingTimeMicros);
DEFINE_STRING(Ext_OrigSendingTimeMicros);

}


#endif //IBAPI_FIELDS_H_
