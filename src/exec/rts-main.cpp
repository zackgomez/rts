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
#include "common/kissnet.h"
#include "common/Logger.h"
#include "common/ParamReader.h"
#include "common/NetConnection.h"
#include "common/util.h"
#include "rts/Controller.h"
#include "rts/Game.h"
#include "rts/GameController.h"
#include "rts/GameServer.h"
#include "rts/Graphics.h"
#include "rts/Map.h"
#include "rts/Matchmaker.h"
#include "rts/MatchmakerController.h"
#include "rts/Renderer.h"
#include "rts/ResourceManager.h"
#include "rts/Player.h"

using rts::Game;
using rts::GameServer;
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

void gameThread(Game *game) {
  auto action_func = [&](id_t pid, const PlayerAction &act) {
    return game->addAction(pid, act);
  };

  // find local player
  Player *lp = nullptr;
  for (auto&& player : Game::get()->getPlayers()) {
    if (player->isLocal()) {
      lp = player;
      break;
    }
  }
  invariant(lp != nullptr, "Unable to find local player");

  auto controller = new rts::GameController(
    (rts::LocalPlayer *) lp,
    action_func);
  Renderer::get()->postToMainThread([=] () {
    Renderer::get()->setController(controller);
    Renderer::get()->setGameTime(0.f);
    Renderer::get()->setTimeMultiplier(0.f);
  });

  game->run();

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

  rts::MatchmakerController *controller =
    new rts::MatchmakerController(&matchmaker);
  Renderer::get()->postToMainThread([=] () {
    Renderer::get()->setController(controller);
  });

  Game *game = nullptr;
  while (!game) {
    try {
      game = matchmaker.waitGame();
    } catch (rts::matchmaker_quit_exception &e) {
      LOG(INFO) << "got quit signal from matchmaker\n";
      return;
    } catch (std::exception &e) {
      LOG(FATAL) << "caught exception: " << e.what() << '\n';
    }
  }

  std::thread gamet(gameThread, game);
  gamet.detach();
}

void set_working_directory(int argc, char **argv) {
#ifndef _MSC_VER
  namespace fs = boost::filesystem;
	fs::path full_path = fs::system_complete(fs::path(argv[0]));
	auto exec_dir = full_path.branch_path();
	fs::current_path(exec_dir);
	// In bundle.app/Contents/MacOS
	// going to bundle.app/Contents/Resources
	auto resource_path = exec_dir/fs::path("../Resources");
	fs::current_path(resource_path);
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
