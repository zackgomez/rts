#include "rts/Lobby.h"
#include <json/json.h>
#include "common/kissnet.h"
#include "common/FPSCalculator.h"
#include "common/NetConnection.h"
#include "common/ParamReader.h"
#include "rts/GameServer.h"
#include "rts/ResourceManager.h"

namespace rts {

void game_server_loop(Json::Value game_def, std::vector<NetConnectionPtr> connections) {
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

void lobby_main(std::string listen_port, size_t num_players, size_t num_dummy_players) {
  auto server_sock = kissnet::tcp_socket::create();
  server_sock->listen(listen_port, 11);

  const float starting_requisition = fltParam("global.startingRequisition");

  Json::Value player_defs(Json::arrayValue);
  id_t pid = STARTING_PID;
  int tid_offset = 0;

  Json::Value game_def;
  std::vector<NetConnectionPtr> conns;
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

  game_server_loop(game_def, conns);
}
};