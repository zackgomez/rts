#include <cstdlib>
#include <ctime>
#include <thread>
#include <string>
#include <queue>
#include <vector>
#include <SDL/SDL.h>
#include <GL/glew.h>
#include "common/Clock.h"
#include "common/kissnet.h"
#include "common/Logger.h"
#include "common/ParamReader.h"
#include "common/NetConnection.h"
#include "common/util.h"
#include "rts/Controller.h"
#include "rts/Game.h"
#include "rts/Graphics.h"
#include "rts/Map.h"
#include "rts/Matchmaker.h"
#include "rts/Renderer.h"
#include "rts/Player.h"
#include "rts/Unit.h"
#include "rts/UI.h"

using rts::Game;
using rts::Renderer;
using rts::Map;
using rts::Matchmaker;
using rts::Player;
using rts::UI;

void render();
void handleInput(float dt);

int initLibs();
void cleanup();

// Matchmaker port
const std::string mmport = "11100";

void gameThread(Game *game, rts::id_t localPlayerID) {
  const float simrate = fltParam("game.simrate");
  const float simdt = 1.f / simrate;

  UI::get()->initGameWidgets(localPlayerID);
  auto controller = new rts::GameController(
      (rts::LocalPlayer *) Game::get()->getPlayer(localPlayerID));
  Renderer::get()->setController(controller);

  game->start();

  Clock::time_point last = Clock::now();
  while (game->isRunning()) {
    game->update(simdt);

    float delay = glm::clamp(2 * simdt - Clock::secondsSince(last), 0.f, simdt);
    last = Clock::now();
    std::chrono::milliseconds delayms(static_cast<int>(1000 * delay));
    std::this_thread::sleep_for(delayms);
  }

  UI::get()->clearGameWidgets();
  delete game;

  // TODO(move to matchmaker menu)
  Renderer::get()->signalShutdown();
}

int initLibs() {
  seedRandom(time(nullptr));
  kissnet::init_networking();

  atexit(cleanup);

  return 1;
}

void cleanup() {
  LOG(INFO) << "Cleaning up...\n";
  teardownEngine();
}

void matchmakerThread(std::vector<std::string> args) {
  // Set up players and game
  Matchmaker matchmaker(getParam("local.player"));
  Renderer::get()->setController(new rts::MatchmakerController(&matchmaker));
  UI::get()->initMatchmakerWidgets();

  std::vector<Player *> players;

  try {
      if (args.size() > 0 && args[0] == "--2p") {
        std::string ip = args.size() > 1 ? args[1] : "";
        players = matchmaker.doDirectSetup(ip);
      } else if (args.size() > 1 && args[0] == "--mm") {
        players = matchmaker.doServerSetup(args[1], mmport);
      } else {
        players = matchmaker.doDebugSetup();
      }
  } catch (rts::matchmaker_exception &e) {
    players.clear();
    LOG(ERROR) << "unable to matchmake for " << args[0] << '\n';
  }
  UI::get()->clearMatchmakerWidgets();
  if (players.empty()) {
    exit(0);
  }

  Map *map = new Map(matchmaker.getMapName());
  Game *game = new Game(map, players);

  std::thread gamet(gameThread, game, matchmaker.getLocalPlayerID());
  gamet.detach();
}

int main(int argc, char **argv) {
  std::string progname = argv[0];
  std::vector<std::string> args;
  for (int i = 1; i < argc; i++) {
    args.push_back(std::string(argv[i]));
  }

  ParamReader::get()->loadFile("config.json");

  if (!initLibs()) {
    exit(1);
  }

  // Initialize Engine
  Renderer::get();

  // RUN IT
  std::thread mmthread(matchmakerThread, args);
  mmthread.detach();

  Renderer::get()->startMainloop();

  return 0;
}
