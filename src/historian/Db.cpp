
#include <sstream>
#include <leveldb/db.h>

#include "historian/historian.hpp"


using proto::ib::MarketData;
using proto::ib::MarketDepth;
using proto::historian::SessionLog;
using proto::historian::Record;
using proto::historian::Record_Type;

using proto::historian::QueryByRange;
using proto::historian::QueryBySymbol;
using proto::historian::QuerySessionLogs;

namespace historian {

class Db::implementation
{
 public:
  implementation(const std::string& file) :
      dbFile_(file), levelDb_(NULL)
  {
  }

  ~implementation()
  {
    if (levelDb_ != NULL) {
      delete levelDb_;
    }
  }

  bool open()
  {
    leveldb::Options options;
    options.create_if_missing = true;
    options.block_size = 4096 * 100;
    leveldb::Status status = leveldb::DB::Open(options, dbFile_, &levelDb_);
    if (status.ok()) {
      return true;
    } else {
      levelDb_ = NULL;
      return false;
    }
  }

  template <typename T>
  bool write(const std::string& key, const T& value, bool overwrite = true)
  {
    bool okToWrite = overwrite;
    if (!overwrite) {
      std::string readBuffer;
      leveldb::Status readStatus = levelDb_->Get(leveldb::ReadOptions(),
                                                 key, &readBuffer);
      okToWrite = readStatus.IsNotFound();
    }

    if (okToWrite) {
      std::string buffer;
      value.SerializeToString(&buffer);

      leveldb::Status putStatus = levelDb_->Put(leveldb::WriteOptions(),
                                                key, buffer);
      return putStatus.ok();
    }
    return false;
  }

  int query(const std::string& start, const std::string& stop,
            Visitor* visit)
  {
    if (levelDb_ == NULL) return 0;

    boost::scoped_ptr<leveldb::Iterator> iterator(
        levelDb_->NewIterator(leveldb::ReadOptions()));

    int count = 0;
    for (iterator->Seek(start);
         iterator->Valid() && iterator->key().ToString() < stop;
         iterator->Next(), ++count) {

      leveldb::Slice key = iterator->key();
      leveldb::Slice value = iterator->value();

      using namespace proto::historian;

      Record record;
      if (record.ParseFromString(value.ToString())) {
        bool readMore = false;
        switch (record.type()) {
          case Record_Type_SESSION_LOG:
            readMore = (*visit)(record.session_log());
            break;
          case Record_Type_IB_MARKET_DATA:
            readMore = (*visit)(record.ib_marketdata());
            break;
          case Record_Type_IB_MARKET_DEPTH:
            readMore = (*visit)(record.ib_marketdepth());
            break;
        }
        if (!readMore) break;
      }
    }
    return count;
  }

  // int query(const std::string& symbol,
  //           const ptime& startUtc, const ptime& endUtc,
  //           Visitor* visit, bool depth_only = false)
  // {
  //   if (levelDb_ == NULL) return 0;
  //   std::ostringstream startkey;
  //   startkey << symbol << ":" << as_micros(startUtc);
  //   std::ostringstream endkey;
  //   endkey << symbol << ":" << as_micros(endUtc);
  //   return query(startkey.str(), endkey.str(), visit);
  // }

 private:
  std::string dbFile_;
  leveldb::DB* levelDb_;
};


Db::Db(const std::string& file) : impl_(new Db::implementation(file))
{
}

Db::~Db()
{
}

bool Db::open()
{
  return impl_->open();
}

template <typename T> inline const std::string GetPrefix()
{ return ""; }
template <> inline const std::string GetPrefix<MarketDepth>()
{ return "depth:"; }
template <> inline const std::string GetPrefix<SessionLog>()
{ return "sessionlog:"; }

template <typename T>
static const std::string buildDbKey(const T& data)
{
  using std::string;
  using std::ostringstream;
  // build the key
  ostringstream key;
  key << GetPrefix<T>()
      << data.symbol() << ':' << data.timestamp();
  return key.str();
}

int Db::query(const QueryByRange& query, Visitor* visit)
{
  FilterVisitor<QueryByRange> filtered(visit, query);
  return impl_->query(query.first(), query.last(), &filtered);
}

int Db::query(const std::string& start, const std::string& stop,
             Visitor* visit)
{
  return impl_->query(start, stop, visit);
}

int Db::query(const QueryBySymbol& query, Visitor* visit)
{
  FilterVisitor<QueryBySymbol> filtered(visit, query);
  string symbol = (query.has_depth_only() && query.depth_only()) ?
      "depth:" + query.symbol() : query.symbol();
  return Db::query(symbol,
                   historian::as_ptime(query.utc_first_micros()),
                   historian::as_ptime(query.utc_last_micros()),
                   &filtered);
}

int Db::query(const std::string& symbol,
              const ptime& startUtc, const ptime& stopUtc,
              Visitor* visit)
{
    std::ostringstream startkey;
    startkey << symbol << ":" << as_micros(startUtc);
    std::ostringstream endkey;
    endkey << symbol << ":" << as_micros(stopUtc);
    return impl_->query(startkey.str(), endkey.str(), visit);
}

template <typename T>
inline bool validate(const T& value)
{
  return value.IsInitialized();
}

bool Db::write(const MarketData& value, bool overwrite)
{
  if (!validate(value)) return false;
  Record record;
  record.set_type(proto::historian::Record_Type_IB_MARKET_DATA);
  record.mutable_ib_marketdata()->CopyFrom(value);
  return impl_->write(buildDbKey(value), record, overwrite);
}

bool Db::write(const MarketDepth& value, bool overwrite)
{
  if (!validate(value)) return false;
  Record record;
  record.set_type(proto::historian::Record_Type_IB_MARKET_DEPTH);
  record.mutable_ib_marketdepth()->CopyFrom(value);
  return impl_->write(buildDbKey(value), record, overwrite);
}

bool Db::write(const SessionLog& value, bool overwrite)
{
  if (!validate(value)) return false;
  Record record;
  record.set_type(proto::historian::Record_Type_SESSION_LOG);
  record.mutable_session_log()->CopyFrom(value);
  std::ostringstream key;
  key << "sessionlog:" << value.symbol() << ":"
      << value.start_timestamp() << ":"
      << value.stop_timestamp();
  return impl_->write(key.str(), record, overwrite);
}

} // namespace historian
