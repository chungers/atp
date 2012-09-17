
#include <string>

#include <gtest/gtest.h>
#include <glog/logging.h>

#include "utils.hpp"
#include "common/Executor.hpp"


using std::string;
using atp::common::Executor;

typedef boost::uint64_t ts;


size_t COUNT = 0;
ts LAST = now_micros();

void work()
{
  ts now = now_micros();
  COUNT++;
  LOG(INFO) << "work " << (now - LAST);
  LAST = now;
}

void work(const string& message)
{
  ts now = now_micros();
  COUNT++;
  LOG(INFO) << "work: " << message << ","  << (now - LAST);
  LAST = now;
}

struct work_functor
{
  void operator()()
  {
    ts now = now_micros();
    COUNT++;
    LOG(INFO) << "functor " << (now - LAST);
    LAST = now;
  }
};

/**
Note a sample run:
I0916 15:16:35.481716 35590144 ExecutorTest.cpp:42] functor 1645
I0916 15:16:35.481722 36126720 ExecutorTest.cpp:42] functor 1678
I0916 15:16:35.482532 35590144 ExecutorTest.cpp:42] functor 861
I0916 15:16:35.482542 36126720 ExecutorTest.cpp:42] functor 839
I0916 15:16:35.482555 35590144 ExecutorTest.cpp:42] functor 26
I0916 15:16:35.482560 36126720 ExecutorTest.cpp:42] functor 21
I0916 15:16:35.482579 35590144 ExecutorTest.cpp:42] functor 24
I0916 15:16:35.482585 36126720 ExecutorTest.cpp:42] functor 24
I0916 15:16:35.482604 35590144 ExecutorTest.cpp:42] functor 24
I0916 15:16:35.482630 36126720 ExecutorTest.cpp:42] functor 45
I0916 15:16:35.482636 35590144 ExecutorTest.cpp:42] functor 34
I0916 15:16:35.482655 36126720 ExecutorTest.cpp:42] functor 24
I0916 15:16:35.482658 35590144 ExecutorTest.cpp:42] functor 22
I0916 15:16:35.482676 36126720 ExecutorTest.cpp:42] functor 22
I0916 15:16:35.482679 35590144 ExecutorTest.cpp:42] functor 21
I0916 15:16:35.482697 36126720 ExecutorTest.cpp:42] functor 21
I0916 15:16:35.482702 35590144 ExecutorTest.cpp:42] functor 22
I0916 15:16:35.482715 36126720 ExecutorTest.cpp:42] functor 18
I0916 15:16:35.482720 35590144 ExecutorTest.cpp:42] functor 18
I0916 15:16:35.482740 36126720 ExecutorTest.cpp:42] functor 25
I0916 15:16:35.482838 35590144 ExecutorTest.cpp:42] functor 117
I0916 15:16:35.482848 36126720 ExecutorTest.cpp:24] work 107
I0916 15:16:35.482862 35590144 ExecutorTest.cpp:32] work: other,25

**/
TEST(ExecutorTest, UsageSyntax)
{
  work_functor w;

  Executor executor(1);
  executor.Submit(work_functor());

  for (int i = 0; i < 20; ++i) {
    executor.Submit(w);
  }

  executor.Submit(boost::bind(work));
  executor.Submit(boost::bind(work, "other"));

  sleep(1); // need to let the executor run a bit so work actually gets done
}

TEST(ExecutorTest, Verify)
{
  LOG(INFO) << "COUNT = " << COUNT;
  EXPECT_EQ(23, COUNT);
}
