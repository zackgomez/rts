#pragma once
#include <glm/glm.hpp>
#include <GL/glew.h>

class Shader {
 public:
  Shader(GLuint program);
  ~Shader();

  void makeActive();

  void uniform1f(const char *name, float value);
  void uniform2f(const char *name, const glm::vec2 &value);
  void uniform3f(const char *name, const glm::vec3 &value);
  void uniform4f(const char *name, const glm::vec4 &value);
  void uniform1i(const char *name, int value);
  void uniformMatrix4f(const char *name, const glm::mat4 &value);

  GLuint getRawProgram() const {
    return program_;
  }
  GLint getUniformLocation(const char *name) {
    return glGetUniformLocation(program_, name);
  }

 private:
  GLuint program_;
};
