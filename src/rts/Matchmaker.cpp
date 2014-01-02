#include "rts/Matchmaker.h"
#include <boost/algorithm/string.hpp>
#include <sstream>
#include "common/kissnet.h"
#include "common/FPSCalculator.h"
#include "common/NetConnection.h"
#include "common/ParamReader.h"
#include "common/util.h"
#include "rts/Game.h"
#include "rts/GameServer.h"
#include "rts/Player.h"
#include "rts/Map.h"
#include "rts/ResourceManager.h"

namespace rts {

void lobby_main(size_t num_players, kissnet::tcp_socket_ptr server_sock, std::vector<NetConnectionPtr> &ret) {
  while (ret.size() < num_players) {
    auto client_sock = server_sock->accept();
    NetConnectionPtr conn(new NetConnection(client_sock));
    ret.push_back(conn);
  }
}

Json::Value build_game_def(Map *map, std::vector<Player*> players) {
  Json::Value player_defs;
  const float starting_requisition = fltParam("global.startingRequisition");
  for (int i = 0; i < players.size(); i++) {
    auto *player = players[i];
    auto starting_location = map->getStartingLocation(i);
    Json::Value json_player_def;
    json_player_def["pid"] = toJson(player->getPlayerID());
    json_player_def["tid"] = toJson(player->getTeamID());
    json_player_def["starting_requisition"] = starting_requisition;
    json_player_def["starting_location"] = starting_location;
    player_defs[player_defs.size()] = json_player_def;
  }

  Json::Value game_def;
  game_def["player_defs"] = player_defs;
  game_def["map_def"] = map->getMapDefinition();
  game_def["vps_to_win"] = fltParam("global.pointsToWin");

  return game_def;
}

void game_server_thread(Json::Value game_def, std::vector<NetConnectionPtr> connections) {
  const float simrate = fltParam("game.simrate");
  const float simdt = 1.f / simrate;

  GameServer server;
  server.start(game_def);

  FPSCalculator updateTimer(10);
  Clock::time_point start = Clock::now();
	int tick_count = 0;
  while (server.isRunning()) {
    // TODO(zack): add actions here
    for (auto &&conn : connections) {
      auto actions = conn->drainQueue();
      for (auto&& action : actions) {
        server.addAction(action);
      }
    }

    auto json = server.update(simdt);

    for (auto&& conn : connections) {
      conn->sendPacket(json);
    }

    // handle framerate
		tick_count++;
		float delay = simdt * tick_count - Clock::secondsSince(start);
    std::chrono::milliseconds delayms(static_cast<int>(1000 * delay));
    std::this_thread::sleep_for(delayms);

    float fps = updateTimer.sample();
    if (fabs(fps - simrate) / simrate > 0.01) {
      LOG(WARNING) << "Simulation update rate off: "
        << fps << " / " << simrate << '\n';
    }
  }
}

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

  std::vector<NetConnectionPtr> server_conns;
  NetConnectionPtr client_conn;

  auto listen_port = strParam("local.player.port");
  auto server_sock = kissnet::tcp_socket::create();
  server_sock->listen(listen_port, 11);
  std::thread lobby_thread(lobby_main, 1, server_sock, std::ref(server_conns));
  client_conn = attemptConnection("127.0.0.1", listen_port);
  lobby_thread.join();
  invariant(server_conns.size() == 1, "should have 1 client attached to server");

  auto action_func = [=](const Json::Value &v) {
    client_conn->sendPacket(v);
  };
  auto render_provider = [=]() -> Json::Value {
    // TODO(zack): handle exceptions here
    return client_conn->readNext();
  };

  auto game_def = build_game_def(map, players);
  std::thread server_thread(game_server_thread, game_def, server_conns);
  server_thread.detach();

  return new Game(map, players, render_provider, action_func);
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
