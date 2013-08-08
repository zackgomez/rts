//
// Players API. 
//

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
  PlayersAPI.playerInit = function (pid, tid, starting_def) {
    if (players[pid]) {
      throw new Error('player '+pid+' already exists!');
    }

    Teams.addPlayer(tid, pid);

    var base_id = SpawnEntity(
      'base',
      {
        pid: pid,
        pos: starting_def.pos,
        angle: starting_def.angle,
      }
    );
    var base_entity = GetEntity(base_id);

    var retreat_location = vecAdd(
      base_entity.getPosition2(),
      vecMul(base_entity.getDirection(), 3)
    );

    var unit_id = SpawnEntity(
      'melee_unit',
      {
        pid: pid,
        pos: retreat_location,
        angle: base_entity.getAngle(),
      });
      var player =  {
        retreat_location: retreat_location,
        units: {
          base: base_id,
          melee_unit: unit_id,
        },
        getRetreatLocation: function () {
          return retreat_location;
        }
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
    for (var pid in players) {
      PlayersAPI.playerUpdate(players[pid]);
    }
  };

  return PlayersAPI;
})();
