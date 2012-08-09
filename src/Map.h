#pragma once
#include "glm.h"

namespace rts {

class Map {
public:
  explicit Map(const glm::vec2 &size) : size_(size) { }

  const glm::vec2 &getSize() const {
    return size_;
  }
  // Initializes the map and any start entities/etc
  void init();

private:
  glm::vec2 size_;
};

}; // rts
