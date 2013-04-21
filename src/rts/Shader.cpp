#include "rts/Shader.h"
#include <glm/gtc/type_ptr.hpp>

Shader::Shader(GLuint program)
  : program_(program) {
}

Shader::~Shader() {
  // TODO(zack): cleanup
}

void Shader::makeActive() {
  glUseProgram(program_);
}

void Shader::uniform1f(const char *name, float value) {
  GLint uniform = getUniformLocation(name);
  glUniform1fv(uniform, 1, &value);
}

void Shader::uniform2f(const char *name, const glm::vec2 &value) {
  GLint uniform = getUniformLocation(name);
  glUniform2fv(uniform, 1, glm::value_ptr(value));
}

void Shader::uniform3f(const char *name, const glm::vec3 &value) {
  GLint uniform = getUniformLocation(name);
  glUniform3fv(uniform, 1, glm::value_ptr(value));
}

void Shader::uniform4f(const char *name, const glm::vec4 &value) {
  GLint uniform = getUniformLocation(name);
  glUniform4fv(uniform, 1, glm::value_ptr(value));
}

void Shader::uniform1i(const char *name, int value) {
  GLint uniform = getUniformLocation(name);
  glUniform1iv(uniform, 1, &value);
}

void Shader::uniformMatrix4f(const char *name, const glm::mat4 &value) {
  GLint uniform = getUniformLocation(name);
  glUniform4fv(uniform, 1, glm::value_ptr(value));
}
