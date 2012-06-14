#ifndef ATP_ASYNC_RESPONSE_H_
#define ATP_ASYNC_RESPONSE_H_


#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>


namespace atp {

template <typename T>
class AsyncResponse
{
 public:

  AsyncResponse() : result_(NULL) {}
  ~AsyncResponse()
  {
    // Has ownership of the response object
    if (result_ != NULL) delete result_;
  }

  // blocks until received async
  const T& get() const
  {
    boost::unique_lock<boost::mutex> lock(mutex_);
    if (!is_ready()) {
      waiter_.wait(lock);
    }
    return *result_;
  }

  const bool is_ready() const
  {
    return result_ != NULL;
  }

 protected:
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



} // atp

#endif //ATP_ASYNC_RESPONSE_H_
