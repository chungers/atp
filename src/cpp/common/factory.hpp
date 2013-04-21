#ifndef ATP_COMMON_FACTORY_H_
#define ATP_COMMON_FACTORY_H_

#include <string>

#include <boost/function.hpp>
#include <boost/unordered_map.hpp>

namespace atp {
namespace common {


using boost::function;
using boost::unordered_map;
using std::string;



template <
  class abstract_product,
  typename identifier_t = string,
  typename serialized_data_t = string,
  class product_creator =
  function< abstract_product*(const identifier_t& key,
                              const serialized_data_t& msg) >
  >
class factory
{
 public:
  bool register_creator(const identifier_t& id, product_creator creator)
  {
    return creators_.insert(
        std::pair<identifier_t, product_creator>(id, creator)).second;
  }

  bool unregister_creator(const identifier_t& id)
  {
    return creators_.erase(id) == 1;
  }

  bool is_supported(const identifier_t& id)
  {
    typename creator_map::const_iterator itr = creators_.find(id);
    return (itr != creators_.end()) ;
  }

  abstract_product* create_object(const identifier_t& id,
                                 const serialized_data_t& data)
  {
    typename creator_map::const_iterator itr = creators_.find(id);
    if (itr != creators_.end()) {
      return (itr->second)(id, data);
    }
    return NULL;
  }

 private:

  typedef unordered_map<identifier_t, product_creator> creator_map;
  creator_map creators_;
};

} // common
} // atp

#endif //ATP_COMMON_FACTORY_H_
