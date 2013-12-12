#include "rts/Graphics.h"
#include <fstream>
#include <sstream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <boost/algorithm/string.hpp>
#include <fstream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <sstream>
#include "common/Clock.h"
#include "common/Exception.h"
#include "common/Logger.h"
#include "common/NavMesh.h"
#include "common/ParamReader.h"
#include "common/util.h"
#include "rts/ResourceManager.h"
#define STBI_HEADER_FILE_ONLY
#include "stb_image.c"

static bool initialized = false;
static GLFWwindow *glfw_window;

static MatrixStack viewStack;
static MatrixStack projStack;
static glm::vec2 screenRes;

static struct {
  GLuint colorProgram;
  GLuint rectBuffer;
  GLuint circleBuffer;
  GLuint hexBuffer;
  GLuint texProgram;
} resources;

struct vert_p4n4t2 {
  glm::vec4 pos;
  glm::vec4 norm;
  glm::vec2 coord;
};

struct Bone {
  glm::vec3 origin;
};

struct Mesh {
  size_t start, end;
  size_t material_index;
  std::vector<Bone> bones;
};
struct Model {
  glm::mat4 transform;
  GLuint buffer;
  vert_p4n4t2 *verts;
  size_t nverts;

  std::vector<Mesh> meshes;
  std::vector<Material *> materials;
};

struct Material {
  glm::vec3 baseColor;
  GLuint texture;
  float shininess;
};

void *get_native_window_handle() {
  return glfw_window;
}

// Static helper functions
static void loadResources();

static void glfw_error_callback(int error, const char *description) {
  LOG(ERROR) << "GLFW Error (" << error << "): " << description << '\n';
  invariant_violation("glfw error");
}

glm::vec2 initEngine() {
  // TODO(zack) check to see if we're changing resolution
  if (initialized) {
    return screenRes;
  }

  glfwSetErrorCallback(glfw_error_callback);

  if (!glfwInit()) {
    throw engine_exception("Unable to initialize GLFW");
  }

  const char *buildstr = "  BUILT  " __DATE__ " " __TIME__;
  std::string caption = strParam("game.name") + buildstr;

  glfwWindowHint(GLFW_DECORATED, GL_FALSE);
  glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

  auto glfw_monitor = glfwGetPrimaryMonitor();
  auto *glfw_video_mode = glfwGetVideoMode(glfw_monitor);
  screenRes = glm::vec2(glfw_video_mode->width, glfw_video_mode->height);
  glfw_window = glfwCreateWindow(
      screenRes.x,
      screenRes.y,
      caption.c_str(),
      NULL,
      NULL);
  glfwSetWindowPos(glfw_window, 0, 0);
  if (!glfw_window) {
    glfwTerminate();
    throw new engine_exception("Unable to open GLFW window");
  }
  glfwMakeContextCurrent(glfw_window);
  glfwSetInputMode(glfw_window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

  GLenum err = glewInit();
  if (err != GLEW_OK) {
    std::stringstream ss;
    ss <<  "Error: " << glewGetErrorString(err) << '\n';
    throw engine_exception(ss.str());
  }

  LOG(INFO) << "GLSL Version: " <<
      glGetString(GL_SHADING_LANGUAGE_VERSION) << '\n';
  LOG(DEBUG) << "Resolution: " << screenRes << '\n';

  initialized = true;

  ResourceManager::get()->loadResources();
  loadResources();

  return screenRes;
}

void teardownEngine() {
  if (!initialized) {
    return;
  }

  ResourceManager::get()->unloadResources();

  glfwDestroyWindow(glfw_window);
  glfwTerminate();
  initialized = false;
}

void swapBuffers() {
  glfwSwapBuffers(glfw_window);
}

void renderCircleColor(
    const glm::mat4 &modelMatrix,
    const glm::vec4 &color,
    float width) {
  record_section("renderCircleColor");
  GLuint program = resources.colorProgram;
  GLuint projectionUniform = glGetUniformLocation(program, "projectionMatrix");
  GLuint modelViewUniform = glGetUniformLocation(program, "modelViewMatrix");
  GLuint colorUniform = glGetUniformLocation(program, "color");

  // Enable program and set up values
  glUseProgram(program);
  glUniformMatrix4fv(projectionUniform, 1, GL_FALSE,
      glm::value_ptr(projStack.current()));
  glUniformMatrix4fv(modelViewUniform, 1, GL_FALSE,
      glm::value_ptr(viewStack.current() * modelMatrix));
  glUniform4fv(colorUniform, 1, glm::value_ptr(color));

  // Bind attributes
  glBindBuffer(GL_ARRAY_BUFFER, resources.circleBuffer);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);

  // save some old state
  bool wasEnabled = glIsEnabled(GL_LINE_SMOOTH);
  float oldWidth;
  glGetFloatv(GL_LINE_WIDTH, &oldWidth);

  glEnable(GL_LINE_SMOOTH);
  glLineWidth(width);

  // RENDER
  glDrawArrays(GL_LINE_LOOP, 0, intParam("engine.circle_segments"));

  // Clean up
  if (!wasEnabled) {
    glDisable(GL_LINE_SMOOTH);
  }
  glLineWidth(oldWidth);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glDisableVertexAttribArray(0);
}

