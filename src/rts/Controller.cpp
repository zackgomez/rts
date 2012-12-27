#include "rts/Controller.h"
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
    leftDrag_(false),
    leftDragMinimap_(false),
    message_(),
    state_(PlayerState::DEFAULT),
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
    case SDL_QUIT:
      quitEvent();
      break;
    }
  }
  renderUpdate(dt);
}

void Controller::renderUpdate(float dt) {
  // No input while game is paused, not even panning
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

  const float CAMERA_PAN_SPEED = fltParam("camera.panspeed");
  glm::vec3 delta = CAMERA_PAN_SPEED * dt * glm::vec3(dir.x, dir.y, 0.f);
  Renderer::get()->updateCamera(delta);

  // Deselect dead entities
  std::set<id_t> newsel;
  for (auto eid : selection_) {
    assertEid(eid);
    const Entity *e = Game::get()->getEntity(eid);
    if (e && e->getPlayerID() == player_->getPlayerID()) {
      newsel.insert(eid);
    }
  }
  setSelection(newsel);

  // Draw drag rectangle if over threshold size
  glm::vec3 loc = Renderer::get()->screenToTerrain(screenCoord);
  if (leftDrag_
      && glm::distance(loc, leftStart_) > fltParam("hud.minDragDistance")) {
    Renderer::get()->setDragRect(leftStart_, loc);
  } else if (leftDragMinimap_) {
    Renderer::get()->minimapUpdateCamera(screenCoord);
  }
}

void Controller::quitEvent() {
  // Send the quit game event
  PlayerAction action;
  action["type"] = ActionTypes::LEAVE_GAME;
  action["pid"] = toJson(player_->getPlayerID());
  MessageHub::get()->addAction(action);
}

void Controller::mouseDown(const glm::vec2 &screenCoord, int button) {
  PlayerAction action;
  std::set<id_t> newSelect = selection_;

  glm::vec3 loc = Renderer::get()->screenToTerrain(screenCoord);
  // The entity (maybe) under the cursor
  id_t eid = Renderer::get()->selectEntity(screenCoord);
  const Entity *entity = Game::get()->getEntity(eid);

  if (button == SDL_BUTTON_LEFT) {
    glm::vec2 minimapPos = Renderer::get()
        ->convertUIPos(vec2Param("ui.minimap.pos"));
    glm::vec2 minimapDim = vec2Param("ui.minimap.dim");
    // TODO(connor) perhaps clean this up with some sort of click state?
    if (screenCoord.x >= minimapPos.x &&
        screenCoord.x <= minimapPos.x + minimapDim.x &&
        screenCoord.y >= minimapPos.y &&
        screenCoord.y <= minimapPos.y + minimapDim.y) {
      leftDragMinimap_ = true;
    } else {
      leftDrag_ = true;
      leftStart_ = loc;
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
        action["entity"] = toJson(selection_);
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
          && !selection_.empty()) {
        // Visual feedback
        // TODO(zack) highlight the target not the ground
        Renderer::get()->highlight(glm::vec2(loc.x, loc.y));

        // Queue up action
        if (entity->getType() == Building::TYPE) {
          action["type"] = ((Building*)entity)->isCappable() ?
            ActionTypes::CAPTURE : ActionTypes::ATTACK;
        } else {
          action["type"] = ActionTypes::ATTACK;
        }
        action["entity"] = toJson(selection_);
        action["enemy_id"] = toJson(eid);
        action["pid"] = toJson(player_->getPlayerID());
      // If we have a selection, and they didn't click on the current
      // selection, move them to target
      } else if (!selection_.empty() && (!entity || !selection_.count(eid))) {
        if (loc.x != HUGE_VAL && loc.y != HUGE_VAL) {
          // Visual feedback
          Renderer::get()->highlight(glm::vec2(loc.x, loc.y));

          // Queue up action
          action["type"] = ActionTypes::MOVE;
          action["entity"] = toJson(selection_);
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
  setSelection(newSelect);
  if (action.isMember("type")) {
    MessageHub::get()->addAction(action);
  }
      // If we have a selection, and they didn't click on the current
      // selection, move them to target
}

void Controller::mouseUp(const glm::vec2 &screenCoord, int button) {
  glm::vec3 loc = Renderer::get()->screenToTerrain(screenCoord);
  if (button == SDL_BUTTON_LEFT) {
    std::set<id_t> newSelect;
    if (glm::distance(leftStart_, loc) > fltParam("hud.minDragDistance")) {
      newSelect = Renderer::get()
          ->selectEntities(leftStart_, loc, player_->getPlayerID());

      if (!shift_) {
        selection_.clear();
      }
      selection_.insert(newSelect.begin(), newSelect.end());
      setSelection(selection_);
    }

    leftDrag_ = false;
    leftDragMinimap_ = false;
  }
  // nop
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
    cameraPanDir_.y = 1.f;
  } else if (key == SDLK_DOWN) {
    cameraPanDir_.y = -1.f;
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
        setSelection(std::set<id_t>());
      }
    } else if (key == SDLK_LSHIFT || key == SDLK_RSHIFT) {
      shift_ = true;
    } else if (key == SDLK_g) {
      // Debug commands
      SDL_WM_GrabInput(SDL_GRAB_ON);
    } else if (!selection_.empty()) {
      // Handle unit commands
      // Order types
      if (key == SDLK_a) {
        order_ = ActionTypes::ATTACK;
      } else if (key == SDLK_m) {
        order_ = ActionTypes::MOVE;
      } else if (key == SDLK_s) {
        action["type"] = ActionTypes::STOP;
        action["entity"] = toJson(selection_);
        action["pid"] = toJson(player_->getPlayerID());
        MessageHub::get()->addAction(action);
      } else {
        for (unsigned int i = 0; i < 4; i++) {
          if (key == MAIN_KEYS[i]) {
            // TODO(zack): this assumption that head of selection is always
            // the unit we're building on is not good
            auto sel = selection_.begin();
            const Entity *ent = Game::get()->getEntity(*sel);
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
  } else if (key == SDLK_LSHIFT || key == SDLK_RSHIFT) {
    shift_ = false;
  }
}

void Controller::setSelection(const std::set<id_t> &s) {
  selection_ = s;
  Renderer::get()->setSelection(selection_);
}
};  // rts
