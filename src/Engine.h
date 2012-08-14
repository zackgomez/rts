#pragma once
#include <GL/glew.h>
#include <string>
#include <glm/glm.hpp>
#include "MatrixStack.h"

struct Mesh;


void initEngine(const glm::vec2 &resolution);
void teardownEngine();

MatrixStack& getViewStack();
MatrixStack& getProjectionStack();

GLuint loadProgram(const std::string &vert, const std::string &frag);
GLuint loadShader(GLenum type, const std::string &filename);
GLuint linkProgram(GLuint vert, GLuint frag);
// TODO(zack) free program

GLuint makeBuffer(GLenum type, const void *data, GLsizei size);
// free buffer
// TODO(zack) rename to loadTexture
GLuint makeTexture(const std::string &filename);
void   freeTexture(GLuint tex);

Mesh * loadMesh(const std::string &objFile);
void freeMesh(Mesh *mesh);
void setMeshTransform(Mesh *mesh, const glm::mat4 &transform);

void renderRectangleColor(
  const glm::mat4 &modelMatrix,
  const glm::vec4 &color);
// Uses the currently bound program
void renderRectangleProgram(
  const glm::mat4 &modelMatrix);
// Uses the currently bound program
void renderMesh(
  const glm::mat4 &modelMatrix,
  const Mesh *mesh);

/* Draws a rectangle at the given pixels.
 * (0,0) is the top-left corner.
 * drawRectCenter will use pos as the center of the rect being drawn,
 * drawRext will use pos as the top-left corner of the rect being drawn.
 * NOTE: the depth test should be disabled.
 */
void drawRectCenter(
  const glm::vec2 &pos, // center
  const glm::vec2 &size, // width/height
  const glm::vec4 &color);
void drawRect(
  const glm::vec2 &pos, // top left corner
  const glm::vec2 &size, // width/height
  const glm::vec4 &color);
void drawTextureCenter(
  const glm::vec2 &pos, // center
  const glm::vec2 &size, // width/height
  const GLuint texture,
  const glm::vec4 &texcoord = glm::vec4(0, 0, 1, 1)); // u0,v0, u1,v1
void drawTexture(
  const glm::vec2 &pos, // top left corner
  const glm::vec2 &size, // width/height
  const GLuint texture,
  const glm::vec4 &texcoord = glm::vec4(0, 0, 1, 1)); // u0,v0, u1,v1