void renderLineColor(
    const glm::vec3 &start,
    const glm::vec3 &end,
    const glm::vec4 &color) {
  record_section("renderLineColor");
  int buffer;
  glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &buffer);
  invariant(buffer == 0, "Array buffer cannot be bound during renderLine");

  GLuint program = resources.colorProgram;
  GLuint colorUniform = glGetUniformLocation(program, "color");
  GLuint projectionUniform = glGetUniformLocation(program, "projectionMatrix");
  GLuint modelViewUniform = glGetUniformLocation(program, "modelViewMatrix");

  // Enable program and set up values
  glUseProgram(program);
  glUniform4fv(colorUniform, 1, glm::value_ptr(color));
  glUniformMatrix4fv(projectionUniform, 1, GL_FALSE,
      glm::value_ptr(projStack.current()));
  glUniformMatrix4fv(modelViewUniform, 1, GL_FALSE,
      glm::value_ptr(viewStack.current()));

  glm::vec3 verts[] = {start, end};
  glEnableClientState(GL_VERTEX_ARRAY);
  glVertexPointer(3, GL_FLOAT, 0, verts);
  glDrawArrays(GL_LINES, 0, 2);
  glDisableClientState(GL_VERTEX_ARRAY);
}

void renderRectangleColor(
    const glm::mat4 &modelMatrix,
    const glm::vec4 &color) {
  record_section("renderRectangleColor");
  GLuint program = resources.colorProgram;
  GLuint colorUniform = glGetUniformLocation(program, "color");

  // Enable program and set up values
  glUseProgram(program);
  glUniform4fv(colorUniform, 1, glm::value_ptr(color));

  renderRectangleProgram(modelMatrix);
}

void renderRectangleTexture(
    const glm::mat4 &modelMatrix,
    GLuint texture,
    const glm::vec4 &texcoord) {
  record_section("renderRectangleTexture");

  GLuint program = resources.texProgram;
  GLuint textureUniform = glGetUniformLocation(program, "texture");
  GLuint tcUniform = glGetUniformLocation(program, "texcoord");

  // Enable program and set up values
  glUseProgram(program);
  glUniform1i(textureUniform, 0);
  glUniform4fv(tcUniform, 1, glm::value_ptr(texcoord));

  glActiveTexture(GL_TEXTURE0);
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, texture);

  renderRectangleProgram(modelMatrix);
}

