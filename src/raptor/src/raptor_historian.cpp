
#include <string>
#include <vector>

#include "proto/historian.hpp"
#include "historian/historian.hpp"
#include "raptor_historian.h"


using namespace Rcpp ;

using std::string;
using boost::optional;

using historian::Db;

SEXP raptor_historian_open(SEXP db)
{
  string leveldbFile = Rcpp::as<string>(db);
  Db* leveldb = new Db(leveldbFile);

  // try to open the file
  bool status = leveldb->Open();

  if (status) {
    XPtr<Db> dbPtr(leveldb);
    Rprintf("opened @ %s\n", leveldbFile.c_str());
    List result = List::create(Named("dbPtr", dbPtr),
                               Named("open", wrap(status)));
    return result;
  }
  return wrap(R_NilValue);
}

SEXP raptor_historian_close(SEXP dbHandle)
{
  List handleList(dbHandle);
  XPtr<Db> leveldb(handleList["dbPtr"], R_NilValue, R_NilValue);
  bool ok = as<bool>(handleList["open"]);

  leveldb.setDeleteFinalizer();
  return wrap(ok);
}

SEXP raptor_historian_ib_marketdata(SEXP dbHandle,
                                    SEXP symbol, SEXP event,
                                    SEXP start, SEXP stop,
                                    SEXP callback, SEXP est)
{
  using std::string;
  using boost::optional;
  using boost::posix_time::ptime;
  using proto::common::Value;
  using proto::historian::IndexedValue;
  using proto::historian::QueryBySymbol;
  using proto::historian::Record;

  List handleList(dbHandle);
  XPtr<Db> leveldb(handleList["dbPtr"], R_NilValue, R_NilValue);
  bool ok = as<bool>(handleList["open"]);

  if (!ok) {
    return wrap(false);
  }
  string qSymbol = as<string>(symbol);
  string qEvent = as<string>(event);
  ptime utcStart, utcStop;
  historian::parse(as<string>(start), &utcStart, true);  // as EST
  historian::parse(as<string>(stop), &utcStop, true);

  Function* cb = (callback != R_NilValue) ? new Function(callback) : NULL;

  NumericVector tsVector; //(5000);
  NumericVector valueVector; //(5000);


  struct Visitor : public historian::DefaultVisitor
  {
    Visitor(const string& symbol, Function* cb, const string& event,
            NumericVector* tsVectorPtr,
            NumericVector* valueVectorPtr) :
        symbol_(symbol), callback_(cb), event_(event),
        tsVectorPtr_(tsVectorPtr),
        valueVectorPtr_(valueVectorPtr)
    {
      Rprintf("event = %s\t", event_.c_str());
      if (callback_ == NULL) Rprintf("No callback given.\n");
    }

    ~Visitor()
    {
    }

    virtual bool operator()(const Record& data)
    {
      SEXP value;
      Value v;
      boost::uint64_t utcMicros = 0LL;

      optional<IndexedValue> iv = proto::historian::as<IndexedValue>(data);
      if (iv) {
        v = iv->value();
        utcMicros = iv->timestamp();
        switch (v.type()) {
          case proto::common::Value_Type_INT :
            value = wrap(static_cast<long>(v.int_value()));
            break;
          case proto::common::Value_Type_STRING :
            value = wrap(v.string_value());
            break;
          case proto::common::Value_Type_DOUBLE :
            value = wrap(v.double_value());
            break;
        }
        if (callback_ != NULL) {
          return as<bool>((*callback_)(
              wrap(symbol_),
              wrap(static_cast<double>(utcMicros) / 1000000.f),
              wrap(event_),
              value));
        } else {
          tsVectorPtr_->push_back(
              static_cast<double>(utcMicros) / 1000000.f);
          valueVectorPtr_->push_back(as<double>(value));

          if (tsVectorPtr_->size() % 1000 == 0) {
            Rprintf(".");
          }
        }
      }

      return true; // continue as we collect the values.
    }

    const string symbol_;
    Function* callback_;
    string event_;
    NumericVector* tsVectorPtr_;
    NumericVector* valueVectorPtr_;
  } visitor(qSymbol, cb, qEvent, &tsVector, &valueVector);

  QueryBySymbol query;
  query.set_symbol(qSymbol);
  query.set_utc_first_micros(historian::as_micros(utcStart));
  query.set_utc_last_micros(historian::as_micros(utcStop));
  // set index
  query.set_type(proto::historian::INDEXED_VALUE);
  query.set_index(qEvent);

  int count = leveldb->Query(query, &visitor);

  return List::create(Named("symbol", qSymbol),
                      Named("utc_t", tsVector),
                      Named("value", valueVector)
                      );
}

SEXP raptor_historian_sessionlogs(SEXP dbHandle, SEXP symbol)
{
  List handleList(dbHandle);
  XPtr<Db> leveldb(handleList["dbPtr"], R_NilValue, R_NilValue);
  bool ok = as<bool>(handleList["open"]);

  if (ok) {
    string searchSymbol = as<string>(symbol);

  }
  return wrap(ok);
}
