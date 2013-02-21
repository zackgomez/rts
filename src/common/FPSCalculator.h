#pragma once
#include <utility>

class FPSCalculator {
 public:
  FPSCalculator(size_t nsamples)
    : sampleIdx_(0),
    times_(nsamples, Clock::now()) {
   }

  float sample() {
    times_[sampleIdx_++] = Clock::now();
    sampleIdx_ = sampleIdx_ % times_.size();
    return (times_.size() - 1) / Clock::secondsSince(times_[sampleIdx_]);
  }

 private:
  size_t sampleIdx_;
  std::vector<Clock::time_point> times_;
};
