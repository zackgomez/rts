#include "Unit.h"
#include <cstdlib>
#include <ctime>
#include <SDL/SDL.h>
#include <GL/glew.h>
#include "Logger.h"
#include "ParamReader.h"
#include "Game.h"
#include "Renderer.h"
#include "Player.h"
#include "Map.h"
#include "Engine.h"

void mainloop();
void render();
void handleInput();

int initLibs();
void cleanup();

LoggerPtr logger;
bool running;

LocalPlayer *player;
Game *game;
OpenGLRenderer *renderer;

int main (int argc, char **argv)
{
    logger = Logger::getLogger("Main");

    ParamReader::get()->loadFile("config.params");

    if (!initLibs())
        exit(1);

    mainloop();

    return 0;
}

void mainloop()
{
    glm::vec2 res(getParam("resolution.x"), getParam("resolution.y"));
    const float framerate = getParam("framerate");
    float fps = 1.f / framerate;
    renderer = new OpenGLRenderer(res);
    player = new LocalPlayer(1, renderer);
	std::vector<Player *> players; players.push_back(player);
    game = new HostGame(new Map(glm::vec2(20, 20)), players);
    game->addRenderer(renderer);

    running = true;

    while (running)
    {
        handleInput();

        game->update(fps);
        game->render(fps);
        // Regulate frame rate
        SDL_Delay(int(1000*fps));
    }
}

void handleInput()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_KEYDOWN:
            if (event.key.keysym.sym == SDLK_F10)
            {
                running = false;
                break;
            }
            player->handleEvent(event);
            break;
        case SDL_QUIT:
            running = false;
            break;
        default:
            player->handleEvent(event);
        }
    }
}

int initLibs()
{
    srand(time(NULL));

    atexit(cleanup);

    return 1;
}

void cleanup()
{
    logger->info() << "Cleaning up...\n";
    teardownEngine();
}

