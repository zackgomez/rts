#include "rts/ResourceManager.h"
#include "common/ParamReader.h"
#include "common/util.h"

#define BOOST_FILESYSTEM_NO_DEPRECATED
#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"

ResourceManager *ResourceManager::instance_ = NULL;

ResourceManager::ResourceManager() {
}

ResourceManager::~ResourceManager() {
  unloadResources();
}

void ResourceManager::unloadResources() {
	for (auto model = models_.begin(); model != models_.end(); model++) {
    freeModel(model->second);
	}
  for (auto texture = textures_.begin(); texture != textures_.end(); texture++) {
    freeTexture(texture->second);
  }
  for (auto shader = shaders_.begin(); shader != shaders_.end(); shader++) {
    delete (shader->second);
  }

  models_.clear();
  textures_.clear();
  shaders_.clear();
}

ResourceManager * ResourceManager::get() {
  if (!instance_) {
    instance_ = new ResourceManager();
  }
  return instance_;
}

glm::mat4 getModelTransform(const Json::Value &model_desc) {
  glm::mat4 ret(1.f);
  if (!model_desc.isMember("transform")) {
    return ret;
  }
  Json::Value transform_desc = model_desc["transform"];
  if (transform_desc.isMember("origin")) {
    ret = glm::translate(ret, -toVec3(transform_desc["origin"]));
  }
  if (transform_desc.isMember("up")) {
    glm::vec3 up = glm::normalize(
        toVec3(transform_desc["up"]));
    glm::vec3 forward = glm::normalize(
        toVec3(must_have_idx(transform_desc, "forward")));
    glm::vec3 right = glm::cross(up, forward);
    glm::vec4 cols[] = {
      glm::vec4(forward, 0.f),
      glm::vec4(right, 0.f),
      glm::vec4(up, 0.f),
    };
    for (int i = 0; i < 3; i++) {
      ret[i] = cols[i];
    }
  }
  if (transform_desc.isMember("scale")) {
    ret = glm::scale(ret, glm::vec3(transform_desc["scale"].asFloat()));
  }

  return ret;
}

void ResourceManager::loadResources() {
  Json::Value models = ParamReader::get()->getParam("resources.models");
  Json::Value textures = ParamReader::get()->getParam("resources.textures");
  Json::Value shaders = ParamReader::get()->getParam("resources.shaders");
  Json::Value dfdescs = ParamReader::get()->getParam("resources.depthFields");

  for (Json::ValueIterator texture = textures.begin();
          texture != textures.end(); texture++) {
    textures_[texture.key().asString()] = makeTexture((*texture).asString());
  }

  LOG(DEBUG) << "Loading models\n";
  for (Json::ValueIterator it = models.begin(); it != models.end(); it++) {
    std::string model_name = it.key().asString();
    Json::Value model_desc = *it;
    std::string model_file = must_have_idx(model_desc, "file").asString();
    LOG(DEBUG) << "loading " << model_file << '\n';
    auto *model = loadModel(model_file);
    setModelTransform(model, getModelTransform(model_desc));
    models_[model_name] = model;
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

  namespace fs = boost::filesystem;
  boost::system::error_code ec;
  fs::path maps_path("maps");
  auto status = fs::status(maps_path);
  invariant(
    fs::exists(status) && fs::is_directory(status),
    "missing maps directory");

  Json::Reader reader;
  auto it = fs::directory_iterator(maps_path);
  for ( ; it != fs::directory_iterator(); it++) {
    auto dir_ent = *it;
    if (fs::is_regular_file(dir_ent.status())) {
      auto filepath = dir_ent.path().filename();
      auto filename = filepath.string();
      // TODO(zack): some logging here
      if (filepath.extension() != ".map" || filename.empty() || filename[0] == '.') {
        continue;
      }
      std::ifstream f(dir_ent.path().c_str());
      if (!f) {
        continue;
      }
      Json::Value mapDef;
      if (!reader.parse(f, mapDef)) {
        LOG(FATAL) << "Cannot parse param file " << filename << " : "
          << reader.getFormattedErrorMessages() << '\n';
        invariant_violation("Couldn't parse " + dir_ent.path().string());
      }
      invariant(mapDef.isMember("name"), "map missing name");
      auto mapName = mapDef["name"].asString();
      LOG(INFO) << "Read map " << mapName << " from file " << filename << '\n';
      maps_[mapName] = mapDef;
    }
  }
}

std::vector<std::pair<std::string, std::string>> ResourceManager::getOrderedScriptNames() {
  namespace fs = boost::filesystem;
  boost::system::error_code ec;
  fs::path scripts_path("scripts");
  auto status = fs::status(scripts_path);
  invariant(
    fs::exists(status) && fs::is_directory(status),
    "missing scripts directory");

  std::vector<std::pair<int, std::string>> paths;
  auto it = fs::directory_iterator(scripts_path);
  for ( ; it != fs::directory_iterator(); it++) {
    auto dir_ent = *it;
    if (!fs::is_regular_file(dir_ent.status())) {
      continue;
    }
    auto filepath = dir_ent.path().filename();
    auto filename = filepath.string();
    if (filepath.extension() != ".js" || filename.empty() || filename[0] == '.') {
      continue;
    }
    uint32_t n;
    if (sscanf(filename.c_str(), "%d-", &n) == EOF) {
      // on error, wrap to end
      n = -1;
    }

    // Insert into sorted array
    bool inserted = false;
    auto pit = paths.begin(); 
    for (; pit != paths.end(); pit++) {
      if (n < pit->first) {
        paths.insert(pit, make_pair(n, dir_ent.path().string()));
        inserted = true;
        break;
      }
    }
    // Put at end if haven't inserted already
    if (!inserted) {
      paths.insert(pit, make_pair(n, dir_ent.path().string()));
    }
  }

  std::vector<std::pair<std::string, std::string>> path_files;

  for (const auto& pair : paths) {
    auto script_name = pair.second;
    std::ifstream f(script_name.c_str());
    if (!f) {
      continue;
    }

    std::string contents;
    std::getline(f, contents, (char)EOF);
    path_files.emplace_back(script_name, contents);

    f.close();
  }

  return path_files;
}

Model * ResourceManager::getModel(const std::string &name) {
  invariant(models_.count(name), "cannot find model: " + name);
  return models_[name];
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

Json::Value ResourceManager::getMapDefinition(const std::string &name) {
  invariant(maps_.count(name), "cannot find Map: " + name);
  return maps_[name];
}
