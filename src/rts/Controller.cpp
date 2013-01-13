#define GLM_SWIZZLE_XYZW
#include "rts/Controller.h"
#include <glm/gtx/norm.hpp>
#include "common/util.h"
#include "common/ParamReader.h"
#include "rts/Building.h"
#include "rts/Game.h"
#include "rts/MessageHub.h"
#include "rts/Player.h"
#include "rts/PlayerAction.h"
#include "rts/Renderer.h"
#include "rts/UI.h"

namespace rts {

namespace PlayerState {
const std::string DEFAULT = "DEFAULT";
const std::string CHATTING = "CHATTING";
}

Controller::Controller(LocalPlayer *player)
  : player_(player),
    shift_(false),
    ctrl_(false),
    alt_(false),
    leftDrag_(false),
    leftDragMinimap_(false),
    message_(),
    state_(PlayerState::DEFAULT),
    zoom_(0.f),
    order_() {
}

Controller::~Controller() {
}

void Controller::processInput(float dt) {
  glm::vec2 screenCoord;
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    switch (event.type) {
    case SDL_KEYDOWN:
      keyPress(event.key.keysym);
      break;
    case SDL_KEYUP:
      keyRelease(event.key.keysym);
      break;
    case SDL_MOUSEBUTTONDOWN:
      screenCoord = glm::vec2(event.button.x, event.button.y);
      mouseDown(screenCoord, event.button.button);
      break;
    case SDL_MOUSEBUTTONUP:
      screenCoord = glm::vec2(event.button.x, event.button.y);
      mouseUp(screenCoord, event.button.button);
      break;
    case SDL_MOUSEMOTION:
      screenCoord = glm::vec2(event.button.x, event.button.y);
      mouseMotion(screenCoord);
      break;
    // TODO(zack) add MOUSE_WHEEL
    case SDL_QUIT:
      quitEvent();
      break;
    }
  }
  renderUpdate(dt);
}

void Controller::renderUpdate(float dt) {
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

glm::vec4 Controller::getDragRect() const {
  float dist = glm::distance(leftStart_, lastMousePos_);
  if (leftDrag_ && dist > fltParam("hud.minDragDistance")) {
    return glm::vec4(leftStart_, lastMousePos_);
  }
  return glm::vec4(-HUGE_VAL);
}

void Controller::quitEvent() {
  // Send the quit game event
  PlayerAction action;
  action["type"] = ActionTypes::LEAVE_GAME;
  action["pid"] = toJson(player_->getPlayerID());
  MessageHub::get()->addAction(action);
}

void Controller::mouseDown(const glm::vec2 &screenCoord, int button) {
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
    glm::vec2 minimapPos = UI::convertUIPos(vec2Param("ui.minimap.pos"));
    glm::vec2 minimapDim = vec2Param("ui.minimap.dim");
    // TODO(connor) perhaps clean this up with some sort of click state?
    if (screenCoord.x >= minimapPos.x &&
        screenCoord.x <= minimapPos.x + minimapDim.x &&
        screenCoord.y >= minimapPos.y &&
        screenCoord.y <= minimapPos.y + minimapDim.y) {
      leftDragMinimap_ = true;
    } else {
      leftDrag_ = true;
      leftStart_ = screenCoord;
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
    }
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
        Renderer::get()->getUI()->highlightEntity(entity->getID());

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
          Renderer::get()->getUI()->highlight(glm::vec2(loc.x, loc.y));

          // Queue up action
          action["type"] = ActionTypes::MOVE;
          action["entity"] = toJson(player_->getSelection());
          action["target"] = toJson(loc);
          action["pid"] = toJson(player_->getPlayerID());
        }
      }
    }
  }

  if (Game::get()->isPaused()) {
    return;
  }

  // Mutate, if game isn't paused
  player_->setSelection(newSelect);
  if (action.isMember("type")) {
    MessageHub::get()->addAction(action);
  }
      // If we have a selection, and they didn't click on the current
      // selection, move them to target
}

