#include "common/Geometry.h"
#include "common/Logger.h"
#include "common/util.h"

static const float SQRT3 = sqrtf(3.f);
static const float SQRT3_2 = sqrtf(3.f) / 2.f;
static const float SQRT2_2 = sqrtf(2.f) / 2.f;
static HexTiling hex_tilings[] = {
  HexTiling::ABOVE,
  HexTiling::BELOW,
  HexTiling::HIGH_LEFT,
  HexTiling::LOW_LEFT,
  HexTiling::HIGH_RIGHT,
  HexTiling::LOW_RIGHT,
};

const HexTiling * getHexTilings() {
  return hex_tilings;
}

glm::vec2 computeTiledHexPosition(
    HexTiling tile,
    const glm::vec2 &pos,
    const glm::vec2 &size,
    float margin) {
  glm::vec2 offset;
  glm::vec2 absolute_offset;
  if (tile == HexTiling::ABOVE) {
    offset = glm::vec2(0.f, SQRT3 / 2.f);
    absolute_offset = glm::vec2(0.f, margin);
  } else if (tile == HexTiling::BELOW) {
    offset = glm::vec2(0.f, -SQRT3 / 2.f);
    absolute_offset = glm::vec2(0.f, -margin);
  } else if (tile == HexTiling::HIGH_LEFT) {
    offset = glm::vec2(-0.5f - SQRT3 / 8.f, SQRT3 / 4.f);
    absolute_offset = margin * glm::vec2(-SQRT3_2, 0.5);
  } else if (tile == HexTiling::LOW_LEFT) {
    offset = glm::vec2(-0.5f - SQRT3 / 8.f, -SQRT3 / 4.f);
    absolute_offset = margin * glm::vec2(-SQRT3_2, -0.5);
  } else if (tile == HexTiling::HIGH_RIGHT) {
    offset = glm::vec2(0.5f + SQRT3 / 8.f, SQRT3 / 4.f);
    absolute_offset = margin * glm::vec2(SQRT3_2, 0.5);
  } else if (tile == HexTiling::LOW_RIGHT) {
    offset = glm::vec2(0.5f + SQRT3 / 8.f, -SQRT3 / 4.f);
    absolute_offset = margin * glm::vec2(SQRT3_2, -0.5);
  } else {
    invariant_violation("Unknown hex tile");
  }

  return pos + offset * size + absolute_offset;
}
