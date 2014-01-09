#include "rts/Lobby.h"
#include "common/kissnet.h"
#include "common/Logger.h"
#include "common/ParamReader.h"

int main(int argc, char **argv) {
  ParamReader::get()->loadFile("config.json");
  kissnet::init_networking();
  Logger::initLogger();

  // Defaults
  int num_players = 3;
  int num_bots = 0;
  std::string map_name = "gg";
  std::string port = strParam("local.matchmaker.port");

  std::vector<std::string> argvs(argc);
  for (int i = 0; i < argc; i++) {
    argvs[i] = argv[i];
  }

  rts::lobby_main(port, num_players, num_bots, map_name);
}