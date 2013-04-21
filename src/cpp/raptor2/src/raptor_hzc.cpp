
#include <string>
#include <vector>

#include <zmq.hpp>

#include "proto/historian.hpp"
#include "historian/historian.hpp"
#include "historian/DbReactorClient.hpp"

#include "raptor_hzc.h"

using namespace Rcpp ;

using std::string;
using boost::optional;
using boost::posix_time::ptime;
using boost::uint64_t;
using zmq::context_t;

using proto::common::Value;
using proto::historian::IndexedValue;
using proto::historian::QueryBySymbol;
using proto::historian::Record;

using historian::DbReactorClient;



SEXP raptor_hzc_connect(SEXP address, SEXP callbackAddr)
{
  string hzAddress = Rcpp::as<string>(address);
  string callbackAddress = Rcpp::as<string>(callbackAddr);

  if (hzAddress.length() == 0 || callbackAddress.length() == 0) {
    return wrap(R_NilValue);
  }

  // create a zmq context
  context_t* context = new context_t(1);
  DbReactorClient* client = new DbReactorClient(hzAddress, callbackAddress,
                                                context);

  bool status = client->connect();
  if (!status) {
    Rprintf("Cannot connect to %s\n", hzAddress.c_str());
    return wrap(R_NilValue);
  }

  Rprintf("Connected to %s\n", hzAddress.c_str());
  XPtr<context_t> contextPtr(context);
  XPtr<DbReactorClient> clientPtr(client);
  List result = List::create(Named("contextPtr", contextPtr),
                             Named("clientPtr", clientPtr),
                             Named("connected", wrap(status)));
  return result;
}


RcppExport SEXP raptor_hzc_close(SEXP connectionHandle)
{
  List handleList(connectionHandle);
  XPtr<context_t> contextPtr(handleList["contextPtr"],
                             R_NilValue, R_NilValue);
  XPtr<DbReactorClient> clientPtr(handleList["clientPtr"],
                                  R_NilValue, R_NilValue);
  bool ok = as<bool>(handleList["connected"]);

  clientPtr.setDeleteFinalizer();
  contextPtr.setDeleteFinalizer();
  return wrap(ok);
}


/**
 * Visitor to accept incoming stream and either calls the callback
 * or populates the timestamp and value vectors.
 */
class Visitor : public historian::Visitor {

 public:
  Visitor(const string& event, Function* cb,
          NumericVector* tsVectorPtr, NumericVector* valueVectorPtr) :
      event_(event), callback_(cb),
      tsVectorPtr_(tsVectorPtr), valueVectorPtr_(valueVectorPtr)
  {
  }

  virtual bool operator()(const Record& data)
  {
    using namespace proto::common;
    using namespace proto::historian;

    SEXP value = Rcpp::wrap(NA_REAL);
    Value v;
    uint64_t utcMicros = 0LL;
    double utcSeconds = 0.;

    optional<IndexedValue> iv = as<IndexedValue>(data);
    if (iv) {
      v = iv->value();
      utcMicros = iv->timestamp();
      utcSeconds = static_cast<double>(utcMicros) / 1000000.f;

      switch (v.type()) {
        case proto::common::Value_Type_INT :
          value = Rcpp::wrap(static_cast<long>(v.int_value()));
          break;
        case proto::common::Value_Type_DOUBLE :
          value = Rcpp::wrap(v.double_value());
          break;
      }
      if (callback_ != NULL) {
        return as<bool>((*callback_)(
            Rcpp::wrap(data.key()),
            Rcpp::wrap(utcSeconds),
            Rcpp::wrap(event_),
            value));
      } else {
        tsVectorPtr_->push_back(utcSeconds);
        valueVectorPtr_->push_back(as<double>(value));
      }
    }
    return true;
  }

 private:
  string event_;
  Function* callback_;
  NumericVector* tsVectorPtr_;
  NumericVector* valueVectorPtr_;
};

RcppExport SEXP raptor_hzc_query(SEXP connectionHandle,
                                 SEXP symbol, SEXP event,
                                 SEXP rStart, SEXP rStop,
                                 SEXP callback, SEXP est)
{
  List handleList(connectionHandle);
  XPtr<DbReactorClient> clientPtr(handleList["clientPtr"],
                                  R_NilValue, R_NilValue);
  bool ok = as<bool>(handleList["connected"]);
  if (!ok) {
    return R_NilValue;
  }

  bool asEastern = as<bool>(est);
  string qSymbol = as<string>(symbol);
  string qEvent = as<string>(event);
  ptime utcStart, utcStop;
  historian::parse(as<string>(rStart), &utcStart, asEastern);
  historian::parse(as<string>(rStop), &utcStop, asEastern);

  QueryBySymbol q;
  q.set_symbol(qSymbol);
  q.set_utc_first_micros(historian::as_micros(utcStart));
  q.set_utc_last_micros(historian::as_micros(utcStop));
  q.set_type(proto::historian::INDEXED_VALUE);
  q.set_index(qEvent);

  Function* cb = (callback != R_NilValue) ? new Function(callback) : NULL;
  NumericVector tsVector;
  NumericVector valueVector;

  Visitor visitor(qEvent, cb, &tsVector, &valueVector);

  uint64_t start_micros = now_micros();
  int count = clientPtr->Query(q, &visitor);
  uint64_t finish_micros = now_micros();

  double totalSeconds = static_cast<double>(finish_micros - start_micros)
      / 1000000.;
  Rprintf("%d samples in %f seconds.\n", count, totalSeconds);

  if (count > 0) {
    return List::create(Named("symbol", qSymbol),
                        Named("utc_t", tsVector),
                        Named("value", valueVector)
                        );
  } else {
    return R_NilValue;
  }
}

