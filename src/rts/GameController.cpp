#define GLM_SWIZZLE_XYZW
#include "rts/GameController.h"
#include <sstream>
#include <glm/gtx/norm.hpp>
#include "common/util.h"
#include "common/ParamReader.h"
#include "rts/Building.h"
#include "rts/CommandWidget.h"
#include "rts/Game.h"
#include "rts/Map.h"
#include "rts/Matchmaker.h"
#include "rts/MessageHub.h"
#include "rts/MinimapWidget.h"
#include "rts/Player.h"
#include "rts/PlayerAction.h"
#include "rts/Renderer.h"
#include "rts/UI.h"
#include "rts/Widgets.h"

namespace rts {

namespace PlayerState {
const std::string DEFAULT = "DEFAULT";
const std::string CHATTING = "CHATTING";
}

static void renderHighlights(
    std::vector<MapHighlight> &highlights,
    std::map<id_t, float> &entityHighlights,
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
    renderCircleColor(transform, glm::vec4(1, 0, 0, 1));
  }
  // Remove done highlights
  for (size_t i = 0; i < highlights.size(); ) {
    if (highlights[i].remaining <= 0.f) {
      std::swap(highlights[i], highlights[highlights.size() - 1]);
      highlights.pop_back();
    } else {
      i++;
    }
  }

