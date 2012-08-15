#pragma once
#include <cstdint>
#include <string>

namespace rts {

typedef int64_t tick_t;
typedef uint64_t id_t;

id_t assertPid(id_t pid);
id_t assertEid(id_t eid);

namespace ResourceTypes {
const std::string REQUISITION = "REQUISITION";
const std::string VICTORY     = "VICTORY";
};

}; // rts

