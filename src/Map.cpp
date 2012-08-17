#include <vector>
#include "Map.h"
#include "Player.h"
#include "MessageHub.h"
#include "Unit.h"
#include "Building.h"
#include "ParamReader.h"

namespace rts {

Map::Map(const std::string &mapName) :
  name_("maps." + mapName),
  size_(vec2Param(name_ + ".size")) {
}

void Map::init(const std::vector<Player *> &players) {
  // TODO(zack) have it read this from a map file
  // we should also create a "base" macro that expands into a headquarters
  // building and starting hero/unit/base defenses unicorns etc

  // TODO(zack): currently these message are from the game to the game.
  // Perhaps we should create a map id

  invariant(players.size() <= intParam(name_ + ".players"),
      "too many players for map");

  for (int i = 0; i < players.size(); i++) {
    const Player *p = players[i];
    id_t pid = p->getPlayerID();

    // Give starting resources
    MessageHub::get()->sendResourceMessage(
      GAME_ID,
      pid,
      ResourceTypes::REQUISITION,
      fltParam(name_ + ".startingReq")
    );

    // Spawn starting units and building
    // TODO(zack) improve this
    int ind = pid - 1;
    glm::vec3 pos = toVec3(getParam(name_ + ".startingPositions")[ind]);
    for (int i = 0; i < 3; i++) {
      Json::Value params;
      params["entity_pid"] = toJson(pid);
      params["entity_pos"] = toJson(pos);
      MessageHub::get()->sendSpawnMessage(
        GAME_ID, // from
        Unit::TYPE, // class
        "melee_unit", // name
        params
      );

      pos.x += 2.f;
    }
    pos.x -= 10.f;
    Json::Value params;
    params["entity_pid"] = toJson(pid);
    params["entity_pos"] = toJson(pos);
    MessageHub::get()->sendSpawnMessage(
      GAME_ID,
      Building::TYPE,
      "building",
      params
    );
  }

  Json::Value victoryPoints = getParam(name_ + ".victoryPoints");
  for (int i = 0; i < victoryPoints.size(); i++)
  {
    glm::vec3 pos = toVec3(victoryPoints[i]);
    Json::Value params;
    params["entity_pid"] = toJson(NO_PLAYER);
    params["entity_pos"] = toJson(pos);
    MessageHub::get()->sendSpawnMessage(
        GAME_ID,
        Building::TYPE,
        "victory_point",
        params);
  }
}

}; // rts
