#include "rts/MatchmakerController.h"
#include <sstream>
#include "rts/Matchmaker.h"
#include "rts/Renderer.h"
#include "rts/UI.h"
#include "rts/Widgets.h"

namespace rts {

MatchmakerController::MatchmakerController(Matchmaker *mm)
  : matchmaker_(mm),
    elapsedTime_(0.f) {
}

MatchmakerController::~MatchmakerController() {
}

void MatchmakerController::initWidgets() {
  std::string widgetNames[] = {
    "matchmaker_menu.single_player_button",
    "matchmaker_menu.matchmaking_button"
  };
  for (auto name : widgetNames) {
    auto widget = (TextWidget *)createWidget(name);
    widget->setClickable(widget->getCenter(), widget->getSize());
    UI::get()->addWidget(name, widget);
  }

  UI::get()->getWidget("matchmaker_menu.single_player_button")
    ->setOnClickListener([&] (const glm::vec2 &pos) -> bool {
        matchmaker_->signalReady();
        return true;
        });
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

void MatchmakerController::keyPress(SDL_keysym keysym) {
  if (keysym.sym == SDLK_ESCAPE) {
    Renderer::get()->signalShutdown();
  }
}
};  // rts
