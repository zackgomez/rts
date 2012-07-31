#include "Types.h"
#include "Entity.h"

namespace rts {

id_t assertPid(id_t id)
{
  assert(id < Entity::STARTING_ID);
  return id;
}

id_t assertEid(id_t id)
{
  assert(id >= Entity::STARTING_ID);
  return id;
}

};
