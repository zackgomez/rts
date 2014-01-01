#include "rts/Matchmaker.h"
#include <boost/algorithm/string.hpp>
#include <sstream>
#include "common/kissnet.h"
#include "common/NetConnection.h"
#include "common/ParamReader.h"
#include "common/util.h"
#include "rts/Game.h"
#include "rts/Player.h"
#include "rts/Map.h"
#include "rts/ResourceManager.h"

namespace rts {

// Tries a connection with some retries as per config
NetConnectionPtr attemptConnection(
    const std::string &ip,
    const std::string &port,
    const std::string &bindPort = "") {
  auto sock = kissnet::tcp_socket::create();
  if (!bindPort.empty()) {
    sock->bind(bindPort);
  }
  sock->nonblockingConnect(ip, port);
  kissnet::socket_set sockset;
  sockset.add_write_socket(sock);

  double timeout = intParam("network.connectAttempts")
      * fltParam("network.connectInterval");
  while (timeout > 0) {
    LOG(INFO) << "time to connect remaining: " << timeout << '\n';
    auto socks = sockset.poll_sockets(timeout);
    if (socks.empty()) {
      break;
    }
    int sockerr = sock->getError();
    if (!sockerr) {
      break;
    }

    LOG(INFO) << "SOCK ERROR: " << strerror(sockerr) << '\n';
    sockset.remove_socket(sock);
    sock = kissnet::tcp_socket::create();
    if (!bindPort.empty()) {
      sock->bind(bindPort);
    }
    sock->nonblockingConnect(ip, port);
    sockset.add_write_socket(sock);

    timeout -= 0.5;
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }
  if (timeout <= 0) {
    throw matchmaker_exception(
        "Unable to connect to remote host @ "+ip+":"+port);
  }
  sock->setBlocking();

  return NetConnectionPtr(new NetConnection(sock));
}

const std::string Matchmaker::MODE_SINGLEPLAYER = "sp";
const std::string MODE_QUIT = "q";

Matchmaker::Matchmaker(const Json::Value &player)
  : playerDef_(player) {
}

Game* Matchmaker::waitGame() {
  auto lock = std::unique_lock<std::mutex>(workMutex_);
  doneCondVar_.wait(lock);

  if (mode_ == MODE_SINGLEPLAYER) {
    return doSinglePlayerSetup();
  } else if (mode_ == MODE_QUIT) {
    throw matchmaker_quit_exception();
  } else {
    invariant_violation("Unknown matchmaking mode");
  }
}

void Matchmaker::signalReady(const std::string &mode) {
  invariant(mode == MODE_SINGLEPLAYER, "unknown matchmaking mode");
  mode_ = mode;
  doneCondVar_.notify_all();
}

void Matchmaker::signalStop() {
  mode_ = MODE_QUIT;
  doneCondVar_.notify_all();
}

Game* Matchmaker::doSinglePlayerSetup() {
  auto map_def = ResourceManager::get()->getMapDefinition("debugMap");
  Map *map = new Map(map_def);

  std::vector<Player *> players;
  LocalPlayer *lp = makeLocalPlayer(STARTING_PID, STARTING_TID, playerDef_);
  players.push_back(lp);

  DummyPlayer *dp = new DummyPlayer(STARTING_PID + 1, STARTING_TID + 1);
  players.push_back(dp);

  return new Game(map, players);
}

LocalPlayer* Matchmaker::makeLocalPlayer(id_t pid, id_t tid, const Json::Value &player_def) {
  return new LocalPlayer(
     pid,
     tid,
     must_have_idx(player_def, "name").asString(),
     toVec3(must_have_idx(player_def, "color")));
}

void Matchmaker::registerListener(MatchmakerListener func) {
  matchmakerStatusCallback_ = func;
}
}  // rts
