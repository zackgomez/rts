#include "rts/Matchmaker.h"
#include <boost/algorithm/string.hpp>
#include <sstream>
#include "common/kissnet.h"
#include "common/NetConnection.h"
#include "common/ParamReader.h"
#include "common/util.h"
#include "rts/Controller.h"
#include "rts/Player.h"
#include "rts/NetPlayer.h"
#include "rts/Renderer.h"

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
const std::string Matchmaker::MODE_MATCHMAKING = "mm";
const std::string MODE_QUIT = "q";

Matchmaker::Matchmaker(const Json::Value &player)
  : name_(player["name"].asString()),
    color_(toVec3(player["color"])),
    listenPort_(player["port"].asString()),
    pid_(NO_PLAYER),
    tid_(NO_TEAM),
    error_(false) {
}

std::vector<Player *> Matchmaker::waitPlayers() {
  auto lock = std::unique_lock<std::mutex>(workMutex_);
  playersReadyCondVar_.wait(lock);

  if (mode_ == MODE_SINGLEPLAYER) {
    return doDebugSetup();
  } else if (mode_ == MODE_MATCHMAKING) {
    return doServerSetup(
        strParam("local.matchmakingHost"),
        strParam("local.matchmakingPort"));
  } else if (mode_ == MODE_QUIT) {
    throw matchmaker_quit_exception();
  } else {
    invariant_violation("Unknown matchmaking mode");
  }
}

void Matchmaker::signalReady(const std::string &mode) {
  invariant(mode == MODE_SINGLEPLAYER || mode == MODE_MATCHMAKING,
      "unknown matchmaking mode");
  mode_ = mode;
  playersReadyCondVar_.notify_all();
}

void Matchmaker::signalStop() {
  mode_ = MODE_QUIT;
  playersReadyCondVar_.notify_all();
}

std::vector<Player *> Matchmaker::doDebugSetup() {
  pid_ = STARTING_PID;
  tid_ = STARTING_TID;

  mapName_ = "gg_map";
  mapName_ = "debugMap";

  makeLocalPlayer();

  DummyPlayer *dp = new DummyPlayer(STARTING_PID + 1, STARTING_TID + 1);
  players_.push_back(dp);

  return players_;
}

