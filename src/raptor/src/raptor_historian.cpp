
#include <string>
#include <vector>

#include "historian/historian.hpp"
#include "raptor_historian.h"


using namespace Rcpp ;

using std::string;
using historian::Db;

SEXP raptor_historian_open(SEXP db)
{
  string leveldbFile = Rcpp::as<string>(db);
  Db* leveldb = new Db(leveldbFile);

  // try to open the file
  bool status = leveldb->open();

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
  using boost::posix_time::ptime;
  using proto::common::Value;
  using proto::ib::MarketData;

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
    Visitor(Function* cb, const string& event,
            NumericVector* tsVectorPtr,
            NumericVector* valueVectorPtr) :
        callback_(cb), event_(event),
        tsVectorPtr_(tsVectorPtr),
        valueVectorPtr_(valueVectorPtr)
    {
      Rprintf("event = %s\t", event_.c_str());
      if (callback_ == NULL) Rprintf("No callback given.\n");
    }
    ~Visitor()
    {
      if (callback_ != NULL) delete callback_;
    }

    bool operator()(const MarketData& data)
    {
      SEXP value;
      Value v = data.value();
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
            wrap(data.symbol()),
            wrap(static_cast<double>(data.timestamp()) / 1000000.f),
            wrap(data.event()),
            value));
      } else if (data.event() == event_) {
        tsVectorPtr_->push_back(
            static_cast<double>(data.timestamp()) / 1000000.f);
        valueVectorPtr_->push_back(as<double>(value));

        if (tsVectorPtr_->size() % 1000 == 0) {
          Rprintf(".");
        }
      }
      return true; // continue as we collect the values.
    }

    Function* callback_;
    string event_;
    NumericVector* tsVectorPtr_;
    NumericVector* valueVectorPtr_;
  } visitor(cb, qEvent, &tsVector, &valueVector);

  // Now actually perform the query
  int count = leveldb->query(qSymbol, utcStart, utcStop, &visitor);

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
