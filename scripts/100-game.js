var Game = function () {
  var exports = {};

  exports.init = function (map_def, player_defs) {
    // Spawn map entities
    for (var i = 0; i < map_def.entities.length; i++) {
      var entity = map_def.entities[i];
      exports.spawnEntity(
        must_have_idx(entity, 'type'),
        must_have_idx(entity, 'params')
      );
    }

    // initialize players and teams
    for (var i = 0; i < player_defs.length; i++) {
      var player_def = player_defs[i];
      Players.playerInit(player_def);
    }
  };

  exports.update = function (player_inputs) {
    // reset state
    MessageHub.clearMessages();

    // issue player input
    for (var i = 0; i < player_inputs.length; i++) {
      var input = player_inputs[i];
      // clean up some input as entity expects
      if (input.target) {
        input.target = input.target.slice(0, 2);
      }

      // send to each ordered entity
      var eid_arr = must_have_idx(input, 'entity');
      for (var j = 0; j < eid_arr.length; j++) {
        var entity = GetEntity(eid_arr[j]);
        invariant(
          must_have_idx(input, 'from_pid') === entity.getPlayerID(),
          'can only recieve input from controlling player'
        );
        if (!entity) {
          continue;
        }
        entityHandleOrder(entity, input);
      }
    }

    // TODO update entities
  };

  exports.render = function () {
    // TODO render entities
    
    return {
      players: Players.getRequisitionCounts(),
      teams: Teams.getVictoryPoints(),
    };
  };

  // TODO(zack): make this a private helper when entity spawning is by message
  // returns the ID of the spawned entity
  exports.spawnEntity = function (name, params) {
    var entity = SpawnEntity(name, params);
    entityInit(entity, name, params);
    return entity.getID();
  };

  return exports;
}();
