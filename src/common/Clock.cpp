#include "common/Clock.h"

using std::chrono::duration_cast;

Clock::Clock() {
}

Clock& Clock::start() {
  start_ = clock::now();
  return *this;
}

size_t Clock::microseconds() const {
  auto end = clock::now();
  return duration_cast<std::chrono::microseconds>(end - start_).count();
}

float Clock::milliseconds() const {
  return microseconds() / 1000.f;
}

float Clock::seconds() const {
  return microseconds() / 1e6;
}