  auto it = entityHighlights.begin();
  while (it != entityHighlights.end()) {
    auto cur = (entityHighlights[it->first] -= dt);
    if (cur <= 0.f) {
      it = entityHighlights.erase(it);
    } else {
      it++;
    }
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

GameController::GameController(LocalPlayer *player)
  : player_(player),
    shift_(false),
    ctrl_(false),
    alt_(false),
    leftDrag_(false),
    leftDragMinimap_(false),
    state_(PlayerState::DEFAULT),
    zoom_(0.f),
    order_() {
}

GameController::~GameController() {
}

void GameController::initWidgets() {
  createWidgets("ui.widgets");

  auto minimapWidget =
      new MinimapWidget("ui.widgets.minimap", player_->getPlayerID());
  UI::get()->addWidget("ui.widgets.minimap", minimapWidget);
  minimapWidget->setClickable(minimapWidget->getCenter(), minimapWidget->getSize());
  minimapWidget->setOnClickListener(
    [&](const glm::vec2 &pos) -> bool {
      leftDragMinimap_ = true;
      return true;
    });

  auto chatWidget = new CommandWidget("ui.chat");
  chatWidget->setCloseOnSubmit(true);
  chatWidget->setOnTextSubmittedHandler([&, chatWidget](const std::string &t) {
    Message msg;
    msg["pid"] = toJson(player_->getPlayerID());
    msg["type"] = ActionTypes::CHAT;
    msg["chat"] = t;
    MessageHub::get()->addAction(msg);
  });
  UI::get()->addWidget("ui.widgets.chat", chatWidget);
  UI::get()->addWidget("ui.widgets.highlights",
    createCustomWidget(std::bind(
        renderHighlights,
        std::ref(highlights_),
        std::ref(entityHighlights_),
        std::placeholders::_1)));
  UI::get()->addWidget("ui.widgets.dragRect",
    createCustomWidget(std::bind(
      renderDragRect,
      std::ref(leftDrag_),
      std::ref(leftStart_),
      std::ref(lastMousePos_),
      std::placeholders::_1)));

  Game::get()->setChatListener([&](const Message &m) {
    const Player* from = Game::get()->getPlayer(toID(m["pid"]));
    invariant(from, "No playyayayaya");
    std::stringstream ss;
    ss << from->getName() << ": " << m["chat"].asString();
    LOG(DEBUG) << " message: " << m << '\n';
    ((CommandWidget *)UI::get()->getWidget("ui.widgets.chat"))
      ->addMessage(ss.str())
      // TODO(zxack): param
      ->show(1.f);
  });
}

void GameController::clearWidgets() {
  UI::get()->clearWidgets();
}

void GameController::renderUpdate(float dt) {
  // No input while game is paused, not even camera motion
  if (Game::get()->isPaused()) {
    return;
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
  action["pid"] = toJson(player_->getPlayerID());
  MessageHub::get()->addAction(action);
}

void GameController::mouseDown(const glm::vec2 &screenCoord, int button) {
  if (alt_) {
    return;
  }
  PlayerAction action;
  std::set<id_t> newSelect = player_->getSelection();

  glm::vec3 loc = Renderer::get()->screenToTerrain(screenCoord);
  // The entity (maybe) under the cursor
  id_t eid = selectEntity(screenCoord);
  const GameEntity *entity = Game::get()->getEntity(eid);

  if (button == SDL_BUTTON_LEFT) {
    leftStart_ = screenCoord;
    /*
    glm::vec2 minimapPos = UI::convertUIPos(vec2Param("ui.minimap.pos"));
    glm::vec2 minimapDim = vec2Param("ui.minimap.dim");
    // TODO(connor) perhaps clean this up with some sort of click state?
    if (screenCoord.x >= minimapPos.x &&
        screenCoord.x <= minimapPos.x + minimapDim.x &&
        screenCoord.y >= minimapPos.y &&
        screenCoord.y <= minimapPos.y + minimapDim.y) {
      leftDragMinimap_ = true;
    } else {
    */
      leftDrag_ = true;
      // If no order, then adjust selection
      if (order_.empty()) {
        // If no shift held, then deselect (shift just adds)
        if (!shift_) {
          newSelect.clear();
        }
        // If there is an entity and its ours, select
        if (entity && entity->getPlayerID() == player_->getPlayerID()) {
          newSelect.insert(eid);
        }
      } else {
        // If order, then execute it
        action["type"] = order_;
        action["entity"] = toJson(player_->getSelection());
        action["target"] = toJson(loc);
        if (entity && entity->getTeamID() != player_->getTeamID()) {
          action["enemy_id"] = toJson(entity->getID());
        }
        action["pid"] = toJson(player_->getPlayerID());

        // Clear order
        order_.clear();
      }
    //}
  } else if (button == SDL_BUTTON_RIGHT) {
    // TODO(connor) make right click actions on minimap
    // If there is an order, it is canceled by right click
    if (!order_.empty()) {
      order_.clear();
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
          action["type"] = ActionTypes::CAPTURE;
        } else {
          action["type"] = ActionTypes::ATTACK;
        }
        action["entity"] = toJson(player_->getSelection());
        action["enemy_id"] = toJson(eid);
        action["pid"] = toJson(player_->getPlayerID());
      // If we have a selection, and they didn't click on the current
      // selection, move them to target
      } else if (!player_->getSelection().empty()
          && (!entity || !player_->getSelection().count(eid))) {
				//loc.x/y wil be -/+HUGE_VAL if outside map bounds
        if (loc.x != HUGE_VAL && loc.y != HUGE_VAL && loc.x != -HUGE_VAL && loc.y != -HUGE_VAL) {
          // Visual feedback
          highlight(glm::vec2(loc.x, loc.y));

          // Queue up action
          action["type"] = ActionTypes::MOVE;
          action["entity"] = toJson(player_->getSelection());
          action["target"] = toJson(loc);
          action["pid"] = toJson(player_->getPlayerID());
        }
      }
    }
  } else if (button == SDL_BUTTON_WHEELUP) {
    Renderer::get()->zoomCamera(-fltParam("local.mouseZoomSpeed"));
  } else if (button == SDL_BUTTON_WHEELDOWN) {
    Renderer::get()->zoomCamera(fltParam("local.mouseZoomSpeed"));
  }

  if (Game::get()->isPaused()) {
    return;
  }

  // Mutate, if game isn't paused
  player_->setSelection(newSelect);
  if (action.isMember("type")) {
    MessageHub::get()->addAction(action);
  }
}

void GameController::mouseUp(const glm::vec2 &screenCoord, int button) {
  if (button == SDL_BUTTON_LEFT) {
    std::set<id_t> newSelect;
    if (glm::distance(leftStart_, screenCoord) > fltParam("hud.minDragDistance")) {
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
  static const SDLKey MAIN_KEYS[] = {SDLK_q, SDLK_w, SDLK_e, SDLK_r};
  SDLKey key = keysym.sym;
  // TODO(zack) watch out for pausing here
  PlayerAction action;
  // Actions available in all player states:
  if (key == SDLK_F10) {
    action["type"] = ActionTypes::LEAVE_GAME;
    action["pid"] = toJson(player_->getPlayerID());
    MessageHub::get()->addAction(action);
  // Camera panning
  } else if (key == SDLK_UP) {
    if (alt_) {
      zoom_ = -fltParam("local.keyZoomSpeed");
    } else {
      cameraPanDir_.y = 1.f;
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
      ((CommandWidget *)UI::get()->getWidget("ui.widgets.chat"))
        ->captureText();
    } else if (key == SDLK_ESCAPE) {
      // ESC clears out current states
      if (!order_.empty()) {
        order_.clear();
      } else {
        player_->setSelection(std::set<id_t>());
      }
    } else if (key == SDLK_LSHIFT || key == SDLK_RSHIFT) {
      shift_ = true;
    } else if (key == SDLK_LCTRL || key == SDLK_RCTRL) {
      ctrl_ = true;
    } else if (key == SDLK_LALT || key == SDLK_RALT) {
      alt_ = true;
    } else if (key == SDLK_BACKSPACE) {
      Renderer::get()->resetCameraRotation();
    } else if (key == SDLK_g) {
      // Debug commands
      SDL_WM_GrabInput(SDL_GRAB_ON);
    } else if (!player_->getSelection().empty()) {
      // Handle unit commands
      // Order types
      if (key == SDLK_a) {
        order_ = ActionTypes::ATTACK;
      } else if (key == SDLK_m) {
        order_ = ActionTypes::MOVE;
      } else if (key == SDLK_s) {
        action["type"] = ActionTypes::STOP;
        action["entity"] = toJson(player_->getSelection());
        action["pid"] = toJson(player_->getPlayerID());
        MessageHub::get()->addAction(action);
      } else {
        for (unsigned int i = 0; i < 4; i++) {
          if (key == MAIN_KEYS[i]) {
            // TODO(zack): this assumption that head of selection is always
            // the unit we're building on is not good
            auto sel = player_->getSelection().begin();
            const GameEntity *ent = Game::get()->getEntity(*sel);
            // The main action of a building is production
            if (ent->getType() == "BUILDING") {
              std::vector<std::string> prod = arrParam(ent->getName() +
                  ".prod");
              if (i < prod.size()) {
                std::string prodName = prod[i];
                float cost = fltParam(prodName + ".cost.requisition");
                auto &resources = Game::get()
                    ->getResources(player_->getPlayerID());

                if (cost <= resources.requisition) {
                  action["type"] = ActionTypes::ENQUEUE;
                  action["entity"] = toJson(*sel);
                  action["pid"] = toJson(player_->getPlayerID());
                  action["prod"] = prod[i];
                  MessageHub::get()->addAction(action);
                }
                // TODO(zack): else send a message telling the player they
                // don't have enough of resource X (that annoying voice...)
              }
            }
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
  // TODO(zack): fix this up
  const glm::vec2 minimapPos = UI::convertUIPos(vec2Param("ui.widgets.minimap.pos"));
  const glm::vec2 minimapDim = vec2Param("ui.widgets.minimap.dim");
  glm::vec2 mapCoord = screenCoord;
  mapCoord -= minimapPos;
  mapCoord -= glm::vec2(minimapDim.x / 2, minimapDim.y / 2);
  mapCoord *= Renderer::get()->getMapSize() / minimapDim;
  mapCoord.y *= -1;
  glm::vec3 finalPos(mapCoord, Renderer::get()->getCameraPos().z);
  Renderer::get()->setCameraLookAt(finalPos);
}

id_t GameController::selectEntity(const glm::vec2 &screenCoord) const {
  glm::vec2 pos = glm::vec2(Renderer::get()->screenToTerrain(screenCoord));
  id_t eid = NO_ENTITY;
  float bestDist = HUGE_VAL;
  // Find the best entity
  for (const auto& pair : Renderer::get()->getEntities()) {
    auto entity = pair.second;
    if (!entity->hasProperty(GameEntity::P_ACTOR)) {
      continue;
    }
    auto gameEntity = (GameEntity *) entity;
    auto rect = gameEntity->getRect(Renderer::get()->getSimDT());
    float dist = glm::distance2(pos, rect.pos);
    if (rect.contains(pos) && dist < bestDist) {
      bestDist = dist;
      eid = pair.first;
    }
  }

  return eid;
}

std::set<id_t> GameController::selectEntities(
  const glm::vec2 &start, const glm::vec2 &end, id_t pid) const {
  assertPid(pid);
  std::set<id_t> ret;

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

  for (const auto &pair : Renderer::get()->getEntities()) {
    auto e = pair.second;
    // Must be an actor owned by the passed player
    if (!e->hasProperty(GameEntity::P_ACTOR)) {
      continue;
    }
    auto ge = (GameEntity *) e;
    if (ge->getPlayerID() == pid
        && boxInBox(dragRect, ge->getRect(Renderer::get()->getSimDT()))) {
      ret.insert(ge->getID());
    }
  }

  return ret;
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


};  // rts
