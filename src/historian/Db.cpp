
#include <sstream>

#include <boost/optional.hpp>
#include <leveldb/db.h>
#include <leveldb/write_batch.h>

#include "proto/common.pb.h"
#include "proto/ib.pb.h"
#include "proto/historian.pb.h"

#include "proto/common.hpp"
#include "proto/historian.hpp"

#include "historian/historian.hpp"

using proto::common::Value;
using proto::ib::MarketData;
using proto::ib::MarketDepth;
using proto::historian::SessionLog;
using proto::historian::Record;
using proto::historian::Record_Type;

using proto::historian::QueryByRange;
using proto::historian::QueryBySymbol;
using proto::historian::QuerySessionLogs;

namespace historian {
namespace internal {

using std::ostringstream;
using std::string;
using boost::optional;
using namespace leveldb;

bool write_db(leveldb::WriteBatch* batch, leveldb::DB* levelDb)
{
  Status s = levelDb->Write(WriteOptions(), batch);
  return s.ok();
}

template <typename V>
bool write_db(const optional<string>& key, const V& value,
              leveldb::DB* levelDb,
              bool overwrite = true)
{
  if (!key) {
    return false; // do nothing -- no key
  }
  bool okToWrite = overwrite;
  if (!overwrite) {
    string readBuffer;
    Status readStatus = levelDb->Get(ReadOptions(), *key, &readBuffer);
    okToWrite = readStatus.IsNotFound();
  }

  if (okToWrite) {
    string buffer;
    value.SerializeToString(&buffer);
    Status putStatus = levelDb->Put(WriteOptions(), *key, buffer);
    return putStatus.ok();
  }
  return false;
}

template <typename V>
bool write_batch(const optional<string>& key, const V& value,
                 leveldb::WriteBatch* batch, leveldb::DB* levelDb,
                 bool overwrite = true)
{
  if (!key) {
    return false; // do nothing -- no key
  }
  bool okToWrite = overwrite;
  if (!overwrite) {
    string readBuffer;
    Status readStatus = levelDb->Get(ReadOptions(), *key, &readBuffer);
    okToWrite = readStatus.IsNotFound();
  }
  if (okToWrite) {
    string buffer;
    value.SerializeToString(&buffer);
    batch->Put(*key, buffer);
    return true;
  }
  return false;
}

template <typename V>
struct Writer
{
  const optional<string> buildKey(const V& value)
  {
    return optional<string>(); // uninitialized.
  }

  bool operator()(const V& value,
                  leveldb::DB* levelDb,
                  bool overwrite = true)
  {
    optional<string> key = buildKey(value);
    return write_db<V>(key, value, levelDb, overwrite);
  }
};

template <>
struct Writer<SessionLog>
{
  const optional<string> buildKey(const SessionLog& value)
  {
    ostringstream key;
    key << ENTITY_SESSION_LOG << ':'
        << value.symbol() << ':'
        << value.start_timestamp() << ':'
        << value.stop_timestamp();
    return key.str();
  }

  bool operator()(const SessionLog& value,
                  leveldb::DB* levelDb,
                  bool overwrite = true)
  {
    optional<string> key = buildKey(value);
    Record record;
    record.set_type(proto::historian::Record_Type_SESSION_LOG);
    record.mutable_session_log()->CopyFrom(value);
    return write_db<Record>(key, record, levelDb, overwrite);
  }
};

template <>
struct Writer<MarketDepth>
{
  const optional<string> buildKey(const MarketDepth& value)
  {
    ostringstream key;
    key << ENTITY_IB_MARKET_DEPTH << ':'
        << value.symbol() << ':'
        << value.timestamp();
    return key.str();
  }

  bool operator()(const MarketDepth& value,
                  leveldb::DB* levelDb,
                  bool overwrite = true)
  {
    optional<string> key = buildKey(value);
    Record record;
    record.set_type(proto::historian::Record_Type_IB_MARKET_DEPTH);
    record.mutable_ib_marketdepth()->CopyFrom(value);
    return write_db<Record>(key, record, levelDb, overwrite);
  }
};

template <>
struct Writer<MarketData>
{
  const optional<string> buildKey(const MarketData& value)
  {
    ostringstream key;
    key << ENTITY_IB_MARKET_DATA << ':'
        << value.symbol() << ':'
        << value.timestamp();
    return key.str();
  }

  const optional<string> buildIndexKey(const MarketData& value)
  {
    ostringstream key;
    key << INDEX_IB_MARKET_DATA_BY_EVENT << ':'
        << value.symbol() << ':'
        << value.event() << ':' << value.timestamp();
    return key.str();
  }

  bool operator()(const MarketData& value,
                  leveldb::DB* levelDb,
                  bool overwrite = true)
  {
    leveldb::WriteBatch batch;
    optional<string> key = buildKey(value);
    Record record = proto::historian::wrap<MarketData>(value);

    // Try to batch first by the primary record. If ok (e.g. based on
    // overwrite value, etc.), then update the secondary index as well.
    if (write_batch<Record>(key, record, &batch, levelDb, overwrite)) {
      // Index the value
      Record indexRecord = proto::historian::wrap<Value>(value.value());
      optional<string> indexKey = buildIndexKey(value);
      write_batch<Record>(indexKey, indexRecord, &batch, levelDb, true);
      return write_db(&batch, levelDb);
    }
    return false;
  }
};

} // internal



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

  /**
   * Writing to db via Writer objects.
   */
  template <typename T>
  bool write(const T& value, bool overwrite = true)
  {
    internal::Writer<T> writer;
    return writer(value, levelDb_, overwrite);
  }

  /**
   * Writing to db explicitly by key and value.
   */
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
  return impl_->write(value, overwrite);
}

bool Db::write(const MarketDepth& value, bool overwrite)
{
  if (!validate(value)) return false;
  return impl_->write(value, overwrite);
}

bool Db::write(const SessionLog& value, bool overwrite)
{
  if (!validate(value)) return false;
  return impl_->write(value, overwrite);
}

} // namespace historian
