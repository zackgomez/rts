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

NetPlayer * handshake(NetConnection *conn, rts::id_t localPlayerID,
    rts::id_t localPlayerTID, const std::string &localPlayerName,
    const glm::vec3 &localPlayerColor);

LoggerPtr logger;

LocalPlayer *player;
Game *game;
Renderer *renderer;

// TODO(zack) take this in as an argument!
const std::string port = "27465";

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

  LOG(INFO) << "Connected to " << sock->getHostname() << ":" << sock->getPort()
    << '\n';
  NetConnection *conn = new NetConnection(sock);
  // If we're hosting, then we'll take the lower id
  // TODO(zack): eventually this ID will be assigned to use by the match
  // maker
  rts::id_t localPlayerID = ip.empty() ? STARTING_PID : STARTING_PID + 1;
  rts::id_t localPlayerTID = ip.empty() ? STARTING_TID : STARTING_TID + 1;
  return handshake(conn, localPlayerID, localPlayerTID,
      strParam("local.username"), vec3Param("local.playerColor"));
}

NetPlayer * handshake(NetConnection *conn, rts::id_t localPlayerID,
    rts::id_t localPlayerTID, const std::string &localPlayerName,
    const glm::vec3 &localPlayerColor) {
  // Some chaced params
  const std::string version = strParam("game.version");
  const float maxT = fltParam("network.handshake.maxWait");
  const float interval = fltParam("network.handshake.checkInterval");
  const uint32_t paramChecksum = ParamReader::get()->getFileChecksum();

  // First send our message
  Json::Value v;
  v["type"] = "HANDSHAKE";
  v["version"] = version;
  v["pid"] = toJson(localPlayerID);
  v["tid"] = toJson(localPlayerTID);
  v["color"] = toJson(localPlayerColor);
  v["name"] = localPlayerName;
  v["param_checksum"] = paramChecksum;
  conn->sendPacket(v);

  std::queue<Json::Value> &queue = conn->getQueue();
  for (float t = 0.f; t <= maxT; t += interval) {
    if (!queue.empty()) {
      Json::Value msg = queue.front();
      queue.pop();

      LOG(INFO) << "Received handshake message " << msg << '\n';

      if (!msg.isMember("type") || msg["type"] != "HANDSHAKE") {
        LOG(FATAL) << "Received unexpected message during handshake\n";
        // Fail
        break;
      }

      if (!msg.isMember("version") || msg["version"] != version) {
        LOG(FATAL) << "Mismatched game version from connection "
          << "(theirs: " << msg["version"] << " ours: " << version << ")\n";
        // Fail
        break;
      }

      if (!msg.isMember("param_checksum")
          || msg["param_checksum"].asUInt() != paramChecksum) {
        LOG(FATAL) << "Mismatched params file checksum (theirs: " << std::hex
          << msg["param_checksum"].asUInt() << " ours: " << paramChecksum
          << std::dec << ")\n";
        // fail
        break;
      }

      rts::id_t pid;
      if (!msg.isMember("pid") || !(pid = assertPid(toID(msg["pid"])))) {
        LOG(FATAL) << "Missing pid from handshake message.\n";
        // fail
        break;
      }

      rts::id_t tid;
      if (!msg.isMember("tid") || !(tid = assertTid(toID(msg["tid"])))) {
        LOG(FATAL) << "Missing tid from handshake message.\n";
        // fail
        break;
      }

      glm::vec3 color;
      // TODO(zack): or isn't vec3/valid color
      if (!msg.isMember("color")) {
        LOG(FATAL) << "Mssing color in handshake message.\n";
        // fail
        break;
      }
      color = toVec3(msg["color"]);

      std::string name;
      if (!msg.isMember("name")) {
        LOG(FATAL) << "Mssing name in handshake message.\n";
        // fail
        break;
      }
      name = msg["name"].asString();

      // Success, make a new player
      return new NetPlayer(pid, tid, name, color, conn);
    }

    // No message, wait and check again
    SDL_Delay(1000 * interval);
  }

  // Didn't get the handshake message...
  LOG(ERROR) << "Unable to handshake...\n";
  conn->stop();
  return NULL;
};

std::vector<Player *> getPlayers(const std::vector<std::string> &args) {
  // TODO(zack) use matchmaker, or just create local and dummy players
  rts::id_t playerID = STARTING_PID;
  rts::id_t teamID = STARTING_TID;

  std::vector<Player *> players;
  // First get opponent if exists
  if (!args.empty() && args[0] == "--2p") {
    std::string ip = args.size() > 1 ? args[1] : std::string();
    if (!ip.empty()) {
      playerID++;
      teamID++;
    }
    NetPlayer *opp = getOpponent(ip);
    if (!opp) {
      logger->info() << "Couldn't get opponent\n";
      exit(0);
    }
    opp->setLocalPlayer(playerID);
    players.push_back(opp);
  } else if (!args.empty() && args[0] == "--slowp") {
    players.push_back(new SlowPlayer(playerID + 1, teamID + 1));
  } else {
    players.push_back(new DummyPlayer(playerID + 1, teamID + 1));
  }

  // Now set up local player
  glm::vec2 res = vec2Param("window.resolution");
  renderer = new Renderer(res);

  glm::vec3 color = vec3Param("local.playerColor");
  std::string name = strParam("local.username");
  player = new LocalPlayer(playerID, teamID, name, color, renderer);
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
      player->keyPress(event.key.keysym);
      break;
    case SDL_KEYUP:
      player->keyRelease(event.key.keysym);
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
  Map *map = new Map("debugMap");
  game = new Game(map, players, renderer);

  // RUN IT
  mainloop();

  return 0;
}

