#include "rts/Lobby.h"
#include <json/json.h>
#define BOOST_FILESYSTEM_NO_DEPRECATED
#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"
#include "common/kissnet.h"
#include "common/FPSCalculator.h"
#include "common/NetConnection.h"
#include "common/ParamReader.h"
#include "rts/GameServer.h"
#include "rts/ResourceManager.h"

namespace rts {

Json::Value get_map_definition(const std::string &map_name) {
  std::string full_path = "maps/" + map_name + ".map";
  std::ifstream file(full_path.c_str());
  if (!file) {
    throw new file_exception("Unable to open file " + full_path);
  }
  Json::Reader reader;
  Json::Value map_def;
  if (!reader.parse(file, map_def)) {
    LOG(FATAL) << "Cannot parse param file " << full_path << " : "
      << reader.getFormattedErrorMessages() << '\n';
    invariant_violation("Couldn't parse " + full_path);
  }
  return map_def;
  /*
  namespace fs = boost::filesystem;
  boost::system::error_code ec;
  fs::path maps_path("maps");
  auto status = fs::status(maps_path);
  invariant(
    fs::exists(status) && fs::is_directory(status),
    "missing maps directory");

  Json::Reader reader;
  auto it = fs::directory_iterator(maps_path);
  for ( ; it != fs::directory_iterator(); it++) {
    auto dir_ent = *it;
    if (fs::is_regular_file(dir_ent.status())) {
      auto filepath = dir_ent.path().filename();
      auto filename = filepath.string();
      // TODO(zack): some logging here
      if (filepath.extension() != ".map" || filename.empty() || filename[0] == '.') {
        continue;
      }
      std::ifstream f(dir_ent.path().c_str());
      if (!f) {
        continue;
      }
      Json::Value mapDef;
      if (!reader.parse(f, mapDef)) {
        LOG(FATAL) << "Cannot parse param file " << filename << " : "
          << reader.getFormattedErrorMessages() << '\n';
        invariant_violation("Couldn't parse " + dir_ent.path().string());
      }
      invariant(mapDef.isMember("name"), "map missing name");
      auto mapName = mapDef["name"].asString();
      LOG(INFO) << "Read map " << mapName << " from file " << filename << '\n';
      maps_[mapName] = mapDef;
    }
  }
   */
}

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
  game_def["map_def"] = get_map_definition("debugMap");
  game_def["vps_to_win"] = fltParam("global.pointsToWin");

  for (auto &pair : pid_to_conn) {
    auto personalized_game_def = game_def;
    personalized_game_def["local_player_id"] = toJson(pair.first);
    pair.second->sendPacket(personalized_game_def);
  }

  game_server_loop(game_def, conns);
}
};