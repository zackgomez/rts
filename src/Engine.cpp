#include "Engine.h"
#include <SDL/SDL.h>
#include <GL/glew.h>
#include <glm/gtc/type_ptr.hpp>
#include "Logger.h"
#include "ParamReader.h"
#include "stb_image.c"

static bool initialized = false;
static LoggerPtr logger;

static MatrixStack viewStack;
static MatrixStack projStack;
static glm::vec2 screenRes;

static struct {
  GLuint colorProgram;
  GLuint rectBuffer;
  GLuint texProgram;
} resources;

struct vert_p4n4t2 {
  glm::vec4 pos;
  glm::vec4 norm;
  glm::vec2 coord;
};

struct Mesh {
  glm::mat4    transform;
  GLuint       buffer;
  vert_p4n4t2 *verts;
  size_t       nverts;
};

// Static helper functions
static int loadResources();
static int loadVerts(const std::string &filename,
                     vert_p4n4t2 **verts, size_t *nverts);

int initEngine(const glm::vec2 &resolution) {
  // TODO(zack) check to see if we're changing resolution
  if (initialized) {
    return 1;
  }

  screenRes = resolution;

  if (!logger.get()) {
    logger = Logger::getLogger("Engine");
  }

  if (SDL_Init(SDL_INIT_VIDEO)) {
    logger->fatal() << "Couldn't initialize SDL: " <<
                    SDL_GetError() << '\n';
    return 0;
  }

  int flags = SDL_OPENGL;

  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_Surface *screen = SDL_SetVideoMode(resolution.x, resolution.y, 32, flags);
  if (screen == NULL) {
    logger->fatal() << "Couldn't set video mode: " << SDL_GetError() << '\n';
    return 0;
  }
  GLenum err = glewInit();
  if (err != GLEW_OK) {
    logger->fatal() <<  "Error: " << glewGetErrorString(err) << '\n';
    return 0;
  }

  logger->info() << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << '\n';

  SDL_WM_SetCaption((strParam("game.name") + "  BUILT  " __DATE__ " " __TIME__).c_str(), "rts");
  //SDL_WM_GrabInput(SDL_GRAB_ON);

  initialized = true;

  if (!loadResources()) {
    logger->fatal() << "Unable to load resources, bailing...\n";
    return 0;
  }

  // success
  return 1;
}

void teardownEngine() {
  if (!initialized) {
    return;
  }

  SDL_Quit();
  initialized = false;
}

void renderRectangleColor(const glm::mat4 &modelMatrix, const glm::vec4 &color) {
  GLuint program = resources.colorProgram;
  GLuint projectionUniform = glGetUniformLocation(program, "projectionMatrix");
  GLuint modelViewUniform = glGetUniformLocation(program, "modelViewMatrix");
  GLuint colorUniform = glGetUniformLocation(program, "color");

  // Enable program and set up values
  glUseProgram(program);
  glUniformMatrix4fv(projectionUniform, 1, GL_FALSE, glm::value_ptr(projStack.current()));
  glUniformMatrix4fv(modelViewUniform, 1, GL_FALSE, glm::value_ptr(viewStack.current() * modelMatrix));
  glUniform4fv(colorUniform, 1, glm::value_ptr(color));

  // Bind attributes
  glBindBuffer(GL_ARRAY_BUFFER, resources.rectBuffer);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);

  // RENDER
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

  // Clean up
  glDisableVertexAttribArray(0);
  glUseProgram(0);
}

void renderRectangleProgram(const glm::mat4 &modelMatrix) {
  GLuint program;
  glGetIntegerv(GL_CURRENT_PROGRAM, (GLint*) &program);
  if (!program) {
    logger->warning() << "No active program on call to renderRectangleProgram\n";
    return;
  }
  GLuint projectionUniform = glGetUniformLocation(program, "projectionMatrix");
  GLuint modelViewUniform = glGetUniformLocation(program, "modelViewMatrix");

  // Enable program and set up values
  glUseProgram(program);
  glUniformMatrix4fv(projectionUniform, 1, GL_FALSE, glm::value_ptr(projStack.current()));
  glUniformMatrix4fv(modelViewUniform, 1, GL_FALSE, glm::value_ptr(viewStack.current() * modelMatrix));

  // Bind attributes
  glBindBuffer(GL_ARRAY_BUFFER, resources.rectBuffer);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);

  // RENDER
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

  // Clean up
  glDisableVertexAttribArray(0);
  glUseProgram(0);
}

