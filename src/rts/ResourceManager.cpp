#include "rts/ResourceManager.h"
#include "common/ParamReader.h"
#include "common/util.h"

ResourceManager *ResourceManager::instance_ = NULL;

ResourceManager::ResourceManager() {
}

ResourceManager::~ResourceManager() {
  unloadResources();
}

void ResourceManager::unloadResources() {
	for (auto mesh = meshes_.begin(); mesh != meshes_.end(); mesh++) {
    freeMesh(mesh->second);
	}
  for (auto texture = textures_.begin(); texture != textures_.end(); texture++) {
    freeTexture(texture->second);
  }
  for (auto shader = shaders_.begin(); shader != shaders_.end(); shader++) {
    delete (shader->second);
  }

  meshes_.clear();
  textures_.clear();
  shaders_.clear();
}

ResourceManager * ResourceManager::get() {
  if (!instance_) {
    instance_ = new ResourceManager();
  }
  return instance_;
}

void ResourceManager::loadResources() {
  Json::Value meshes = ParamReader::get()->getParam("resources.meshes");
  Json::Value textures = ParamReader::get()->getParam("resources.textures");
  Json::Value shaders = ParamReader::get()->getParam("resources.shaders");
  Json::Value dfdescs = ParamReader::get()->getParam("resources.depthFields");

  for (Json::ValueIterator mesh = meshes.begin();
          mesh != meshes.end(); mesh++) {
    meshes_[mesh.key().asString()] = loadMesh((*mesh).asString());
  }

  for (Json::ValueIterator texture = textures.begin();
          texture != textures.end(); texture++) {
    textures_[texture.key().asString()] = makeTexture((*texture).asString());
  }

  for (Json::ValueIterator shader = shaders.begin();
          shader != shaders.end(); shader++) {
    GLuint program =
      loadProgram((*shader)["vert"].asString(), (*shader)["frag"].asString());
    shaders_[shader.key().asString()] = new Shader(program);
  }

  for (Json::ValueIterator dfdesc = dfdescs.begin();
          dfdesc != dfdescs.end(); dfdesc++) {
    DepthField *df = new DepthField;
    df->texture = makeTexture((*dfdesc)["file"].asString());
    df->minDist = (*dfdesc)["minDist"].asFloat();
    df->maxDist = (*dfdesc)["maxDist"].asFloat();
    depthFields_[dfdesc.key().asString()] = df;
  }
}

Mesh * ResourceManager::getMesh(const std::string &name) {
  invariant(meshes_.count(name), "cannot find mesh: " + name);
  return meshes_[name];
}

GLuint ResourceManager::getTexture(const std::string &name) {
  invariant(textures_.count(name), "cannot find texture: " + name);
  return textures_[name];
}

Shader* ResourceManager::getShader(const std::string &name) {
  invariant(shaders_.count(name), "cannot find shader: " + name);
  return shaders_[name];
}

DepthField *ResourceManager::getDepthField(const std::string &name) {
  invariant(depthFields_.count(name), "cannot find DepthField: " + name);
  return depthFields_[name];
}
