#ifndef ATP_PLATFORM_MESSAGE_PROCESSOR_H_
#define ATP_PLATFORM_MESSAGE_PROCESSOR_H_

#include <string>

#include <boost/function.hpp>
#include <boost/unordered_map.hpp>

namespace atp {
namespace platform {

template <
  typename identifier_t = string,
  typename serialized_data_t = string,
  class handler_t =
  function< bool(const identifier_t& key,
                 const serialized_data_t& msg) >
  >
class handlers
{
 public:

  bool register_handler(const identifier_t& id, handler_t handler)
  {
    return handlers_.insert(
        std::pair<identifier_t, handler_t>(id, handler)).second;
  }

  bool is_supported(const identifier_t& id)
  {
    typename creator_map::const_iterator itr = handlers_.find(id);
    return (itr != handlers_.end()) ;
  }

  bool process_raw_message(const identifier_t& id,
                           const serialized_data_t& data)
  {
    typename handler_map_t::const_iterator itr = handlers_.find(id);
    if (itr != handlers_.end()) {
      return (itr->second)(id, data);
    }
    return false;
  }

 private:

  typedef unordered_map<identifier_t, handler_t> handler_map_t;
  handler_map_t handlers_;

};


class message_processor
{
 public:

  message_processor(const string& endpoint,
                    const handlers& handlers)
  {

  }

};


} // platform
} // atp

#endif //ATP_PLATFORM_MESSAGE_PROCESSOR_H_

