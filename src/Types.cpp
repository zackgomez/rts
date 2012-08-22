#include "Types.h"
#include "Entity.h"

namespace rts {

id_t assertPid(id_t id) {
  assert(id == NO_PLAYER ||
    (id >= STARTING_PID && id < STARTING_TID));
  return id;
}

id_t assertEid(id_t id) {
  assert(id == NO_ENTITY || id >= STARTING_EID);
  return id;
}

id_t assertTid(id_t id) {
  assert(id == NO_TEAM ||
    (id >= STARTING_TID && id < STARTING_EID));
  return id;
}



};
