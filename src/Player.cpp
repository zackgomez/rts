#include "Player.h"
#include <SDL/SDL.h>
#include "Renderer.h"
#include "ParamReader.h"
#include "Entity.h"
#include "Game.h"

LocalPlayer::LocalPlayer(int64_t playerID, OpenGLRenderer *renderer) :
    Player(playerID)
,   renderer_(renderer)
,   selection_(NO_ENTITY)
{
}

LocalPlayer::~LocalPlayer()
{
}

void LocalPlayer::update(float dt)
{
    // TODO what would go here?
}

void LocalPlayer::renderUpdate(float dt)
{
    int x, y, buttons;
    buttons = SDL_GetMouseState(&x, &y);
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

PlayerAction
LocalPlayer::getAction()
{
    PlayerAction ret;

    if (actions_.empty())
    {
        ret["type"] = ActionTypes::NONE;
    }
    else
    {
        ret = actions_.front();
        actions_.pop();
    }

    return ret;
}

void
LocalPlayer::handleEvent(const SDL_Event &event) {
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
                actions_.push(action);
            }
        }
        /* XXX(zack) I commented this out
        if (selection_) {
        	log->info() << event.key.keysym.sym << "\n";
        }
        */


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
                actions_.push(action);
        	}
            // If we have a selection, move them to target
        	else if (selection_ != NO_ENTITY)
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
                    actions_.push(action);
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

