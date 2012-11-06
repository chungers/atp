#ifndef ATP_PLATFORM_MARKETDATA_HANDLER_H_
#define ATP_PLATFORM_MARKETDATA_HANDLER_H_


#include <string>

#include <boost/function.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/unordered_map.hpp>

using std::string;
using boost::function;
using boost::unordered_map;


namespace atp {
namespace platform {

typedef boost::uint64_t timestamp_t;


namespace callback {

typedef function<bool(const timestamp_t&, const double&) > double_updater;
typedef function<bool(const timestamp_t&, const int&) > int_updater;
typedef function<bool(const timestamp_t&, const string&) > string_updater;

} // callback

namespace message {

template <typename EventClass, typename serialized_data_t>
inline bool parse(const serialized_data_t& raw, EventClass& event);

template <typename EventClass>
inline const timestamp_t get_timestamp(const EventClass& event);

template <typename EventClass, typename event_code_t>
inline const event_code_t& get_event_code(const EventClass& event);

template <typename EventClass, typename V>
inline const V& get_value(const EventClass& event);

template <typename EventClass, typename event_code_t>
class value_updater
{
 public:

  typedef unordered_map<event_code_t, callback::double_updater>
  double_updaters;

  typedef unordered_map<event_code_t, callback::int_updater>
  int_updaters;

  typedef unordered_map<event_code_t, callback::string_updater>
  string_updaters;


  void bind(const event_code_t& event, callback::double_updater updater)
  {
    double_updaters_.insert(
        std::pair<event_code_t, callback::double_updater>(event, updater));
  }

  void bind(const event_code_t& event, callback::int_updater updater)
  {
    int_updaters_.insert(
        std::pair<event_code_t, callback::int_updater>(event, updater));
  }

  void bind(const event_code_t& event, callback::string_updater updater)
  {
    string_updaters_.insert(
        std::pair<event_code_t, callback::string_updater>(event, updater));
  }

  bool operator()(const timestamp_t& ts,
                  const event_code_t& event_code,
                  const EventClass& event);

 private:

  double_updaters double_updaters_;
  int_updaters int_updaters_;
  string_updaters string_updaters_;
};

} // message


template <class EventClass,
          typename event_code_t = string,
          typename message_key_t = string,
          typename serialized_data_t = string
          >
class marketdata_handler
{

 public:

  bool process_event(const message_key_t& key,
                     const serialized_data_t& msg)
  {
    EventClass event;
    if (message::parse(msg, event)) {

      timestamp_t ts = message::get_timestamp<EventClass>(event);
      event_code_t event_code =
          message::get_event_code<EventClass, event_code_t>(event);

      return updaters_(ts, event_code, event);

    } else {
      return false;
    }
  }

  inline void bind(const event_code_t& event, callback::int_updater updater)
  {
    updaters_.bind(event, updater);
  }

  inline void bind(const event_code_t& event, callback::double_updater updater)
  {
    updaters_.bind(event, updater);
  }

  inline void bind(const event_code_t& event, callback::string_updater updater)
  {
    updaters_.bind(event, updater);
  }

 private:

  message::value_updater<EventClass, event_code_t> updaters_;

};


} // platform
} // atp


#endif //ATP_PLATFORM_MARKETDATA_HANDLER_H_
