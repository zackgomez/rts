#include "MatrixStack.h"

MatrixStack::MatrixStack() :
    current_(1.f)
{
}

MatrixStack::~MatrixStack()
{
}

glm::mat4 & MatrixStack::current()
{
    return current_;
}

const glm::mat4 & MatrixStack::current() const
{
    return current_;
}

void MatrixStack::push()
{
    stack_.push(current_);
}

void MatrixStack::pop()
{
    assert(!stack_.empty());

    current_ = stack_.top();
    stack_.pop();
}

void MatrixStack::clear()
{
    current_ = glm::mat4(1.f);
    while (!stack_.empty())
        stack_.pop();
}
