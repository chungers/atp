#ifndef IB_EXCEPTIONS_H_
#define IB_EXCEPTIONS_H_

#include <string>
#include <stdexcept>

namespace IBAPI {

/// Base exception type.
struct Exception : public std::logic_error
{
  Exception( const std::string& t, const std::string& d )
  : std::logic_error( d.size() ? t + ": " + d : t ),
    type( t ), detail( d )
  {}
  ~Exception() throw() {}

  std::string type;
  std::string detail;
};

/// %Application is not configured correctly
struct ConfigError : public Exception
{
  ConfigError( const std::string& what = "" )
    : Exception( "Configuration failed", what ) {}
};

/// %Application encountered serious error during runtime
struct RuntimeError : public Exception
{
  RuntimeError( const std::string& what = "" )
    : Exception( "Runtime error", what ) {}
};

struct DoNotSend : public Exception {

};

struct IncorrectDataFormat : public Exception {

};

struct IncorrectTagValue : public Exception{

};

struct UnsupportedMessageType : public Exception {

};

struct RejectLogon : public Exception {

};



} // namespace IBAPI
#endif // IB_EXCEPTIONS_H_