void renderRectangleProgram(const glm::mat4 &modelMatrix) {
  record_section("renderRectangleProgram");
  GLuint program;
  glGetIntegerv(GL_CURRENT_PROGRAM, (GLint*) &program);
  if (!program) {
    LOG(WARNING) << "No active program on call to renderRectangleProgram\n";
    return;
  }
  GLuint projectionUniform = glGetUniformLocation(program, "projectionMatrix");
  GLuint modelViewUniform = glGetUniformLocation(program, "modelViewMatrix");

  // Enable program and set up values
  glUniformMatrix4fv(projectionUniform, 1, GL_FALSE,
      glm::value_ptr(projStack.current()));
  glUniformMatrix4fv(modelViewUniform, 1, GL_FALSE,
      glm::value_ptr(viewStack.current() * modelMatrix));

  // Bind attributes
  glBindBuffer(GL_ARRAY_BUFFER, resources.rectBuffer);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);

  // RENDER
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

  // Clean up
  glDisableVertexAttribArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void renderHexagonColor(const glm::mat4 &modelMatrix, const glm::vec4 &color) {
  record_section("renderHexagonColor");
  GLuint program = resources.colorProgram;
  GLuint colorUniform = glGetUniformLocation(program, "color");

  // Enable program and set up values
  glUseProgram(program);
  glUniform4fv(colorUniform, 1, glm::value_ptr(color));

  renderHexagonProgram(modelMatrix);
}

void renderHexagonProgram(const glm::mat4 &modelMatrix) {
  record_section("renderHexagonProgram");
  GLuint program;
  glGetIntegerv(GL_CURRENT_PROGRAM, (GLint*) &program);
  if (!program) {
    LOG(WARNING) << "No active program on call to renderHexagonProgram\n";
    return;
  }
  GLuint projectionUniform = glGetUniformLocation(program, "projectionMatrix");
  GLuint modelViewUniform = glGetUniformLocation(program, "modelViewMatrix");

  // Enable program and set up values
  glUniformMatrix4fv(projectionUniform, 1, GL_FALSE,
      glm::value_ptr(projStack.current()));
  glUniformMatrix4fv(modelViewUniform, 1, GL_FALSE,
      glm::value_ptr(viewStack.current() * modelMatrix));

  // Bind attributes
  glBindBuffer(GL_ARRAY_BUFFER, resources.hexBuffer);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);

  // RENDER
  glDrawArrays(GL_TRIANGLE_FAN, 0, 8);

  // Clean up
  glDisableVertexAttribArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void renderMesh(
    const glm::mat4 &modelMatrix,
    const Model *m,
    size_t start,
    size_t end) {
  record_section("renderModel");
  GLuint program;
  glGetIntegerv(GL_CURRENT_PROGRAM, (GLint*) &program);
  if (!program) {
    LOG(WARNING) << "No active program on call to " << __FUNCTION__
      << "\n";
    return;
  }

  glPolygonMode(GL_FRONT_AND_BACK, GL_LINES);
  const glm::mat4 projMatrix = projStack.current();
  const glm::mat4 modelViewMatrix =
      viewStack.current() * modelMatrix * m->transform;
  const glm::mat4 normalMatrix = glm::transpose(glm::inverse(modelViewMatrix));

  // Uniform locations
  GLuint modelViewUniform = glGetUniformLocation(program, "modelViewMatrix");
  GLuint projectionUniform = glGetUniformLocation(program, "projectionMatrix");
  GLuint normalUniform = glGetUniformLocation(program, "normalMatrix");
  // Attributes
  GLuint positionAttrib = glGetAttribLocation(program, "position");
  GLuint normalAttrib   = glGetAttribLocation(program, "normal");
  GLuint texcoordAttrib = glGetAttribLocation(program, "texcoord");
  // Enable program and set up values
  glUniformMatrix4fv(modelViewUniform, 1, GL_FALSE,
      glm::value_ptr(modelViewMatrix));
  glUniformMatrix4fv(projectionUniform, 1, GL_FALSE,
      glm::value_ptr(projMatrix));
  glUniformMatrix4fv(normalUniform, 1, GL_FALSE,
      glm::value_ptr(normalMatrix));

  // Lighting
  auto lightPos = vec3Param("renderer.lightPos");
  GLuint lightPosUniform = glGetUniformLocation(program, "lightPos");
  glUniform3fv(lightPosUniform, 1, glm::value_ptr(lightPos));
  GLuint ambientUniform = glGetUniformLocation(program, "ambientColor");
  GLuint diffuseUniform = glGetUniformLocation(program, "diffuseColor");
  GLuint specularUniform = glGetUniformLocation(program, "specularColor");
  glUniform3fv(ambientUniform, 1, glm::value_ptr(vec3Param("renderer.light.ambient")));
  glUniform3fv(diffuseUniform, 1, glm::value_ptr(vec3Param("renderer.light.diffuse")));
  glUniform3fv(specularUniform, 1, glm::value_ptr(vec3Param("renderer.light.specular")));

  // Bind data
  glBindBuffer(GL_ARRAY_BUFFER, m->buffer);
  glEnableVertexAttribArray(positionAttrib);
  glEnableVertexAttribArray(normalAttrib);
  glEnableVertexAttribArray(texcoordAttrib);
  glVertexAttribPointer(positionAttrib, 4, GL_FLOAT, GL_FALSE,
      sizeof(struct vert_p4n4t2), (void*) (0));
  glVertexAttribPointer(normalAttrib,   4, GL_FLOAT, GL_FALSE,
      sizeof(struct vert_p4n4t2), (void*) (4 * sizeof(float)));
  glVertexAttribPointer(texcoordAttrib, 2, GL_FLOAT, GL_FALSE,
      sizeof(struct vert_p4n4t2), (void*) (8 * sizeof(float)));

  // Check endpoints and expand -1 to end
  if (end == -1) {
    end = m->nverts;
  }
  invariant(start < end, "out of order endpoints");
  // Draw
  glDrawArrays(GL_TRIANGLES, start, end - start);

  // Clean up
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glDisableVertexAttribArray(positionAttrib);
  glDisableVertexAttribArray(normalAttrib);
  glDisableVertexAttribArray(texcoordAttrib);
}

