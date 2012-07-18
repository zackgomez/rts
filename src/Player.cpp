#include "Player.h"
#include <SDL/SDL.h>
#include "Renderer.h"
#include "ParamReader.h"
#include "Entity.h"
#include "Game.h"
#include "MessageHub.h"

const int tickOffset = 1;

LocalPlayer::LocalPlayer(int64_t playerID, OpenGLRenderer *renderer) :
    Player(playerID)
,   renderer_(renderer)
,   targetTick_(0)
,   doneTick_(-1e6) // no done ticks
,   selection_(NO_ENTITY)
,   cameraPanDir_(0.f)
,   order_()
{
    logger_ = Logger::getLogger("LocalPlayer");
}

LocalPlayer::~LocalPlayer()
{
}

bool LocalPlayer::update(int64_t tick)
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

void LocalPlayer::playerAction(int64_t playerID, const PlayerAction &action)
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
    const glm::vec2 &res = renderer_->getResolution();
    const int CAMERA_PAN_THRESHOLD = getParam("camera.panthresh");

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

    const float CAMERA_PAN_SPEED = getParam("camera.panspeed");
    glm::vec3 delta = CAMERA_PAN_SPEED * dt * glm::vec3(dir.x, 0.f, dir.y);
    renderer_->updateCamera(delta);

    // Deselect Dead Entity
    if (!MessageHub::get()->getEntity(selection_))
        setSelection(NO_ENTITY);
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
    eid_t newSelect = selection_;

    glm::vec3 loc = renderer_->screenToTerrain(screenCoord);
    // The entity (maybe) under the cursor
    eid_t eid = renderer_->selectEntity(screenCoord);
    const Entity *entity = game_->getEntity(eid);

    if (button == SDL_BUTTON_LEFT)
    {
        // If no order, then adjust selection
        if (order_.empty())
        {
            // If there is an entity and its ours, select
            if (entity && entity->getPlayerID() == playerID_)
                newSelect = eid;
            // Otherwise deselect
            else 
                newSelect = 0;
        }
        // If order, then execute it
        else
        {
            // TODO execute order_
            logger_->info() << "Would execute order " << order_
                << " on selection " << selection_ << " args... eid: "
                << eid << " loc " << loc << '\n';

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
                && selection_ != NO_ENTITY)
            {
                // Visual feedback
                // TODO make this something related to the unit clicked on
                renderer_->highlight(glm::vec2(loc.x, loc.z));

                // Queue up action
                action["type"] = ActionTypes::ATTACK;
                action["entity"] = (Json::Value::UInt64) selection_;
                action["enemy_id"] = (Json::Value::UInt64) eid;
                action["pid"] = (Json::Value::Int64) playerID_;
                action["tick"] = (Json::Value::Int64) targetTick_;
            }
            // If we have a selection, and they didn't click on the current
            // selection, move them to target
            else if (selection_ != NO_ENTITY && (!entity || eid != selection_))
            {
                if (loc != glm::vec3 (HUGE_VAL))
                {
                    // Visual feedback
                    renderer_->highlight(glm::vec2(loc.x, loc.z));

                    // Queue up action
                    action["type"] = ActionTypes::MOVE;
                    action["entity"] = (Json::Value::UInt64) selection_;
                    action["target"] = toJson(loc);
                    action["pid"] = (Json::Value::Int64) playerID_;
                    action["tick"] = (Json::Value::Int64) targetTick_;
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
    // nop
}

void LocalPlayer::keyPress(SDLKey key)
{
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

    // Order types
    else if (key == SDLK_a && selection_ != NO_ENTITY)
        order_ = ActionTypes::ATTACK;
    else if (key == SDLK_m && selection_ != NO_ENTITY)
        order_ = ActionTypes::MOVE;
    else if (key == SDLK_s && selection_ != NO_ENTITY)
    {
        if (selection_)
        {
            action["type"] = ActionTypes::STOP;
            action["entity"] = toJson(selection_);
            action["pid"] = toJson(playerID_);
            action["tick"] = toJson(targetTick_);
            game_->addAction(playerID_, action);
        }
    }
    // ESC clears out current states
    else if (key == SDLK_ESCAPE)
    {
        if (!order_.empty())
            order_.clear();
        else
            setSelection(NO_ENTITY);
    }

    // Debug commands
    else if (key == SDLK_g)
        SDL_WM_GrabInput(SDL_GRAB_ON);
}

void LocalPlayer::keyRelease(SDLKey key)
{
    if (key == SDLK_RIGHT || key == SDLK_LEFT)
        cameraPanDir_.x = 0.f;
    else if (key == SDLK_UP || key == SDLK_DOWN)
        cameraPanDir_.y = 0.f;
}

void
LocalPlayer::setSelection(eid_t eid)
{
    selection_ = eid;
    renderer_->setSelection(selection_);
}


bool
SlowPlayer::update(int64_t tick)
{
    // Just blast through the sync frames, for now
    if (tick < 0)
        return true;

    if (frand() < getParam("slowPlayer.dropChance"))
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