void renderMesh(const glm::mat4 &modelMatrix, const Mesh *m) {
  GLuint program;
  glGetIntegerv(GL_CURRENT_PROGRAM, (GLint*) &program);
  if (!program) {
    logger->warning() << "No active program on call to " << __FUNCTION__ << "\n";
    return;
  }
  const glm::mat4 projMatrix = projStack.current();
  const glm::mat4 modelViewMatrix = viewStack.current() * modelMatrix * m->transform;
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
  glUseProgram(program);
  glUniformMatrix4fv(modelViewUniform, 1, GL_FALSE, glm::value_ptr(modelViewMatrix));
  glUniformMatrix4fv(projectionUniform, 1, GL_FALSE, glm::value_ptr(projMatrix));
  glUniformMatrix4fv(normalUniform, 1, GL_FALSE, glm::value_ptr(normalMatrix));

  // Bind data
  glBindBuffer(GL_ARRAY_BUFFER, m->buffer);
  glEnableVertexAttribArray(positionAttrib);
  glEnableVertexAttribArray(normalAttrib);
  glEnableVertexAttribArray(texcoordAttrib);
  glVertexAttribPointer(positionAttrib, 4, GL_FLOAT, GL_FALSE, sizeof(struct vert_p4n4t2), (void*) (0));
  glVertexAttribPointer(normalAttrib,   4, GL_FLOAT, GL_FALSE, sizeof(struct vert_p4n4t2), (void*) (4 * sizeof(float)));
  glVertexAttribPointer(texcoordAttrib, 2, GL_FLOAT, GL_FALSE, sizeof(struct vert_p4n4t2), (void*) (8 * sizeof(float)));

  // Draw
  glDrawArrays(GL_TRIANGLES, 0, m->nverts);

  // Clean up
  glDisableVertexAttribArray(positionAttrib);
  glDisableVertexAttribArray(normalAttrib);
  glDisableVertexAttribArray(texcoordAttrib);
  glUseProgram(0);
}

void drawRectCenter(
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
  projStack.current() =
    glm::ortho(0.f, screenRes.x, screenRes.y, 0.f);

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

void drawTextureCenter(
  const glm::vec2 &pos, // center
  const glm::vec2 &size, // width/height
  const GLuint texture,
  const glm::vec4 &texcoord) {
  glm::mat4 transform =
    glm::scale(
      glm::translate(glm::mat4(1.f), glm::vec3(pos, 0.f)),
      glm::vec3(size, 1.f));

  viewStack.push();
  viewStack.current() = glm::mat4(1.f);
  projStack.push();
  projStack.current() =
    glm::ortho(0.f, screenRes.x, screenRes.y, 0.f);

  glUseProgram(resources.texProgram);
  GLuint textureUniform = glGetUniformLocation(resources.texProgram, "texture");
  GLuint tcUniform = glGetUniformLocation(resources.texProgram, "texcoord");
  glUniform1i(textureUniform, 0);
  glUniform4fv(tcUniform, 1, glm::value_ptr(texcoord));

  glActiveTexture(GL_TEXTURE0);
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, texture);

  renderRectangleProgram(transform);

  viewStack.pop();
  projStack.pop();
}

void drawTexture(
  const glm::vec2 &pos, // top left corner
  const glm::vec2 &size, // width/height
  const GLuint texture,
  const glm::vec4 &texcoord) {
  glm::vec2 center = pos + size / 2.f;
  drawTextureCenter(center, size, texture, texcoord);
}

MatrixStack& getViewStack() {
  return viewStack;
}

MatrixStack& getProjectionStack() {
  return projStack;
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
    logger->error() << "Unable to read shader from " << filename << '\n';
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
    glGetShaderInfoLog(shader, infoLogLength, NULL, strInfoLog);

    const char *strShaderType = NULL;
    switch(type) {
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

    fprintf(stderr, "Compile failure in %s shader:\n%s\n", strShaderType, strInfoLog);
    delete[] strInfoLog;
    return 0;
  }

  return shader;
}

