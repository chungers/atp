#ifndef HISTORIAN_INTERNAL_H_
#define HISTORIAN_INTERNAL_H_

#include <sstream>

#include <boost/optional.hpp>
#include <leveldb/db.h>
#include <leveldb/write_batch.h>

#include <glog/logging.h>

#include "proto/common.pb.h"
#include "proto/ib.pb.h"
#include "proto/historian.pb.h"

#include "proto/common.hpp"
#include "proto/historian.hpp"
#include "historian/time_utils.hpp"

using proto::common::Value;
using proto::ib::MarketData;
using proto::ib::MarketDepth;
using proto::historian::IndexedValue;
using proto::historian::SessionLog;
using proto::historian::Record;
using proto::historian::Type;

using proto::historian::QueryByRange;
using proto::historian::QueryBySymbol;

namespace historian {
namespace internal {

using std::ostringstream;
using std::string;
using boost::optional;
using boost::posix_time::ptime;
using boost::uint64_t;

using namespace leveldb;

bool write_db(leveldb::WriteBatch* batch, leveldb::DB* levelDb)
{
  Status s = levelDb->Write(WriteOptions(), batch);
  if (!s.ok()) {
    LOG(ERROR) << "Error: write batch failed: " << s.ToString();
    return false;
  } else {
    return true;
  }
}

template <typename V>
bool write_db(const string& key, const V& value,
              leveldb::DB* levelDb,
              bool overwrite = true)
{
  if (key.size() == 0) {
    LOG(ERROR) << "No key";
    return false; // do nothing -- no key
  }
  bool okToWrite = overwrite;
  if (!overwrite) {
    string readBuffer;
    Status readStatus = levelDb->Get(ReadOptions(), key, &readBuffer);
    okToWrite = readStatus.IsNotFound();
  }

  if (okToWrite) {
    string buffer;
    value.SerializeToString(&buffer);
    Status putStatus = levelDb->Put(WriteOptions(), key, buffer);
    if (!putStatus.ok()) {
      LOG(ERROR) << "Error: write failed: " << putStatus.ToString()
                 << ", key = " << key << ", buff = " << buffer;
    }
    return putStatus.ok();
  } else {
    LOG(ERROR) << "Not ok to write: skipped.";
  }
  return false;
}

template <typename V>
bool write_batch(const string& key, const V& value,
                 leveldb::WriteBatch* batch, leveldb::DB* levelDb,
                 bool overwrite = true)
{
  if (key.size() == 0) {
    LOG(ERROR) << "No key";
    return false; // do nothing -- no key
  }
  bool okToWrite = overwrite;
  if (!overwrite) {
    string readBuffer;
    Status readStatus = levelDb->Get(ReadOptions(), key, &readBuffer);
    okToWrite = readStatus.IsNotFound();
  }
  if (okToWrite) {
    string buffer;
    value.SerializeToString(&buffer);
    batch->Put(key, buffer);
    return true;
  } else {
    LOG(ERROR) << "Not ok to write: skipped batch.";
  }
  return false;
}

template <typename T>
struct KeyBuilder
{
  const string operator()(const T& value)
  {
    return "";
  }

  const string operator()(const string& symbol, uint64_t time_micros)
  {
    return "";
  }

