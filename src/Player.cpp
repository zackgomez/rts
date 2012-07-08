#include "Player.h"
#include <SDL/SDL.h>
#include "Renderer.h"
#include "ParamReader.h"
#include "Entity.h"

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
        if (event.key.keysym.sym == SDLK_DOWN) {
            renderer_->updateCamera(glm::vec3(0.f, 0.f, -CAMERA_PAN_SPEED));
        }
        if (event.key.keysym.sym == SDLK_RIGHT) {
            renderer_->updateCamera(glm::vec3(CAMERA_PAN_SPEED, 0.f, 0.f));
        }
        if (event.key.keysym.sym == SDLK_LEFT) {
            renderer_->updateCamera(glm::vec3(-CAMERA_PAN_SPEED, 0.f, 0.f));
        }
        if (selection_) {
        	log->info() << event.key.keysym.sym << "\n";
        }


        break;
    case SDL_MOUSEBUTTONUP:
        const glm::vec2 &res = renderer_->getResolution ();
        /*
        glm::vec2 screenCoord = glm::vec2 (
                event.button.x / res.x,
                1 - (event.button.y / res.y));
                */
        glm::vec2 screenCoord = glm::vec2(event.button.x, event.button.y);

        if (event.button.button == SDL_BUTTON_LEFT)
        {
            setSelection (renderer_->selectEntity (screenCoord));
        }
        else if (event.button.button == SDL_BUTTON_RIGHT)
        {
        	eid_t enemy;
        	if (enemy = renderer_->selectEntity (screenCoord) && selection_ != NO_ENTITY)
        	{
        		glm::vec3 target = renderer_->screenToTerrain (screenCoord);
        		if (target != glm::vec3 (HUGE_VAL))
        		{
        			// Visual feedback
        		    renderer_->highlight(glm::vec2(target.x, target.z));

        		    // Queue up action
        			PlayerAction action;
        		    action["type"] = ActionTypes::ATTACK;
        		    action["entity"] = (Json::Value::UInt64) selection_;
        		    action["target"] = toJson(target);
        		    action["enemy_id"] = (Json::Value::UInt64) enemy;
        		    actions_.push(action);
        		}

        	}

            if (selection_ != NO_ENTITY)
            {
                glm::vec3 target = renderer_->screenToTerrain (screenCoord);
                if (target != glm::vec3 (HUGE_VAL))
                {
                    // Visual feedback
                    renderer_->highlight(glm::vec2(target.x, target.z));

                    // Queue up action
                    PlayerAction action;
                    action["type"] = ActionTypes::MOVE;
                    action["entity"] = (Json::Value::UInt64) selection_;
                    action["target"] = toJson(target);
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

