#include "rts/CommandWidget.h"
#include "common/ParamReader.h"
#include "rts/FontManager.h"
#include "rts/Graphics.h"
#include "rts/UI.h"

namespace rts {
CommandWidget::CommandWidget(const std::string &name)
  : StaticWidget(name),
    name_(name),
    capturing_(false),
    closeOnSubmit_(false),
    showDuration_(0.f) {
}

CommandWidget::~CommandWidget() {
}

CommandWidget* CommandWidget::addMessage(const std::string &s) {
  std::unique_lock<std::mutex> lock(lock_);
  messages_.push_back(s);
  return this;
}

CommandWidget* CommandWidget::show(float duration) {
  showDuration_ = duration;
  return this;
}

CommandWidget* CommandWidget::hide() {
  showDuration_ = 0.f;
  stopCapturing();
  return this;
}

void CommandWidget::stopCapturing() {
  capturing_ = false;
  getUI()->clearKeyCapturer();
  getUI()->clearCharCapturer();
}

CommandWidget* CommandWidget::captureText(const std::string &prefix) {
  capturing_ = true;
  buffer_ = prefix;
  getUI()->setKeyCapturer([&](const KeyEvent &ev) -> bool {
    if (ev.key == INPUT_KEY_ESC) {
      stopCapturing();
    } else if (ev.key == INPUT_KEY_RETURN) {
      if (!buffer_.empty() && textSubmittedHandler_) {
        textSubmittedHandler_(buffer_);
      }
      buffer_.clear();
      if (closeOnSubmit_) {
        stopCapturing();
      }
    } else if (ev.key == INPUT_KEY_BACKSPACE) {
      buffer_.pop_back();
    }
    return true;
  });
  getUI()->setCharCapturer([&](unsigned int unicode) -> bool {
    if (unicode >= 32 && unicode < 128) {
      buffer_.push_back(unicode);
    }
    return true;
  });
  return this;
}

void CommandWidget::render(float dt) {
  std::unique_lock<std::mutex> lock(lock_);
  if (showDuration_ <= 0.f && !capturing_) {
    return;
  }

  showDuration_ -= dt;

  drawRectCenter(
    getCenter(),
    getSize(),
    vec4Param(name_ + ".backgroundColor"));

  auto fontHeight = fltParam(name_ + ".fontHeight");
  auto messageHeight = fltParam(name_ + ".messageHeight");
  auto inputHeight = fontHeight + fltParam(name_ + ".inputHeight");

  auto pos = glm::vec2(
      getCenter().x - getSize().x/2.f,
      getCenter().y + getSize().y/2.f);
  auto messagePos = glm::vec2(pos.x, pos.y - inputHeight);
  int maxMessages = intParam(name_ + ".maxMessages");
  for (auto it = messages_.rbegin();
       it != messages_.rend() && maxMessages > 0;
       it++, maxMessages--) {
    messagePos.y -= messageHeight;
    FontManager::get()->drawString(*it, messagePos, fontHeight);
  }

  if (capturing_) {
    drawRect(
        glm::vec2(pos.x, pos.y - inputHeight),
        glm::vec2(getSize().x, inputHeight),
        vec4Param(name_ + ".inputColor"));
    FontManager::get()->drawString(
        buffer_,
        glm::vec2(pos.x, pos.y - inputHeight/2.f - fontHeight/2.f),
        fontHeight);
  }
}
};  // rts
