#ifndef ATP_COMMON_EXECUTOR_H_
#define ATP_COMMON_EXECUTOR_H_

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>

namespace atp {
namespace common {

using boost::bind;
using boost::thread;
using boost::thread_group;
using boost::asio::io_service;

/// Executor that manages a thread pool and
/// processes work and as they are submitted.
class executor
{
 public:

  executor(size_t threads) :
      service_(threads),
      work_(service_),
      jobs_(0)
  {
    for (size_t i = 0; i < threads; ++i) {
      pool_.create_thread(bind(&io_service::run, &service_));
    }
  }

  ~executor()
  {
    service_.stop();
    pool_.join_all();
  }

  template <typename Task>
  void Submit(Task task)
  {
    service_.post(task);
    jobs_++;
  }

  size_t Jobs()
  {
    return jobs_;
  }

 private:
  thread_group pool_;
  io_service service_;
  io_service::work work_;
  size_t jobs_;
};

} // common
} // atp
#endif //ATP_COMMON_EXECUTOR_H_
