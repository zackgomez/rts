#ifndef SRC_COMMON_TYPES_H_
#define SRC_COMMON_TYPES_H_
#include <cstdint>
#include <string>
#include <glm/glm.hpp>

namespace rts {

typedef int64_t tick_t;
typedef uint64_t id_t;

id_t assertPid(id_t pid);
id_t assertEid(id_t eid);
id_t assertTid(id_t tid);

// The ID space is partitioned as follows
// 0    : NULL ID
// 1    : Game ID
// 2    : Map ID
// 3-99 : Unused
// 100-199 : Player IDs
// 200-299 : Team IDs
// 300-2^64-1 : Entity IDs

// Null ID symbols
const id_t NULL_ID   = 0;
const id_t NO_ENTITY = 0;
const id_t NO_PLAYER = 0;
const id_t NO_TEAM   = 0;
// Special IDs
const id_t GAME_ID   = 1;
const id_t MAP_ID    = 2;
// IDs less than 100 can have special meanings
// Space for 100 players/teams
const id_t STARTING_PID = 100;
const id_t STARTING_TID = 200;
// And effectively infinite entities ~2^64
const id_t STARTING_EID = 300;

// NULL position
const glm::vec3 NO_POSITION = glm::vec3(HUGE_VAL);

enum class ResourceType {
  REQUISITION = 1,
};
};  // rts

#endif  // SRC_COMMON_TYPES_H_
