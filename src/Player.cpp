#include "Player.h"
#include "Renderer.h"

LocalPlayer::LocalPlayer(int64_t playerID, OpenGLRenderer *renderer) :
    Player(playerID)
,   renderer_(renderer)
{
}

LocalPlayer::~LocalPlayer()
{
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
