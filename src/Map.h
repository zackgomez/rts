#pragma once
#include <string>
#include <glm/glm.hpp>

namespace rts {

class Player;

class Map {
public:
  explicit Map(const std::string &mapName);

  const glm::vec2 &getSize() const {
    return size_;
  }
  // Initializes the map and any start entities/etc
  void init(const std::vector<Player *> &players);

private:
  std::string name_;
  glm::vec2 size_;
};

}; // rts
