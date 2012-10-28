#ifndef ATP_PLATFORM_MESSAGE_PROCESSOR_H_
#define ATP_PLATFORM_MESSAGE_PROCESSOR_H_

#include <string>

#include <boost/function.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/unordered_map.hpp>

#include <zmq.hpp>

#include "common.hpp"

using std::string;
using boost::function;
using boost::scoped_ptr;
using boost::unordered_map;


namespace atp {
namespace platform {


class message_processor : NoCopyAndAssign
{
 public:

  template <
   typename message_key_t = string,
   typename serialized_data_t = string,
   class handler_t =
   function< void(const message_key_t& key,
                  const serialized_data_t& msg) >
   >
  class handlers_map : NoCopyAndAssign
  {

   public:

    typedef typename unordered_map<message_key_t, handler_t>::const_iterator
    message_keys_itr;

    bool register_handler(const message_key_t& id, handler_t handler)
    {
      return handlers_.insert(
          std::pair<message_key_t, handler_t>(id, handler)).second;
    }

    const message_keys_itr begin() const
    {
      return handlers_.begin();
    }

    const message_keys_itr end() const
    {
      return handlers_.end();
    }

    bool is_supported(const message_key_t& id) const
    {
      typename handler_map_t::const_iterator itr = handlers_.find(id);
      return (itr != handlers_.end()) ;
    }

    bool process_raw_message(const message_key_t& id,
                             const serialized_data_t& data) const
    {
      typename handler_map_t::const_iterator itr = handlers_.find(id);

      if (itr != handlers_.end()) {
        try {
          (itr->second)(id, data);
          return true;
        } catch (...) {
          return false;
        }
      } else {
        return false;
      }
    }

   private:

    typedef unordered_map<message_key_t, handler_t> handler_map_t;
    handler_map_t handlers_;

  };



  typedef handlers_map<string, string> protobuf_handlers_map;

  message_processor(const string& endpoint,
                    const protobuf_handlers_map& handlers,
                    size_t threads,
                    ::zmq::context_t* context = NULL);

  ~message_processor();

 private:

  class implementation;
  scoped_ptr<implementation> impl_;
};


} // platform
} // atp

#endif //ATP_PLATFORM_MESSAGE_PROCESSOR_H_

