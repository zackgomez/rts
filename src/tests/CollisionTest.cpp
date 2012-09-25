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
    ASSERT_TRUE(boxInBox(box1, size, 0, box2, size, a));
  }
}

float randomFloat(float min, float max) {
  return (rand() / (float)RAND_MAX) * (max - min) + min;
}

glm::vec2 randomVec(float min, float max) {
  return glm::vec2(randomFloat(min, max), randomFloat(min, max));
}

std::vector<std::tuple<glm::vec2, glm::vec2, float>> generateBoxes(size_t n) {
  // generate some data
  std::vector<std::tuple<glm::vec2, glm::vec2, float>> boxes;
  for (auto i = 0; i < 500; i++) {
    boxes.emplace_back(
      randomVec(-1, 1),
      randomVec(-1, 1),
      randomFloat(0, 360));
  }
  return boxes;
}

// Tests that the new boxInBox is faster than the old BoxInBox
TEST(CollisionTest, SpeedImprovement) {
  auto boxes = generateBoxes(1000);

  Clock timer;

  timer = Clock().start();
  for (auto i = 0; i < boxes.size(); i++) {
    for (auto j = i; j < boxes.size(); j++) {
      boxInBox(
        std::get<0>(boxes[i]),
        std::get<1>(boxes[i]),
        std::get<2>(boxes[i]),
        std::get<0>(boxes[j]),
        std::get<1>(boxes[j]),
        std::get<2>(boxes[j]));
    }
  }
  auto newT = timer.microseconds();

  timer = Clock().start();
  for (auto i = 0; i < boxes.size(); i++) {
    for (auto j = i; j < boxes.size(); j++) {
      boxInBoxOld(
        std::get<0>(boxes[i]),
        std::get<1>(boxes[i]),
        std::get<2>(boxes[i]),
        std::get<0>(boxes[j]),
        std::get<1>(boxes[j]),
        std::get<2>(boxes[j]));
    }
  }
  auto oldT = timer.microseconds();
  ASSERT_TRUE(newT < oldT);
}

// tests connor vs zack's box implementation
TEST(CollisionTest, BoxInBoxSameness) {
  // for repeatability
  srand(28420);

  auto boxes = generateBoxes(500);

  auto count = 0;
  auto n = 0;
  for (auto i = 0; i < boxes.size(); i++) {
    for (auto j = i; j < boxes.size(); j++) {
      n++;
      count +=
        boxInBoxOld(
          std::get<0>(boxes[i]),
          std::get<1>(boxes[i]),
          std::get<2>(boxes[i]),
          std::get<0>(boxes[j]),
          std::get<1>(boxes[j]),
          std::get<2>(boxes[j])) !=
        boxInBox(
          std::get<0>(boxes[i]),
          std::get<1>(boxes[i]),
          std::get<2>(boxes[i]),
          std::get<0>(boxes[j]),
          std::get<1>(boxes[j]),
          std::get<2>(boxes[j]));
    }
  }

  ASSERT_EQ(0.f, ((float) count) / n);
}
