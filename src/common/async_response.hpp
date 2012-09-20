#ifndef ATP_COMMON_ASYNC_RESPONSE_H_
#define ATP_COMMON_ASYNC_RESPONSE_H_


#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>


namespace atp {
namespace common {



template <typename T>
class async_response
{
 public:

  async_response() : result_(NULL) {}
  ~async_response()
  {
    // Has ownership of the response object
    if (result_ != NULL) delete result_;
  }

  // blocks until received async
  const T& get(long msec = 0)
  {
    boost::unique_lock<boost::mutex> lock(mutex_);
    if (!is_ready()) {
      if (msec > 0) {
        boost::system_time const abs_timeout =
            boost::get_system_time()+ boost::posix_time::milliseconds(msec);
        waiter_.timed_wait(lock, abs_timeout);
      } else {
        waiter_.wait(lock);
      }
    }
    return *result_;
  }

  const bool is_ready() const
  {
    return result_ != NULL;
  }

  void set_response(T* response)
  {
    boost::unique_lock<boost::mutex> lock(mutex_);
    // Takes ownership of the response object.
    result_ = response;
    waiter_.notify_all();
  }

 private:
  T* result_;
  boost::condition_variable waiter_;
  boost::mutex mutex_;
};


} // common
} // atp

#endif //ATP_COMMON_ASYNC_RESPONSE_H_
