#include "rts/MatchmakerController.h"
#include <sstream>
#include "common/Geometry.h"
#include "rts/CommandWidget.h"
#include "rts/Matchmaker.h"
#include "rts/Renderer.h"
#include "rts/ResourceManager.h"
#include "rts/UI.h"
#include "rts/Widgets.h"

namespace rts {

void renderLogo(float dt) {
  glm::vec2 p = glm::vec2(100, 100);
  glm::vec2 size = glm::vec2(50, 50);
  drawHexCenter(
      p,
      size,
      glm::vec4(0.5f, 0.5f, 0.5f, 0.5f));

  const float margin = 2.f;
  for (int i = 0; i < 6; i++) {
    auto tile = getHexTilings()[i];
    drawHexCenter(
        computeTiledHexPosition(tile, p, size, margin),
        size,
        glm::vec4(
          0.2f + ((i % 3 == 0) ? 0.7f : 0.f),
          0.2f + ((i % 3 == 1) ? 0.7f : 0.f),
          0.2f + ((i % 3 == 2) ? 0.7f : 0.f),
          0.5f));
  }
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
  renderLogo(dt);
  renderCursor("cursor_normal");
}

void MatchmakerController::onDestroy() {
}

void MatchmakerController::quitEvent() {
  Renderer::get()->signalShutdown();
}

void MatchmakerController::keyPress(const KeyEvent &ev) {
  if (ev.key == KeyCodes::INPUT_KEY_ESC) {
    Renderer::get()->signalShutdown();
  }
}
};  // rts