  const string operator()(const string& symbol, ptime timstamp)
  {
    return "";
  }
};

template <> struct KeyBuilder<MarketData>
{
  const string operator()(const MarketData& value)
  {
    return (*this)(value.symbol(), value.timestamp());
  }

  const string operator()(const string& symbol, uint64_t time_micros)
  {
    ostringstream key;
    key << ENTITY_IB_MARKET_DATA << ':' << symbol << ':' << time_micros;
    return key.str();
  }

  const string operator()(const string& symbol, ptime timestamp)
  {
    return (*this)(symbol, as_micros(timestamp));
  }
};

template <> struct KeyBuilder<MarketDepth>
{
  const string operator()(const MarketDepth& value)
  {
    return (*this)(value.symbol(), value.timestamp());
  }

  const string operator()(const string& symbol, uint64_t time_micros)
  {
    ostringstream key;
    key << ENTITY_IB_MARKET_DEPTH << ':' << symbol << ':' << time_micros;
    return key.str();
  }

  const string operator()(const string& symbol, ptime timestamp)
  {
    return (*this)(symbol, as_micros(timestamp));
  }
};

template <> struct KeyBuilder<SessionLog>
{
  const string operator()(const SessionLog& value)
  {
    ostringstream key;
    key << ENTITY_SESSION_LOG << ':'
        << value.symbol() << ':'
        << value.start_timestamp() << ':'
        << value.stop_timestamp();
    return key.str();
  }

  const string operator()(const string& symbol, uint64_t time_micros)
  {
    ostringstream key;
    key << ENTITY_SESSION_LOG << ':' << symbol << ':' << time_micros;
    return key.str();
  }

  const string operator()(const string& symbol, ptime timestamp)
  {
    return (*this)(symbol, as_micros(timestamp));
  }
};


template <typename V>
struct Writer
{
  bool operator()(const V& value,
                  leveldb::DB* levelDb,
                  bool overwrite = true)
  {
    KeyBuilder<V> buildKey;
    const string key = buildKey(value);
    return write_db<V>(key, value, levelDb, overwrite);
  }
};

template <>
struct Writer<SessionLog>
{
  bool operator()(const SessionLog& value,
                  leveldb::DB* levelDb,
                  bool overwrite = true)
  {
    using namespace proto::historian;
    KeyBuilder<SessionLog> buildKey;
    string key = buildKey(value);
    Record record;
    record.set_type(SESSION_LOG);
    record.mutable_session_log()->CopyFrom(value);
    return write_db<Record>(key, record, levelDb, overwrite);
  }
};

template <>
struct Writer<MarketDepth>
{
  bool operator()(const MarketDepth& value,
                  leveldb::DB* levelDb,
                  bool overwrite = true)
  {
    using namespace proto::historian;
    KeyBuilder<MarketDepth> buildKey;
    string key = buildKey(value);
    Record record;
    record.set_type(IB_MARKET_DEPTH);
    record.mutable_ib_marketdepth()->CopyFrom(value);
    return write_db<Record>(key, record, levelDb, overwrite);
  }
};

template <>
struct Writer<MarketData>
{
  const string buildIndexKey(const MarketData& value)
  {
    return this->buildIndexKey(value.symbol(),
                               value.event(),
                               value.timestamp());
  }

  const string buildIndexKey(const string& symbol,
                             const string& event,
                             uint64_t timestamp)
  {
    ostringstream key;
    key << INDEX_IB_MARKET_DATA_BY_EVENT << ':'
        << symbol << ':'
        << event << ':' << timestamp;
    return key.str();
  }

  bool operator()(const MarketData& value,
                  leveldb::DB* levelDb,
                  bool overwrite = true)
  {
    leveldb::WriteBatch batch;
    KeyBuilder<MarketData> buildKey;
    string key = buildKey(value);
    Record record = proto::historian::wrap<MarketData>(value);

    // Try to batch first by the primary record. If ok (e.g. based on
    // overwrite value, etc.), then update the secondary index as well.
    if (write_batch<Record>(key, record, &batch, levelDb, overwrite)) {
      // Index the value
      IndexedValue iv;
      iv.set_timestamp(value.timestamp());
      iv.mutable_value()->CopyFrom(value.value());

      Record indexRecord = proto::historian::wrap<IndexedValue>(iv);
      string indexKey = buildIndexKey(value);
      write_batch<Record>(indexKey, indexRecord, &batch, levelDb, true);
      return write_db(&batch, levelDb);
    }
    return false;
  }
};

} // internal
} // namespace historian

#endif //HISTORIAN_INTERNAL_H_
