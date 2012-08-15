#include "ResourceManager.h"
#include "ParamReader.h"
#include "util.h"

ResourceManager::ResourceManager() {
  logger_ = Logger::getLogger("ResourceManager");
}

ResourceManager::~ResourceManager() {
  unloadResources();
}

void ResourceManager::unloadResources() {
  for (auto mesh = meshes_.begin(); mesh != meshes_.end(); mesh++)
    freeMesh(mesh->second);
  for (auto texture = textures_.begin(); texture != textures_.end(); texture++)
    freeTexture(texture->second);

  meshes_.clear();
  textures_.clear();
}

ResourceManager * ResourceManager::get() {
  static ResourceManager rm;
  return &rm;
}

void ResourceManager::loadResources() {
  Json::Value meshes = ParamReader::get()->getParam("resources.meshes");
  Json::Value textures = ParamReader::get()->getParam("resources.textures");

  for (Json::ValueIterator mesh = meshes.begin(); 
          mesh != meshes.end(); mesh++)
    meshes_[mesh.key().asString()] = loadMesh((*mesh).asString());

  for (Json::ValueIterator texture = textures.begin(); 
          texture != textures.end(); texture++)
    textures_[texture.key().asString()] = makeTexture((*texture).asString());
}

Mesh * ResourceManager::getMesh(const std::string &name) {
  invariant(meshes_.count(name), "cannot find mesh: " + name);
  return meshes_[name];
}

GLuint ResourceManager::getTexture(const std::string &name){
  invariant(textures_.count(name), "cannot find texture: " + name);
  return textures_[name];
}
