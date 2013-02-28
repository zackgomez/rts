#include "rts/MatchmakerController.h"
#include <sstream>
#include "rts/CommandWidget.h"
#include "rts/Matchmaker.h"
#include "rts/Renderer.h"
#include "rts/ResourceManager.h"
#include "rts/UI.h"
#include "rts/Widgets.h"

namespace rts {

void renderSteamLOL(float dt) {
  drawDepthField(
      glm::vec2(25, 25),
      glm::vec2(200),
      glm::vec4(1.f),
      ResourceManager::get()->getDepthField("steamLogo"));
}

MatchmakerController::MatchmakerController(Matchmaker *mm)
  : matchmaker_(mm),
    infoWidget_(nullptr) {
}

MatchmakerController::~MatchmakerController() {
}

void MatchmakerController::onCreate() {
  createWidgets("matchmaker_menu");

  infoWidget_ = ((CommandWidget *)UI::get()->getWidget("matchmaker_menu.info_display"));
  infoWidget_->show(HUGE_VAL);
  infoWidget_->addMessage("Info Window.");

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
        
  matchmaker_->registerListener([&] (const std::string &s) {
    UI::get()->postToMainThread([=] () {
      infoWidget_->addMessage(s);
    });
  });

  UI::get()->addWidget("testwidget", createCustomWidget(renderSteamLOL));
}

void MatchmakerController::onDestroy() {
  UI::get()->clearWidgets();
}

void MatchmakerController::renderUpdate(float dt) {
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
