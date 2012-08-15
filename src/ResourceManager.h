#pragma once
#include <GL/glew.h>
#include "Engine.h"
#include <map>
#include "Logger.h"

class ResourceManager {
public:
  ~ResourceManager();

  void unloadResources();

  static ResourceManager *get();

  void loadResources();

  Mesh * getMesh(const std::string &name);
  GLuint getTexture(const std::string &name);
  
private:
  ResourceManager();

  std::map<std::string, Mesh*> meshes_;
  std::map<std::string, GLuint> textures_;

  LoggerPtr logger_;
};
