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
#include "util.h"

using namespace rts;

void mainloop();
void render();
void handleInput(float dt);

int initLibs();
void cleanup();

LoggerPtr logger;

LocalPlayer *player;
Game *game;
OpenGLRenderer *renderer;

// TODO take this in as an argument!
const std::string port = "27465";

// player 1...n
// TODO(zack) eventually read these from player config and in network handshake
glm::vec3 getPlayerColor(int player) {
  invariant(player == 1 || player == 2, "unknown player");
  std::stringstream ss;
  ss << "colors.player" << player;
  return vec3Param(ss.str());
}

NetPlayer * getOpponent(const std::string &ip) {
  kissnet::tcp_socket_ptr sock;
  // Host
  if (ip.empty()) {
    kissnet::tcp_socket_ptr serv_sock = kissnet::tcp_socket::create();
    serv_sock->listen(port, 1);

    logger->info() << "Waiting for connection\n";
    sock = serv_sock->accept();

    logger->info() << "Got client\n";
  } else {
    bool connected = false;
    sock = kissnet::tcp_socket::create();
    for (int i = 0; i < intParam("network.connectAttempts"); i++) {
      try {
        logger->info() << "Trying connection to " << ip << ":" << port << '\n';
        sock->connect(ip, port);
        // If connection is successful, yay
        connected = true;
        break;
      } catch (kissnet::socket_exception &e) {
        logger->warning() << "Failed to connected: '"
                          << e.what() << "'\n";
      }

      SDL_Delay(1000 * fltParam("network.connectInterval"));
    }
    if (!connected) {
      logger->info() << "Unable to connect after " <<
                     fltParam("network.connectAttempts") << " attempts\n";
      return NULL;
    }
  }

  logger->info() << "Initiating handshake with "
                 << sock->getHostname() << ":" << sock->getPort() << '\n';
  NetConnection *conn = new NetConnection(sock);

  // HANDSHAKE
  // TODO(zack): Wow, this is just atrocious... look into "workflows" or
  // something...
  // Send our version
  Json::Value v;
  v["type"] = "HANDSHAKE";
  v["handshake_version"] = strParam("game.version");
  conn->sendPacket(v);
  // Wait for responses
  std::queue<Json::Value> &queue = conn->getQueue();
  bool version_checked = false;
  while (!version_checked) {
    logger->info() << "Handshaking...\n";
    if (!queue.empty()) {
      Json::Value msg = queue.front();
      queue.pop();

      logger->info() << "Received handshake message " << msg << '\n';
      invariant(msg["type"] == "HANDSHAKE",
                "bad network message during handshake");
      if (!version_checked) {
        invariant(msg.isMember("handshake_version"), "expected version message");
        if (msg["handshake_version"] != strParam("game.version")) {
          logger->fatal() << "Wrong handshake version from connection\n";
          conn->stop();
          return NULL;
        }
        version_checked = true;
        continue;
      }
    } else {
      SDL_Delay(100);
    }
  }

  int64_t playerID = ip.empty() ? 2 : 1;
  logger->info() << "Creating NetPlayer with pid: " << playerID << '\n';
  return new NetPlayer(playerID, getPlayerColor(playerID), conn);
}

std::vector<Player *> getPlayers(const std::vector<std::string> &args) {
  // TODO(zack) streamline this, add some handshake in network setup that assigns
  // IDs correctly
  int64_t playerID = 1;
  std::vector<Player *> players;
  // First get opponent if exists
  if (!args.empty() && args[0] == "--2p") {
    std::string ip = args.size() > 1 ? args[1] : std::string();
    if (!ip.empty()) {
      playerID = 2;
    }
    NetPlayer *opp = getOpponent(ip);
    if (!opp) {
      logger->info() << "Couldn't get opponent\n";
      exit(0);
    }
    opp->setLocalPlayer(playerID);
    players.push_back(opp);
  } else if (!args.empty() && args[0] == "--slowp") {
    players.push_back(new SlowPlayer(2));
  } else {
    players.push_back(new DummyPlayer(2));
  }

  // Now set up local player
  glm::vec2 res = vec2Param("window.resolution");
  renderer = new OpenGLRenderer(res);

  player = new LocalPlayer(playerID, getPlayerColor(playerID), renderer);
  renderer->setLocalPlayer(player);

  players.push_back(player);

  return players;
}

void gameThread() {
  const float simrate = fltParam("game.simrate");
  const float simdt = 1.f / simrate;

  game->start(simdt);
  float delay;

  while (game->isRunning()) {
    // TODO(zack) tighten this so that EVERY 10 ms this will go
    uint32_t start = SDL_GetTicks();

    game->update(simdt);
    delay = int(1000*simdt - (SDL_GetTicks() - start));
    // If we want to delay for zero seconds, we've used our time slice
    // and are lagging.
    delay = glm::clamp(delay, 0.0f, 1000.0f * simdt);
    invariant(delay <= 1000 * simdt, "invalid delay, longer than interval");
    invariant(delay >= 0, "cannot have negative delay");
    SDL_Delay(delay);
  }
}

void mainloop() {
  const float framerate = fltParam("game.framerate");
  float fps = 1.f / framerate;

  std::thread gamet(gameThread);
  // render loop
  while (game->isRunning()) {
    handleInput(fps);
    game->render(fps);
    // Regulate frame rate
    SDL_Delay(int(1000*fps));
  }

  gamet.join();
}

void handleInput(float dt) {
  glm::vec2 screenCoord;
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    switch (event.type) {
    case SDL_KEYDOWN:
      player->keyPress(event.key.keysym.sym);
      break;
    case SDL_KEYUP:
      player->keyRelease(event.key.keysym.sym);
      break;
    case SDL_MOUSEBUTTONDOWN:
      screenCoord = glm::vec2(event.button.x, event.button.y);
      player->mouseDown(screenCoord, event.button.button);
      break;
    case SDL_MOUSEBUTTONUP:
      screenCoord = glm::vec2(event.button.x, event.button.y);
      player->mouseUp(screenCoord, event.button.button);
      break;
    case SDL_QUIT:
      player->quitEvent();
      break;
    }
  }
  player->renderUpdate(dt);
}

int initLibs() {
  seedRandom(time(NULL));
  kissnet::init_networking();

  atexit(cleanup);

  return 1;
}

void cleanup() {
  logger->info() << "Cleaning up...\n";
  teardownEngine();
}

int main (int argc, char **argv) {
  std::string progname = argv[0];
  std::vector<std::string> args;
  for (int i = 1; i < argc; i++) {
    args.push_back(std::string(argv[i]));
  }

  logger = Logger::getLogger("Main");

  ParamReader::get()->loadFile("config.json");

  if (!initLibs()) {
    exit(1);
  }

  // Set up players and game
  std::vector<Player *> players = getPlayers(args);
  game = new Game(new Map(glm::vec2(20, 20)), players);
  game->addRenderer(renderer);

  // RUN IT
  mainloop();

  return 0;
}

