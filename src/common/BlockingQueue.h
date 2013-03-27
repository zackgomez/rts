#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>

template<typename T>
class BlockingQueue {
 public:

   T pop() {
     std::unique_lock<std::mutex> lock(mutex_);
     while (queue_.empty()) {
       cv_.wait(lock);
     }
     T ret = queue_.front();
     queue_.pop();
     return ret;
   }

   void push(T val) {
     std::unique_lock<std::mutex> lock(mutex_);
     queue_.push(val);
     cv_.notify_one();
   }

 private:
  std::queue<T> queue_;
  std::mutex mutex_;
  std::condition_variable cv_;
};
