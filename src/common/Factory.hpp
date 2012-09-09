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
  class AbstractProduct,
  typename IdentifierType = string,
  typename SerializedDataType = string,
  class ProductCreator =
  function< AbstractProduct*(const IdentifierType& key,
                             const SerializedDataType& msg) >
  >
class Factory
{
 public:
  bool Register(const IdentifierType& id, ProductCreator creator)
  {
    return creators_.insert(
        std::pair<IdentifierType, ProductCreator>(id, creator)).second;
  }

  bool Unregister(const IdentifierType& id)
  {
    return creators_.erase(id) == 1;
  }

  bool IsSupported(const IdentifierType& id)
  {
    typename CreatorMap::const_iterator itr = creators_.find(id);
    return (itr != creators_.end()) ;
  }

  AbstractProduct* CreateObject(const IdentifierType& id,
                                const SerializedDataType& data)
  {
    typename CreatorMap::const_iterator itr = creators_.find(id);
    if (itr != creators_.end()) {
      return (itr->second)(id, data);
    }
    return NULL;
  }

 private:

  typedef unordered_map<IdentifierType, ProductCreator> CreatorMap;
  CreatorMap creators_;
};

} // common
} // atp

#endif //ATP_COMMON_FACTORY_H_
