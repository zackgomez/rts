#ifndef SRC_RTS_MAP_H_
#define SRC_RTS_MAP_H_

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

  const glm::vec4 &getMinimapColor() const {
    return color_;
  }
  // Initializes the map and any start entities/etc
  void init(const std::vector<Player *> &players);

 private:
  std::string name_;
  glm::vec2 size_;
  // TODO(connor) replace this with an image
  glm::vec4 color_;
};
};  // rts

#endif  // SRC_RTS_MAP_H_
