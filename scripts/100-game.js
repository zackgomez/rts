var Game = function () {
  var exports = {};

  var entities = {};
  var eid_to_render_entity = {};
  var render_entities = {};
  var last_id = STARTING_EID;

  // returns the ID of the spawned entity
  var spawnEntity = function (name, params) {
    var id = last_id++;
    var entity = {};
    entityInit(entity, id, name, params);

    entities[entity.getID()] = entity;

    return entity.getID();
  };

  var handleMessages = function () {
    var messages = MessageHub.getMessagesForEntity(GAME_ID);
    // Spawn new entities
    for (var i = 0; i < messages.length; i++) {
      var message = messages[i];
      var type = must_have_idx(message, 'type');
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

  exports.getNearbyEntities = function (pos2, range, callback) {
    GetNearbyEntities(pos2, range, function (render_id) {
      var render_entity = render_entities[render_id];
      invariant(render_entity, "unknown render id passed to getNearbyEntities");
      var game_entity = entities[render_entity.eid];
      invariant(game_entity, "invalid entity for getNearbyEntities callback");
      return callback(game_entity);
    });
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
      var render_id_arr = must_have_idx(input, 'entity');
      for (var j = 0; j < render_id_arr.length; j++) {
        var render_entity = render_entities[render_id_arr[j]];
        if (!render_entity) {
          continue;
        }
        var entity = Game.getEntity(render_entity.eid);
        invariant(entity, "missing game entity for order");
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
        delete entities[eid];
        if (eid in eid_to_render_entity) {
          var render_entity = eid_to_render_entity[eid];
          DestroyRenderEntity(render_entity.getID());
          delete eid_to_render_entity[eid];
        }
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
    for (var eid in entities) {
      var game_entity = entities[eid];
      var render_entity = eid_to_render_entity[eid];
      if (!render_entity) {
        render_entity = SpawnRenderEntity();
        render_entity.eid = eid;
        eid_to_render_entity[eid] = render_entity;
        render_entities[render_entity.getID()] = render_entity;
      }

      var entity_def = game_entity.getDefinition();
      if (entity_def.model) {
        render_entity.setModel(entity_def.model);
      }
      if (entity_def.size) {
        render_entity.setSize(entity_def.size)
      }
      render_entity.setPosition2(game_entity.getPosition2());
      render_entity.setProperties(game_entity.properties_);
      render_entity.setMaxSpeed(game_entity.currentSpeed_);
      render_entity.setSight(game_entity.getSight());

      var ui_info = entityGetUIInfo(game_entity);
      render_entity.setUIInfo(ui_info);
      var actions = entityGetActions(game_entity);
      render_entity.setActions(actions);
    }
    
    return {
      players: Players.getRequisitionCounts(),
      teams: Teams.getVictoryPoints(),
    };
  };

  return exports;
}();
