#ifndef SRC_RTS_GRAPHICS_H_
#define SRC_RTS_GRAPHICS_H_

#include <GL/glew.h>
#include <cstdint>
#include <string>
#include <glm/glm.hpp>
#include "rts/MatrixStack.h"

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
// Renders a circle of radius 0.5 with the given color.
void renderCircleColor(
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
    const glm::vec2 &pos,  // center
    const glm::vec2 &size,  // width/height
    const glm::vec4 &color,
    float angle = 0);
void drawRect(
    const glm::vec2 &pos,  // top left corner
    const glm::vec2 &size,  // width/height
    const glm::vec4 &color);
void drawShaderCenter(
    const glm::vec2 &pos,  // center
    const glm::vec2 &size);  // width/height
void drawShader(
    const glm::vec2 &pos,  // top left corner
    const glm::vec2 &size);  // width/height
// Draws a texture
void drawTextureCenter(
    const glm::vec2 &pos,  // center
    const glm::vec2 &size,  // width/height
    const GLuint texture,
    const glm::vec4 &texcoord = glm::vec4(0, 0, 1, 1));  // u0,v0, u1,v1
void drawTexture(
    const glm::vec2 &pos,  // top left corner
    const glm::vec2 &size,  // width/height
    const GLuint texture,
    const glm::vec4 &texcoord = glm::vec4(0, 0, 1, 1));  // u0,v0, u1,v1
void drawLine(
    const glm::vec2 &p1,
    const glm::vec2 &p2,
    const glm::vec4 &color);
// Returns a vertex buffer with a circle centered at the origin, oriented
// in the x,y plane with radius r and nsegments segments.
GLuint makeCircleBuffer(float r, uint32_t nsegments);

#endif  // SRC_RTS_GRAPHICS_H_