GLuint linkProgram(GLuint vert, GLuint frag) {
  GLuint program = glCreateProgram();

  glAttachShader(program, vert);
  glAttachShader(program, frag);

  glLinkProgram(program);

  GLint status;
  glGetProgramiv (program, GL_LINK_STATUS, &status);
  if (!status) {
    GLint infoLogLength;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);

    GLchar *strInfoLog = new GLchar[infoLogLength + 1];
    glGetProgramInfoLog(program, infoLogLength, NULL, strInfoLog);
    fprintf(stderr, "Linker failure: %s\n", strInfoLog);
    delete[] strInfoLog;
    return 0;
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
    // TODO(zack) make this throw an exception
    logger->warning() << "Unable to load texture from " << filename << "\n";
    return 0;
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
    pixels                      /* pixels */
  );
  stbi_image_free(pixels);
  return texture;
}

void freeTexture(GLuint tex) {
  glDeleteTextures(1, &tex);
}

Mesh * loadMesh(const std::string &objFile) {
  Mesh *ret = (Mesh *) malloc(sizeof(Mesh));
  ret->transform = glm::mat4(1.f);

  loadVerts(objFile, &ret->verts, &ret->nverts);

  ret->buffer = makeBuffer(
                  GL_ARRAY_BUFFER,
                  ret->verts,
                  ret->nverts * sizeof(vert_p4n4t2));

  free(ret->verts);
  ret->verts = 0;

  return ret;
}

void freeMesh(Mesh *mesh) {
  if (!mesh) {
    return;
  }

  glDeleteBuffers(1, &mesh->buffer);
  free(mesh->verts);
  free(mesh);
}

void setMeshTransform(Mesh *mesh, const glm::mat4 &transform) {
  mesh->transform = transform;
}

// -------- STATIC FUNCTIONS ---------

static int loadResources() {
  const float rectPositions[] = {
    0.5, 0.5, 0.0f, 1.0f,
    0.5, -0.5, 0.0f, 1.0f,
    -0.5, 0.5, 0.0f, 1.0f,
    -0.5, -0.5, 0.0f, 1.0f,
  };

  resources.rectBuffer = makeBuffer(
                           GL_ARRAY_BUFFER,
                           rectPositions,
                           sizeof(rectPositions));

  // Create solid color program for renderRectangleColor
  resources.colorProgram = loadProgram("shaders/color.v.glsl", "shaders/color.f.glsl");
  resources.texProgram = loadProgram("shaders/texsimple.v.glsl", "shaders/texsimple.f.glsl");

  return 1;
}

struct facevert {
  int v, vt, vn;
};
struct fullface {
  facevert fverts[3];
};
static facevert parseFaceVert(const char *vdef) {
  facevert fv;
  assert(vdef);

  if (sscanf(vdef, "%d/%d/%d", &fv.v, &fv.vt, &fv.vn) == 3) {
    // 1 indexed to 0 indexed
    fv.v -= 1;
    fv.vn -= 1;
    fv.vt -= 1;
  } else if (sscanf(vdef, "%d//%d", &fv.v, &fv.vn) == 2) {
    fv.v -= 1;
    fv.vn -= 1;
    fv.vt = 0; // dummy vt @ 0 index
  } else {
    assert(false && "unable to read unfull .obj");
  }

  return fv;
}


