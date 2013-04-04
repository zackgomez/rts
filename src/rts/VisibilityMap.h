#pragma once
#include <functional>
#include <glm/glm.hpp>
#include "rts/Graphics.h"

namespace rts {

class GameEntity;
class LocalPlayer;
class Map;

class VisibilityMap {
 public:
  typedef std::function<bool(const GameEntity*)> VisibilityFunc;
  VisibilityMap(const glm::vec2 &mapSize, VisibilityFunc func);
  ~VisibilityMap();

  void clear();
  void processEntity(const GameEntity *entity);
  bool locationVisible(glm::vec2 pos) const;
  GLuint fillTexture(GLuint tex) const;

 private:
  glm::ivec2 worldToGrid(glm::vec2 world) const;

  glm::vec2 mapSize_;
  VisibilityFunc func_;

  glm::ivec2 gridDim_;
  uint8_t *grid_;
};

};  // rts
