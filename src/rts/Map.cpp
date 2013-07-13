#include "rts/Map.h"
#include <vector>
#include "common/Collision.h"
#include "common/ParamReader.h"
#include "rts/Game.h"
#include "rts/Player.h"
#include "rts/Renderer.h"
#include "rts/ResourceManager.h"

namespace rts {

Map::Map(const Json::Value &definition)
  : definition_(definition) {
}

glm::vec2 Map::getSize() const {
  return toVec2(definition_["size"]);
}

glm::vec4 Map::getColor() const {
  return toVec4(definition_["color"]);
}

float Map::getMapHeight(const glm::vec2 &pos) const {
  // TODO(zack): maps that have dynamic terrain will need to override this
  // to return the height at the passed in position
  return definition_["height"].asFloat();
}

void Map::update(float dt) {
  // nop
}

void Map::init(const std::vector<Player *> &players) {
  invariant(players.size() <= definition_["players"].asInt(),
      "too many players for map");

  Renderer::get()->setMapSize(getSize());
  Renderer::get()->setMapColor(getColor());

  Json::Value entities = definition_["entities"];
  for (int i = 0; i < entities.size(); i++) {
    Json::Value entity_def = entities[i];
    invariant(entity_def.isMember("type"), "missing type param");
    std::string type = entity_def["type"].asString();
    if (type == "starting_location") {
      spawnStartingLocation(entity_def, players);
      continue;
    }
    if (type == "collision_object") {
      id_t eid = Renderer::get()->newEntityID();
      ModelEntity *obj = new ModelEntity(eid);
      obj->setPosition(glm::vec3(toVec2(entity_def["pos"]), 0.1f));
      obj->setSize(toVec2(entity_def["size"]));
      obj->setAngle(entity_def["angle"].asFloat());

      obj->setScale(glm::vec3(2.f*obj->getSize(), 1.f));
      GLuint texture = ResourceManager::get()->getTexture("collision-tex");
      obj->setMeshName("square");
      obj->setMaterial(createMaterial(glm::vec3(0.f), 0.f, texture));
      Renderer::get()->spawnEntity(obj);
      continue;
    }

    Json::Value params;
    params["pid"] = toJson(NO_PLAYER);
    if (entity_def.isMember("pos")) {
      params["pos"] = entity_def["pos"];
    }
    if (entity_def.isMember("size")) {
      params["size"] = entity_def["size"];
    }
    if (entity_def.isMember("angle")) {
      params["angle"] = entity_def["angle"];
    }
    Game::get()->spawnEntity(type, params);
  }
  std::vector<glm::vec3> navVerts;
  for (int i = 0; i < 6; i++) {
    for (int j = 0; j < 6; j++) {
      float x = -20.f + 40.f / 5 * j;
      float y = -20.f + 40.f / 5 * i;
      navVerts.emplace_back(x, y, 0.f);
      LOG(DEBUG) << "vert " << i << ", " << j << ": " << navVerts.back() << '\n';
    }
  }
  std::vector<std::vector<glm::vec3> > navFaces;
  for (int i = 0; i < 36 - 6; i++) {
    if (i % 6 == 5) continue;

    if (i == 7) continue;
    if (i == 9) continue;
    if (i == 19) continue;
    if (i == 21) continue;
    std::vector<glm::vec3> face;
    face.push_back(navVerts[i]);
    face.push_back(navVerts[i+1]);
    face.push_back(navVerts[i+7]);
    face.push_back(navVerts[i+6]);

    //navFaces.push_back(face);
  }
  auto extent = getSize() / 2.f;
  std::vector<glm::vec3> face;
  face.push_back(glm::vec3(-extent.x, -extent.y, 0.f));
  face.push_back(glm::vec3(extent.x, -extent.y, 0.f));
  face.push_back(glm::vec3(extent.x, extent.y, 0.f));
  face.push_back(glm::vec3(-extent.x, extent.y, 0.f));
  navFaces.push_back(face);
  navmesh_ = new NavMesh(navFaces);
}

void Map::spawnStartingLocation(const Json::Value &definition,
    const std::vector<Player *> players) {
  invariant(definition.isMember("player"),
      "missing player for starting base defintion");
  invariant(definition.isMember("pos"),
      "missing position for starting base defintion");
  invariant(definition.isMember("angle"),
      "missing angle for starting base defintion");
  auto idx = definition["player"].asInt() - 1;
  if (idx >= players.size()) {
    return;
  }
  auto player = players[idx];
  id_t pid = player->getPlayerID();


  using namespace v8;
  auto script = Game::get()->getScript();
  HandleScope scope(script->getIsolate());
  TryCatch try_catch;
  auto global = script->getGlobal();
  const int argc = 2;
  Handle<Value> argv[argc] = {Integer::New(pid), script->jsonToJS(definition)};

  Handle<Object> playersModule = Handle<Object>::Cast(
     global->Get(String::New("Players")));
  Handle<Function> playerInit = Handle<Function>::Cast(
        playersModule->Get(String::New("playerInit")));
  Handle<Value> ret = playerInit->Call(global, argc, argv);
  checkJSResult(ret, try_catch.Exception(), "playerInit:");
}
};  // rts