static int loadVerts(const std::string &filename,
                     vert_p4n4t2 **out, size_t *size) {
  std::ifstream cs(filename);
  if (!cs) {
    logger->error() << "Couldn't open obj file " << filename << '\n';
    return 1;
  }

  // First count the number of vertices and faces
  unsigned nverts = 0, nfaces = 0, nnorms = 0, ncoords = 0;
  // Are we reading an objfile with geosmash extensions?
  bool extended = false;
  // lines should never be longer than 1024 in a .obj...
  char buf[1024];

  while (cs.getline(buf, sizeof buf)) {
    if (*buf == '\0') {
      continue;
    }

    char *cmd = strtok(buf, " ");
    char *arg = strtok(NULL, " ");
    if (strcmp(cmd, "v") == 0) {
      nverts++;
    }
    if (strcmp(cmd, "f") == 0) {
      nfaces++;
    }
    if (strcmp(cmd, "vn") == 0) {
      nnorms++;
    }
    if (strcmp(cmd, "vt") == 0) {
      ncoords++;
    }
    if (strcmp(cmd, "ext") == 0 && strcmp(arg, "geosmash") == 0) {
      extended = true;
    }
  }

  // TODO make a logger call
  printf("verts: %d, faces: %d, norms: %d, coords: %d, extended: %d\n",
         nverts, nfaces, nnorms, ncoords, extended);

  // Allocate an extra, dummy texcoord in case vert doesn't specify one
  if (ncoords == 0) {
    ncoords++;
  }

  // Allocate space
  glm::vec4 *verts   = (glm::vec4*) malloc(sizeof(glm::vec4) * nverts);
  fullface  *ffaces  = (fullface*)  malloc(sizeof(fullface) * nfaces);
  glm::vec2 *coords  = (glm::vec2*) malloc(sizeof(glm::vec2) * ncoords);
  glm::vec3 *norms   = (glm::vec3*) malloc(sizeof(glm::vec3) * nnorms);
  assert(verts && norms && coords && ffaces);
  // TODO replace asserts will some real memory checks

  // reset to beginning of file
  cs.clear();
  cs.seekg(0, std::ios::beg);

  size_t verti = 0, facei = 0, normi = 0, coordi = 0;
  // Read each line and act on it as necessary
  while (cs.getline(buf, sizeof buf)) {
    if (*buf == '\0') {
      continue;
    }
    char *cmd = strtok(buf, " ");

    if (strcmp(cmd, "v") == 0) {
      // read the 3 floats, and pad with a fourth 1
      verts[verti][0] = atof(strtok(NULL, " "));
      verts[verti][1] = atof(strtok(NULL, " "));
      verts[verti][2] = atof(strtok(NULL, " "));
      verts[verti][3] = 1.f; // homogenous coords
      ++verti;
    } else if (strcmp(cmd, "f") == 0) {
      // This assumes the faces are triangles
      ffaces[facei].fverts[0] = parseFaceVert(strtok(NULL, " "));
      ffaces[facei].fverts[1] = parseFaceVert(strtok(NULL, " "));
      ffaces[facei].fverts[2] = parseFaceVert(strtok(NULL, " "));
      ++facei;
    } else if (strcmp(cmd, "vn") == 0) {
      norms[normi][0] = atof(strtok(NULL, " "));
      norms[normi][1] = atof(strtok(NULL, " "));
      norms[normi][2] = atof(strtok(NULL, " "));
      ++normi;
    } else if (strcmp(cmd, "vt") == 0) {
      coords[coordi][0] = atof(strtok(NULL, " "));
      coords[coordi][1] = atof(strtok(NULL, " "));
      ++coordi;
    }
  }

  // Last texcoord could be dummy
  if (coordi == 0) {
    coords[coordi++] = glm::vec2(0.f);
  }

  assert(verti == nverts);
  assert(facei == nfaces);
  assert(normi == nnorms);
  assert(coordi == ncoords);

  // 3 verts per face
  *size = nfaces * 3;
  vert_p4n4t2 *ret = (vert_p4n4t2 *) malloc(sizeof(vert_p4n4t2) * *size);
  assert(ret);

  size_t vi = 0;
  for (size_t i = 0; i < nfaces; i++) {
    const fullface &ff = ffaces[i];

    for (size_t j = 0; j < 3; j++) {
      size_t v = ff.fverts[j].v;
      size_t vn = ff.fverts[j].vn;
      size_t vt = ff.fverts[j].vt;
      ret[vi].pos   = verts[v];
      ret[vi].norm  = glm::vec4(norms[vn], 1.f);
      ret[vi].coord = coords[vt];
      ++vi;
    }
  }
  assert(vi == *size);

  free(ffaces);
  free(norms);
  free(verts);
  free(coords);

  *out = ret;
  // size is already set
  return 0;
}

