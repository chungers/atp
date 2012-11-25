#ifndef ATP_PLATFORM_MARKETDATA_HANDLER_H_
#define ATP_PLATFORM_MARKETDATA_HANDLER_H_


#include <string>

#include <boost/function.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/unordered_map.hpp>

#include "platform/types.hpp"
#include "platform/callback.hpp"

using std::string;
using boost::function;
using boost::unordered_map;
using atp::platform::types::timestamp_t;


namespace atp {
namespace platform {
namespace marketdata {


namespace error_code {

const int DISPATCHED = 1;
const int NO_UPDATER_CONTINUE = 2;
const int NO_VALUE_TYPE_MATCH = -100;
const int EXCEPTION = -200;

} // error_code

template <typename EventClass, typename serialized_data_t>
inline bool deserialize(const serialized_data_t& raw, EventClass& event);

template <typename EventClass>
inline const timestamp_t get_timestamp(const EventClass& event);

template <typename EventClass, typename event_code_t>
inline const event_code_t& get_event_code(const EventClass& event);

template <typename EventClass, typename V>
inline const V& get_value(const EventClass& event);


/// based on the event code and the type of the event,
/// dispatches to the appropriate callback that's been registered
/// for a particular event (e.g. 'ASK', which is double).
template <typename EventClass, typename event_code_t = string>
class value_updater
{
 public:

  /// Typed dispatcher that contains the mapping of the event code
  /// and the typed callback (e.g. a callback that takes timestamp
  /// and a double for a 'BID' event.)
  template <typename V>
  class typed_dispatcher
  {
   public:

    typedef function<void(const timestamp_t&, const V&)> callback;

    /// binds a typed callback to a specific event code.
    void bind(const event_code_t& event_code, callback cb)
    {
      dispatch_map_.insert(
          std::pair<event_code_t, callback >(event_code, cb));
    }

    /// dispatches an event, with a timestamp and a typed value to the
    /// typed callback that's been registered.
    /// returns true if dispatch occurred.
    int dispatch(const event_code_t& event, const timestamp_t& ts, const V& v)
    {
      typename dispatch_map::iterator itr = dispatch_map_.find(event);
      if (itr != dispatch_map_.end()) {
        try {
          itr->second(ts, v);
          return error_code::DISPATCHED; // dispatched.

        } catch (...) {
          LOG(WARNING) << "Exception while processing " << event
                       << ": " << ts << "," << v;
          return error_code::EXCEPTION;
        }
      }
      return error_code::NO_UPDATER_CONTINUE;
    }

   private:
    typedef unordered_map<event_code_t, callback > dispatch_map;

    dispatch_map dispatch_map_;
  };

  inline void bind(const event_code_t& event,
                   callback::update_event<double>::func updater)
  {
    double_dispatcher_.bind(event, updater);
  }

  inline void bind(const event_code_t& event,
                   callback::update_event<int>::func updater)
  {
    int_dispatcher_.bind(event, updater);
  }

  inline void bind(const event_code_t& event,
                   callback::update_event<string>::func updater)
  {
    string_dispatcher_.bind(event, updater);
  }


  /// actual operator called that performs the dispatch.
  /// There are template specializations for support for protobuffer
  /// (proto::ib::MarketData for example).
  int operator()(const timestamp_t& ts,
                 const event_code_t& event_code,
                 const EventClass& event);

 private:

  typed_dispatcher<int> int_dispatcher_;
  typed_dispatcher<double> double_dispatcher_;
  typed_dispatcher<string> string_dispatcher_;
};



/// Market data handler that generalizes
/// 1. an event class
/// 2. special serialized data type (e.g. string)
/// 3. type of message key (e.g. string)
/// 4. event type code that can be extracted after deserialization
template <class EventClass,
          typename event_code_t = string,
          typename message_key_t = string,
          typename serialized_data_t = string
          >
class marketdata_handler
{

 public:

  typedef event_code_t event_code_type;

  int operator()(const message_key_t& key,
                 const serialized_data_t& msg)
  {
    return process_event(key, msg);
  }

  /// processes the raw event in the form of the message key
  /// which is oftentimes the topic in a subscription, and
  /// some serialized form of data.
  int process_event(const message_key_t& key,
                    const serialized_data_t& msg)
  {
    EventClass event;
    if (deserialize(msg, event)) {

      timestamp_t ts = get_timestamp<EventClass>(event);
      event_code_t event_code =
          get_event_code<EventClass, event_code_t>(event);

      return updaters_(ts, event_code, event);

    } else {
      LOG(WARNING) << "Cannot deserialize: " << key << ", msg = " << msg;
      return -1;
    }
  }


  inline void bind(const event_code_t& event,
                   callback::update_event<double>::func updater)
  {
    updaters_.bind(event, updater);
  }

  inline void bind(const event_code_t& event,
                   callback::update_event<int>::func updater)
  {
    updaters_.bind(event, updater);
  }

  inline void bind(const event_code_t& event,
                   callback::update_event<string>::func updater)
  {
    updaters_.bind(event, updater);
  }


 private:
  value_updater<EventClass, event_code_t> updaters_;

};



} // marketdata
} // platform
} // atp


#endif //ATP_PLATFORM_MARKETDATA_HANDLER_H_