std::vector<Player *> Matchmaker::doDirectSetup(const std::string &ip) {
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
      std::bind(&Matchmaker::acceptConnections, this, listenPort_, 1));
  } else {
    worker = std::thread(
      std::bind(&Matchmaker::connectToPlayer, this, ip, listenPort_));
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

std::vector<Player *> Matchmaker::doServerSetup(
    const std::string &ip,
    const std::string &port) {
  matchmakerStatusCallback_(
    "Connecting to matchmaker at " + ip + ":" + port);
  // First connect to matchmaker
  NetConnectionPtr server;
  try {
    server = attemptConnection(ip, port);
  } catch (std::exception &e) {
    matchmakerStatusCallback_("Failed to connect to matchmaker.");
    return std::vector<Player *>();
  }
  matchmakerStatusCallback_("Connection to matchmaker successful.");
  
  std::string localAddr = server->getSocket()->getLocalAddr();
  std::string localPort = server->getSocket()->getLocalPort();

  LOG(INFO) << "Sending information to matchmaker\n";
  // Send out local matchmaking data
  Json::Value mmdata;
  mmdata["name"] = name_;
  mmdata["version"] = strParam("game.version");
  mmdata["localAddr"] = localAddr;
  server->sendPacket(mmdata);

  LOG(INFO) << "waiting for matchmaker responses...\n";
  Json::Value peers;
  // Wait for responses until we're ready
  while (pid_ == NO_PLAYER) {
    Json::Value msg = server->readNext();
    LOG(DEBUG) << "received from matchmaking server: " << msg;

    invariant(msg.isMember("type"), "Missing type from matchmaker message");
    if (msg["type"] == "game_setup") {
      invariant(msg.isMember("pid"), "Expected pid in game setup message");
      invariant(msg.isMember("tid"), "Expected tid in game setup message");
      invariant(msg.isMember("numPlayers"),
          "Expected numPlayers in game setup message");
      invariant(msg.isMember("peers"), "Expected ips in game setup message");
      invariant(msg.isMember("mapName"),
          "Expected mapName in game setup message");

      pid_ = toID(msg["pid"]);
      tid_ = toID(msg["tid"]);
      mapName_ = msg["mapName"].asString();
      numPlayers_ = msg["numPlayers"].asLargestUInt();
      peers = msg["peers"];
    }
  } 
  
  // Server no longer needed
  server->stop();

  // Use server assigned pid/tid to create local player
  makeLocalPlayer();

  // Get remaining players
  std::unique_lock<std::mutex> lock(playerMutex_);
  std::vector<std::thread> workers;
  double timeout = intParam("network.connectAttempts")
    * fltParam("network.connectInterval");
  // connect P2P to every peer
  for (const Json::Value& peer : peers) {
    // TODO(zack): error checking on these
    std::string publicaddr = peer["remoteAddr"].asString();
    std::string privateaddr = peer["localAddr"].asString();
    std::string connectport = peer["remotePort"].asString();
    std::string listenport = peer["localPort"].asString();
    LOG(INFO) << "Trying [" << publicaddr << "|" << privateaddr << "]:" << connectport
      << " from port " << listenport << '\n';
    workers.push_back(std::thread(std::bind(
      &Matchmaker::connectP2P,
      this,
      publicaddr,
      privateaddr,
      connectport,
      listenport,
      timeout)));
  }

  // Wait...
  std::stringstream ss;
  while (players_.size() < numPlayers_) {
    doneCondVar_.wait(lock);
    if (error_) {
      matchmakerStatusCallback_("Failed to connect to player.");
      throw matchmaker_exception("Error in server setup");
    }
    ss.clear();
    ss << "Player successfully added. (" << (int) players_.size() << ").";
    matchmakerStatusCallback_(ss.str());
  }

  // clean up
  for (auto &worker : workers) {
    worker.join();
  }

  // Done!
  return players_;
}

std::string Matchmaker::getMapName() const {
  invariant(!mapName_.empty(),
    "you must call a setup method before map name is available");
  return mapName_;
}
id_t Matchmaker::getLocalPlayerID() const {
  invariant(localPlayer_, "local player uninitialized");
  return localPlayer_->getPlayerID();
}

void Matchmaker::makeLocalPlayer() {
  localPlayer_ = new LocalPlayer(pid_, tid_, name_, color_);
  players_.push_back(localPlayer_);
}

void Matchmaker::connectToPlayer(
    const std::string &ip,
    const std::string &port) {
  try {
    NetConnectionPtr conn = attemptConnection(ip, port);
    NetPlayer *np = handshake(conn);

    std::unique_lock<std::mutex> lock(playerMutex_);
    players_.push_back(np);
    // lock release @ end of scope
  } catch (const matchmaker_exception &e) {
    LOG(ERROR) << "error connecting to remote player: '" << e.what() << "'\n";
    error_ = true;
  }
  doneCondVar_.notify_one();
}

void Matchmaker::acceptConnections(
    const std::string &port,
    int numConnections) {
  try {
    auto serv_sock = kissnet::tcp_socket::create();
    serv_sock->listen(port, 11);
    for (int i = 0; i < numConnections; i++) {
      auto sock = serv_sock->accept();
      NetConnectionPtr conn = NetConnectionPtr(new NetConnection(sock));
      NetPlayer *np = handshake(conn);

      std::unique_lock<std::mutex> lock(playerMutex_);
      players_.push_back(np);
    }
  } catch (const std::exception &e) {
    LOG(ERROR) << "error accepting connection: '" << e.what() << "'\n";
    error_ = true;
  }
  doneCondVar_.notify_one();
}

void Matchmaker::connectP2P(
    const std::string &publicAddr,
    const std::string &privateAddr,
    const std::string &connectPort,
    const std::string &listenPort,
    double timeout) {
  kissnet::socket_set sockset;

  // TODO(zack): don't keep these as variables, just let the sockset keep track
  auto public_connect_sock = kissnet::tcp_socket::create();
  public_connect_sock = kissnet::tcp_socket::create();
  public_connect_sock->bind(listenPort);
  public_connect_sock->nonblockingConnect(publicAddr, connectPort);
  sockset.add_write_socket(public_connect_sock);

  auto private_connect_sock = kissnet::tcp_socket::create();
  if (publicAddr != privateAddr) {
    private_connect_sock = kissnet::tcp_socket::create();
    private_connect_sock->bind(listenPort);
    private_connect_sock->nonblockingConnect(privateAddr, connectPort);
    sockset.add_write_socket(private_connect_sock);
  }

  auto listen_sock = kissnet::tcp_socket::create();
  listen_sock->listen(listenPort, 11);
  sockset.add_read_socket(listen_sock);

  kissnet::tcp_socket_ptr ret;
  while (!ret && timeout > 0) {
    auto socks = sockset.poll_sockets(timeout);
    bool delay = false;
    for (auto sock : socks) {
      if (sock == listen_sock) {
        ret = listen_sock->accept();
        LOG(DEBUG) << "got listen connect\n";
        break;
        // TODO(zack): error checking
      } else {
        int sockerr = sock->getError();
        if (!sockerr) {
          ret = sock;
          break;
        } else {
          LOG(DEBUG) << "SOCK ERROR: " << strerror(sockerr) << " || public " <<
              (sock == public_connect_sock) << '\n';
          sockset.remove_socket(sock);
          // A connect error occured, retry connect after short timeout
          delay = true;
          if (sock == public_connect_sock) {
            try {
              public_connect_sock = kissnet::tcp_socket::create();
              public_connect_sock->bind(listenPort);
              public_connect_sock->nonblockingConnect(
                  publicAddr,
                  connectPort);
              sockset.add_write_socket(public_connect_sock);
            } catch (kissnet::socket_exception &e) {
              sockset.remove_socket(public_connect_sock);
              LOG(DEBUG) << "Unable to retry connection: " << e.what();
            }
          } else if (sock == private_connect_sock) {
            try {
              private_connect_sock = kissnet::tcp_socket::create();
              private_connect_sock->bind(listenPort);
              private_connect_sock->nonblockingConnect(
                  privateAddr,
                  connectPort);
              sockset.add_write_socket(private_connect_sock);
            } catch (kissnet::socket_exception &e) {
              sockset.remove_socket(private_connect_sock);
              LOG(DEBUG) << "Unable to retry connection: " << e.what();
            }
          }
        }
      }
    }
    if (delay && !ret) {
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
      timeout -= 0.5;
    }
  }
  if (ret) {
    std::unique_lock<std::mutex> lock(playerMutex_);
    ret->setBlocking();
    LOG(INFO) << "Connected p2p to " << ret->getHostname() << ":"
        << ret->getPort() << '\n';
    auto conn = NetConnectionPtr(new NetConnection(ret));
    NetPlayer *np = handshake(conn);
    players_.push_back(np);
  } else {
    LOG(ERROR) << "unable to connect to " << publicAddr << '\n';
    error_ = true;
  }
  doneCondVar_.notify_one();
}

NetPlayer * Matchmaker::handshake(NetConnectionPtr conn) const {
  // Some cached params
  const std::string version = strParam("game.version");
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

  // throws on timeout
  size_t timeout = 1000 * fltParam("network.handshake.timeout");
  Json::Value msg = conn->readNext(timeout);
  LOG(INFO) << "Received handshake message " << msg << '\n';

  // TODO(zack): clean up this error checking code...
  if (!msg.isMember("type") || msg["type"] != "HANDSHAKE") {
    LOG(FATAL) << "Received unexpected message during handshake\n";
    throw matchmaker_exception("failed handshake");
  }
  if (!msg.isMember("version") || msg["version"] != version) {
    LOG(FATAL) << "Mismatched game version from connection "
        << "(theirs: " << msg["version"] << " ours: " << version << ")\n";
    throw matchmaker_exception("failed handshake");
  }
  if (!msg.isMember("param_checksum")
      || msg["param_checksum"].asUInt() != paramChecksum) {
    LOG(FATAL) << "Mismatched params file checksum (theirs: " << std::hex
        << msg["param_checksum"].asUInt() << " ours: " << paramChecksum
        << std::dec << ")\n";
    throw matchmaker_exception("failed handshake");
  }
  rts::id_t pid;
  if (!msg.isMember("pid") || !(pid = rts::assertPid(toID(msg["pid"])))) {
    LOG(FATAL) << "Missing pid from handshake message.\n";
    throw matchmaker_exception("failed handshake");
  }
  rts::id_t tid;
  if (!msg.isMember("tid") || !(tid = rts::assertTid(toID(msg["tid"])))) {
    LOG(FATAL) << "Missing tid from handshake message.\n";
    throw matchmaker_exception("failed handshake");
  }
  // TODO(zack): or isn't vec3/valid color
  if (!msg.isMember("color")) {
    LOG(FATAL) << "Missing color in handshake message.\n";
    throw matchmaker_exception("failed handshake");
  }
  if (!msg.isMember("name")) {
    LOG(FATAL) << "Missing name in handshake message.\n";
    throw matchmaker_exception("failed handshake");
  }

  // Success, make a new player
  glm::vec3 color = toVec3(msg["color"]);
  std::string name = msg["name"].asString();
  NetPlayer *ret = new NetPlayer(pid, tid, name, color, conn);
  ret->setLocalPlayer(localPlayer_->getPlayerID());
  return ret;
}

void Matchmaker::registerListener(MatchmakerListener func) {
  matchmakerStatusCallback_ = func;
}
}  // rts
