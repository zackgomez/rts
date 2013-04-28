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
  createWidgets(getUI(), "matchmaker_menu");

  infoWidget_ = ((CommandWidget *)getUI()->getWidget("matchmaker_menu.info_display"));
  infoWidget_->show(HUGE_VAL);
  infoWidget_->addMessage("Info Window.");

  ((SizedWidget*)getUI()->getWidget("matchmaker_menu.single_player_button"))
    ->setOnClickListener([&] (const glm::vec2 &pos) -> bool {
        matchmaker_->signalReady(Matchmaker::MODE_SINGLEPLAYER);
        return true;
        });

  ((SizedWidget*)getUI()->getWidget("matchmaker_menu.matchmaking_button"))
    ->setOnClickListener([&] (const glm::vec2 &pos) -> bool {
        matchmaker_->signalReady(Matchmaker::MODE_MATCHMAKING);
        return true;
        });
        
  matchmaker_->registerListener([=] (const std::string &s) {
    infoWidget_->addMessage(s);
  });

  getUI()->addWidget("testwidget", createCustomWidget(renderSteamLOL));
}

void MatchmakerController::onDestroy() {
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
