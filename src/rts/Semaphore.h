#pragma once
#include <mutex>
#include <condition_variable>

namespace std {
class semaphore {
private:
  mutex mutex_;
  condition_variable condition_;
  unsigned long count_;

public:
  semaphore()
    : count_()
  {}

  void inc() {
    unique_lock<mutex> lock(mutex_);
    ++count_;
    condition_.notify_one();
  }

  void dec() {
    unique_lock<mutex> lock(mutex_);
    while(!count_) {
      condition_.wait(lock);
    }
    --count_;
  }
};
}; /* std */

