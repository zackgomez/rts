#include <cstdlib>
#include <ctime>
#include <thread>
#include <string>
#include <queue>
#include <vector>
#include <SDL/SDL.h>
#include <GL/glew.h>
#include "common/Clock.h"
#include "common/FPSCalculator.h"
#include "common/kissnet.h"
#include "common/Logger.h"
#include "common/ParamReader.h"
#include "common/NetConnection.h"
#include "common/util.h"
#include "rts/Controller.h"
#include "rts/GameController.h"
#include "rts/Game.h"
#include "rts/Graphics.h"
#include "rts/Map.h"
#include "rts/Matchmaker.h"
#include "rts/MatchmakerController.h"
#include "rts/Renderer.h"
#include "rts/ResourceManager.h"
#include "rts/Player.h"

using rts::Game;
using rts::Renderer;
using rts::Map;
using rts::Matchmaker;
using rts::Player;

void render();
void handleInput(float dt);

int initLibs();
void cleanup();

// Matchmaker port
const std::string mmport = "11100";

void matchmakerThread();

void gameThread(Game *game, rts::id_t localPlayerID) {
  const float simrate = fltParam("game.simrate");
  const float simdt = 1.f / simrate;

  Clock::time_point last = Clock::now();
  FPSCalculator updateTimer(10);

  game->start();

  auto controller = new rts::GameController(
    (rts::LocalPlayer *) Game::get()->getPlayer(localPlayerID));
  Renderer::get()->postToMainThread([=] () {
    Renderer::get()->setController(controller);
  });

  while (game->isRunning()) {
    game->update(simdt);

    float delay = glm::clamp(2 * simdt - Clock::secondsSince(last), 0.f, simdt);
    last = Clock::now();
    std::chrono::milliseconds delayms(static_cast<int>(1000 * delay));
    std::this_thread::sleep_for(delayms);

    float fps = updateTimer.sample();
    if (fps < (simrate * 0.95f)) {
      LOG(WARNING) << "Simulation update rate low: "
        << fps << " / " << simrate << '\n';
    }
  }

  Renderer::get()->postToMainThread([=]() {
    Renderer::get()->clearEntities();
    Renderer::get()->setController(nullptr);
  });

  {
    auto lock = Renderer::get()->lockEngine();
    delete game;
  }

  // TODO(zack): move to score screen menu
  std::thread mmthread(matchmakerThread);
  mmthread.detach();
}

int initLibs() {
  seedRandom(time(nullptr));
  kissnet::init_networking();
  Logger::initLogger();

  atexit(cleanup);

  return 1;
}

void cleanup() {
  LOG(INFO) << "Cleaning up...\n";
  teardownEngine();
}

void matchmakerThread() {
  Matchmaker matchmaker(getParam("local.player"));
  std::vector<Player *> players;

  rts::MatchmakerController *controller =
    new rts::MatchmakerController(&matchmaker);
  Renderer::get()->postToMainThread([=] () {
    Renderer::get()->setController(controller);
  });

  while (players.empty()) {
    try {
      players = matchmaker.waitPlayers();
    } catch (rts::matchmaker_quit_exception &e) {
      LOG(INFO) << "got quit signal from matchmaker\n";
      return;
    } catch (std::exception &e) {
      LOG(FATAL) << "caught exception: " << e.what() << '\n';
    }
  }

  Map *map = new Map(
      ResourceManager::get()->getMapDefinition(matchmaker.getMapName()));
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
  std::thread mmthread(matchmakerThread);
  mmthread.detach();

  Renderer::get()->startMainloop();

  return 0;
}