void renderModel(
    const glm::mat4 &modelMatrix,
    const Model *m) {
  GLuint program;
  glGetIntegerv(GL_CURRENT_PROGRAM, (GLint*) &program);
  if (!program) {
    LOG(WARNING) << "No active program on call to " << __FUNCTION__
      << "\n";
    return;
  }
  GLuint useTextureUniform = glGetUniformLocation(program, "useTexture");
  GLuint textureUniform = glGetUniformLocation(program, "texture");

  for (int i = 0; i < m->meshes.size(); i++) {
    const auto &mesh = m->meshes[i];
    // TODO(zack): improve material support
    const Material *material = m->materials.empty()
      ? nullptr
      : m->materials[mesh.material_index];
    // Default to no texture
    glUniform1i(useTextureUniform, 0);
    if (material && material->texture) {
      glUniform1i(useTextureUniform, 1);
      glActiveTexture(GL_TEXTURE0);
      glEnable(GL_TEXTURE);
      glBindTexture(GL_TEXTURE_2D, material->texture);
      glUniform1i(textureUniform, 0);
    }

    renderMesh(modelMatrix, m, mesh.start, mesh.end);
  }
}

void renderBones(
    const glm::mat4 &modelMatrix,
    const Model *m) {
  for (auto mesh : m->meshes) {
    for (auto bone : mesh.bones) {
      renderRectangleColor(
          modelMatrix * glm::translate(glm::mat4(1.f), bone.origin),
          glm::vec4(1.f));
    }
  }
}

void renderNavMesh(
    const NavMesh &navmesh,
    const glm::mat4 &modelMatrix,
    const glm::vec4 &color) {
  glm::vec3 center(0.f);
  glm::vec2 size(0.f);
  navmesh.iterate(
      [&]() {
        center /= 4.f;
        size = 2.f * glm::abs(size - glm::vec2(center));
        renderRectangleColor(
          glm::scale(
            glm::translate(glm::mat4(1.f), glm::vec3(center) + glm::vec3(0, 0, 0.25f)),
            glm::vec3(size, 1.f)),
          0.8f * glm::vec4(0.7, 0.3, 0.9, 1.0));
        center = glm::vec3(0.f);
        size = glm::vec2(0.f);
      },
      [&](const glm::vec3 &v) {
        center += v;
        size = glm::vec2(v);
      });
}

