#ifndef SRC_RTS_CAMERA_H_
#define SRC_RTS_CAMERA_H_
#include <glm/glm.hpp>

namespace rts {

class Camera {
 public:
  Camera();
  Camera(const glm::vec3 &lookAt, float radius, float theta, float phi);

  glm::mat4 calculateViewMatrix() const;

  const glm::vec3 &getLookAt() const {
    return center_;
  }
  void setLookAt(const glm::vec3 &lookAt) {
    center_ = lookAt;
  }

  // Returns degrees rotation around up vector
  float getTheta() const {
    return theta_;
  }
  // returns degrees rotation around right vector
  float getPhi() const {
    return phi_;
  }
  float getZoom() const {
    return r_;
  }

  // @param t rotation in degrees
  void setTheta(float t);
  // @param p rotation in degrees (will be clamped to [0, 90])
  void setPhi(float p);
  // @param r the new radius (distance) from lookAt
  void setZoom(float r) {
    r_ = r;
  }

 private:
  glm::vec3 center_;
  float r_, theta_, phi_;
};
}  // rts
#endif  // SRC_RTS_CAMERA_H_
