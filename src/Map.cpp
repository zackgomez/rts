#include "Map.h"
#include "MessageHub.h"
#include "Unit.h"
#include "Building.h"

namespace rts {

void Map::init()
{
  // TODO(zack) have it read this from a map file
  // we should also create a "base" macro that expands into a headquarters
  // building and starting hero/unit/base defenses unicorns etc

  Message rangedMsg;
  rangedMsg["to"] = toJson(GAME_ID);
  rangedMsg["type"] = MessageTypes::SPAWN_ENTITY;
  rangedMsg["entity_class"] = "UNIT";
  rangedMsg["entity_name"] = "unit";

  Message meleeMsg;
  meleeMsg["to"] = toJson(GAME_ID);
  meleeMsg["type"] = MessageTypes::SPAWN_ENTITY;
  meleeMsg["entity_class"] = "UNIT";
  meleeMsg["entity_name"] = "melee_unit";

  Message buildingMsg;
  buildingMsg["to"] = toJson(GAME_ID);
  buildingMsg["type"] = MessageTypes::SPAWN_ENTITY;
  buildingMsg["entity_class"] = Building::TYPE;
  buildingMsg["entity_name"] = "building";

  for (id_t pid = 1; pid <= 2; pid++)
  {
    glm::vec3 pos(0.f, 0.5f, 0.f);
    float z = pid == 1 ? 8.f : -8.f;
    for (int i = 0; i < 3; i++)
    {
      Message &msg = meleeMsg;
      msg["entity_pid"] = toJson(pid);
      msg["entity_pos"] = toJson(glm::vec3(pos.x, pos.y, z));

      MessageHub::get()->sendMessage(msg);
      pos.x += 2.f;
    }
    pos.x -= 10.f;
    buildingMsg["entity_pid"] = toJson(pid);
    buildingMsg["entity_pos"] = toJson(glm::vec3(pos.x, pos.y, z));
    MessageHub::get()->sendMessage(buildingMsg);
  }
}

}; // rts
