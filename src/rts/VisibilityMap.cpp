#include "rts/VisibilityMap.h"
#include "common/ParamReader.h"
#include "rts/Actor.h"
#include "rts/GameEntity.h"
#include "rts/Map.h"
#include "rts/Player.h"

namespace rts {

VisibilityMap::VisibilityMap(
    const glm::vec2 &mapSize,
    std::function<bool(const GameEntity*)> visFunc)
  : mapSize_(mapSize),
    func_(visFunc) {
  auto gridRes = fltParam("game.visibility_grid_res");
  gridDim_ = gridRes * mapSize_;
  gridDim_ = gridDim_ - (gridDim_ % 4) + 4;

  grid_ = new uint8_t[gridDim_.x * gridDim_.y];
}

VisibilityMap::~VisibilityMap() {
  delete grid_;
}

void VisibilityMap::clear() {
  size_t len = gridDim_.x * gridDim_.y;
  memset(grid_, 0, len);
}

glm::ivec2 VisibilityMap::worldToGrid(glm::vec2 world) const {
  auto grid = (world + mapSize_/2.f) / mapSize_ * glm::vec2(gridDim_);
  return glm::ivec2(grid.x, grid.y);
}

void VisibilityMap::processEntity(const GameEntity *entity) {
  // TODO(zack): A bit hacky.. only allow for actors
  if (!entity->hasProperty(GameEntity::P_ACTOR)) {
    return;
  }
  if (func_(entity)) {
    auto actor = (const Actor *) entity;
    auto pos = actor->getPosition2();
    auto sight = actor->getSight();
    glm::ivec2 min = glm::max(worldToGrid(pos - sight), 0);
    glm::ivec2 max = glm::min(worldToGrid(pos + sight), gridDim_);

    for (size_t y = min.y; y < max.y; y++) {
      size_t idx = y * gridDim_.x;
      float world_y = (y + 0.5f) / (float)gridDim_.y * mapSize_.y - mapSize_.y / 2.f;
      for (size_t x = min.x; x < max.x; x++) {
        float world_x = (x + 0.5f) / (float)gridDim_.x * mapSize_.x - mapSize_.x / 2.f;
        glm::vec2 center(world_x, world_y);
        if (glm::distance(center, pos) < sight) {
          grid_[idx + x] = 255;
        }
      }
    }
  }
}

bool VisibilityMap::locationVisible(glm::vec2 pos) const {
  glm::ivec2 cell = worldToGrid(pos);
  return grid_[cell.x + cell.y * gridDim_.x];
}

GLuint VisibilityMap::fillTexture(GLuint tex) const {
  glBindTexture(GL_TEXTURE_2D, tex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA,
      gridDim_.x, gridDim_.y, 0,
      GL_ALPHA, GL_UNSIGNED_BYTE, grid_);

  return tex;
}

}; // rts
