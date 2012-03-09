#ifndef HISTORIAN_INTERNAL_H_
#define HISTORIAN_INTERNAL_H_

#include <sstream>

#include <boost/optional.hpp>
#include <leveldb/db.h>
#include <leveldb/write_batch.h>

#include "proto/common.pb.h"
#include "proto/ib.pb.h"
#include "proto/historian.pb.h"

#include "proto/common.hpp"
#include "proto/historian.hpp"

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
} // namespace historian

#endif //HISTORIAN_INTERNAL_H_
