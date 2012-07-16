#include "Map.h"
#include "MessageHub.h"

void Map::init()
{
    // TODO(zack) have it read this from a map file
    // we should also create a "base" macro that expands into a headquarters
    // building and starting hero/unit/base defenses unicorns etc

    Message msg;
    msg["to"] = toJson(NO_ENTITY);
    msg["type"] = MessageTypes::SPAWN_ENTITY;
    msg["entity_type"] = "UNIT";
    msg["unit_name"] = "unit";

    glm::vec3 pos(0.f, 0.5f, 0.f);
    for (int i = 0; i < 3; i++)
    {
        for (int64_t pid = 1; pid <= 2; pid++)
        {
            float z = pid == 1 ? 8.f : -8.f;
            msg["entity_pid"] = toJson(pid);
            msg["entity_pos"] = toJson(glm::vec3(pos.x, pos.y, z));

            MessageHub::get()->sendMessage(msg);
        }

        pos.x += 2.f;
    }
}

