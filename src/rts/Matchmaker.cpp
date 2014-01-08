#include "rts/Matchmaker.h"
#include <boost/algorithm/string.hpp>
#include <sstream>
#include "common/kissnet.h"
#include "common/NetConnection.h"
#include "common/ParamReader.h"
#include "common/util.h"
#include "rts/Game.h"
#include "rts/Lobby.h"
#include "rts/Player.h"
#include "rts/Map.h"

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
    //LOG(INFO) << "time to connect remaining: " << timeout << '\n';
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
const std::string Matchmaker::MODE_JOINGAME = "jg";
const std::string MODE_QUIT = "q";

Matchmaker::Matchmaker(const Json::Value &player)
  : playerDef_(player) {
}

Game* Matchmaker::waitGame() {
  auto lock = std::unique_lock<std::mutex>(workMutex_);
  doneCondVar_.wait(lock);

  if (mode_ == MODE_SINGLEPLAYER) {
    return doSinglePlayerSetup();
  } else if (mode_ == MODE_JOINGAME) {
    return doJoinGame();
  } else if (mode_ == MODE_QUIT) {
    throw matchmaker_quit_exception();
  } else {
    invariant_violation("Unknown matchmaking mode");
  }
}

void Matchmaker::signalReady(const std::string &mode) {
  invariant(mode == MODE_SINGLEPLAYER || mode == MODE_JOINGAME, "unknown matchmaking mode");
  mode_ = mode;
  doneCondVar_.notify_all();
}

void Matchmaker::signalStop() {
  mode_ = MODE_QUIT;
  doneCondVar_.notify_all();
}

Game* Matchmaker::connectToServer(const std::string &addr, const std::string &port) {
  NetConnectionPtr client_conn = attemptConnection(addr, port);
  matchmakerStatusCallback_("Connected to server at " + addr + " : " + port);

  // send the server our information
  client_conn->sendPacket(playerDef_);
  // read back the information necessary for the game
  auto&& game_def = client_conn->readNext();

  Map *map = new Map(must_have_idx(game_def, "map_def"));

  id_t local_pid = toID(must_have_idx(game_def, "local_player_id"));
  auto&& player_defs = must_have_idx(game_def, "player_defs");
  std::vector<Player*> players;
  for (auto&& player_def : player_defs) {
    id_t pid = toID(must_have_idx(player_def, "pid"));
    id_t tid = toID(must_have_idx(player_def, "tid"));
    auto name = must_have_idx(player_def, "name").asString();
    auto color = toVec3(must_have_idx(player_def, "color"));

    Player *p = nullptr;
    if (pid == local_pid) {
      p = new LocalPlayer(pid, tid, name, color);
    } else {
      p = new Player(pid, tid, name, color);
    }
    players.push_back(p);
  }


  auto action_func = [=](const Json::Value &v) {
    client_conn->sendPacket(v);
  };
  auto render_provider = [=]() -> Json::Value {
    // TODO(zack): handle exceptions here
    return client_conn->readNext();
  };

  return new Game(map, players, render_provider, action_func);
}

Game* Matchmaker::doSinglePlayerSetup() {
  auto port = strParam("local.matchmaker.port");

  std::thread lobby_thread(lobby_main, port, 1, 1);
  lobby_thread.detach();

  Game* game = connectToServer("localhost", port);

  return game;
}

Game* Matchmaker::doJoinGame() {
  auto host = strParam("local.matchmaker.host");
  auto port = strParam("local.matchmaker.port");
  Game* game = connectToServer(host, port);

  return game;
}

void Matchmaker::registerListener(MatchmakerListener func) {
  matchmakerStatusCallback_ = func;
}
}  // rts
