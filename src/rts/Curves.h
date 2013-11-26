#ifndef SRC_RTS_CURVES_H_
#define SRC_RTS_CURVES_H_
#include <glm/glm.hpp>
#include <vector>

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
}; // rts

#include "rts/Curves.inl"

#endif  // SRC_RTS_CURVES_H_
