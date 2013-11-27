#include <cstdlib>
#include <ctime>
#include <thread>
#include <string>
#include <queue>
#include <vector>
#include <GL/glew.h>
#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"
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

  FPSCalculator updateTimer(10);

  game->start();

  auto controller = new rts::GameController(
    (rts::LocalPlayer *) Game::get()->getPlayer(localPlayerID));
  Renderer::get()->postToMainThread([=] () {
    Renderer::get()->setController(controller);
  });

  Clock::time_point start = Clock::now();
	int tick_count = 0;

  while (game->isRunning()) {
    game->update(simdt);

		tick_count++;
		float delay = simdt * tick_count - Clock::secondsSince(start);
    std::chrono::milliseconds delayms(static_cast<int>(1000 * delay));
    std::this_thread::sleep_for(delayms);

    float fps = updateTimer.sample();
    if (fps < simrate) {
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

void set_working_directory(int argc, char **argv) {
#ifndef _MSC_VER
  namespace fs = boost::filesystem;
	fs::path full_path = fs::system_complete(fs::path(argv[0]));
	auto exec_dir = full_path.branch_path();
	fs::current_path(exec_dir);
  fs::path bundle_marker_path("bundle_marker");
	if (fs::exists(bundle_marker_path)) {
		// In bundle.app/Contents/MacOS
		// going to bundle.app/Contents/Resources
		auto resource_path = exec_dir/fs::path("../Resources");
		fs::current_path(resource_path);
	}
#endif
}

int main(int argc, char **argv) {
	set_working_directory(argc, argv);

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
