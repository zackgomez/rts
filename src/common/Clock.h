#ifndef SRC_COMMON_CLOCK_H_
#define SRC_COMMON_CLOCK_H_
#include <chrono>

class Clock {
 public:
  Clock();

  Clock& start();

  float seconds() const;
  float milliseconds() const;
  size_t microseconds() const;

  typedef std::chrono::high_resolution_clock clock;

 private:
  std::chrono::time_point<clock> start_;
};

#endif  // SRC_COMMON_CLOCK_H_
