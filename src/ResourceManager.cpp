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
  // TODO(connor) implement this once freeProgram has been implemented.
  // for (auto shader = shaders_.begin(); shader != shaders_.end(); shader++)
  //   freeProgram(shader->second);
  

  meshes_.clear();
  textures_.clear();
  shaders_.clear();
}

ResourceManager * ResourceManager::get() {
  static ResourceManager rm;
  return &rm;
}

void ResourceManager::loadResources() {
  Json::Value meshes = ParamReader::get()->getParam("resources.meshes");
  Json::Value textures = ParamReader::get()->getParam("resources.textures");
  Json::Value shaders = ParamReader::get()->getParam("resources.shaders");

  for (Json::ValueIterator mesh = meshes.begin(); 
          mesh != meshes.end(); mesh++)
    meshes_[mesh.key().asString()] = loadMesh((*mesh).asString());

  for (Json::ValueIterator texture = textures.begin(); 
          texture != textures.end(); texture++)
    textures_[texture.key().asString()] = makeTexture((*texture).asString());

  for (Json::ValueIterator shader = shaders.begin(); 
          shader != shaders.end(); shader++)
    shaders_[shader.key().asString()] = 
      loadProgram((*shader)["vert"].asString(), (*shader)["frag"].asString());
}

Mesh * ResourceManager::getMesh(const std::string &name) {
  invariant(meshes_.count(name), "cannot find mesh: " + name);
  return meshes_[name];
}

GLuint ResourceManager::getTexture(const std::string &name){
  invariant(textures_.count(name), "cannot find texture: " + name);
  return textures_[name];
}

GLuint ResourceManager::getShader(const std::string &name){
  invariant(shaders_.count(name), "cannot find shader: " + name);
  return shaders_[name];
}
