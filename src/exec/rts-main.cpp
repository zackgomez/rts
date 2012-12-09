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
#include "rts/Game.h"
#include "rts/Engine.h"
#include "rts/NetPlayer.h"
#include "rts/Map.h"
#include "rts/Matchmaker.h"
#include "rts/Player.h"
#include "rts/Renderer.h"
#include "rts/Unit.h"

using rts::Player;
using rts::LocalPlayer;
using rts::DummyPlayer;
using rts::NetPlayer;
using rts::Game;
using rts::Renderer;
using rts::Map;
using rts::Matchmaker;

void mainloop();
void render();
void handleInput(float dt);

int initLibs();
void cleanup();

Game *game;
Renderer *renderer;

// Matchmaker port
const std::string mmport = "11100";

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
  Clock::setThread();
  const float framerate = fltParam("local.framerate");
  float fps = 1.f / framerate;

  std::thread gamet(gameThread);
  // render loop
  while (game->isRunning()) {
    uint32_t start = SDL_GetTicks();
    handleInput(fps);
    Clock::startSection("render");
    game->render(fps);
    Clock::endSection("render");
    Clock::dumpTimes();
    // Regulate frame rate
    uint32_t end = SDL_GetTicks();
    uint32_t delay = std::max(int(1000*fps) - int(end - start), 0);
    LOG(DEBUG) << "delay: " << delay << '\n';
    SDL_Delay(delay);
  }

  gamet.join();
}

void handleInput(float dt) {
  LocalPlayer *player = renderer->getLocalPlayer();
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
  seedRandom(time(nullptr));
  kissnet::init_networking();

  atexit(cleanup);

  return 1;
}

void cleanup() {
  LOG(INFO) << "Cleaning up...\n";
  teardownEngine();
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

  // Set up players and game
  // TODO(zack): make renderer a true singleton
  renderer = Renderer::get();
  Matchmaker matchmaker(getParam("local.player"), renderer);

  std::vector<Player *> players;

  if (args.size() > 0 && args[0] == "--2p") {
    std::string ip = args.size() > 1 ? args[1] : "";
    players = matchmaker.doDirectSetup(ip);
  } else if (args.size() > 1 && args[0] == "--mm") {
    players = matchmaker.doServerSetup(args[1], mmport);
  } else {
    players = matchmaker.doDebugSetup();
  }

  Map *map = new Map(matchmaker.getMapName());
  game = new Game(map, players, renderer);

  // RUN IT
  mainloop();

  return 0;
}