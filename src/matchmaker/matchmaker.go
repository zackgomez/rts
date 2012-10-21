package main

import (
	"net"
	"log"
	"encoding/json"
	"encoding/binary"
)

const port = ":7788"

type Player struct {
	Name string
  Port string
	Conn net.Conn
}

func main() {
	ln, err := net.Listen("tcp", port)
	if (err != nil) {
		log.Println("Unable to listen on ", port)
		return;
	}

	playerChan := make(chan Player)
	for {
		conn, err := ln.Accept()
		if err != nil {
			continue;
		}

		go handleConnection(conn, playerChan)
	}
}

func handleConnection(conn net.Conn, playerChan chan Player) {
	log.Println("Got connection from", conn.RemoteAddr())

  var size uint32
  err := binary.Read(conn, binary.LittleEndian, &size)
  if err != nil {
    log.Println("Error reading header from", conn.RemoteAddr(), ":", err)
    return
  }
  buf := make([]byte, size)
  _, err = conn.Read(buf)
  if err != nil {
    log.Println("error reading payload of size", size, "from", conn.RemoteAddr())
  }
  log.Println("read buf", buf)

  // Parse message
	var player Player
	err = json.Unmarshal(buf, &player)
	if err != nil {
		log.Println("Unable to read message from", conn.RemoteAddr(), "reason", err)
	}

  log.Println("read player info", player.Name, player.Port, conn.RemoteAddr())
  player.Conn = conn

  playerChan <- player
}
