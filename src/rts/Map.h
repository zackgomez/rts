#ifndef SRC_RTS_MAP_H_
#define SRC_RTS_MAP_H_

#include <string>
#include <vector>
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

  // Returns the height of the map at the passed position.  This is currently
  // only used for cosmetic purposes
  float getMapHeight(const glm::vec2 &pos) const;

  // Initializes the map and any start entities/etc
  void init(const std::vector<Player *> &players);
  void update(float dt);

  const glm::vec2 &getPathingGridDim() const {
    return gridDim_;
  }
  const std::vector<char>& getPathingGrid() const {
    return pathingGrid_;
  }

 private:
  void calculatePathingGrid();
  void dumpPathingGrid();

  std::string name_;
  glm::vec2 size_;
  // TODO(connor) replace this with an image
  glm::vec4 color_;

  glm::vec2 gridDim_;
  std::vector<char> pathingGrid_;
};
};  // rts

#endif  // SRC_RTS_MAP_H_
