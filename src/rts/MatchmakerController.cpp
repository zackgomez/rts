#include "rts/MatchmakerController.h"
#include <sstream>
#include "rts/Matchmaker.h"
#include "rts/Renderer.h"
#include "rts/UI.h"

namespace rts {

MatchmakerController::MatchmakerController(Matchmaker *mm)
  : matchmaker_(mm),
    elapsedTime_(0.f) {
}

MatchmakerController::~MatchmakerController() {
}

void MatchmakerController::initWidgets() {
  auto pos = glm::vec2(400.f);
  auto size = glm::vec2(200.f);
  auto fontHeight = 12.f;
  auto bgcolor = glm::vec4(1.f);
  auto textFunc = [=]()->std::string {
    std::stringstream ss;
    ss << "Time spent waiting: " << elapsedTime_;
    return ss.str();
  };
  auto widget = 
    createTextWidget(pos, size, fontHeight, bgcolor, textFunc);
  widget->setClickable(pos + size/2.f, size);
  widget->setOnClickListener([&] (const glm::vec2 &pos) -> bool {
    matchmaker_->signalReady();
    return true;
  });

  UI::get()->addWidget("matchmaker", widget);
}

void MatchmakerController::clearWidgets() {
  UI::get()->clearWidgets();
}

void MatchmakerController::renderUpdate(float dt) {
  elapsedTime_ += dt;
}

void MatchmakerController::quitEvent() {
  Renderer::get()->signalShutdown();
}

void MatchmakerController::keyPress(SDL_keysym key) {
  // TODO(zack): ESC quits
}
};  // rts
