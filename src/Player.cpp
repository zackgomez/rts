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
    a["type"] = ActionTypes::NONE;
    a["pid"] = (Json::Value::Int64) playerID_;
    a["tick"] = (Json::Value::Int64) targetTick_;
    game_->addAction(playerID_, a);

    int64_t ret = targetTick_;

    // We've generated for this tick
    doneTick_ = tick;
    // Now generate input for the next + offset
    targetTick_ = tick + game_->getTickOffset();

    // We've completed input for the previous target frame
    return true;
}

void LocalPlayer::playerAction(int64_t playerID, const PlayerAction &action)
{
    // TODO hmm, not much to do here
}

void LocalPlayer::renderUpdate(float dt)
{
    // No input while game is paused, not even panning
    if (game_->isPaused())
        return;

    int x, y;
    SDL_GetMouseState(&x, &y);
    const glm::vec2 &res = renderer_->getResolution();

    glm::vec2 dir;

    if (x == 0) 
        dir.x = -1;
    else if (x == res.x - 1)
        dir.x = 1;
    if (y == 0) 
        dir.y = -1;
    else if (y == res.y - 1)
        dir.y = 1;

    const float CAMERA_PAN_SPEED = getParam("camera.panspeed");

    renderer_->updateCamera(glm::vec3(dir.x, 0.f, dir.y) * CAMERA_PAN_SPEED * dt);
}

void LocalPlayer::setGame(Game *game)
{
    Player::setGame(game);
}

void LocalPlayer::handleEvent(const SDL_Event &event)
{
    // No input while game is paused, not even panning
    if (game_->isPaused())
        return;

	const float CAMERA_PAN_SPEED = getParam("camera.panspeed.key");
	LoggerPtr log = Logger::getLogger("Player");
    switch (event.type) {
    case SDL_KEYDOWN:
        if (event.key.keysym.sym == SDLK_UP) {
            renderer_->updateCamera(glm::vec3(0.f, 0.f, CAMERA_PAN_SPEED));
        }
        else if (event.key.keysym.sym == SDLK_DOWN) {
            renderer_->updateCamera(glm::vec3(0.f, 0.f, -CAMERA_PAN_SPEED));
        }
        else if (event.key.keysym.sym == SDLK_RIGHT) {
            renderer_->updateCamera(glm::vec3(CAMERA_PAN_SPEED, 0.f, 0.f));
        }
        else if (event.key.keysym.sym == SDLK_LEFT) {
            renderer_->updateCamera(glm::vec3(-CAMERA_PAN_SPEED, 0.f, 0.f));
        }
        else if (event.key.keysym.sym == SDLK_g) {
            SDL_WM_GrabInput(SDL_GRAB_ON);
        }
        else if (event.key.keysym.sym == SDLK_s) {
            if (selection_)
            {
                PlayerAction action;
                action["type"] = ActionTypes::STOP;
                action["entity"] = (Json::Value::UInt64) selection_;
                action["pid"] = (Json::Value::Int64) playerID_;
                action["tick"] = (Json::Value::Int64) targetTick_;
                game_->addAction(playerID_, action);
            }
        }


        break;
    case SDL_MOUSEBUTTONUP:
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
                PlayerAction action;
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
                    PlayerAction action;
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
    a["type"] = ActionTypes::NONE;
    a["tick"] = (Json::Value::Int64) tick;
    a["pid"] = (Json::Value::Int64) playerID_;
    game_->addAction(playerID_, a);
    return true;
}

