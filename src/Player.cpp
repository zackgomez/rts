#include "Player.h"
#include <SDL/SDL.h>
#include "Renderer.h"
#include "ParamReader.h"
#include "Entity.h"
#include "Game.h"
#include "MessageHub.h"

namespace rts {

const int tickOffset = 1;

LocalPlayer::LocalPlayer(id_t playerID, OpenGLRenderer *renderer) :
  Player(playerID),
  renderer_(renderer),
  targetTick_(0),
  doneTick_(-1e6), // no done ticks
  selection_(),
  cameraPanDir_(0.f),
  shift_(false),
  leftDrag_(false),
  order_()
{
  assertPid(playerID);
  logger_ = Logger::getLogger("LocalPlayer");
}

LocalPlayer::~LocalPlayer()
{
}

bool LocalPlayer::update(tick_t tick)
{
  // Don't do anything if we're already done
  if (tick <= doneTick_)
    return true;

  // Finish the last frame by sending the NONE action
  PlayerAction a;
  a["type"] = ActionTypes::DONE;
  a["pid"] = (Json::Value::Int64) playerID_;
  a["tick"] = (Json::Value::Int64) targetTick_;
  game_->addAction(playerID_, a);

  // We've generated for this tick
  doneTick_ = tick;
  // Now generate input for the next + offset
  targetTick_ = tick + game_->getTickOffset();

  // We've completed input for the previous target frame
  return true;
}

void LocalPlayer::playerAction(id_t playerID, const PlayerAction &action)
{
  // nop, mostly used by network players
}

void LocalPlayer::renderUpdate(float dt)
{
  // No input while game is paused, not even panning
  if (game_->isPaused())
    return;

  int x, y;
  SDL_GetMouseState(&x, &y);
  glm::vec2 screenCoord(x, y);
  const glm::vec2 &res = renderer_->getResolution();
  const int CAMERA_PAN_THRESHOLD = fltParam("camera.panthresh");

  // Mouse overrides keyboard
  glm::vec2 dir = cameraPanDir_;

  if (x <= CAMERA_PAN_THRESHOLD) 
    dir.x = 2 * (x - CAMERA_PAN_THRESHOLD) / CAMERA_PAN_THRESHOLD;
  else if (x > res.x - CAMERA_PAN_THRESHOLD)
    dir.x = -2 * ((res.x - x) - CAMERA_PAN_THRESHOLD) / CAMERA_PAN_THRESHOLD;
  if (y <= CAMERA_PAN_THRESHOLD) 
    dir.y = 2 * (y - CAMERA_PAN_THRESHOLD) / CAMERA_PAN_THRESHOLD;
  else if (y > res.y - CAMERA_PAN_THRESHOLD)
    dir.y = -2 * ((res.y - y) - CAMERA_PAN_THRESHOLD) / CAMERA_PAN_THRESHOLD;

  const float CAMERA_PAN_SPEED = fltParam("camera.panspeed");
  glm::vec3 delta = CAMERA_PAN_SPEED * dt * glm::vec3(dir.x, 0.f, dir.y);
  renderer_->updateCamera(delta);

  // Deselect dead entities
  std::set<id_t> newsel;
  for (auto eid : selection_)
  {
    assertEid(eid);
    if (MessageHub::get()->getEntity(eid))
      newsel.insert(eid);
  }
  setSelection(newsel);

  // Draw drag rectangle if over threshold size
  glm::vec3 loc = renderer_->screenToTerrain(screenCoord);
  if (leftDrag_
      && glm::distance(loc, leftStart_) > fltParam("hud.minDragDistance"))
  {
    renderer_->setDragRect(leftStart_, loc);
  }
}

void LocalPlayer::setGame(Game *game)
{
  Player::setGame(game);
}

void LocalPlayer::quitEvent()
{
  // Send the quit game event
  PlayerAction action;
  action["type"] = ActionTypes::LEAVE_GAME;
  action["pid"] = toJson(playerID_);
  game_->addAction(playerID_, action);
}

void LocalPlayer::mouseDown(const glm::vec2 &screenCoord, int button)
{
  PlayerAction action;
  std::set<id_t> newSelect = selection_;

  glm::vec3 loc = renderer_->screenToTerrain(screenCoord);
  // The entity (maybe) under the cursor
  id_t eid = renderer_->selectEntity(screenCoord);
  const Entity *entity = game_->getEntity(eid);

  if (button == SDL_BUTTON_LEFT)
  {
    leftDrag_ = true;
    leftStart_ = loc;
    // If no order, then adjust selection
    if (order_.empty())
    {
      // If no shift held, then deselect (shift just adds)
      if (!shift_)
        newSelect.clear();
      // If there is an entity and its ours, select
      if (entity && entity->getPlayerID() == playerID_)
        newSelect.insert(eid);
    }
    // If order, then execute it
    else
    {
      action["type"] = order_;
      action["entity"] = toJson(selection_);
      action["target"] = toJson(loc);
      if (entity && entity->getPlayerID() != playerID_)
        action["enemy_id"] = toJson(entity->getID());
      action["pid"] = (Json::Value::Int64) playerID_;
      action["tick"] = (Json::Value::Int64) targetTick_;

      // Clear order
      order_.clear();
    }
  }
  else if (button == SDL_BUTTON_RIGHT)
  {
    // If there is an order, it is canceled by right click
    if (!order_.empty())
      order_.clear();
    // Otherwise default to some right click actions
    else
    {
      // If right clicked on enemy unit (and we have a selection)
      // go attack them
      if (entity && entity->getPlayerID() != playerID_
          && !selection_.empty())
      {
        // Visual feedback
        // TODO make this something related to the unit clicked on
        renderer_->highlight(glm::vec2(loc.x, loc.z));

        // Queue up action
        action["type"] = ActionTypes::ATTACK;
        action["entity"] = toJson(selection_);
        action["enemy_id"] = toJson(eid);
        action["pid"] = toJson(playerID_);
        action["tick"] = toJson(targetTick_);
      }
      // If we have a selection, and they didn't click on the current
      // selection, move them to target
      else if (!selection_.empty() && (!entity || !selection_.count(eid)))
      {
        if (loc != glm::vec3 (HUGE_VAL))
        {
          // Visual feedback
          renderer_->highlight(glm::vec2(loc.x, loc.z));

          // Queue up action
          action["type"] = ActionTypes::MOVE;
          action["entity"] = toJson(selection_);
          action["target"] = toJson(loc);
          action["pid"] = toJson(playerID_);
          action["tick"] = toJson(targetTick_);
        }
      }
    }
  }

  if (game_->isPaused())
    return;

  // Mutate, if game isn't paused
  setSelection(newSelect);
  if (action.isMember("type"))
    game_->addAction(playerID_, action);
}

void LocalPlayer::mouseUp(const glm::vec2 &screenCoord, int button)
{
  glm::vec3 loc = renderer_->screenToTerrain(screenCoord);
  if (button == SDL_BUTTON_LEFT)
  {
    std::set<id_t> newSelect;
    if (glm::distance(leftStart_, loc) > fltParam("hud.minDragDistance"))
    {
      newSelect = renderer_->selectEntities(leftStart_, loc, playerID_);

      if (!shift_)
        selection_.clear();
      selection_.insert(newSelect.begin(), newSelect.end());
      setSelection(selection_);
    }

    leftDrag_ = false;
  }
  // nop
}

void LocalPlayer::keyPress(SDLKey key)
{
  static const SDLKey MAIN_KEYS[] = {SDLK_q, SDLK_w, SDLK_e, SDLK_r};
  static const SDLKey UPGRADE_KEYS[] = {SDLK_t, SDLK_g, SDLK_b};
  // TODO(zack) watch out for pausing here
  PlayerAction action;
  if (key == SDLK_F10)
  {
    action["type"] = ActionTypes::LEAVE_GAME;
    action["pid"] = toJson(playerID_);
    game_->addAction(playerID_, action);
  }
  // Camera panning
  else if (key == SDLK_UP)
    cameraPanDir_.y = -1.f;
  else if (key == SDLK_DOWN)
    cameraPanDir_.y = 1.f;
  else if (key == SDLK_RIGHT)
    cameraPanDir_.x = 1.f;
  else if (key == SDLK_LEFT)
    cameraPanDir_.x = -1.f;

  // ESC clears out current states
  else if (key == SDLK_ESCAPE)
  {
    if (!order_.empty())
      order_.clear();
    else
      setSelection(std::set<id_t>());
  }
  else if (key == SDLK_LSHIFT || key == SDLK_RSHIFT)
    shift_ = true;

  // Debug commands
  else if (key == SDLK_g)
    SDL_WM_GrabInput(SDL_GRAB_ON);
  // Handle unit commands
  else if (!selection_.empty())
  {
    // Order types
    if (key == SDLK_a)
      order_ = ActionTypes::ATTACK;
    else if (key == SDLK_m)
      order_ = ActionTypes::MOVE;
    else if (key == SDLK_s)
    {
      action["type"] = ActionTypes::STOP;
      action["entity"] = toJson(selection_);
      action["pid"] = toJson(playerID_);
      action["tick"] = toJson(targetTick_);
      game_->addAction(playerID_, action);
    }
    else 
    {
      for (unsigned int i = 0; i < 4; i++)
      {
        if (key == MAIN_KEYS[i])
        {
          auto sel = selection_.begin();
          const Entity *ent = MessageHub::get()->getEntity(*sel);
          // The main action of a building is production
          if (ent->getType() == "BUILDING")
          {
            std::vector<std::string> prod = arrParam(ent->getName() + ".prod");
            if (i < prod.size())
            {
              action["type"] = ActionTypes::ENQUEUE;
              action["entity"] = toJson(*sel);
              action["pid"] = toJson(playerID_);
              action["prod"] = prod.at(i);
              action["tick"] = toJson(targetTick_);
              game_->addAction(playerID_, action);
            }
          }
          break;
        }
      }
    }
  }
}

void LocalPlayer::keyRelease(SDLKey key)
{
  if (key == SDLK_RIGHT || key == SDLK_LEFT)
    cameraPanDir_.x = 0.f;
  else if (key == SDLK_UP || key == SDLK_DOWN)
    cameraPanDir_.y = 0.f;
  else if (key == SDLK_LSHIFT || key == SDLK_RSHIFT)
    shift_ = false;
}

void
  LocalPlayer::setSelection(const std::set<id_t> &s)
  {
    selection_ = s;
    renderer_->setSelection(selection_);
  }


bool
  SlowPlayer::update(tick_t tick)
  {
    // Just blast through the sync frames, for now
    if (tick < 0)
      return true;

    if (frand() < fltParam("slowPlayer.dropChance"))
    {
      Logger::getLogger("SlowPlayer")->info() << "I strike again!\n";
      return false;
    }

    PlayerAction a;
    a["type"] = ActionTypes::DONE;
    a["tick"] = (Json::Value::Int64) tick;
    a["pid"] = (Json::Value::Int64) playerID_;
    game_->addAction(playerID_, a);
    return true;
  }

}; // rts
