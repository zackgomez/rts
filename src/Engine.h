#pragma once
#include <GL/glew.h>
#include <string>
#include "glm.h"
#include "MatrixStack.h"

struct Mesh;


int initEngine(const glm::vec2 &resolution);
void teardownEngine();



MatrixStack& getViewStack();
MatrixStack& getProjectionStack();



GLuint loadProgram(const std::string &vert, const std::string &frag);
GLuint loadShader(GLenum type, const std::string &filename);
GLuint linkProgram(GLuint vert, GLuint frag);
// free program

GLuint makeBuffer(GLenum type, const void *data, GLsizei size);
// free buffer

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

