#include "gtest/gtest.h"
#include <tuple>
#include "common/Clock.h"
#include "common/Collision.h"

// Tests pointInBox function with various points
TEST(CollisionTest, PointInBox) {
  glm::vec2 boxp(1, 3);
  glm::vec2 boxs(0.4, 0.6);
  float boxangle = glm::radians(212.f);

  // WOOOO initializer list
  std::vector<std::pair<glm::vec2, bool>> data = {
    {glm::vec2(1, 3), true},
    {glm::vec2(0, 0), false},
    {glm::vec2(1.15, 2.8), true},
    {glm::vec2(HUGE_VAL, HUGE_VAL), false},
    {glm::vec2(-1, 3), false}};

  for (auto datum : data) {
    ASSERT_EQ(datum.second, pointInBox(datum.first, boxp, boxs, boxangle));
  }
}

// Tests two boxes always colliding at different angles
TEST(CollisionTest, BoxInBoxRotation) {
  glm::vec2 box1(-1, -1);
  glm::vec2 box2(2, -1);
  glm::vec2 size(4, 3);

  for (float a = 0; a < 360.f; a++) {
    ASSERT_TRUE(boxInBox(
      Rect(box1, size, 0),
      Rect(box2, size, a)));
  }
}

float randomFloat(float min, float max) {
  return (rand() / (float)RAND_MAX) * (max - min) + min;
}

glm::vec2 randomVec(float min, float max) {
  return glm::vec2(randomFloat(min, max), randomFloat(min, max));
}

// Generates n random boxes for testing
std::vector<Rect> generateBoxes(size_t n) {
  std::vector<Rect> boxes;
  for (auto i = 0; i < 500; i++) {
    // Emplace back directly calls the constructor with the passed arguments,
    // avoiding a copy/move operation and involving less typing
    boxes.emplace_back(
      randomVec(-1, 1),
      randomVec(-1, 1),
      randomFloat(0, 360));
  }
  return boxes;
}

TEST(CollisionTest, BoxBoxCollision) {
  glm::vec2 boxp(1, 3);
  glm::vec2 boxs(1, 1);
  float angle = glm::radians(212.f);

  glm::vec2 boxp2(3, 3);
  float angle2 = glm::radians(43.f);


  // Sanity, no motion, no overlap checks
  ASSERT_FALSE(boxInBox(Rect(boxp, boxs, angle), Rect(boxp2, boxs, angle2)));
  ASSERT_FALSE(boxBoxCollision(
        Rect(boxp, boxs, angle), glm::vec2(0.f),
        Rect(boxp2, boxs, angle2), glm::vec2(0.f),
        10.f) != NO_INTERSECTION);

  // Move towards, with enough time
  ASSERT_TRUE(boxBoxCollision(
        Rect(boxp, boxs, angle), glm::vec2(3, 0),
        Rect(boxp2, boxs, angle2), glm::vec2(0.f),
        10.f) != NO_INTERSECTION);

  // Move towards, would hit, not enough time
  ASSERT_FALSE(boxBoxCollision(
        Rect(boxp, boxs, angle), glm::vec2(3, 0),
        Rect(boxp2, boxs, angle2), glm::vec2(0.f),
        0.1f) != NO_INTERSECTION);

  // Move different directions
  ASSERT_FALSE(boxBoxCollision(
        Rect(boxp, boxs, angle), glm::vec2(-3, 0),
        Rect(boxp2, boxs, angle2), glm::vec2(0.f),
        10.f) != NO_INTERSECTION);
  ASSERT_FALSE(boxBoxCollision(
        Rect(boxp, boxs, angle), glm::vec2(-3, 3),
        Rect(boxp2, boxs, angle2), glm::vec2(0.f),
        10.f) != NO_INTERSECTION);
  ASSERT_FALSE(boxBoxCollision(
        Rect(boxp, boxs, angle), glm::vec2(0, -3),
        Rect(boxp2, boxs, angle2), glm::vec2(0.f),
        10.f) != NO_INTERSECTION);
  ASSERT_FALSE(boxBoxCollision(
        Rect(boxp, boxs, angle), glm::vec2(3, -5),
        Rect(boxp2, boxs, angle2), glm::vec2(0.f),
        10.f) != NO_INTERSECTION);
  ASSERT_FALSE(boxBoxCollision(
        Rect(boxp, boxs, angle), glm::vec2(3, 5),
        Rect(boxp2, boxs, angle2), glm::vec2(0.f),
        10.f) != NO_INTERSECTION);

  // Make sure velocites are combined correctly
  ASSERT_TRUE(boxBoxCollision(
        Rect(boxp, boxs, angle), glm::vec2(0.f),
        Rect(boxp2, boxs, angle2), glm::vec2(-3, 0),
        10) != NO_INTERSECTION);
  ASSERT_TRUE(boxBoxCollision(
        Rect(boxp, boxs, angle), glm::vec2(-1, 0),
        Rect(boxp2, boxs, angle2), glm::vec2(-3, 0),
        10) != NO_INTERSECTION);

  // simple time return check
  ASSERT_EQ(
    1.f,
    boxBoxCollision(
      Rect(boxp, boxs, 0.f), glm::vec2(1, 0),
      Rect(boxp2, boxs, 0.f), glm::vec2(0.f),
      10.f));
}