void drawHexCenter(
    const glm::vec2 &pos,
    const glm::vec2 &size,
    const glm::vec4 &color) {
  glm::mat4 transform =
    glm::scale(
        glm::translate(glm::mat4(1.f), glm::vec3(pos, 0.f)),
        glm::vec3(size, 1.f));

  viewStack.push();
  viewStack.current() = glm::mat4(1.f);
  projStack.push();
  projStack.current() = glm::scale(
      glm::translate(
        glm::mat4(1.f),
        glm::vec3(-1, 1, 0)),
      glm::vec3(2.f / glm::vec2(screenRes.x, -screenRes.y), -1.f));

  renderHexagonColor(transform, color);

  viewStack.pop();
  projStack.pop();
}

void drawRectCenter(
    const glm::vec2 &pos,
    const glm::vec2 &size,
    const glm::vec4 &color,
    float angle) {
  glm::mat4 transform =
    glm::scale(
      glm::rotate(
        glm::translate(glm::mat4(1.f), glm::vec3(pos, 0.f)),
        glm::degrees(angle), glm::vec3(0, 0, 1)),
      glm::vec3(size, 1.f));

  viewStack.push();
  viewStack.current() = glm::mat4(1.f);
  projStack.push();
  projStack.current() = glm::scale(
      glm::translate(
        glm::mat4(1.f),
        glm::vec3(-1, 1, 0)),
      glm::vec3(2.f / glm::vec2(screenRes.x, -screenRes.y), -1.f));

  renderRectangleColor(transform, color);

  viewStack.pop();
  projStack.pop();
}

void drawRect(
    const glm::vec2 &pos,
    const glm::vec2 &size,
    const glm::vec4 &color) {
  glm::vec2 center = pos + size/2.f;
  drawRectCenter(center, size, color);
}

void drawShaderCenter(
    const glm::vec2 &pos,
    const glm::vec2 &size) {
  record_section("drawShader");
  glm::mat4 transform =
    glm::scale(
      glm::translate(glm::mat4(1.f), glm::vec3(pos, 0.f)),
      glm::vec3(size, 1.f));

  viewStack.push();
  viewStack.current() = glm::mat4(1.f);
  projStack.push();
  projStack.current() =
    glm::ortho(0.f, screenRes.x, screenRes.y, 0.f);

  renderRectangleProgram(transform);

  viewStack.pop();
  projStack.pop();
}

void drawShader(
    const glm::vec2 &pos,
    const glm::vec2 &size) {
  glm::vec2 center = pos + size/2.f;
  drawShaderCenter(center, size);
}

void drawTextureCenter(
    const glm::vec2 &pos,  // center
    const glm::vec2 &size,  // width/height
    const GLuint texture,
    const glm::vec4 &texcoord,
    const glm::vec4 &color) {
  record_section("drawTexture");

  glUseProgram(resources.texProgram);
  GLuint textureUniform = glGetUniformLocation(resources.texProgram, "texture");
  GLuint tcUniform = glGetUniformLocation(resources.texProgram, "texcoord");
  GLuint colorUniform = glGetUniformLocation(resources.texProgram, "color");
  glUniform1i(textureUniform, 0);
  glUniform4fv(tcUniform, 1, glm::value_ptr(texcoord));
  glUniform4fv(colorUniform, 1, glm::value_ptr(color));

  glActiveTexture(GL_TEXTURE0);
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, texture);

  drawShaderCenter(pos, size);
}

void drawTexture(
    const glm::vec2 &pos,  // top left corner
    const glm::vec2 &size,  // width/height
    const GLuint texture,
    const glm::vec4 &texcoord,
    const glm::vec4 &color) {
  glm::vec2 center = pos + size / 2.f;
  drawTextureCenter(center, size, texture, texcoord, color);
}

void drawDepthField(
    const glm::vec2 &pos,
    const glm::vec2 &size,
    const glm::vec4 &color,
    const DepthField *depthField) {
  auto program = ResourceManager::get()->getShader("depthfield");
  program->makeActive();
  program->uniform4f("color", color);
  program->uniform1f("minDist", depthField->minDist);
  program->uniform1f("maxDist", depthField->maxDist);
  program->uniform4f("texcoord", glm::vec4(0.f, 0.f, 1.f, 1.f));

  glActiveTexture(GL_TEXTURE0);
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, depthField->texture);

  drawShader(pos, size);
}

