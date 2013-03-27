#pragma once
#include <thread>
#include "common/BlockingQueue.h"

class WorkerThread {
 public:
   WorkerThread(): done_(false) {
     thread_ = std::thread(std::bind(&WorkerThread::workerFunc, this));
   }

   ~WorkerThread() {
     queue_.push([&]() {
        done_ = true;
     });
     thread_.join();
   }

   typedef std::function<void()> WorkFunc;

   void post(WorkFunc func) {
     queue_.push(func);
   }

 private:
  void workerFunc() {
    while (!done_) {
      WorkFunc f = queue_.pop();
      f();
    }
  }

  BlockingQueue<WorkFunc> queue_;
  bool done_;

  std::thread thread_;
};
