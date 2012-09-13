#include "rts/Matchmaker.h"
#include "rts/Player.h"
#include "rts/NetPlayer.h"
#include "rts/Renderer.h"
#include "common/kissnet.h"
#include "common/NetConnection.h"
#include "common/ParamReader.h"
#include "common/util.h"

namespace rts {

// Tries a connection with some retries as per config
kissnet::tcp_socket_ptr attemptConnection(
    const std::string &ip,
    const std::string &port) {
  auto sock = kissnet::tcp_socket::create();

  for (int i = 0; i < intParam("network.connectAttempts"); i++) {
    try {
      LOG(DEBUG) << "Trying connection to " << ip << ":" << port << '\n';
      sock->connect(ip, port);
      return sock;
    } catch (const kissnet::socket_exception &e) {
      // Ignore exception, just try again
    }

    SDL_Delay(1000 * fltParam("network.connectInterval"));
  }

  throw matchmaker_exception("Unable to connect to remote host @ "+ip+":"+port);
}

Matchmaker::Matchmaker(const Json::Value &player, Renderer *renderer)
  : name_(player["name"].asString()),
    color_(toVec3(player["color"])),
    pid_(NO_PLAYER),
    tid_(NO_TEAM),
    renderer_(renderer),
    error_(false) {
}

std::vector<Player *> Matchmaker::doDebugSetup() {
  pid_ = STARTING_PID;
  tid_ = STARTING_TID;
  mapName_ = "debugMap";

  makeLocalPlayer();

  DummyPlayer *dp = new DummyPlayer(STARTING_PID + 1, STARTING_TID + 1);
  players_.push_back(dp);

  return players_;
}

std::vector<Player *> Matchmaker::doDirectSetup(
    const std::string &ip,
    const std::string &port) {
  pid_ = ip.empty() ? STARTING_PID : STARTING_PID + 1;
  tid_ = ip.empty() ? STARTING_TID : STARTING_TID + 1;
  numPlayers_ = 2;
  mapName_ = "debugMap";

  makeLocalPlayer();

  std::unique_lock<std::mutex> lock(playerMutex_);

  std::thread worker;

  // Hosting?
  if (ip.empty()) {
    worker = std::thread(
      std::bind(&Matchmaker::acceptConnections, this, port, 1));
  } else {
    worker = std::thread(
      std::bind(&Matchmaker::connectToPlayer, this, ip, port));
  }

  // wait until we're well and truly ready
  while (players_.size() < numPlayers_) {
    doneCondVar_.wait(lock);
    if (error_) {
      throw matchmaker_exception("Error in direct setup");
    }
  }

  worker.join();

  return players_;
}

std::string Matchmaker::getMapName() const {
  invariant(!mapName_.empty(),
    "you must call a setup method before map name is available");
  return mapName_;
}

void Matchmaker::makeLocalPlayer() {
  localPlayer_ = new LocalPlayer(pid_, tid_, name_, color_, renderer_);
  renderer_->setLocalPlayer(localPlayer_);
  players_.push_back(localPlayer_);
}

void Matchmaker::connectToPlayer(
    const std::string &ip,
    const std::string &port) {
  try {
    auto sock = attemptConnection(ip, port);
    NetConnection *conn = new NetConnection(sock);
    NetPlayer *np = handshake(conn);
    np->setLocalPlayer(localPlayer_->getPlayerID());

    std::unique_lock<std::mutex> lock(playerMutex_);
    players_.push_back(np);
    doneCondVar_.notify_one();
    // lock release @ end of scope
  } catch (const matchmaker_exception &e) {
    error_ = true;
  }
}

void Matchmaker::acceptConnections(
    const std::string &port,
    int numConnections) {
  auto serv_sock = kissnet::tcp_socket::create();
  serv_sock->listen(port, 11);
  for (int i = 0; i < numConnections; i++) {
    auto sock = serv_sock->accept();
    NetConnection *conn = new NetConnection(sock);
    NetPlayer *np = handshake(conn);
    np->setLocalPlayer(localPlayer_->getPlayerID());

    std::unique_lock<std::mutex> lock(playerMutex_);
    players_.push_back(np);
    doneCondVar_.notify_one();
  }
}

NetPlayer * Matchmaker::handshake(NetConnection *conn) const {
  // Some cached params
  const std::string version = strParam("game.version");
  const float maxT = fltParam("network.handshake.maxWait");
  const float interval = fltParam("network.handshake.checkInterval");
  const uint32_t paramChecksum = ParamReader::get()->getFileChecksum();

  // First send our message
  Json::Value v;
  v["type"] = "HANDSHAKE";
  v["version"] = version;
  v["pid"] = toJson(pid_);
  v["tid"] = toJson(tid_);
  v["color"] = toJson(color_);
  v["name"] = name_;
  v["param_checksum"] = paramChecksum;
  conn->sendPacket(v);

  // TODO(zack): use a blocking NetConnection method here
  std::queue<Json::Value> &queue = conn->getQueue();
  for (float t = 0.f; t <= maxT; t += interval) {
    if (!queue.empty()) {
      Json::Value msg = queue.front();
      queue.pop();

      LOG(INFO) << "Received handshake message " << msg << '\n';

      if (!msg.isMember("type") || msg["type"] != "HANDSHAKE") {
        LOG(FATAL) << "Received unexpected message during handshake\n";
        // Fail
        break;
      }

      if (!msg.isMember("version") || msg["version"] != version) {
        LOG(FATAL) << "Mismatched game version from connection "
          << "(theirs: " << msg["version"] << " ours: " << version << ")\n";
        // Fail
        break;
      }

      if (!msg.isMember("param_checksum")
          || msg["param_checksum"].asUInt() != paramChecksum) {
        LOG(FATAL) << "Mismatched params file checksum (theirs: " << std::hex
          << msg["param_checksum"].asUInt() << " ours: " << paramChecksum
          << std::dec << ")\n";
        // fail
        break;
      }

      rts::id_t pid;
      if (!msg.isMember("pid") || !(pid = rts::assertPid(toID(msg["pid"])))) {
        LOG(FATAL) << "Missing pid from handshake message.\n";
        // fail
        break;
      }

      rts::id_t tid;
      if (!msg.isMember("tid") || !(tid = rts::assertTid(toID(msg["tid"])))) {
        LOG(FATAL) << "Missing tid from handshake message.\n";
        // fail
        break;
      }

      glm::vec3 color;
      // TODO(zack): or isn't vec3/valid color
      if (!msg.isMember("color")) {
        LOG(FATAL) << "Missing color in handshake message.\n";
        // fail
        break;
      }
      color = toVec3(msg["color"]);

      std::string name;
      if (!msg.isMember("name")) {
        LOG(FATAL) << "Missing name in handshake message.\n";
        // fail
        break;
      }
      name = msg["name"].asString();

      // Success, make a new player
      return new NetPlayer(pid, tid, name, color, conn);
    }

    // No message, wait and check again
    SDL_Delay(1000 * interval);
  }

  // Didn't get the handshake message
  conn->stop();
  throw matchmaker_exception("Error while handshaking");
}
}  // rts
