
#include <sstream>

#include <boost/optional.hpp>
#include <leveldb/db.h>
#include <leveldb/write_batch.h>

#include <gflags/gflags.h>

#include "proto/common.pb.h"
#include "proto/ib.pb.h"
#include "proto/historian.pb.h"

#include "proto/common.hpp"
#include "proto/historian.hpp"

#include "historian/historian.hpp"
#include "historian/internal.hpp"

#include "varz/varz.hpp"


using namespace atp::common;

using proto::common::Value;
using proto::ib::MarketData;
using proto::ib::MarketDepth;
using proto::historian::SessionLog;
using proto::historian::IndexedValue;
using proto::historian::Record;
using proto::historian::Type;

using proto::historian::QueryByRange;
using proto::historian::QueryBySymbol;


DEFINE_int32(leveldb_max_open_files, 0,
             "Leveldb max open files - default is 1000.");
DEFINE_int32(leveldb_block_size, 0,
             "Leveldb block size - default is 4k.");
DEFINE_int32(leveldb_write_buffer_size, 0,
             "Leveldb write buffer size - default is 4MB");

DEFINE_VARZ_int64(leveldb_writes, 0, "total writes");
DEFINE_VARZ_int64(leveldb_write_start, 0, "timestamp for start of write");
DEFINE_VARZ_int64(leveldb_write_finish, 0, "timestamp for finish of write");
DEFINE_VARZ_int64(leveldb_write_elapsed, 0, "micros from write start to finish");
DEFINE_VARZ_int64(leveldb_write_micros, 0, "micros taken to write");


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

  bool Open()
  {
    leveldb::Options options;
    options.create_if_missing = true;

    if (FLAGS_leveldb_block_size > 0) {
      options.block_size = FLAGS_leveldb_block_size;
    }
    if (FLAGS_leveldb_max_open_files > 0) {
      options.max_open_files = FLAGS_leveldb_max_open_files;
    }
    if (FLAGS_leveldb_write_buffer_size > 0) {
      options.write_buffer_size = FLAGS_leveldb_write_buffer_size;
    }

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
    VARZ_leveldb_write_start = now_micros();
    VARZ_leveldb_write_micros = VARZ_leveldb_write_start;

    // If write blocks for a long time, VARZ_leveldb_write_micros will be a large
    // value that can be easily detected.

    bool written = writer(value, levelDb_, overwrite);
    VARZ_leveldb_write_finish = now_micros();
    VARZ_leveldb_write_elapsed = VARZ_leveldb_write_finish - VARZ_leveldb_write_start;
    VARZ_leveldb_write_micros = VARZ_leveldb_write_finish - VARZ_leveldb_write_micros;
    VARZ_leveldb_writes++;
    return written;
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
      Record record;
      if (record.ParseFromString(value.ToString())) {
        const string& k = key.ToString();
        record.set_key(k);
        bool readMore = (*visit)(record);
        if (!readMore) break;
      }
    }
    return count;
  }

  const std::string GetDbPath()
  {
    return dbFile_;
  }

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

bool Db::Open()
{
  return impl_->Open();
}

int Db::Query(const QueryByRange& query, Visitor* visit)
{
  return impl_->query(query.first(), query.last(), visit);
}

int Db::Query(const std::string& start, const std::string& stop,
             Visitor* visit)
{
  return impl_->query(start, stop, visit);
}

int Db::Query(const QueryBySymbol& query, Visitor* visit)
{
  using namespace historian::internal;
  using namespace proto::historian;
  switch (query.type()) {
    case IB_MARKET_DATA: {
      KeyBuilder<MarketData> buildKey;
      return this->Query(buildKey(query.symbol(), query.utc_first_micros()),
                         buildKey(query.symbol(), query.utc_last_micros()),
                         visit);
    }
    case IB_MARKET_DEPTH: {
      KeyBuilder<MarketDepth> buildKey;
      return this->Query(buildKey(query.symbol(), query.utc_first_micros()),
                         buildKey(query.symbol(), query.utc_last_micros()),
                         visit);
    }
    case SESSION_LOG: {
      KeyBuilder<SessionLog> buildKey;
      return this->Query(buildKey(query.symbol(), query.utc_first_micros()),
                         buildKey(query.symbol(), query.utc_last_micros()),
                         visit);
    }
    case INDEXED_VALUE: {
      if (query.has_index()) {
        // TODO figure out a cleaner way
        Writer<MarketData> writer;
        return this->Query(writer.buildIndexKey(query.symbol(),
                                                query.index(),
                                                query.utc_first_micros()),
                           writer.buildIndexKey(query.symbol(),
                                                query.index(),
                                                query.utc_last_micros()),
                           visit);

      } else {
        return 0;
      }
    }
    default:
      return 0;
  }
}


template <typename T>
inline bool validate(const T& value)
{
  bool init = value.IsInitialized();
  if (!init) {
    LOG(ERROR) << "Not initialized!";
  }
  return init;
}

template <typename T>
bool Db::Write(const T& value, bool overwrite)
{
  if (!validate(value)) return false;
  return impl_->write(value, overwrite);
}

// Instantiate the templates for the closed set of types we support:
template bool Db::Write<MarketData>(const MarketData&, bool);
template bool Db::Write<MarketDepth>(const MarketDepth&, bool);
template bool Db::Write<SessionLog>(const SessionLog&, bool);

const std::string Db::GetDbPath()
{
  return impl_->GetDbPath();
}

} // namespace historian
