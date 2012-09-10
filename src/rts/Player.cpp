#include "rts/Player.h"
#include <SDL/SDL.h>
#include "common/Checksum.h"
#include "common/ParamReader.h"
#include "rts/Building.h"
#include "rts/Entity.h"
#include "rts/Game.h"
#include "rts/MessageHub.h"
#include "rts/Renderer.h"

namespace rts {

LocalPlayer::LocalPlayer(id_t playerID, id_t teamID, const std::string &name,
    const glm::vec3 &color, Renderer *renderer)
  : Player(playerID, teamID, name, color),
    renderer_(renderer),
    doneTick_(-1e6),  // no done ticks
    selection_(),
    cameraPanDir_(0.f),
    shift_(false),
    leftDrag_(false),
    leftDragMinimap_(false),
    message_(),
    state_(PlayerState::DEFAULT),
    order_() {
  logger_ = Logger::getLogger("LocalPlayer");
}

LocalPlayer::~LocalPlayer() {
}

bool LocalPlayer::update(tick_t tick, tick_t targetTick) {
  // Don't do anything if we're already done with this frame
  // NOTE don't use targetTick here as it has the wrong sync behavior
  if (tick <= doneTick_) {
    return true;
  }

  // Finish the last frame by sending the DONE action
  Json::Value checksum;
  checksum.append(toJson(tick));
  checksum.append(Checksum::checksumToString(game_->getChecksum()));
  PlayerAction a;
  a["type"] = ActionTypes::DONE;
  a["pid"] = toJson(playerID_);
  a["tick"] = toJson(tick);
  a["checksum"] = checksum;
  game_->addAction(playerID_, a);

  // We've generated for this tick
  doneTick_ = tick;

  // We've completed input for the previous target frame
  return true;
}

void LocalPlayer::playerAction(id_t playerID, const PlayerAction &action) {
  // nop, mostly used by network players
}

void LocalPlayer::renderUpdate(float dt) {
  // No input while game is paused, not even panning
  if (game_->isPaused()) {
    return;
  }

  int x, y;
  SDL_GetMouseState(&x, &y);
  glm::vec2 screenCoord(x, y);
  const glm::vec2 &res = renderer_->getResolution();
  const int CAMERA_PAN_THRESHOLD = fltParam("camera.panthresh");

  // Mouse overrides keyboard
  glm::vec2 dir = cameraPanDir_;

  if (x <= CAMERA_PAN_THRESHOLD) {
    dir.x = 2 * (x - CAMERA_PAN_THRESHOLD) / CAMERA_PAN_THRESHOLD;
  } else if (x > res.x - CAMERA_PAN_THRESHOLD) {
    dir.x = -2 * ((res.x - x) - CAMERA_PAN_THRESHOLD) / CAMERA_PAN_THRESHOLD;
  }
  if (y <= CAMERA_PAN_THRESHOLD) {
    dir.y = -2 * (y - CAMERA_PAN_THRESHOLD) / CAMERA_PAN_THRESHOLD;
  } else if (y > res.y - CAMERA_PAN_THRESHOLD) {
    dir.y = 2 * ((res.y - y) - CAMERA_PAN_THRESHOLD) / CAMERA_PAN_THRESHOLD;
  }

  const float CAMERA_PAN_SPEED = fltParam("camera.panspeed");
  glm::vec3 delta = CAMERA_PAN_SPEED * dt * glm::vec3(dir.x, dir.y, 0.f);
  renderer_->updateCamera(delta);

  // Deselect dead entities
  std::set<id_t> newsel;
  for (auto eid : selection_) {
    assertEid(eid);
    const Entity *e = game_->getEntity(eid);
    if (e && e->getPlayerID() == playerID_) {
      newsel.insert(eid);
    }
  }
  setSelection(newsel);

  // Draw drag rectangle if over threshold size
  glm::vec3 loc = renderer_->screenToTerrain(screenCoord);
  if (leftDrag_
      && glm::distance(loc, leftStart_) > fltParam("hud.minDragDistance")) {
    renderer_->setDragRect(leftStart_, loc);
  } else if (leftDragMinimap_) {
    renderer_->minimapUpdateCamera(screenCoord);
  }
}

void LocalPlayer::setGame(Game *game) {
  Player::setGame(game);
}

void LocalPlayer::quitEvent() {
  // Send the quit game event
  PlayerAction action;
  action["type"] = ActionTypes::LEAVE_GAME;
  action["pid"] = toJson(playerID_);
  game_->addAction(playerID_, action);
}

void LocalPlayer::mouseDown(const glm::vec2 &screenCoord, int button) {
  PlayerAction action;
  std::set<id_t> newSelect = selection_;

  glm::vec3 loc = renderer_->screenToTerrain(screenCoord);
  // The entity (maybe) under the cursor
  id_t eid = renderer_->selectEntity(screenCoord);
  const Entity *entity = game_->getEntity(eid);

  if (button == SDL_BUTTON_LEFT) {
    glm::vec2 minimapPos = renderer_->convertUIPos(vec2Param("ui.minimap.pos"));
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
        if (entity && entity->getPlayerID() == playerID_) {
          newSelect.insert(eid);
        }
      } else {
        // If order, then execute it
        action["type"] = order_;
        action["entity"] = toJson(selection_);
        action["target"] = toJson(loc);
        if (entity && entity->getTeamID() != teamID_) {
          action["enemy_id"] = toJson(entity->getID());
        }
        action["pid"] = toJson(playerID_);
        action["tick"] = toJson(doneTick_ + 1);

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
      if (entity && entity->getTeamID() != teamID_
          && !selection_.empty()) {
        // Visual feedback
        // TODO(zack) highlight the target not the ground
        renderer_->highlight(glm::vec2(loc.x, loc.y));

        // Queue up action
        if (entity->getType() == Building::TYPE) {
          action["type"] = ((Building*)entity)->isCappable() ?
            ActionTypes::CAPTURE : ActionTypes::ATTACK;
        } else {
          action["type"] = ActionTypes::ATTACK;
        }
        action["entity"] = toJson(selection_);
        action["enemy_id"] = toJson(eid);
        action["pid"] = toJson(playerID_);
        action["tick"] = toJson(doneTick_ + 1);
      // If we have a selection, and they didn't click on the current
      // selection, move them to target
      } else if (!selection_.empty() && (!entity || !selection_.count(eid))) {
        if (loc.x != HUGE_VAL && loc.y != HUGE_VAL) {
          // Visual feedback
          renderer_->highlight(glm::vec2(loc.x, loc.y));

          // Queue up action
          action["type"] = ActionTypes::MOVE;
          action["entity"] = toJson(selection_);
          action["target"] = toJson(loc);
          action["pid"] = toJson(playerID_);
          action["tick"] = toJson(doneTick_ + 1);
        }
      }
    }
  }

