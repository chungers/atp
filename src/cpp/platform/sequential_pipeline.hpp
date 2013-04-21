#ifndef ATP_PLATFORM_SEQUENTIAL_PIPELINE_H_
#define ATP_PLATFORM_SEQUENTIAL_PIPELINE_H_

#include <vector>

#include <boost/ref.hpp>

#include "common/moving_window.hpp"
#include "platform/indicator.hpp"


using atp::common::microsecond_t;
using atp::common::moving_window;



namespace atp {
namespace platform {


template <typename V, typename S>
class sequential_pipeline
{
 public:

  typedef typename boost::reference_wrapper< indicator< V > > indicator_ref;
  typedef typename std::vector< indicator_ref > indicator_list;
  typedef typename std::vector< indicator_ref >::iterator indicator_list_itr;

  sequential_pipeline(moving_window<V, S>& source) :
      source_(boost::ref(source))
  {
  }

  void operator()(const microsecond_t& t, const V& v)
  {
    moving_window<V, S> *source = source_.get_pointer();
    size_t len = source->capacity() - 1;

    // update the source
    source->on(t, v);

    // copy the data
    V vbuffer[len];
    microsecond_t tbuffer[len];
    size_t copied = source->copy_last(tbuffer, vbuffer, len);

    indicator_list_itr itr;
    for(itr = list_.begin(); itr != list_.end(); ++itr) {
      V computed = itr->get_pointer()->calculate(tbuffer, vbuffer, copied);
      itr->get_pointer()->on(t, computed);
    }
  }

  sequential_pipeline<V, S>& add(indicator<V>& derived)
  {
    list_.push_back(boost::ref(derived));
    return *this;
  }

  sequential_pipeline<V, S>& operator>>(indicator<V>& derived)
  {
    return add(derived);
  }

 private:
  boost::reference_wrapper<moving_window<V, S> > source_;
  indicator_list list_;
};

template <typename S, typename V>
sequential_pipeline<V, S>& operator>>(moving_window<V, S>& source,
                                      indicator<V>& derived)
{
  sequential_pipeline<V,S>* p = new sequential_pipeline<V,S>(source);
  p->add(derived);
  return *p;
}



} // platform
} // atp

#endif //ATP_PLATFORM_SEQUENTIAL_PIPELINE_H_
