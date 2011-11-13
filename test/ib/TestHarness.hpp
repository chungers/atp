#ifndef TEST_TEST_HARNESS_H_
#define TEST_TEST_HARNESS_H_

#include <algorithm>
#include <map>
#include <set>
#include <vector>
#include <gflags/gflags.h>
#include <glog/logging.h>

#include "utils.hpp"

#include <Shared/Contract.h>
#include <Shared/EWrapper.h>

template <typename T>
class TestHarnessBase
{
 public:
  TestHarnessBase() {}
  ~TestHarnessBase() {}

  /// Returns the count of an event.
  int getCount(T event) {
    if (eventCount_.find(event) != eventCount_.end()) {
      return eventCount_[event];
    } else {
      return 0;
    }
  }

  bool waitForNOccurrences(T event, int N, int secondsTimeout) {
    int64 start = now_micros();
    int64 limit = secondsTimeout * 1000000;
    while (now_micros() - start < limit && getCount(event) < N) {}
    return getCount(event) > 0;
  }

  bool waitForFirstOccurrence(T event, int secondsTimeout) {
    return waitForNOccurrences(event, 1, secondsTimeout);
  }

 protected:
  /// Increment by count
  void incr(T event, int count) {
    if (eventCount_.find(event) == eventCount_.end()) {
      eventCount_[event] = count;
    } else {
      eventCount_[event] += count;
    }
  }

  /// Increment by 1
  inline void incr(T event) {
    incr(event, 1);
  }


 private:
  std::map<T, int> eventCount_;

};


enum EVENT {
  NEXT_VALID_ID = 1,
  CURRENT_TIME,
  TICK_PRICE,
  TICK_SIZE,
  TICK_GENERIC,
  TICK_OPTION_COMPUTATION,
  UPDATE_MKT_DEPTH,
  CONTRACT_DETAILS,
  CONTRACT_DETAILS_END // for option chain
};


class TestHarness : public TestHarnessBase<EVENT> {

 public:
  TestHarness() : optionChain_(NULL)
  {
  }

  ~TestHarness() {}

  // Returns true if the ticker id was seen.
  bool hasSeenTickerId(TickerId id)
  {
    return tickerIds_.find(id) != tickerIds_.end();
  }

  void setOptionChain(std::vector<Contract>* optionChain)
  {
    optionChain_ = optionChain;
  }

 protected:

  void seen(TickerId tickerId)
  {
    if (!hasSeenTickerId(tickerId)) {
      tickerIds_.insert(tickerId);
    }
  }

  std::set<TickerId> tickerIds_;
  std::vector<Contract>* optionChain_;

};


#endif // TEST_TEST_HARNESS_H_

