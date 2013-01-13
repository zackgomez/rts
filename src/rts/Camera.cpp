#include "rts/Camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include "common/util.h"

namespace rts {

Camera::Camera()
  : center_(0.f),
    r_(0.f),
    theta_(0.f),
    phi_(0.f) {
}

Camera::Camera(const glm::vec3 &c, float r, float t, float p)
  : center_(c),
    r_(r),
    theta_(t),
    phi_(p) {
}

glm::mat4 Camera::calculateViewMatrix() const {
  glm::mat4 cameraTransform =
    glm::translate(
        glm::rotate(
          glm::rotate(
            glm::translate(glm::mat4(1.f), center_),
            theta_, glm::vec3(0, 0, 1)),
          phi_, glm::vec3(-1, 0, 0)),
        glm::vec3(0, -r_, 0));

  auto cameraPos = applyMatrix(cameraTransform, glm::vec3(0));

  return glm::lookAt(cameraPos, center_, glm::vec3(0, 0, 1));
}

void Camera::setTheta(float t) {
  theta_ = t;
}

void Camera::setPhi(float p) {
  // lolz, to avoid having to code the degenerate cases
  phi_ = glm::clamp(p, 0.000001f, 89.999f);
}
}  // rts
