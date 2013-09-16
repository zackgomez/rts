var Game = function () {
  var exports = {};

  var entities = {}

  // returns the ID of the spawned entity
  var spawnEntity = function (name, params) {
    var entity = SpawnEntity(name, params);
    entityInit(entity, name, params);

    entities[entity.getID()] = entity;

    return entity.getID();
  };

  var handleMessages = function () {
    var messages = MessageHub.getMessagesForEntity(GAME_ID);
    // Spawn new entities
    for (var i = 0; i < messages.length; i++) {
      var message = messages[i];
      var type = must_have_idx(message, 'type');
      Log('got message', JSON.stringify(messages[i]));
      if (type === MessageTypes.SPAWN) {
        spawnEntity(
          must_have_idx(message, 'name'),
          must_have_idx(message, 'params')
        );
      } else {
        invariant_violation('Unable to handle message with type ' + type);
      }
    }

    var teams = Teams.getTeams();
    for (var tid in teams) {
      var messages = MessageHub.getMessagesForEntity(tid);
      teams[tid].update(messages);
    }
  };

  exports.getEntity = function (eid) {
    return entities[eid];
  };

  exports.init = function (map_def, player_defs) {
    // Spawn map entities
    for (var i = 0; i < map_def.entities.length; i++) {
      var entity = map_def.entities[i];
      spawnEntity(
        must_have_idx(entity, 'type'),
        must_have_idx(entity, 'params')
      );
    }

    // initialize players and teams
    for (var i = 0; i < player_defs.length; i++) {
      var player_def = player_defs[i];
      Players.playerInit(player_def);
    }

    handleMessages();
  };

  exports.update = function (player_inputs, dt) {
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
        var entity = Game.getEntity(eid_arr[j]);
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

    // update entities
    for (var eid in entities) {
      entityUpdate(entities[eid], dt);
    }

    // TODO(zack): remove this, and replace with 'state creation'
    // when the time comes
    // 'resolve' entities
    var eids_by_player = {};
    for (var eid in entities) {
      var entity = entities[eid];
      var status = entityResolve(entity, dt);
      if (status === EntityStatus.DEAD) {
        DestroyEntity(eid);
        delete entities[eid];
        continue;
      }

      var pid = entity.getPlayerID();
      if (!eids_by_player[pid]) {
        eids_by_player[pid] = {};
      }
      eids_by_player[pid][entity.getName()] = eid;
    }

    var players = Players.getPlayers();
    for (var pid in players) {
      players[pid].units = must_have_idx(eids_by_player, pid);
    }

    // spawn entities, handle resources, etc
    handleMessages();
  };

  exports.render = function () {
    // TODO render entities
    
    return {
      players: Players.getRequisitionCounts(),
      teams: Teams.getVictoryPoints(),
    };
  };

  return exports;
}();
