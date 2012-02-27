
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
                                    SEXP symbol,
                                    SEXP start, SEXP stop,
                                    SEXP callback, SEXP est)
{
  using std::string;
  using boost::posix_time::ptime;
  using proto::ib::MarketData;

  List handleList(dbHandle);
  XPtr<Db> leveldb(handleList["dbPtr"], R_NilValue, R_NilValue);
  bool ok = as<bool>(handleList["open"]);

  if (ok) {
    string qSymbol = as<string>(symbol);
    ptime utcStart, utcStop;
    historian::parse(as<string>(start), &utcStart, true);  // as EST
    historian::parse(as<string>(stop), &utcStop, true);

    Function cb(callback);

    struct Visitor : public historian::DefaultVisitor
    {
      Visitor(Function& cb) : callback_(cb) {}

      bool operator()(const std::string& key, const MarketData& data)
      {
        SEXP value;
        switch (data.type()) {
          case proto::ib::MarketData_Type_INT :
            value = wrap(static_cast<long>(data.int_value()));
            break;
          case proto::ib::MarketData_Type_STRING :
            value = wrap(data.string_value());
            break;
          case proto::ib::MarketData_Type_DOUBLE :
            value = wrap(data.double_value());
            break;
        }

        bool ok = as<bool>(callback_(
            wrap(data.symbol()),
            wrap(static_cast<double>(data.timestamp()) / 1000000.f),
            wrap(data.event()),
            value));
        return ok;
      }

      Function& callback_;
    } visitor(cb);

    // Now actually perform the query
    int count = leveldb->query(qSymbol, utcStart, utcStop, &visitor);
    return wrap(count);
  }
  return wrap(ok);
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
