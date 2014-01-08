#include "rts/Lobby.h"
#include "common/kissnet.h"
#include "common/Logger.h"
#include "common/ParamReader.h"

int main(int argc, char **argv) {
  ParamReader::get()->loadFile("config.json");
  kissnet::init_networking();
  Logger::initLogger();
  
  auto port = strParam("local.matchmaker.port");
  rts::lobby_main(port, 2, 0);
}