MatrixStack& getViewStack() {
  return viewStack;
}

MatrixStack& getProjectionStack() {
  return projStack;
}

// TODO(connor) make this draw a line, for now render a thin rectangle
void drawLine(
  const glm::vec2 &p1,
  const glm::vec2 &p2,
  const glm::vec4 &color) {
  glm::vec2 center = p1 + p2;
  center /= 2;
  glm::vec2 size = glm::vec2(glm::distance(p1, p2), 1);
  float angle = glm::atan(p1.y - p2.y, p1.x - p2.x);
  drawRectCenter(center, size, color, angle);
}

GLuint loadProgram(const std::string &vert, const std::string &frag) {
  GLuint program;
  GLuint vertsh = loadShader(GL_VERTEX_SHADER, vert);
  GLuint fragsh = loadShader(GL_FRAGMENT_SHADER, frag);
  program = linkProgram(vertsh, fragsh);
  glDeleteShader(vertsh);
  glDeleteShader(fragsh);

  return program;
}

GLuint loadShader(GLenum type, const std::string &filename) {
  // Load file
  std::ifstream file(filename.c_str());
  if (!file) {
    LOG(ERROR) << "Unable to read shader from " << filename << '\n';
    return 0;
  }
  std::string shaderText;
  std::getline(file, shaderText, (char)EOF);
  const char *text = shaderText.c_str();
  GLint len = shaderText.length();

  GLuint shader = glCreateShader(type);
  glShaderSource(shader, 1, &text, &len);
  glCompileShader(shader);

  GLint status;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
  if (!status) {
    GLint infoLogLength;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);

    GLchar *strInfoLog = new GLchar[infoLogLength + 1];
    glGetShaderInfoLog(shader, infoLogLength, nullptr, strInfoLog);

    const char *strShaderType = nullptr;
    switch (type) {
    case GL_VERTEX_SHADER:
      strShaderType = "vertex";
      break;
    case GL_GEOMETRY_SHADER:
      strShaderType = "geometry";
      break;
    case GL_FRAGMENT_SHADER:
      strShaderType = "fragment";
      break;
    }

    std::stringstream ss;
    ss << "Compile failure in " << strShaderType << " shader:\n"
      << strInfoLog << '\n';
    LOG(ERROR) << ss.str();
    delete[] strInfoLog;
    throw engine_exception(ss.str());
  }

  return shader;
}

GLuint linkProgram(GLuint vert, GLuint frag) {
  GLuint program = glCreateProgram();

  glAttachShader(program, vert);
  glAttachShader(program, frag);

  glLinkProgram(program);

  GLint status;
  glGetProgramiv(program, GL_LINK_STATUS, &status);
  if (!status) {
    GLint infoLogLength;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);

    GLchar *strInfoLog = new GLchar[infoLogLength + 1];
    glGetProgramInfoLog(program, infoLogLength, nullptr, strInfoLog);
    fprintf(stderr, "Linker failure: %s\n", strInfoLog);
    std::stringstream ss;
    ss << "Linker Failure: " << strInfoLog << '\n';
    delete[] strInfoLog;
    throw engine_exception(ss.str());
  }

  glDetachShader(program, vert);
  glDetachShader(program, frag);

  return program;
}

GLuint makeBuffer(GLenum type, const void *data, GLsizei size) {
  GLuint buf;
  glGenBuffers(1, &buf);
  glBindBuffer(GL_ARRAY_BUFFER, buf);
  glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  return buf;
}

GLuint makeTexture(const std::string &filename) {
  int width, height, depth;
  void *pixels = stbi_load(filename.c_str(), &width, &height, &depth, 4);
  GLuint texture;

  if (!pixels) {
    throw engine_exception("Unable to load texture from " + filename);
  }

  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_CLAMP_TO_EDGE);
  glTexImage2D(
    GL_TEXTURE_2D, 0,           /* target, level */
    GL_RGBA8,                   /* internal format */
    width, height, 0,           /* width, height, border */
    GL_RGBA, GL_UNSIGNED_BYTE,  /* external format, type */
    pixels);                    /* pixels */
  stbi_image_free(pixels);
  return texture;
}