  if (game_->isPaused()) {
    return;
  }

  // Mutate, if game isn't paused
  setSelection(newSelect);
  if (action.isMember("type")) {
    game_->addAction(playerID_, action);
  }
      // If we have a selection, and they didn't click on the current
      // selection, move them to target
}

void LocalPlayer::mouseUp(const glm::vec2 &screenCoord, int button) {
  glm::vec3 loc = renderer_->screenToTerrain(screenCoord);
  if (button == SDL_BUTTON_LEFT) {
    std::set<id_t> newSelect;
    if (glm::distance(leftStart_, loc) > fltParam("hud.minDragDistance")) {
      newSelect = renderer_->selectEntities(leftStart_, loc, playerID_);

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

void LocalPlayer::keyPress(SDL_keysym keysym) {
  static const SDLKey MAIN_KEYS[] = {SDLK_q, SDLK_w, SDLK_e, SDLK_r};
  SDLKey key = keysym.sym;
  // TODO(zack) watch out for pausing here
  PlayerAction action;
  // Actions available in all player states:
  if (key == SDLK_F10) {
    action["type"] = ActionTypes::LEAVE_GAME;
    action["pid"] = toJson(playerID_);
    game_->addAction(playerID_, action);
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
        action["pid"] = toJson(playerID_);
        action["tick"] = toJson(doneTick_ + 1);
        game_->addAction(playerID_, action);
      } else {
        for (unsigned int i = 0; i < 4; i++) {
          if (key == MAIN_KEYS[i]) {
            // TODO(zack): this assumption that head of selection is always
            // the unit we're building on is not good
            auto sel = selection_.begin();
            const Entity *ent = game_->getEntity(*sel);
            // The main action of a building is production
            if (ent->getType() == "BUILDING") {
              std::vector<std::string> prod = arrParam(ent->getName() +
                  ".prod");
              if (i < prod.size()) {
                std::string prodName = prod[i];
                float cost = fltParam(prodName + ".cost.requisition");
                auto &resources = game_->getResources(playerID_);

                if (cost <= resources.requisition) {
                  action["type"] = ActionTypes::ENQUEUE;
                  action["entity"] = toJson(*sel);
                  action["pid"] = toJson(playerID_);
                  action["prod"] = prod[i];
                  action["tick"] = toJson(doneTick_ + 1);
                  game_->addAction(playerID_, action);
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
        action["pid"] = toJson(playerID_);
        action["type"] = ActionTypes::CHAT;
        action["chat"] = message_;
        action["tick"] = toJson(doneTick_ + 1);
        game_->addAction(playerID_, action);
      }
      message_.clear();
    } else if (key == SDLK_ESCAPE) {
      // Clears message and exits chat mode.
      SDL_EnableUNICODE(SDL_DISABLE);
      SDL_EnableKeyRepeat(0, SDL_DEFAULT_REPEAT_INTERVAL);
      state_ = PlayerState::DEFAULT;
      message_.clear();
    } else if (key == SDLK_BACKSPACE) {
      if (!message_.empty())
        message_.erase(message_.end() - 1);
    } else if (unicode != 0 &&
        (keysym.mod == KMOD_NONE   ||
         keysym.mod == KMOD_LSHIFT ||
         keysym.mod == KMOD_RSHIFT ||
         keysym.mod == KMOD_CAPS   ||
         keysym.mod == KMOD_NUM)) {
      message_.append(1, unicode);
    }
  }
}

void LocalPlayer::keyRelease(SDL_keysym keysym) {
  SDLKey key = keysym.sym;
  if (key == SDLK_RIGHT || key == SDLK_LEFT) {
    cameraPanDir_.x = 0.f;
  } else if (key == SDLK_UP || key == SDLK_DOWN) {
    cameraPanDir_.y = 0.f;
  } else if (key == SDLK_LSHIFT || key == SDLK_RSHIFT) {
    shift_ = false;
  }
}

void
LocalPlayer::setSelection(const std::set<id_t> &s) {
  selection_ = s;
  renderer_->setSelection(selection_);
}

DummyPlayer::DummyPlayer(id_t playerID, id_t teamID)
  : Player(playerID, teamID, "DummyPlayer", vec3Param("colors.dummyPlayer")) {
}

bool DummyPlayer::update(tick_t tick, tick_t targetTick) {
  Json::Value checksum;
  checksum.append(toJson(tick));
  checksum.append(Checksum::checksumToString(game_->getChecksum()));

  PlayerAction a;
  a["type"] = ActionTypes::DONE;
  a["tick"] = toJson(tick);
  a["pid"] = toJson(playerID_);
  a["checksum"] = checksum;
  game_->addAction(playerID_, a);
  return true;
}
};  // rts