void Controller::mouseUp(const glm::vec2 &screenCoord, int button) {
  if (button == SDL_BUTTON_LEFT) {
    glm::vec3 loc = Renderer::get()->screenToTerrain(screenCoord);
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

void Controller::mouseMotion(const glm::vec2 &screenCoord) {
  // Rotate camera while alt is held
  if (alt_) {
    glm::vec2 delta = screenCoord - lastMousePos_;
    delta *= fltParam("local.cameraRotSpeed");
    Renderer::get()->rotateCamera(delta);
  }
  lastMousePos_ = screenCoord;
}

void Controller::keyPress(SDL_keysym keysym) {
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
      zoom_ = -fltParam("local.zoomSpeed");
    } else {
      cameraPanDir_.y = 1.f;
    }
  } else if (key == SDLK_DOWN) {
    if (alt_) {
      zoom_ = fltParam("local.zoomSpeed");
    } else {
      cameraPanDir_.y = -1.f;
    }
  } else if (key == SDLK_RIGHT) {
    cameraPanDir_.x = 1.f;
  } else if (key == SDLK_LEFT) {
    cameraPanDir_.x = -1.f;
  } else if (state_ == PlayerState::DEFAULT) {
    if (key == SDLK_RETURN) {
      state_ = PlayerState::CHATTING;
      Renderer::get()->getUI()->setChatActive(true);
      Renderer::get()->getUI()->setChatBuffer(message_);
      SDL_EnableUNICODE(SDL_ENABLE);
      SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY,
          SDL_DEFAULT_REPEAT_INTERVAL);
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
  } else if (state_ == PlayerState::CHATTING) {
    char unicode = keysym.unicode;
    if (key == SDLK_RETURN) {
      SDL_EnableUNICODE(SDL_DISABLE);
      SDL_EnableKeyRepeat(0, SDL_DEFAULT_REPEAT_INTERVAL);
      state_ = PlayerState::DEFAULT;
      if (!message_.empty()) {
        Message msg;
        action["pid"] = toJson(player_->getPlayerID());
        action["type"] = ActionTypes::CHAT;
        action["chat"] = message_;
        MessageHub::get()->addAction(action);
      }
      message_.clear();
      Renderer::get()->getUI()->setChatActive(false);
    } else if (key == SDLK_ESCAPE) {
      // Clears message and exits chat mode.
      SDL_EnableUNICODE(SDL_DISABLE);
      SDL_EnableKeyRepeat(0, SDL_DEFAULT_REPEAT_INTERVAL);
      state_ = PlayerState::DEFAULT;
      message_.clear();
      Renderer::get()->getUI()->setChatActive(false);
    } else if (key == SDLK_BACKSPACE) {
      if (!message_.empty())
        message_.erase(message_.end() - 1);
      Renderer::get()->getUI()->setChatBuffer(message_);
    } else if (unicode != 0 &&
        (keysym.mod == KMOD_NONE   ||
         keysym.mod == KMOD_LSHIFT ||
         keysym.mod == KMOD_RSHIFT ||
         keysym.mod == KMOD_CAPS   ||
         keysym.mod == KMOD_NUM)) {
      message_.append(1, unicode);
      Renderer::get()->getUI()->setChatBuffer(message_);
    }
  }
}

void Controller::keyRelease(SDL_keysym keysym) {
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

void Controller::minimapUpdateCamera(const glm::vec2 &screenCoord) {
  const glm::vec2 minimapPos = UI::convertUIPos(vec2Param("ui.minimap.pos"));
  const glm::vec2 minimapDim = vec2Param("ui.minimap.dim");
  glm::vec2 mapCoord = screenCoord;
  mapCoord -= minimapPos;
  mapCoord -= glm::vec2(minimapDim.x / 2, minimapDim.y / 2);
  mapCoord *= Renderer::get()->getMapSize() / minimapDim;
  mapCoord.y *= -1;
  glm::vec3 finalPos(mapCoord, Renderer::get()->getCameraPos().z);
  Renderer::get()->setCameraLookAt(finalPos);
}

id_t Controller::selectEntity(const glm::vec2 &screenCoord) const {
  glm::vec2 pos = glm::vec2(Renderer::get()->screenToTerrain(screenCoord));
  id_t eid = NO_ENTITY;
  float bestDist = HUGE_VAL;
  // Find the best entity
  for (const auto& pair : Game::get()->getEntities()) {
    auto entity = pair.second;
    if (!entity->hasProperty(GameEntity::P_ACTOR)) {
      continue;
    }
    auto rect = entity->getRect(Renderer::get()->getSimDT());
    float dist = glm::distance2(pos, rect.pos);
    if (rect.contains(pos) && dist < bestDist) {
      bestDist = dist;
      eid = entity->getID();
    }
  }

  return eid;
}

std::set<id_t> Controller::selectEntities(
  const glm::vec2 &start, const glm::vec2 &end, id_t pid) const {
  assertPid(pid);
  std::set<id_t> ret;

  glm::vec2 terrainStart = Renderer::get()->screenToTerrain(start).xy;
  glm::vec2 terrainEnd = Renderer::get()->screenToTerrain(end).xy;
  // Defines the rectangle
  glm::vec2 center = (terrainStart + terrainEnd) / 2.f;
  glm::vec2 size = glm::abs(terrainEnd - terrainStart);
  Rect dragRect(center, size, 0.f);

  for (const auto &pair : Game::get()->getEntities()) {
    auto e = pair.second;
    // Must be an actor owned by the passed player
    if (!e->hasProperty(GameEntity::P_ACTOR) || e->getPlayerID() != pid) {
      continue;
    }
    if (boxInBox(dragRect, e->getRect(Renderer::get()->getSimDT()))) {
      ret.insert(e->getID());
    }
  }

  return ret;
}
};  // rts
