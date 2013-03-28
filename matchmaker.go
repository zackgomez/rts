package main

import (
	"net"
  "fmt"
	"log"
	"bytes"
	"flag"
  "math/rand"
	"encoding/json"
	"encoding/binary"
)

type Player struct {
	Name string
  LocalAddr string
	Conn *net.TCPConn
  Ports []int
}

type GameInfo map[string]interface{}

func main() {
	var numPlayers int
	var mapName, port string
	flag.IntVar(&numPlayers, "n", 2, "number of players (2 - 4)")
	flag.StringVar(&port, "port", "11100", "port to listen on")
	flag.StringVar(&mapName, "map", "debugMap", "name of the map to create a match on")
	flag.Parse()

	if numPlayers < 2 || numPlayers > 4 {
		log.Println("Players must be between 2 and 4, value given", numPlayers)
		return
	}


	port = ":" + port
	laddr, err := net.ResolveTCPAddr("tcp", port)
	if (err != nil) {
		log.Println("error resolving tcp addr", port);
	}
	ln, err := net.ListenTCP("tcp", laddr)
	if (err != nil) {
		log.Println("Unable to listen on ", port)
		return
	}

	log.Println("Listening on port", port);

	playerChan := make(chan Player)
	gameChan := make(chan GameInfo)
	go makeMatch(numPlayers, mapName, playerChan, gameChan)
	for {
		conn, err := ln.AcceptTCP()
		if err != nil {
			continue
		}

		go handleConnection(conn, playerChan, gameChan)
	}
}

func makeMatch(numPlayers int, mapName string, playerChan chan Player, gameChan chan GameInfo) {
	players := make([]Player, 0, numPlayers)

	// Loop forever
	for {
		// Wait for enough players to join
		for len(players) < numPlayers {
			player := <-playerChan
			players = append(players, player)
		}

    // TODO(zack) move this to separate function, ensure uniqueness
    // Generate ports
    for key, player := range(players) {
      for i := 0; i < numPlayers - 1; i++ {
        player.Ports = append(player.Ports, 50000 + rand.Intn(5000))
      }
      players[key] = player
    }

		// Send match info
    remoteIdx := 0
		for i, player := range(players) {
			// Fill up game info message
			var gameInfo = GameInfo {
				"type": "game_setup",
				"numPlayers": numPlayers,
				"mapName": mapName,
				"pid": uint64(100 + i),
				"tid": uint64(200 + i % 2), // alternate teams
			}
			peers := make([]interface{}, 0, numPlayers - 1)
      portIdx := 0
			for j := 0; j < numPlayers; j++ {
        if j == i {
          if i != 0 {
            remoteIdx++
          }
          continue
        }
        remoteAddr, _, _ := net.SplitHostPort(players[j].Conn.RemoteAddr().String())
        localPort := player.Ports[portIdx]
        portIdx++
        remotePort := players[j].Ports[remoteIdx]
        peer := map[string]interface{} {
          "remoteAddr": remoteAddr,
          "localAddr": players[j].LocalAddr,
          // expects a string, w/e
          "localPort": fmt.Sprintf("%d", localPort),
          "remotePort": fmt.Sprintf("%d", remotePort),
        }
				peers = append(peers, peer)
			}
			gameInfo["peers"] = peers

			msg, err := json.Marshal(gameInfo)
			if err != nil {
				log.Println("Well fuck json encoding error", err)
				return
			}
			log.Println("Sending", string(msg), "to", player.Conn.RemoteAddr())
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

  // Parse message
	var player Player
	err = json.Unmarshal(buf, &player)
	if err != nil {
		log.Println("Unable to parse message from", conn.RemoteAddr(), "reason", err)
	}

  log.Println("read player info", player.Name, player.LocalAddr, conn.RemoteAddr())
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
