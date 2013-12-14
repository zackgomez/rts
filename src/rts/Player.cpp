#include "rts/Player.h"
#include "common/Checksum.h"
#include "common/ParamReader.h"
#include "rts/GameEntity.h"
#include "rts/Game.h"
#include "rts/Renderer.h"

namespace rts {

LocalPlayer::LocalPlayer(id_t playerID, id_t teamID, const std::string &name,
    const glm::vec3 &color)
  : Player(playerID, teamID, name, color) {
}

LocalPlayer::~LocalPlayer() {
}


DummyPlayer::DummyPlayer(id_t playerID, id_t teamID)
  : Player(playerID, teamID, "DummyPlayer", vec3Param("colors.dummyPlayer")) {
}

bool isControlGroupHotkey(int hotkey) {
  return (hotkey == '`') || (hotkey >= '0' && hotkey <= '9');
}
};  // rts
