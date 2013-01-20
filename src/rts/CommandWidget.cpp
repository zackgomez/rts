#include "rts/CommandWidget.h"
#include <SDL/SDL.h>
#include "common/ParamReader.h"
#include "rts/FontManager.h"
#include "rts/Graphics.h"
#include "rts/UI.h"

namespace rts {
CommandWidget::CommandWidget(const std::string &name)
  : SizedWidget(
      vec2Param(name + ".pos"),
      vec2Param(name + ".dim")),
  name_(name),
    capturing_(false),
    showDuration_(0.f) {
}

CommandWidget::~CommandWidget() {
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
  UI::get()->clearKeyCapturer();
}

CommandWidget* CommandWidget::captureText() {
  capturing_ = true;
  UI::get()->setKeyCapturer([&](SDL_keysym keysym) -> bool {
      // if return
      /*
      if (!message_.empty()) {
        Message msg;
        action["pid"] = toJson(player_->getPlayerID());
        action["type"] = ActionTypes::CHAT;
        action["chat"] = message_;
        MessageHub::get()->addAction(action);
      }
      */
      // if esp
      // if backspace
      // else char
    return false;
  });
  // TODO(zack): capture input
  return this;
}

void CommandWidget::render(float dt) {
  if (showDuration_ <= 0.f && !capturing_) {
    return;
  }

  showDuration_ -= dt;

  drawRectCenter(getCenter(), getSize(), vec4Param(name_ + ".backgroundColor"));

  auto fontHeight = fltParam(name_ + ".fontHeight");
  auto messageHeight = fontHeight + fltParam(name_ + ".heightBuffer");
  auto inputHeight = fontHeight + fltParam(name_ + ".inputHeight");

  auto pos = glm::vec2(getCenter().x, getCenter().y + getSize().y/2.f);
  auto messagePos = pos - inputHeight;
  int maxMessages = intParam(name_ + ".maxMessages");
  for (auto it = messages_.rbegin();
       it != messages_.rend() && maxMessages > 0;
       it++, maxMessages--) {
    messagePos.y -= messageHeight;
    FontManager::get()->drawString(*it, messagePos, fontHeight);
  }

  if (capturing_) {
    drawRect(pos, glm::vec2(getSize().x, 1.2f * fontHeight),
        vec4Param(name_ + ".inputColor"));
    FontManager::get()->drawString(buffer_, pos - inputHeight, fontHeight);
  }
}
};  // rts
