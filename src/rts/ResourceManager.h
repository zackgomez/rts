#ifndef SRC_RTS_RESOURCEMANAGER_H_
#define SRC_RTS_RESOURCEMANAGER_H_

#include <map>
#include <GL/glew.h>
#include "common/Logger.h"
#include "rts/Engine.h"

class ResourceManager {
 public:
  ~ResourceManager();

  void unloadResources();

  static ResourceManager *get();

  void loadResources();

  Mesh * getMesh(const std::string &name);
  GLuint getTexture(const std::string &name);
  GLuint getShader(const std::string &name);

 private:
  ResourceManager();

  std::map<std::string, Mesh*> meshes_;
  std::map<std::string, GLuint> textures_;
  std::map<std::string, GLuint> shaders_;

  LoggerPtr logger_;
};

#endif  // SRC_RTS_RESOURCEMANAGER_H_
