#pragma once
#include <glm/glm.hpp>
#include <GL/glew.h>
#include <string>
#include "MatrixStack.h"

int initEngine(const glm::vec2 &resolution);
void teardownEngine();

void renderRectangleColor(
        const glm::mat4 &modelMatrix,
        const glm::vec4 &color);
// Gets the currently bound program
void renderRectangleProgram(
        const glm::mat4 &modelMatrix);

MatrixStack& getViewStack();
MatrixStack& getProjectionStack();

GLuint loadProgram(const std::string &vert, const std::string &frag);
GLuint loadShader(GLenum type, const std::string &filename);
GLuint linkProgram(GLuint vert, GLuint frag);
