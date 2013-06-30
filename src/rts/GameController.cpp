#define GLM_SWIZZLE_XYZW
#include "rts/GameController.h"
#include <iomanip>
#include <sstream>
#include <glm/gtx/norm.hpp>
#include "common/util.h"
#include "common/ParamReader.h"
#include "rts/ActionWidget.h"
#include "rts/Actor.h"
#include "rts/CommandWidget.h"
#include "rts/Game.h"
#include "rts/Map.h"
#include "rts/Matchmaker.h"
#include "rts/MinimapWidget.h"
#include "rts/Player.h"
#include "rts/PlayerAction.h"
#include "rts/Renderer.h"
#include "rts/VisibilityMap.h"
#include "rts/UI.h"
#include "rts/Widgets.h"

#ifdef _MSC_VER
#define isinf(x) (!_finite(x))
#endif

namespace rts {

namespace PlayerState {
const std::string DEFAULT = "DEFAULT";
const std::string CHATTING = "CHATTING";
}

static void renderEntity(
    const LocalPlayer *localPlayer,
    const std::map<id_t, float>& entityHighlights,
    const ModelEntity *e,
    float dt);

static void renderHighlights(
    std::vector<MapHighlight> &highlights,
    float dt) {
  // Render each of the highlights
  for (auto& hl : highlights) {
    hl.remaining -= dt;
    glm::mat4 transform =
      glm::scale(
          glm::translate(
            glm::mat4(1.f),
            glm::vec3(hl.pos.x, hl.pos.y, 0.01f)),
          glm::vec3(0.33f));
    renderCircleColor(
        transform,
        glm::vec4(1, 0, 0, 1),
        fltParam("ui.highlight.thickness"));
  }
}

void renderDragRect(bool enabled, const glm::vec2 &start, const glm::vec2 &end, float dt) {
  if (!enabled) {
    return;
  }
  float dist = glm::distance(start, end);
  if (dist < fltParam("hud.minDragDistance")) {
    return;
  }
  // TODO(zack): make this color a param
  auto color = glm::vec4(0.2f, 0.6f, 0.2f, 0.3f);
  drawRect(start, end - start, color);
}

std::string getVPString(id_t team) {
  return std::to_string((int)Game::get()->getVictoryPoints(team));
}

GameController::GameController(LocalPlayer *player)
  : player_(player),
    shift_(false),
    ctrl_(false),
    alt_(false),
    leftDrag_(false),
    leftDragMinimap_(false),
    renderNavMesh_(false),
    state_(PlayerState::DEFAULT),
    zoom_(0.f),
    order_(),
    action_() {
}

GameController::~GameController() {
}

void GameController::onCreate() {
  SDL_ShowCursor(0);
  SDL_WM_GrabInput(SDL_GRAB_ON);
  // TODO(zack): delete texture
  glGenTextures(1, &visTex_);
  glBindTexture(GL_TEXTURE_2D, visTex_);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  createWidgets(getUI(), "ui.widgets");

  using namespace std::placeholders;

  auto minimapWidget =
      new MinimapWidget("ui.widgets.minimap", player_->getPlayerID());
  getUI()->addWidget("ui.widgets.minimap", minimapWidget);
  minimapWidget->setMinimapListener(
    [&](const glm::vec2 &pos, int button) {
      if (button == SDL_BUTTON_LEFT) {
        leftDragMinimap_ = true;
      }
      if (button == SDL_BUTTON_RIGHT) {
        glm::vec3 v3;
        v3.x = pos.x * Renderer::get()->getMapSize().x;
        v3.y = -1 * pos.y * Renderer::get()->getMapSize().y;
        Json::Value order = handleRightClick(0, NULL, v3);
        attemptIssueOrder(order);
      }
    });

  auto actionFunc = [=]() -> std::vector<UIAction> {
    auto selection = player_->getSelection();
    if (selection.empty()) {
      return std::vector<UIAction>();
    }
    auto *actor = (Actor *)Game::get()->getEntity(*selection.begin());
    if (!actor) {
      return std::vector<UIAction>();
    }
    return actor->getActions();
  };
  auto actionExecutor = [=](const UIAction &action) {
    handleUIAction(action);
  };

  auto actionWidget = new ActionWidget("ui.widgets.action", actionFunc, actionExecutor);
  getUI()->addWidget("ui.widgets.action", actionWidget);

  auto chatWidget = new CommandWidget("ui.chat");
  chatWidget->setCloseOnSubmit(true);
  chatWidget->setOnTextSubmittedHandler([&, chatWidget](const std::string &t) {
    PlayerAction msg;
    msg["type"] = ActionTypes::CHAT;

    std::string text = t;
    if (t.find("/all ") == 0) {
      msg["target"] = "all";
      // Remove command text
      text = t.substr(5);
    } else {
      msg["target"] = "team";
    }
    // In future add additional check for whisper
    if (!text.empty()) {
      msg["chat"] = text;
      Game::get()->addAction(player_->getPlayerID(), msg);
    }
  });
  getUI()->addWidget("ui.widgets.chat", chatWidget);

  Game::get()->setChatListener([&](id_t pid, const Message &m) {
    const Player* from = Game::get()->getPlayer(pid);
    invariant(from, "No playyayayaya");
    std::stringstream ss;
    if (m["target"] == "all" || (m["target"] == "team" && from->getTeamID() == player_->getTeamID())) {
      ss << from->getName();
      if (m["target"] == "all") {
        ss << " (all)";
      }
      ss << ": " << m["chat"].asString();
      ((CommandWidget *)getUI()->getWidget("ui.widgets.chat"))
        ->addMessage(ss.str())
        ->show(fltParam("ui.chat.chatDisplayTime"));
    } // Note: When we add whisper, append additional conditional
  });

  Renderer::get()->setEntityOverlayRenderer(
      std::bind(renderEntity, player_, std::ref(entityHighlights_), _1, _2));

  ((TextWidget *)getUI()->getWidget("ui.widgets.vicdisplay-1"))
    ->setTextFunc(std::bind(getVPString, STARTING_TID));

  ((TextWidget *)getUI()->getWidget("ui.widgets.vicdisplay-2"))
    ->setTextFunc(std::bind(getVPString, STARTING_TID + 1));

  ((TextWidget *)getUI()->getWidget("ui.widgets.reqdisplay"))
    ->setTextFunc([&]() -> std::string {
      int req = Game::get()->getResources(player_->getPlayerID()).requisition;
      return "Req: " + std::to_string(req);
    });

  ((TextWidget *)getUI()->getWidget("ui.widgets.perfinfo"))
    ->setTextFunc([]() -> std::string {
        glm::vec3 color(0.1f, 1.f, 0.1f);
        int fps = Renderer::get()->getAverageFPS();
        return std::string() + FontManager::makeColorCode(color)
        + "FPS: " + std::to_string(fps);
    });
  Renderer::get()->resetCameraRotation();
}

void GameController::onDestroy() {
  SDL_ShowCursor(1);
  SDL_WM_GrabInput(SDL_GRAB_OFF);
  Renderer::get()->setEntityOverlayRenderer(Renderer::EntityOverlayRenderer());
  getUI()->clearWidgets();
  glDeleteTextures(1, &visTex_);
}

void GameController::renderExtra(float dt) {
  renderDragRect(leftDrag_, leftStart_, lastMousePos_, dt);
  renderHighlights(highlights_, dt);

  if (!action_.name.empty()) {
    GameEntity *e = Game::get()->getEntity(action_.owner);
    invariant(e, "Unable to find action owner");

    glm::mat4 transform = glm::scale(
        glm::translate(glm::mat4(1.f), e->getPosition() + glm::vec3(0, 0, 0.1f)),
        glm::vec3(2 * action_.range));

    renderCircleColor(transform, glm::vec4(0, 0, 1, 1.5));
  }

  // TODO(zack): bit of hack here
  if (renderNavMesh_) {
    renderNavMesh(*Game::get()->getMap()->getNavMesh(),
        glm::scale(glm::translate(glm::mat4(1.f), glm::vec3(0, 0, 0.15f)), glm::vec3(0.8)),
        glm::vec4(0.6, 0.6, 0.2, 0.75f));
  }

  // render VP controller indicators
  std::vector<Actor*> vp;
  for (auto eid : Renderer::get()->getEntities()) {
    if (!eid.second->hasProperty(GameEntity::P_GAMEENTITY)) continue;
    GameEntity *e = (GameEntity*) eid.second;
    if (e->getName() == "victory_point") {
      vp.push_back((Actor*) e);
    }
  }
  for (int i = 0; i < vp.size(); i++) {
    glm::vec2 size = vec2Param("ui.widgets.vpindicators.dim");
    glm::vec2 center = uiVec2Param("ui.widgets.vpindicators.center") + 
      glm::vec2((i - (int)(vp.size() / 2)) * size.x * 1.5, 0);
    id_t pid = vp[i]->getPlayerID();
    glm::vec4 ownerColor = vec4Param("ui.widgets.vpindicators.bgcolor");
    if (pid != NO_PLAYER) 
      ownerColor = glm::vec4(Game::get()->getPlayer(pid)->getColor(), 1.0);
    glm::vec4 capColor = vec4Param("ui.widgets.vpindicators.bgcolor");
    id_t cap_pid = vp[i]->getUIInfo().capture_pid;
    if (cap_pid != NO_PLAYER) 
      capColor = glm::vec4(Game::get()->getPlayer(cap_pid)->getColor(), 1.0);
    glm::vec2 capture = vp[i]->getUIInfo().capture;
    Shader *shader = ResourceManager::get()->getShader("vp_indicator");
    shader->makeActive();
    if (capture[1] > 0) {
      shader->uniform1f("capture", capture[0] / capture[1]);
    } else {
      shader->uniform1f("capture", 0);
    }
    shader->uniform4f("player_color", ownerColor);
    shader->uniform4f("cap_color", capColor);
    shader->uniform4f("texcoord", glm::vec4(0, 0, 1, 1));
    drawShaderCenter(center, size);
  }

  // render cursor
  glDisable(GL_DEPTH_TEST);
  const auto res = Renderer::get()->getResolution();
  const float cursor_size = std::min(res.x, res.y) * 0.05f;
  GLuint texture = getCursorTexture();
  drawTextureCenter(
      lastMousePos_,
      glm::vec2(cursor_size),
      texture, glm::vec4(0, 0, 1, 1));
  glEnable(GL_DEPTH_TEST);
}

GLuint GameController::getCursorTexture() const {
  std::string texname = "cursor_normal";
  if (!action_.name.empty()) {
    texname = "cursor_action";
  }
  return ResourceManager::get()->getTexture(texname);
}

void GameController::frameUpdate(float dt) {
  // No input while game is paused, not even camera motion
  if (Game::get()->isPaused()) {
    return;
  }

  // update visibility
  // TODO(zack): this should be done once per tick instead of once per render
  auto visibilityMap = Game::get()->getVisibilityMap(player_->getPlayerID());
  for (auto it : Renderer::get()->getEntities()) {
    auto e = it.second;
    if (e->hasProperty(GameEntity::P_CAPPABLE)) {
      e->setVisible(true);
    } else if (e->hasProperty(GameEntity::P_ACTOR)) {
      e->setVisible(visibilityMap->locationVisible(e->getPosition2()));
    }
  }

  // Remove done highlights
  for (size_t i = 0; i < highlights_.size(); ) {
    if (highlights_[i].remaining <= 0.f) {
      std::swap(highlights_[i], highlights_[highlights_.size() - 1]);
      highlights_.pop_back();
    } else {
      i++;
    }
  }

  auto it = entityHighlights_.begin();
  while (it != entityHighlights_.end()) {
    auto cur = (entityHighlights_[it->first] -= dt);
    if (cur <= 0.f) {
      it = entityHighlights_.erase(it);
    } else {
      it++;
    }
  }

  int x, y;
  SDL_GetMouseState(&x, &y);
  glm::vec2 screenCoord(x, y);
  const glm::vec2 &res = Renderer::get()->getResolution();
  const int CAMERA_PAN_THRESHOLD = std::max(
    intParam("local.camera.panthresh"),
    1);  // to prevent division by zero

  // Mouse overrides keyboard
  glm::vec2 dir = cameraPanDir_;

  if (x <= CAMERA_PAN_THRESHOLD) {
    dir.x = 2 * (x - CAMERA_PAN_THRESHOLD) / CAMERA_PAN_THRESHOLD;
  } else if (x >= res.x - CAMERA_PAN_THRESHOLD) {
    dir.x = -2 * ((res.x - x) - CAMERA_PAN_THRESHOLD) / CAMERA_PAN_THRESHOLD;
  }
  if (y <= CAMERA_PAN_THRESHOLD) {
    dir.y = -2 * (y - CAMERA_PAN_THRESHOLD) / CAMERA_PAN_THRESHOLD;
  } else if (y >= res.y - CAMERA_PAN_THRESHOLD) {
    dir.y = 2 * ((res.y - y) - CAMERA_PAN_THRESHOLD) / CAMERA_PAN_THRESHOLD;
  }
  Renderer::get()->zoomCamera(zoom_);

  // Don't allow screen translations during rotation
  if (!alt_) {
    const float CAMERA_PAN_SPEED = fltParam("camera.panspeed");
    glm::vec3 delta = CAMERA_PAN_SPEED * dt * glm::vec3(dir.x, dir.y, 0.f);
    Renderer::get()->updateCamera(delta);
  }

  // Deselect dead entities
  std::set<id_t> newsel;
  for (auto eid : player_->getSelection()) {
    assertEid(eid);
    const GameEntity *e = Game::get()->getEntity(eid);
    if (e && e->getPlayerID() == player_->getPlayerID()) {
      newsel.insert(eid);
    }
  }
  if (!action_.name.empty() && !newsel.count(action_.owner)) {
    action_.name.clear();
  }
  player_->setSelection(newsel);

  if (leftDragMinimap_) {
    minimapUpdateCamera(screenCoord);
  }
}

void GameController::quitEvent()
{
  // Send the quit game event
  PlayerAction action;
  action["type"] = ActionTypes::LEAVE_GAME;
  Game::get()->addAction(player_->getPlayerID(), action);
}

void GameController::mouseDown(const glm::vec2 &screenCoord, int button) {
  if (alt_) {
    return;
  }
  Json::Value order;
  std::set<id_t> newSelect = player_->getSelection();

  glm::vec3 loc = Renderer::get()->screenToTerrain(screenCoord);
  if (isinf(loc.x) || isinf(loc.y)) {
    return;
  }
  // The entity (maybe) under the cursor
  id_t eid = selectEntity(screenCoord);
  const GameEntity *entity = Game::get()->getEntity(eid);

  if (button == SDL_BUTTON_LEFT) {
    if (!order_.empty()) {
      order["type"] = order_;
      order["entity"] = toJson(player_->getSelection());
      order["target"] = toJson(loc);
      if (entity && entity->getTeamID() != player_->getTeamID()) {
        order["target_id"] = toJson(entity->getID());
      }
      order_.clear();
    } else if (!action_.name.empty()) {
      if (!newSelect.count(action_.owner)) {
        return;
      }
      std::set<id_t> ids(&action_.owner, (&action_.owner)+1);
      if (action_.targeting == UIAction::TargetingType::NONE) {
        order["type"] = OrderTypes::ACTION;
        order["entity"] = toJson(ids);
        order["action"] = action_.name;
      } else if (action_.targeting == UIAction::TargetingType::LOCATION) {
        order["type"] = OrderTypes::ACTION;
        order["entity"] = toJson(ids);
        order["action"] = action_.name;
        order["target"] = toJson(glm::vec2(loc));
      } else if (action_.targeting == UIAction::TargetingType::ENEMY) {
        if (!entity
            || !entity->hasProperty(GameEntity::P_TARGETABLE)
            || entity->getTeamID() == NO_TEAM
            || entity->getTeamID() == player_->getTeamID()) {
          return;
        }
        order["type"] = OrderTypes::ACTION;
        order["entity"] = toJson(ids);
        order["action"] = action_.name;
        order["target_id"] = toJson(entity->getID());
        highlightEntity(entity->getID());
      } else if (action_.targeting == UIAction::TargetingType::ALLY) {
        if (!entity
            || !entity->hasProperty(GameEntity::P_TARGETABLE)
            || entity->getTeamID() == NO_TEAM
            || entity->getTeamID() != player_->getTeamID()) {
          return;
        }
        order["type"] = OrderTypes::ACTION;
        order["entity"] = toJson(ids);
        order["action"] = action_.name;
        order["target_id"] = toJson(entity->getID());
        highlightEntity(entity->getID());
      } else {
        invariant_violation("Unsupported targeting type");
      }
      action_.name.clear();
    // If no order, then adjust selection
    } else {
      leftStart_ = screenCoord;
      leftDrag_ = true;
      // If no shift held, then deselect (shift just adds)
      if (!shift_) {
        newSelect.clear();
      }
      // If there is an entity and its ours, select
      if (entity && entity->getPlayerID() == player_->getPlayerID()) {
        newSelect.insert(eid);
      }
    }
  } else if (button == SDL_BUTTON_RIGHT) {
    order = handleRightClick(eid, entity, loc);
  } else if (button == SDL_BUTTON_WHEELUP) {
    Renderer::get()->zoomCamera(-fltParam("local.mouseZoomSpeed"));
  } else if (button == SDL_BUTTON_WHEELDOWN) {
    Renderer::get()->zoomCamera(fltParam("local.mouseZoomSpeed"));
  }

  if (!Game::get()->isPaused()) {
    // Mutate, if game isn't paused
    player_->setSelection(newSelect);
    attemptIssueOrder(order);
  }
}

void GameController::attemptIssueOrder(Json::Value order) {
  if (!Game::get()->isPaused() && order.isMember("type")) {
    PlayerAction action;
    action["type"] = ActionTypes::ORDER;
    action["order"] = order;
    Game::get()->addAction(player_->getPlayerID(), action);
  }
}




Json::Value GameController::handleRightClick(const id_t eid, 
  const GameEntity *entity, 
  const glm::vec3 &loc) {
  Json::Value order;
  // TODO(connor) make right click actions on minimap
  // If there is an order, it is canceled by right click
  if (!order_.empty() || !action_.name.empty()) {
    order_.clear();
    action_.name.clear();
  } else {
    // Otherwise default to some right click actions
    // If right clicked on enemy unit (and we have a selection)
    // go attack them
    if (entity && entity->getTeamID() != player_->getTeamID()
        && !player_->getSelection().empty()) {
      // Visual feedback
      highlightEntity(entity->getID());

      // Queue up action
      if (entity->hasProperty(GameEntity::P_CAPPABLE)) {
        order["type"] = OrderTypes::CAPTURE;
      } else {
        order["type"] = OrderTypes::ATTACK;
      }
      order["entity"] = toJson(player_->getSelection());
      order["target_id"] = toJson(eid);
    // If we have a selection, and they didn't click on the current
    // selection, move them to target
    } else if (!player_->getSelection().empty()
        && (!entity || !player_->getSelection().count(eid))) {
      //loc.x/y wil be -/+HUGE_VAL if outside map bounds
      if (loc.x != HUGE_VAL && loc.y != HUGE_VAL && loc.x != -HUGE_VAL && loc.y != -HUGE_VAL) {
        // Visual feedback
        highlight(glm::vec2(loc.x, loc.y));

        // Queue up action
        order["type"] = OrderTypes::MOVE;
        order["entity"] = toJson(player_->getSelection());
        order["target"] = toJson(loc);
      }
    }
  }
  return order;
}

void GameController::mouseUp(const glm::vec2 &screenCoord, int button) {
  if (button == SDL_BUTTON_LEFT) {
    std::set<id_t> newSelect;
    if (leftDrag_ &&
        glm::distance(leftStart_, screenCoord) > fltParam("hud.minDragDistance")) {
      auto selection = player_->getSelection();
      newSelect = selectEntities(
        leftStart_,
        screenCoord,
        player_->getPlayerID());

      if (shift_) {
        newSelect.insert(selection.begin(), selection.end());
      }
      player_->setSelection(std::move(newSelect));
    }

    leftDrag_ = false;
    leftDragMinimap_ = false;
  }
  // nop
}

void GameController::mouseMotion(const glm::vec2 &screenCoord) {
  // Rotate camera while alt is held
  if (alt_) {
    glm::vec2 delta = screenCoord - lastMousePos_;
    delta *= fltParam("local.cameraRotSpeed");
    Renderer::get()->rotateCamera(delta);
  }
  lastMousePos_ = screenCoord;
}

void GameController::keyPress(SDL_keysym keysym) {
  SDLKey key = keysym.sym;
  // TODO(zack) watch out for pausing here
  // Actions available in all player states:
  if (key == SDLK_F10) {
    PlayerAction action;
    action["type"] = ActionTypes::LEAVE_GAME;
    Game::get()->addAction(player_->getPlayerID(), action);
  // Camera panning
  } else if (key == SDLK_UP) {
    if (alt_) {
      zoom_ = -fltParam("local.keyZoomSpeed");
    } else {
      cameraPanDir_.y = 1.f;
    }
  // Control group binding and recalling
  } else if (savedSelection_.count(key)) {
    if (ctrl_) {
      savedSelection_[key] = player_->getSelection();
    } else {
      // center camera for double tapping
      if (!savedSelection_[key].empty()){
        if (player_->getSelection() == savedSelection_[key]) {
          id_t eid = (*player_->getSelection().begin());
          auto entity = Game::get()->getEntity(eid);
          glm::vec3 pos = entity->getPosition();
          Renderer::get()->setCameraLookAt(pos);
        }
      }
      player_->setSelection(savedSelection_[key]);
    }
  } else if (key == SDLK_DOWN) {
    if (alt_) {
      zoom_ = fltParam("local.keyZoomSpeed");
    } else {
      cameraPanDir_.y = -1.f;
    }
  } else if (key == SDLK_RIGHT) {
    cameraPanDir_.x = 1.f;
  } else if (key == SDLK_LEFT) {
    cameraPanDir_.x = -1.f;
  } else if (state_ == PlayerState::DEFAULT) {
    if (key == SDLK_RETURN) {
      std::string prefix = (shift_) ? "/all " : "";
      ((CommandWidget *)getUI()->getWidget("ui.widgets.chat"))
        ->captureText(prefix);
    } else if (key == SDLK_ESCAPE) {
      // ESC clears out current states
      if (!order_.empty() || !action_.name.empty()) {
        order_.clear();
        action_.name.clear();
      } else {
        player_->setSelection(std::set<id_t>());
      }
    } else if (key == SDLK_LSHIFT || key == SDLK_RSHIFT) {
      shift_ = true;
    } else if (key == SDLK_LCTRL || key == SDLK_RCTRL) {
      ctrl_ = true;
    } else if (key == SDLK_LALT || key == SDLK_RALT) {
      alt_ = true;
    } else if (key == SDLK_n) {
      renderNavMesh_ = !renderNavMesh_;
    } else if (key == SDLK_BACKSPACE) {
      Renderer::get()->resetCameraRotation();
    } else if (!player_->getSelection().empty()) {
      // Handle unit commands
      // Order types
      if (key == SDLK_a) {
        order_ = OrderTypes::ATTACK;
      } else if (key == SDLK_m) {
        order_ = OrderTypes::MOVE;
      } else if (key == SDLK_h) {
        Json::Value order;
        order["type"] = OrderTypes::HOLD_POSITION;
        order["entity"] = toJson(player_->getSelection());
        PlayerAction action;
        action["type"] = ActionTypes::ORDER;
        action["order"] = order;
        Game::get()->addAction(player_->getPlayerID(), action);
      } else if (key == SDLK_s) {
        Json::Value order;
        order["type"] = OrderTypes::STOP;
        order["entity"] = toJson(player_->getSelection());
        PlayerAction action;
        action["type"] = ActionTypes::ORDER;
        action["order"] = order;
        Game::get()->addAction(player_->getPlayerID(), action);
      } else {
        auto sel = player_->getSelection().begin();
        auto actor = (const Actor *)Game::get()->getEntity(*sel);
        auto actions = actor->getActions();
        for (auto &action : actions) {
          if (action.hotkey && action.hotkey == key) {
            handleUIAction(action);
            break;
          }
        }
      }
    }
  }
}

void GameController::keyRelease(SDL_keysym keysym) {
  SDLKey key = keysym.sym;
  if (key == SDLK_RIGHT || key == SDLK_LEFT) {
    cameraPanDir_.x = 0.f;
  } else if (key == SDLK_UP || key == SDLK_DOWN) {
    cameraPanDir_.y = 0.f;
    zoom_ = 0.f;
  } else if (key == SDLK_LSHIFT || key == SDLK_RSHIFT) {
    shift_ = false;
  } else if (key == SDLK_LCTRL || key == SDLK_RCTRL) {
    ctrl_ = false;
  } else if (key == SDLK_LALT || key == SDLK_RALT) {
    alt_ = false;
  }
}

void GameController::minimapUpdateCamera(const glm::vec2 &screenCoord) {
  auto minimapWidget = (MinimapWidget *)getUI()->getWidget("ui.widgets.minimap");
  const glm::vec2 minimapPos = minimapWidget->getCenter();
  const glm::vec2 minimapDim = minimapWidget->getSize();
  glm::vec2 mapCoord = screenCoord;
  // [-minimapDim/2, minimapDim/2]
  mapCoord -= minimapPos;
  // [-mapSize/2, mapSize/2]
  mapCoord *= Renderer::get()->getMapSize() / minimapDim;
  mapCoord.y *= -1;
  glm::vec3 finalPos(mapCoord, Renderer::get()->getCameraPos().z);
  Renderer::get()->setCameraLookAt(finalPos);
}

id_t GameController::selectEntity(const glm::vec2 &screenCoord) const {
  glm::vec3 origin, dir;
  std::tie(origin, dir) = Renderer::get()->screenToRay(screenCoord);
  auto entity = Renderer::get()->castRay(
      origin,
      dir,
      [](const ModelEntity *e) {
        return e->isVisible() && e->hasProperty(GameEntity::P_ACTOR);
      });

  return entity ? entity->getID() : NO_ENTITY;
}

std::set<id_t> GameController::selectEntities(
  const glm::vec2 &start, const glm::vec2 &end, id_t pid) const {
  assertPid(pid);
  // returnedEntities is a subset of those the user boxed, depending on some 
  // selection criteria
  std::set<GameEntity *> boxedEntities;
  std::set<id_t> returnedEntities;

  glm::vec2 terrainStart = Renderer::get()->screenToTerrain(start).xy;
  glm::vec2 terrainEnd = Renderer::get()->screenToTerrain(end).xy;
  
  //Bound selection rect by map bounds
  glm::vec2 mapSize = Game::get()->getMap()->getSize()/2.f;
  terrainStart = glm::clamp(terrainStart, -mapSize, mapSize);
  terrainEnd = glm::clamp(terrainEnd, -mapSize, mapSize);
  
  // Defines the rectangle
  glm::vec2 center = (terrainStart + terrainEnd) / 2.f;
  glm::vec2 size = glm::abs(terrainEnd - terrainStart);
  Rect dragRect(center, size, 0.f);
  bool onlySelectUnits = false;

  for (const auto &pair : Renderer::get()->getEntities()) {
    auto e = pair.second;
    // Must be an actor owned by the passed player
    if (!e->hasProperty(GameEntity::P_ACTOR) && e->isVisible()) {
      continue;
    }
    auto ge = (GameEntity *) e;
    if (ge->getPlayerID() == pid
        && boxInBox(dragRect, ge->getRect(Renderer::get()->getSimDT()))) {
      boxedEntities.insert(ge);
      if (ge->hasProperty(GameEntity::P_UNIT)) {
        onlySelectUnits = true;
      }
    }
  }
  for (const auto &e : boxedEntities) {
    if (!onlySelectUnits || e->hasProperty(GameEntity::P_UNIT)) {
      returnedEntities.insert(e->getID());
    }
  }
  return returnedEntities;
}

void GameController::highlight(const glm::vec2 &mapCoord) {
  MapHighlight hl;
  hl.pos = mapCoord;
  hl.remaining = fltParam("ui.highlight.duration");
  highlights_.push_back(hl);
}

void GameController::highlightEntity(id_t eid) {
  entityHighlights_[eid] = fltParam("ui.highlight.duration");
}

void renderEntity(
    const LocalPlayer *localPlayer,
    const std::map<id_t, float>& entityHighlights,
    const ModelEntity *e,
    float dt) {
  if (!e->hasProperty(GameEntity::P_ACTOR) || !e->isVisible()) {
    return;
  }
  auto pos = e->getPosition(dt);
  auto transform = glm::translate(glm::mat4(1.f), pos);
  record_section("renderActorInfo");
  // TODO(zack): only render for actors currently on screen/visible
  auto entitySize = e->getSize();
  auto circleTransform = glm::scale(
      glm::translate(
        transform,
        glm::vec3(0, 0, 0.1)),
      glm::vec3(0.25f + sqrt(entitySize.x * entitySize.y)));
  if (localPlayer->isSelected(e->getID())) {
    // A bit of a hack here...
    renderCircleColor(
        circleTransform,
        glm::vec4(vec3Param("colors.selected"), 1.f),
        fltParam("ui.highlight.thickness"));
  } else if (entityHighlights.find(e->getID()) != entityHighlights.end()) {
    // A bit of a hack here...
    renderCircleColor(
        circleTransform,
        glm::vec4(vec3Param("colors.targeted"), 1.f),
        fltParam("ui.highlight.thickness"));
  }

  glDisable(GL_DEPTH_TEST);
  glm::vec3 placardPos(0.f, 0.f, e->getHeight() + 0.50f);
  auto ndc = getProjectionStack().current() * getViewStack().current()
    * transform * glm::vec4(placardPos, 1.f);
  ndc /= ndc.w;
  auto resolution = Renderer::get()->getResolution();
  auto coord = (glm::vec2(ndc.x, -ndc.y) / 2.f + 0.5f) * resolution;
  auto actor = (const Actor *)e;
  const auto ui_info = actor->getUIInfo();

  // Cap status
  if (ui_info.capture[1]) {
    float capFact = glm::max(ui_info.capture[0] / ui_info.capture[1], 0.f);
    glm::vec2 size = vec2Param("hud.actor_cap.dim");
    glm::vec2 pos = coord - vec2Param("hud.actor_cap.pos");
    // Black underneath
    drawRectCenter(pos, size, glm::vec4(0, 0, 0, 1));
    pos.x -= size.x * (1.f - capFact) / 2.f;
    size.x *= capFact;
    const glm::vec4 cap_color = vec4Param("hud.actor_cap.color");
    drawRectCenter(pos, size, cap_color);
  }

  if (!ui_info.healths.empty()) {
    // Display the health bar
    // Health bar flashes white on red (instead of green on red) when it has
    // recently taken damage.
    glm::vec4 healthBarColor = vec4Param("hud.actor_health.color");
    float timeSinceDamage = Clock::secondsSince(actor->getLastTookDamage());
    // TODO flash the specific bar, not the entire thingy
    if (timeSinceDamage < fltParam("hud.actor_health.flash_duration")) {
      healthBarColor = vec4Param("hud.actor_health.flash_color");
    }
    float total_health = 0.f;
    for (auto h : ui_info.healths) {
      total_health += h[1];
    }

    glm::vec2 size = vec2Param("hud.actor_health.dim");
    glm::vec2 bottom_left = coord - vec2Param("hud.actor_health.pos") - size / 2.f;
    float s = 0.f;
    bool first = true;
    for (auto h : ui_info.healths) {
      float health = h[0];
      float max_health = h[1];
      float bar_size = max_health / total_health;
      glm::vec2 p = bottom_left + glm::vec2(size.x * s / total_health, 0.f);

      glm::vec2 total_size = glm::vec2(bar_size * size.x, size.y);
      glm::vec2 total_center = p + total_size / 2.f;

      glm::vec2 cur_size = glm::vec2(
        total_size.x * health / max_health,
        size.y);
      glm::vec2 cur_center = p + cur_size / 2.f;

      s += max_health;

      // Red underneath for max health
      glm::vec4 bgcolor = health > 0
        ? vec4Param("hud.actor_health.bg_color")
        : vec4Param("hud.actor_health.disabled_bg_color");
      drawRectCenter(total_center, total_size, bgcolor);
      // Green on top for current health
      drawRectCenter(cur_center, cur_size, healthBarColor);
      if (!first) {
        // Draw separator
        drawLine(
          glm::vec2(p.x, p.y),
          glm::vec2(p.x, p.y + size.y),
          glm::vec4(0, 0, 0, 1));
      }
      first = false;
    }
  }

  if (ui_info.mana[1]) {
    // Display the mana bar
    glm::vec4 manaBarColor = vec4Param("hud.actor_mana.color");

    float manaFact = glm::max(
        0.f,
        ui_info.mana[0] / ui_info.mana[1]);
    glm::vec2 size = vec2Param("hud.actor_mana.dim");
    glm::vec2 pos = coord - vec2Param("hud.actor_mana.pos");
    // black underneath for max mana
    drawRectCenter(pos, size, glm::vec4(0, 0, 0, 1));
    // mana color on top
    pos.x -= size.x * (1.f - manaFact) / 2.f;
    size.x *= manaFact;
    drawRectCenter(pos, size, manaBarColor);
  }

  glEnable(GL_DEPTH_TEST);
  // Render path if selected
  if (localPlayer->isSelected(actor->getID())) {
    auto pathQueue = actor->getPathQueue();
    // TODO(zack): remove z hack
    const auto zhack = glm::vec3(0, 0, 0.05f);
    auto prev = pos + zhack;
    for (auto target : pathQueue) {
      target += zhack;
      renderLineColor(prev, target, glm::vec4(1.f));
      prev = target;
    }
  }
}

void GameController::updateMapShader(Shader *shader) const {
  auto visibilityMap = Game::get()->getVisibilityMap(player_->getPlayerID());

  visibilityMap->fillTexture(visTex_);

  shader->uniform1i("texture", 0);

  glActiveTexture(GL_TEXTURE0);
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, visTex_);
}

void GameController::setEntityHotkey(id_t eid, char hotkey) {
  invariant(
    hotkey == '`' || hotkey - '0' >= 0 && hotkey - '0' <= 9,
    "invalid hotkey character");

  auto entity = Game::get()->getEntity(eid);
  if (entity && entity->getPlayerID() == player_->getPlayerID()) {
    std::set<id_t> newsel;
    newsel.insert(eid);
    savedSelection_[hotkey] = newsel;
  }
}

void GameController::handleUIAction(const UIAction &action) {
  if (action.state != UIAction::ENABLED) {
    return;
  }
  if (action.targeting == UIAction::TargetingType::NONE) {
    PlayerAction player_action;
    player_action["type"] = ActionTypes::ORDER;
    Json::Value order;
    order["type"] = OrderTypes::ACTION;
    std::set<id_t> ids(&action.owner, (&action.owner)+1);
    order["entity"] = toJson(ids);
    order["action"] = action.name;
    player_action["order"] = order;
    Game::get()->addAction(player_->getPlayerID(), player_action);
    // No extra params
  } else {
    action_ = action;
  }
}
};  // rts
