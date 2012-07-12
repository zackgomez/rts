#include "Unit.h"
#include <cstdlib>
#include <ctime>
#include <SDL/SDL.h>
#include <GL/glew.h>
#include <thread>
#include "Logger.h"
#include "ParamReader.h"
#include "Game.h"
#include "Renderer.h"
#include "Player.h"
#include "NetPlayer.h"
#include "Map.h"
#include "Engine.h"
#include "kissnet.h"

void mainloop();
void render();
void handleInput(float dt);

int initLibs();
void cleanup();

LoggerPtr logger;
bool running;

LocalPlayer *player;
Game *game;
OpenGLRenderer *renderer;

const std::string port = "27465";

Player * getOpponent(const std::string &ip)
{
    kissnet::tcp_socket_ptr sock;
    // Host
    if (ip.empty())
    {
        kissnet::tcp_socket_ptr serv_sock = kissnet::tcp_socket::create();
        serv_sock->listen(port, 1);

        logger->info() << "Waiting for connection\n";
        sock = serv_sock->accept();

        logger->info() << "Got client\n";
    }
    else
    {
        logger->info() << "Connected to " << ip << ":" << port << '\n';
        sock = kissnet::tcp_socket::create();
        sock->connect(ip, port);
    }

    logger->info() << "Connected to " << sock->getHostname() << ":" << sock->getPort() << '\n';
    int64_t playerID = ip.empty() ? 2 : 1;
    logger->info() << "Creating NetPlayer with pid: " << playerID << '\n';

    return new NetPlayer(playerID, sock);
}

std::vector<Player *> getPlayers(const std::vector<std::string> &args)
{
    int64_t playerID = 1;
    std::vector<Player *> players;
    // First get opponent if exists
    if (!args.empty() && args[0] == "--2p")
    {
        std::string ip = args.size() > 1 ? args[1] : std::string();
        if (!ip.empty())
            playerID = 2;
        Player *opp = getOpponent(ip);
        players.push_back(opp);
    }
    else if (!args.empty() && args[0] == "--slowp")
    {
        players.push_back(new SlowPlayer(2));
    }
    
    // Now set up local player
    glm::vec2 res(getParam("resolution.x"), getParam("resolution.y"));
    renderer = new OpenGLRenderer(res);

    player = new LocalPlayer(playerID, renderer);
    players.push_back(player);

    return players;
}

void gameThread()
{
    const float simrate = getParam("simrate");
    const float simdt = 1.f / simrate;

    game->start(simdt);

    while (running)
    {
        // TODO tighten this so that EVERY 10 ms this will go
        uint32_t start = SDL_GetTicks();

        game->update(simdt);

        SDL_Delay(int(1000*simdt));
    }
}

void mainloop()
{
    const float framerate = getParam("framerate");
    float fps = 1.f / framerate;

    running = true;

    std::thread gamet(gameThread);
    // render loop
    while (running)
    {
        handleInput(fps);
        game->render(fps);
        // Regulate frame rate
        SDL_Delay(int(1000*fps));
    }

    gamet.join();
}

void handleInput(float dt)
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
    player->renderUpdate(dt);
}

int initLibs()
{
    seedRandom(time(NULL));

    atexit(cleanup);

    return 1;
}

void cleanup()
{
    logger->info() << "Cleaning up...\n";
    teardownEngine();
}

int main (int argc, char **argv)
{
    std::string progname = argv[0];
    std::vector<std::string> args;
    for (int i = 1; i < argc; i++)
        args.push_back(std::string(argv[i]));

    logger = Logger::getLogger("Main");

    ParamReader::get()->loadFile("config.params");

    if (!initLibs())
        exit(1);

    /*
    std::string ip;
    if (argc == 2)
        ip = argv[1];

    Player *opp = getOpponent(ip);
    return 0;
    */


	std::vector<Player *> players = getPlayers(args);
    game = new Game(new Map(glm::vec2(20, 20)), players);
    game->addRenderer(renderer);



    mainloop();

    return 0;
}

