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

  ((StaticWidget*)getUI()->getWidget("matchmaker_menu.single_player_button"))
    ->setOnPressListener([=] () -> bool {
        matchmaker_->signalReady(Matchmaker::MODE_SINGLEPLAYER);
        return true;
        });

  ((StaticWidget*)getUI()->getWidget("matchmaker_menu.matchmaking_button"))
    ->setOnPressListener([=] () -> bool {
        matchmaker_->signalReady(Matchmaker::MODE_MATCHMAKING);
        return true;
        });

  ((StaticWidget*)getUI()->getWidget("matchmaker_menu.quit_button"))
    ->setOnPressListener([=] () -> bool {
        matchmaker_->signalStop();
        Renderer::get()->signalShutdown();
        return true;
      });
        
  matchmaker_->registerListener([=] (const std::string &s) {
    infoWidget_->addMessage(s);
  });
}

void MatchmakerController::renderExtra(float dt) {
  renderSteamLOL(dt);
}

void MatchmakerController::onDestroy() {
}

void MatchmakerController::quitEvent() {
  Renderer::get()->signalShutdown();
}

void MatchmakerController::keyPress(int key, int mods) {
  if (key == GLFW_KEY_ESCAPE) {
    Renderer::get()->signalShutdown();
  }
}
};  // rts
