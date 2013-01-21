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

void MatchmakerController::onCreate() {
  createWidgets("matchmaker_menu");

  UI::get()->getWidget("matchmaker_menu.single_player_button")
    ->setClickable()
    ->setOnClickListener([&] (const glm::vec2 &pos) -> bool {
        matchmaker_->signalReady(Matchmaker::MODE_SINGLEPLAYER);
        return true;
        });

  UI::get()->getWidget("matchmaker_menu.matchmaking_button")
    ->setClickable()
    ->setOnClickListener([&] (const glm::vec2 &pos) -> bool {
        matchmaker_->signalReady(Matchmaker::MODE_MATCHMAKING);
        return true;
        });
}

void MatchmakerController::onDestroy() {
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
