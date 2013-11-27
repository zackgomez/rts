#ifndef SRC_RTS_CURVES_H_
#define SRC_RTS_CURVES_H_
#include <glm/glm.hpp>
#include <vector>
#include "common/Logger.h"
#include "rts/Renderer.h"

namespace rts {

template<typename T>
struct curve_sample {
  curve_sample(float tt, const T& vval) : t(tt), val(vval) { }
  float t;
  T val;
};

template<typename T>
class Curve {
public:
  Curve(const T &start);
  void addKeyframe(float t, const T &val);

  T linearSample(float t) const;
  T stepSample(float t) const;
  
private:
  std::vector<curve_sample<T>> data_;
};

typedef Curve<float> FloatCurve;
typedef Curve<glm::vec2> Vec2Curve;
typedef Curve<glm::vec3> Vec3Curve;

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
  LOG(DEBUG) << "adding sample @ " << t << " current time: " 
    << Renderer::get()->getGameTime() << '\n';
  data_.emplace_back(t, val);
}

template<typename T>
T Curve<T>::linearSample(float t) const {
  invariant(t >= 0, "only non-negative times allowed");
  if (t > data_.back().t) {
    return data_.back().val;
  }
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
}; // rts

#endif  // SRC_RTS_CURVES_H_
