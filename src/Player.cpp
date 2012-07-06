#include "Player.h"
#include <SDL/SDL.h>
#include "Renderer.h"
#include "ParamReader.h"

LocalPlayer::LocalPlayer(int64_t playerID, OpenGLRenderer *renderer) :
    Player(playerID)
,   renderer_(renderer)
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

PlayerAction LocalPlayer::getAction()
{
    PlayerAction ret;
    ret.type = PlayerAction::NONE;

    return ret;
}

void LocalPlayer::handleEvent(const SDL_Event &event)
{
    // TODO handle mouse events
    switch (event.type)
    {
    case SDL_KEYDOWN:
        if (event.key.keysym.sym == SDLK_UP)
            renderer_->updateCamera(glm::vec3(0.f, 0.f, 0.1f));
        if (event.key.keysym.sym == SDLK_DOWN)
            renderer_->updateCamera(glm::vec3(0.f, 0.f, -0.1f));
    }
}

