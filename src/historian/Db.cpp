
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
#include "historian/internal.hpp"

using proto::common::Value;
using proto::ib::MarketData;
using proto::ib::MarketDepth;
using proto::historian::SessionLog;
using proto::historian::Record;
using proto::historian::Type;

using proto::historian::QueryByRange;
using proto::historian::QueryBySymbol;

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
        bool readMore = (*visit)(record);
        if (!readMore) break;
      }
    }
    return count;
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

int Db::query(const QueryByRange& query, Visitor* visit)
{
  return impl_->query(query.first(), query.last(), visit);
}

int Db::query(const std::string& start, const std::string& stop,
             Visitor* visit)
{
  return impl_->query(start, stop, visit);
}

int Db::query(const QueryBySymbol& query, Visitor* visit)
{
  using namespace historian::internal;
  using namespace proto::historian;
  switch (query.type()) {
    case IB_MARKET_DATA: {
      KeyBuilder<MarketData> buildKey;
      return this->query(buildKey(query.symbol(), query.utc_first_micros()),
                         buildKey(query.symbol(), query.utc_last_micros()),
                         visit);
    }
    case IB_MARKET_DEPTH: {
      KeyBuilder<MarketDepth> buildKey;
      return this->query(buildKey(query.symbol(), query.utc_first_micros()),
                         buildKey(query.symbol(), query.utc_last_micros()),
                         visit);
    }
    case SESSION_LOG: {
      KeyBuilder<SessionLog> buildKey;
      return this->query(buildKey(query.symbol(), query.utc_first_micros()),
                         buildKey(query.symbol(), query.utc_last_micros()),
                         visit);
    }
    case VALUE: {
      if (query.has_index()) {
        // TODO figure out a cleaner way
        Writer<MarketData> writer;
        return this->query(writer.buildIndexKey(query.symbol(),
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
  return value.IsInitialized();
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

} // namespace historian
