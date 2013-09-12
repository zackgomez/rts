var Players = (function() {
  // API to return
  var PlayersAPI = {};

  // Private state
  var players = {};

  PlayersAPI.getPlayer = function(pid) {
    invariant(pid in players, 'missing player');
    return players[pid];
  };

  // Create a new player.
  // @param pid the player ID
  // @param start_def dictionary with keys for `pos` and `angle`
  PlayersAPI.playerInit = function (def) {
    var pid = def.pid;
    var tid = def.tid;
    var starting_def = def.starting_location;
    if (pid in players) {
      throw new Error('player '+pid+' already exists!');
    }

    Teams.addPlayer(tid, pid);

    MessageHub.sendMessage({
      to: GAME_ID,
      from: pid,
      type: MessageTypes.SPAWN,
      name: 'base',
      params: {
        pid: pid,
        pos: starting_def.pos,
        angle: starting_def.angle,
      },
    })

    var base_dir = [
      Math.sin(starting_def.angle),
      Math.cos(starting_def.angle),
    ];
    var retreat_location = vecAdd(
      starting_def.pos,
      vecMul(base_dir, 3)
    );

    MessageHub.sendMessage({
      to: GAME_ID,
      from: pid,
      type: MessageTypes.SPAWN,
      name: 'melee_unit',
      params: {
        pid: pid,
        pos: retreat_location,
        angle: starting_def.angle,
      },
    });
    var player = {
      retreat_location: retreat_location,
      units: {},
      requisition: def.starting_requisition,

      getRetreatLocation: function () {
        return retreat_location;
      },
      getRequisition: function () {
        return this.requisition;
      },
      addRequisition: function (amount) {
        this.requisition += amount;
      },
    };
    players[pid] = player;
  };

  PlayersAPI.playerUpdate = function (player) {
    for (var entity_name in player.units) {
      var entity = GetEntity(player.units[entity_name]);
      if (!entity) {
        delete player.units[entity_name];
      }
    }
  };

  PlayersAPI.updateAllPlayers = function () {
    // TODO(zack): update players.units here
    for (var pid in players) {
      PlayersAPI.playerUpdate(players[pid]);
    }
  };

  // Returns player -> requisition map
  PlayersAPI.getRequisitionCounts = function () {
    var ret = [];
    for (var pid in players) {
      ret.push({
        pid: pid,
        req: players[pid].getRequisition(),
      });
    }
    return ret;
  };

  return PlayersAPI;
})();
