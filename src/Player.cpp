#include "Player.h"
#include <SDL/SDL.h>
#include "Renderer.h"
#include "ParamReader.h"
#include "Entity.h"
#include "Game.h"

const int tickOffset = 1;

LocalPlayer::LocalPlayer(int64_t playerID, OpenGLRenderer *renderer) :
    Player(playerID)
,   renderer_(renderer)
,   targetTick_(0)
,   doneTick_(-1e6) // no done ticks
,   selection_(NO_ENTITY)
,   cameraPanDir_(0.f)
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
}

void LocalPlayer::setGame(Game *game)
{
    Player::setGame(game);
}

void LocalPlayer::handleEvent(const SDL_Event &event)
{
    float camera_pan_x = cameraPanDir_.x;
    float camera_pan_y = cameraPanDir_.y;
    PlayerAction action;
    switch (event.type) {
    case SDL_QUIT:
        action["type"] = ActionTypes::LEAVE_GAME;
        action["pid"] = toJson(playerID_);
        game_->addAction(playerID_, action);
        break;
    case SDL_KEYDOWN:
        if (event.key.keysym.sym == SDLK_F10) {
            action["type"] = ActionTypes::LEAVE_GAME;
            action["pid"] = toJson(playerID_);
            game_->addAction(playerID_, action);
        }
        if (event.key.keysym.sym == SDLK_UP) {
            camera_pan_y = -1.f;
        }
        else if (event.key.keysym.sym == SDLK_DOWN) {
            camera_pan_y = 1.f;
        }
        else if (event.key.keysym.sym == SDLK_RIGHT) {
            camera_pan_x = 1.f;
        }
        else if (event.key.keysym.sym == SDLK_LEFT) {
            camera_pan_x = -1.f;
        }
        else if (event.key.keysym.sym == SDLK_g) {
            SDL_WM_GrabInput(SDL_GRAB_ON);
        }
        else if (event.key.keysym.sym == SDLK_s) {
            if (selection_)
            {
                action["type"] = ActionTypes::STOP;
                action["entity"] = toJson(selection_);
                action["pid"] = toJson(playerID_);
                action["tick"] = toJson(targetTick_);
                game_->addAction(playerID_, action);
            }
        }
        break;
    case SDL_KEYUP:
        if (event.key.keysym.sym == SDLK_UP || event.key.keysym.sym == SDLK_DOWN) {
            camera_pan_y = 0.f;
        }
        else if (event.key.keysym.sym == SDLK_RIGHT || event.key.keysym.sym == SDLK_LEFT) {
            camera_pan_x = 0.f;
        }
        break;
    case SDL_MOUSEBUTTONUP:
        // TODO(zack) come up with a better system for dealing with certain
        // events while paused and ignore others.
        // No mouse events while paused
        glm::vec2 screenCoord = glm::vec2(event.button.x, event.button.y);
        glm::vec3 loc = renderer_->screenToTerrain (screenCoord);
        // The entity (maybe) under the cursor
        eid_t eid = renderer_->selectEntity(screenCoord);
        const Entity *entity = game_->getEntity(eid);

        if (event.button.button == SDL_BUTTON_LEFT)
        {
            // If there is an entity and its ours, select
            if (entity && entity->getPlayerID() == playerID_)
                setSelection (eid);
            // Otherwise deselect
            else 
                setSelection (0);
        }
        else if (event.button.button == SDL_BUTTON_RIGHT)
        {
            // If right clicked on enemy unit (and we have a selection)
            // go attack them
        	if (entity && entity->getPlayerID() != playerID_ && selection_ != NO_ENTITY)
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
                game_->addAction(playerID_, action);
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
                    game_->addAction(playerID_, action);
                }
            }
        }
        break;
    }
    cameraPanDir_ = glm::vec2(camera_pan_x, camera_pan_y);
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

