#pragma once
#include <glm/glm.hpp>
#include "MatrixStack.h"

int initEngine(const glm::vec2 &resolution);
void teardownEngine();

void renderRectangleColor(
        const glm::mat4 &modelMatrix,
        const glm::vec4 &color);

MatrixStack& getViewStack();
MatrixStack& getProjectionStack();

