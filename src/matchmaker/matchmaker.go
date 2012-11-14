package main

import (
	"net"
	"log"
	"bytes"
	"encoding/json"
	"encoding/binary"
)

const port = ":7788"

type Player struct {
	Name string
  Port string
	Conn *net.TCPConn
}

type GameInfo map[string]interface{}

func main() {
	laddr, err := net.ResolveTCPAddr("tcp", port)
	if (err != nil) {
		log.Println("error resolving tcp addr", port);
	}
	ln, err := net.ListenTCP("tcp", laddr)
	if (err != nil) {
		log.Println("Unable to listen on ", port)
		return;
	}

	log.Println("Listening on port", port);

	playerChan := make(chan Player)
	gameChan := make(chan GameInfo)
	go makeMatch(2, playerChan, gameChan)
	for {
		conn, err := ln.AcceptTCP()
		if err != nil {
			continue;
		}

		go handleConnection(conn, playerChan, gameChan)
	}
}

func makeMatch(numPlayers int, playerChan chan Player, gameChan chan GameInfo) {
	players := make([]Player, 0, numPlayers)

	// Loop forever
	for {
		// Wait for enough players to join
		for len(players) < numPlayers {
			player := <-playerChan
			players = append(players, player)
		}

		// Send match info
		for i, player := range(players) {
			// Fill up game info message
			var gameInfo = GameInfo {
				"type": "game_setup",
				"numPlayers": numPlayers,
				"mapName": "debugMap",
				"pid": uint64(100 + i),
				"tid": uint64(200 + i % 2), // alternate teams
			}
			ips := make([]string, 0, numPlayers - 1)
			for j := i + 1; j < numPlayers; j++ {
				ipstr := players[j].Conn.RemoteAddr().(*net.TCPAddr).IP.String() + ":" + players[j].Port
				log.Println(ipstr)
				ips = append(ips, ipstr)
			}
			gameInfo["ips"] = ips

			msg, err := json.Marshal(gameInfo)
			if err != nil {
				log.Println("Well fuck json encoding error", err)
				return
			}
			sendMessage(player.Conn, msg)
		}

		for _, player := range(players) {
			player.Conn.Close();
		}
		players = players[:0]
	}
}

func handleConnection(conn *net.TCPConn, playerChan chan Player, gameChan chan GameInfo) {
	log.Println("Got connection from", conn.RemoteAddr())

	buf, err := readMessage(conn)
	if err != nil {
		log.Println("unable to read message from", conn.RemoteAddr(), err);
	}
  log.Println("read buf", buf)

  // Parse message
	var player Player
	err = json.Unmarshal(buf, &player)
	if err != nil {
		log.Println("Unable to parse message from", conn.RemoteAddr(), "reason", err)
	}

  log.Println("read player info", player.Name, player.Port, conn.RemoteAddr())
  player.Conn = conn

  playerChan <- player

	gameInfo := <-gameChan
	log.Println("Got game info", gameInfo);
}

func readMessage(conn net.Conn) (buf []byte, err error) {
  var size uint32
  err = binary.Read(conn, binary.LittleEndian, &size)
  if err != nil {
    log.Println("Error reading header from", conn.RemoteAddr(), ":", err)
    return
  }
  buf = make([]byte, size)
	// TODO(zack) make this read the entire message...
  _, err = conn.Read(buf)
  if err != nil {
    log.Println("error reading payload of size", size, "from", conn.RemoteAddr())
  }

	return
}

func sendMessage(conn net.Conn, msg []byte) error {
	buf := new(bytes.Buffer)
  binary.Write(buf, binary.LittleEndian, uint32(len(msg)))
	buf.Write(msg)
	_, err := buf.WriteTo(conn)
	return err
}