void freeTexture(GLuint tex) {
  glDeleteTextures(1, &tex);
}

Model * loadModel(const std::string &objFile) {
  Model *ret = new Model;
  ret->transform = glm::mat4(1.f);

  Assimp::Importer importer;
  const aiScene* scene = importer.ReadFile(
      objFile,
      aiProcess_CalcTangentSpace |
      aiProcess_Triangulate |
      aiProcess_JoinIdenticalVertices |
      aiProcess_SortByPType);
  if (!scene) {
    LOG(FATAL) << "Unable to import mesh " << importer.GetErrorString() << '\n';
    throw new engine_exception(
        std::string("Unable to import mesh ") + importer.GetErrorString());
  }

  if (scene->mNumAnimations > 0) {
    LOG(DEBUG) << "file " << objFile
      << " HAS ANIMATIONS " << scene->mNumAnimations
      << '\n';
  }

  // TODO(zack) deal with more texture types
  std::vector<Material *> materials;
  for (int i = 0; i < scene->mNumMaterials; i++) {
    auto *aiMaterial = scene->mMaterials[i];
    aiString path;
    Material *m = new Material;
    m->texture = 0;
    m->shininess = 0.f;
    m->baseColor = glm::vec3(1, 0, 1);
    if (aiMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &path) == AI_SUCCESS) {
      std::stringstream idss;
      idss << "meshgenned#" << boost::trim_copy(std::string(path.C_Str()));
      std::string id = idss.str();
      m->texture = ResourceManager::get()->getTexture(id);
    }
    ret->materials.push_back(m);
  }

  size_t total_verts = 0;
  for (int i = 0; i < scene->mNumMeshes; i++) {
    const auto *mesh = scene->mMeshes[i];
    for (int j = 0; j < mesh->mNumFaces; j++) {
      total_verts += mesh->mFaces[j].mNumIndices;
    }
  }

  ret->nverts = total_verts;
  invariant(ret->nverts < 300000, "mesh has too many verts");
  ret->verts = new vert_p4n4t2[ret->nverts];

  int n = 0;
  for (int j = 0; j < scene->mNumMeshes; j++) {
    Mesh mesh;
    mesh.start = n;
    mesh.bones = std::vector<Bone>();
    const aiMesh *aiMesh = scene->mMeshes[j];
    invariant(aiMesh->HasFaces(), "mesh must have faces");
    invariant(aiMesh->HasNormals(), "mesh must have normals");
    invariant(aiMesh->HasPositions(), "mesh must have positions");
    LOG(DEBUG) << "Mesh name (" << aiMesh->mName.C_Str() << "):"
      << " nverts: " << aiMesh->mNumVertices
      << " texcoords?: " << aiMesh->HasTextureCoords(0)
      << '\n';
    for (size_t i = 0; i < aiMesh->mNumBones; i++) {
      aiBone *bone = aiMesh->mBones[i];
      aiVector3D o(0, 0, 0);
      o *= bone->mOffsetMatrix;
      LOG(DEBUG) << "Bone info: "
        << "name: '" << bone->mName.C_Str() << "', "
        << "num_weights: " << bone->mNumWeights
        << "origin: " << o.x << ' ' << o.y << ' ' << o.z
        << '\n';
      Bone mesh_bone = { glm::vec3(o.x, o.y, o.z) };
      mesh.bones.push_back(mesh_bone);
    }
    
    for (int fi = 0; fi < aiMesh->mNumFaces; fi++) {
      const auto &face = aiMesh->mFaces[fi];
      invariant(face.mNumIndices == 3, "expected triangle face");
      for (size_t i = 0; i < face.mNumIndices; i++) {
        auto index = face.mIndices[i];
        const auto &aiVert = aiMesh->mVertices[index];
        const auto &aiNorm = aiMesh->mNormals[index];
        ret->verts[n].pos = glm::vec4(aiVert.x, aiVert.y, aiVert.z, 1.f);
        ret->verts[n].norm = glm::vec4(aiNorm.x, aiNorm.y, aiNorm.z, 1.f);
        if (aiMesh->HasTextureCoords(0)) {
          const auto &aiCoord = aiMesh->mTextureCoords[0][index];
          ret->verts[n].coord = glm::vec2(aiCoord.x, aiCoord.y);
        } else {
          ret->verts[n].coord = glm::vec2(0.f);
        }
        n++;
      }
    }
    mesh.end = n;
    mesh.material_index = aiMesh->mMaterialIndex;
    ret->meshes.push_back(mesh);
  }
  invariant(n == ret->nverts, "didn't fill correct number of verts");

  ret->buffer = makeBuffer(
      GL_ARRAY_BUFFER,
      ret->verts,
      ret->nverts * sizeof(vert_p4n4t2));

  delete ret->verts;
  ret->verts = 0;

  return ret;
}

