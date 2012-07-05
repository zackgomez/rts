#pragma once
#include <stack>
#include <glm/glm.hpp>

class MatrixStack
{
public:
    // Creates an empty matrix stack with the current matrix as the identity
    // matrix
    MatrixStack();
    ~MatrixStack();

    // Returns a reference to the current matrix
    glm::mat4 &current();
    const glm::mat4 &current() const;

    // Push a copy of the current matrix on to the top of the stack
    void push();
    // Pops the top of stack and sets current to the popped value
    void pop();

    // resets to identity and empty stack
    void clear();

private:
    glm::mat4 current_;
    std::stack<glm::mat4> stack_;
};

