#pragma once
#include <glm/glm.hpp>

class Map
{
public:
    explicit Map(const glm::vec2 &size) : size_(size) { }

    const glm::vec2 &getSize() const { return size_; }

private:
    glm::vec2 size_;
};