Material * createMaterial(
    const glm::vec3 &baseColor,
    float shininess,
    GLuint texture) {
  Material *m = new Material;
  m->baseColor = baseColor;
  m->texture = texture;
  m->shininess = shininess;
  return m;
}

void freeMaterial(Material *material) {
  if (!material) {
    return;
  }
  delete material;
}


void freeModel(Model *mesh) {
  if (!mesh) {
    return;
  }
  for (auto *material : mesh->materials) {
    delete material;
  }

  glDeleteBuffers(1, &mesh->buffer);
  delete mesh->verts;
  delete mesh;
}

void setModelTransform(Model *mesh, const glm::mat4 &transform) {
  mesh->transform = transform;
}

// -------- STATIC FUNCTIONS ---------

static void loadResources() {
  const float rectPositions[] = {
    0.5, 0.5, 0.0f, 1.0f,
    0.5, -0.5, 0.0f, 1.0f,
    -0.5, 0.5, 0.0f, 1.0f,
    -0.5, -0.5, 0.0f, 1.0f,
  };
  const float root3_over_4 = sqrtf(3) / 4.f;
  const float hexPositions[] = {
    -0.25, -root3_over_4, 0, 1,
    -0.5, 0, 0, 1,
    0, 0, 0, 1,
    0.25, -root3_over_4, 0, 1,
    0.5, 0, 0, 1,
    0.25, root3_over_4, 0, 1,
    -0.25, root3_over_4, 0, 1,
    -0.5, 0, 0, 1,
  };

  resources.rectBuffer = makeBuffer(
      GL_ARRAY_BUFFER,
      rectPositions,
      sizeof(rectPositions));
  resources.circleBuffer = makeCircleBuffer(
      0.5,
      intParam("engine.circle_segments"));
  resources.hexBuffer = makeBuffer(
      GL_ARRAY_BUFFER,
      hexPositions,
      sizeof(hexPositions));

  // Create solid color program for renderRectangleColor
  resources.colorProgram =
    ResourceManager::get()->getShader("color")->getRawProgram();
  resources.texProgram =
    ResourceManager::get()->getShader("texsimple")->getRawProgram();
}

GLuint makeCircleBuffer(float r, uint32_t nsegments) {
  // Code from http://slabode.exofire.net/circle_draw.shtml
  float theta = 2*M_PI / float(nsegments);
  float c = cosf(theta);  // precalculate the sine and cosine
  float s = sinf(theta);
  float t;

  float x = r;  // we start at angle = 0
  float y = 0;

  glm::vec4 *buf = new glm::vec4[nsegments];
  invariant(buf, "Unable to allocate circle buffer");
  for (int ii = 0; ii < nsegments; ii++)
  {
    buf[ii] = glm::vec4(x, y, 0, 1);
    // apply the rotation matrix
    t = x;
    x = c * x - s * y;
    y = s * t + c * y;
  }

  GLuint ret = makeBuffer(GL_ARRAY_BUFFER, buf, nsegments * sizeof(*buf));
  delete buf;
  return ret;
}
