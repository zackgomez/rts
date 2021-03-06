#ifndef SRC_RTS_GRAPHICS_H_
#define SRC_RTS_GRAPHICS_H_

#include <GL/glew.h>
#include <cstdint>
#include <map>
#include <string>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include "rts/MatrixStack.h"

struct Model;
struct Material;
struct DepthField {
  GLuint texture;
  float minDist;
  float maxDist;
};
class NavMesh;

glm::vec2 initEngine();
void teardownEngine();
void swapBuffers();

void *get_native_window_handle();

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

Model * loadModel(const std::string &objFile);
Material * createMaterial(
    const glm::vec3 &baseColor,
    float shininess,
    GLuint texture = 0);
void freeModel(Model *mesh);
void freeMaterial(Material *material);
void setModelTransform(Model *mesh, const glm::mat4 &transform);

void renderLineColor(
    const glm::vec3 &start,
    const glm::vec3 &end,
    const glm::vec4 &color);
void renderRectangleColor(
    const glm::mat4 &modelMatrix,
    const glm::vec4 &color);
void renderRectangleTexture(
    const glm::mat4 &modelMatrix,
    GLuint texture,
    const glm::vec4 &texcoord);
// Renders a circle of radius 0.5 with the given color.
void renderCircleColor(
    const glm::mat4 &modelMatrix,
    const glm::vec4 &color,
    float width = 1.f);
// Uses the currently bound program
void renderRectangleProgram(const glm::mat4 &modelMatrix);
void renderHexagonProgram(const glm::mat4 &model);
void renderHexagonColor(const glm::mat4 &model, const glm::vec4 &color);
// Uses the currently bound program
void renderModel(
    const glm::mat4 &modelMatrix,
    const Model *mesh);

void renderBones(const glm::mat4 &modelMatrix, const Model *mesh);
void renderNavMesh(
    const NavMesh &navmesh,
    const glm::mat4 &modelMatrix,
    const glm::vec4 &color);

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
void drawHexCenter(
    const glm::vec2 &pos,
    const glm::vec2 &size,
    const glm::vec4 &color);
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
    const glm::vec4 &texcoord = glm::vec4(0, 0, 1, 1),  // u0,v0, u1,v1
    const glm::vec4 &color = glm::vec4(1.f));
void drawTexture(
    const glm::vec2 &pos,  // top left corner
    const glm::vec2 &size,  // width/height
    const GLuint texture,
    const glm::vec4 &texcoord = glm::vec4(0, 0, 1, 1),
    const glm::vec4 &color = glm::vec4(1.f));
void drawLine(
    const glm::vec2 &p1,
    const glm::vec2 &p2,
    const glm::vec4 &color);
void drawDepthField(
    const glm::vec2 &pos,
    const glm::vec2 &size,
    const glm::vec4 &color,
    const DepthField *depthField);
// Returns a vertex buffer with a circle centered at the origin, oriented
// in the x,y plane with radius r and nsegments segments.
GLuint makeCircleBuffer(float r, uint32_t nsegments);

#endif  // SRC_RTS_GRAPHICS_H_
