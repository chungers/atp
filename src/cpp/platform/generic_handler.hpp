#ifndef ATP_PLATFORM_GENERIC_HANDLER_H_
#define ATP_PLATFORM_GENERIC_HANDLER_H_


#include <string>

#include <boost/function.hpp>
#include <boost/unordered_map.hpp>

#include "platform/types.hpp"

using std::string;
using atp::platform::types::timestamp_t;


namespace atp {
namespace platform {

template <typename EventClass, typename serialized_data_t>
inline bool deserialize(const serialized_data_t& raw, EventClass& event);

template <typename EventClass>
inline timestamp_t get_timestamp(const EventClass& event);

template <typename message_key_t, typename EventClass>
inline message_key_t get_message_key(const EventClass& event);

using boost::function;

/// Generic data handler
/// 1. an event class
/// 2. an handler functor
/// 3. special serialized data type (e.g. string)
/// 4. type of message key (e.g. string)
/// 5. event type code that can be extracted after deserialization
template <class EventClass,
          typename handler_t = function<
            int(const timestamp_t&, const EventClass&) >,
          typename message_key_t = string,
          typename serialized_data_t = string
          >
class generic_handler
{
 public:

  typedef handler_t handler_type;

  generic_handler() {}
  virtual ~generic_handler() {}

  int operator()(const message_key_t& key,
                 const serialized_data_t& msg)
  {
    EventClass event;
    if (deserialize(msg, event)) {

      timestamp_t ts = get_timestamp<EventClass>(event);
      if (handlers_.find(key) != handlers_.end()) {
        return handlers_[key](ts, event);
      } else {
        return -1;
      }
    } else {
      LOG(WARNING) << "Cannot deserialize: " << key << ", msg = " << msg;
      return -1;
    }
  }

  void bind(const message_key_t key, const handler_t& handler)
  {
    handlers_[key] = handler;
  }

  message_key_t message_key()
  {
    return get_message_key<message_key_t, EventClass>(prototype_);
  }

 private:

  typedef boost::unordered_map<message_key_t, handler_t> handler_map;

  EventClass prototype_;
  handler_map handlers_;
};

} // platform
} // atp


#endif //ATP_PLATFORM_GENERIC_HANDLER_H_
