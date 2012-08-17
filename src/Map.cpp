#include "Map.h"
#include "MessageHub.h"
#include "Unit.h"
#include "Building.h"

namespace rts {

void Map::init() {
  // TODO(zack) have it read this from a map file
  // we should also create a "base" macro that expands into a headquarters
  // building and starting hero/unit/base defenses unicorns etc

  // TODO(zack): currently these message are from the game to the game.
  // Perhaps we should create a map id

  for (id_t pid = 1; pid <= 2; pid++) {
    glm::vec3 pos(0.f, 0.0f, 0.f);
    float z = pid == 1 ? 8.f : -8.f;

    for (int i = 0; i < 3; i++) {
      Json::Value params;
      params["entity_pid"] = toJson(pid);
      params["entity_pos"] = toJson(glm::vec3(pos.x, pos.y, z));
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
    params["entity_pos"] = toJson(glm::vec3(pos.x, pos.y, z));
    MessageHub::get()->sendSpawnMessage(
      GAME_ID,
      Building::TYPE,
      "building",
      params
    );
  }

  // TODO(zack): read these positions from params
  for (int i = 0; i < 3; i++)
  {
    glm::vec3 pos(0.f, 0.0f, 0.f);
    pos.z = 0.f;
    pos.x = (i - 1) * 8.f;
    Json::Value params;
    params["entity_pid"] = toJson(NO_PLAYER);
    params["entity_pos"] = toJson(pos);
    MessageHub::get()->sendSpawnMessage(
        GAME_ID,
        Building::TYPE,
        "victory_point",
        params);
    pos.x += 8.f;
  }
}

}; // rts
