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

void lobby_main(std::string listen_port, size_t num_players, size_t num_dummy_players, std::vector<NetConnectionPtr> &conns, Json::Value &game_def) {
  auto server_sock = kissnet::tcp_socket::create();
  server_sock->listen(listen_port, 11);

  const float starting_requisition = fltParam("global.startingRequisition");

  Json::Value player_defs(Json::arrayValue);
  id_t pid = STARTING_PID;
  int tid_offset = 0;

  std::map<id_t, NetConnectionPtr> pid_to_conn;
  while (conns.size() < num_players) {
    auto client_sock = server_sock->accept();
    NetConnectionPtr conn(new NetConnection(client_sock));

    try {
      auto network_player_def = conn->readNext(500);

      Json::Value player_def;
      player_def["name"] = must_have_idx(network_player_def, "name");
      player_def["color"] = must_have_idx(network_player_def, "color");
      player_def["pid"] = toJson(pid);
      player_def["tid"] = toJson(STARTING_TID + tid_offset);
      player_def["starting_requisition"] = starting_requisition;

      player_defs.append(player_def);
      pid_to_conn[pid] = conn;
      conns.push_back(conn);

      tid_offset = (tid_offset + 1) % 2;
      pid++;
    } catch (std::exception &e) {
      conn->stop();
    }
  }

  // Add any dummy players
  for (int i = 0; i < num_dummy_players; i++) {
    Json::Value dummy_player_def;
    dummy_player_def["name"] = "dummy";
    dummy_player_def["color"] = toJson(vec3Param("colors.dummyPlayer"));
    dummy_player_def["pid"] = toJson(pid++);
    dummy_player_def["tid"] = toJson(STARTING_TID + tid_offset);
    dummy_player_def["starting_requisition"] = starting_requisition;
    tid_offset = (tid_offset + 1) % 2;
    player_defs.append(dummy_player_def);
  }

  game_def["player_defs"] = player_defs;
  game_def["map_def"] = ResourceManager::get()->getMapDefinition("debugMap");
  game_def["vps_to_win"] = fltParam("global.pointsToWin");

  for (auto &pair : pid_to_conn) {
    auto personalized_game_def = game_def;
    personalized_game_def["local_player_id"] = toJson(pair.first);
    pair.second->sendPacket(personalized_game_def);
  }
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

Game* Matchmaker::connectToServer(const std::string &addr, const std::string &port) {
  NetConnectionPtr client_conn = attemptConnection(addr, port);

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
  auto listen_port = strParam("local.player.port");

  std::vector<NetConnectionPtr> server_conns;
  Json::Value game_def;
  std::thread lobby_thread(lobby_main, listen_port, 1, 1, std::ref(server_conns), std::ref(game_def));

  Game* game = connectToServer("127.0.0.1", listen_port);

  lobby_thread.join();
  invariant(server_conns.size() == 1, "should have 1 client attached to server");

  std::thread server_thread(game_server_thread, game_def, server_conns);
  server_thread.detach();

  return game;
}

void Matchmaker::registerListener(MatchmakerListener func) {
  matchmakerStatusCallback_ = func;
}
}  // rts
