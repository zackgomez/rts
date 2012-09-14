#!/usr/bin/rdmd

import std.stdio;
import std.file;
import std.string;
import std.socket;
import std.conv;
import std.bitmanip;
import std.variant;
import std.json;
import json_helpers;

struct Player {
  Socket sock;
  ushort port;
  string name;
};

void main(string args[]) {
  uint nplayers = 2;
  ushort port = 7788;

  Socket listener = new TcpSocket;
  assert(listener.isAlive);
  listener.setOption(SocketOptionLevel.SOCKET, SocketOption.REUSEADDR, true);
  listener.bind(new InternetAddress(port));
  listener.listen(10);

  writefln("Listening on port %s", port);

  Socket[] connections;
  Player[] players;

  while (connections.length < nplayers) {
    Socket sock = listener.accept();
    writefln("Connection from %s established.", sock.remoteAddress().toString());
    connections ~= sock;

    auto msg = readPacket(sock);

    auto p = Player(sock, to!ushort(msg["port"].get!string), msg["name"].get!string);
    players ~= p;
  }

  string[][] ips;
  foreach (pi, player; players) {
    ips ~= new string[players.length - pi - 1];
    for (auto i = pi + 1; i < players.length; i++) {
      string addr = players[i].sock.remoteAddress().toAddrString();
      string addrport = addr ~ ":" ~ to!string(players[i].port);
      ips[$ - 1][i - pi - 1] ~= addrport;
      writefln("ips[%s][%s] = %s", ips.length - 1, ips[$-1].length - 1, ips[$-1][$-1]);
    }
  }

  string mapName = "debugMap";

  auto pid = 100;
  auto tid = 200;
  foreach (i, player; players) {
    JSONValue msg;
    msg.type = JSON_TYPE.OBJECT;
    msg.object["type"] = toJsonValue("game_setup");
    msg.object["pid"] = toJsonValue(pid++);
    msg.object["tid"] = toJsonValue(tid++);
    msg.object["ips"] = toJsonValue(ips[i]);
    msg.object["numPlayers"] = toJsonValue(players.length);
    msg.object["mapName"] = toJsonValue(mapName);

    writefln("here");

    string json = toJSON(&msg);
    writefln("message to %s(%s): '%s'", player.name, i, json);

    sendPacket(player.sock, json);
  }
}

void sendPacket(Socket sock, in string payload) {
  string msg;
  // 4 byte payload size, then payload
  msg ~= nativeToLittleEndian(to!uint(payload.length));
  msg ~= payload;
  auto bytes_sent = sock.send(msg);
  assert(bytes_sent == payload.length + 4);
}

Variant readPacket(Socket sock) {
  uint size;
  long bytes_read = sock.receive((&size)[0..1]);
  assert(bytes_read == 4);

  char buf[] = new char[size];
  bytes_read = sock.receive(buf);
  assert(bytes_read == size);
  auto strbuf = strip(to!string(buf[0 .. bytes_read]));
  writefln("Peer %s sent: '%s'", sock.remoteAddress().toString(), strbuf);

  return jsonToVariant(strbuf);
}
