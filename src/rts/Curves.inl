#include "rts/Curves.h"
#include "common/util.h"

namespace rts {

template<typename T>
Curve<T>::Curve(const T &start) {
  addKeyframe(0, start);
}

template<typename T>
void Curve<T>::addKeyframe(float t, const T &val) {
  invariant(t >= 0, "only non-negative times allowed");
  for (int i = data_.size() - 1; i >= 0; i--) {
    if (data_[i].t > t) {
      data_.resize(i, curve_sample<T>(0, T()));
      break;
    }
  }
  data_.emplace_back(t, val);
}

template<typename T>
T Curve<T>::linearSample(float t) const {
  invariant(t >= 0, "only non-negative times allowed");
  // need two samples at t0 and t1 such that t0 < t < t1
  // find index of sample t1
  size_t i = 1;
  for (; i < data_.size(); i++) {
    if (t < data_[i].t) {
      break;
    }
  }
  // samples are now t0 @ (i-1), t @ (i)
  auto s0 = data_[i - 1];
  auto s1 = data_[i];
  float u = (t - s0.t) / (s1.t - s0.t);
  T val = s1.val * u + s0.val * (1.f - u);
  return val;
}

template<typename T>
T Curve<T>::stepSample(float t) const {
  invariant(t >= 0, "only non-negative times allowed");
  // need two samples at t0 and t1 such that t0 < t < t1
  // find index of sample t1
  size_t i = 1;
  for (; i < data_.size(); i++) {
    if (t < data_[i].t) {
      break;
    }
  }
  return data_[i-1].val;
}
}  // rts
