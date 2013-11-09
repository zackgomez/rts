var must_have_idx = require('must_have_idx');

var Entity = require('Entity');

var constants = require('constants');
var IDConst = constants.IDConst;
var MessageTypes = constants.MessageTypes;
var EntityStatus = constants.EntityStatus;

var entities = {};
var teams = {};
var eid_to_render_entity = {};
var last_id = IDConst.STARTING_EID;

// returns the ID of the spawned entity
var spawnEntity = function (name, params) {
  var id = last_id++;
  var entity = new Entity(id, name, params);

  entities[entity.getID()] = entity;

  return entity.getID();
};

var handleMessages = function () {
  var messages = MessageHub.getMessagesForEntity(IDConst.GAME_ID);

  var vp_changes = object_fill_keys(Object.keys(teams), 0);

  // Spawn new entities
  for (var i = 0; i < messages.length; i++) {
    var message = messages[i];
    var type = must_have_idx(message, 'type');
    if (type === MessageTypes.SPAWN) {
      spawnEntity(
        must_have_idx(message, 'name'),
        must_have_idx(message, 'params')
      );
    } else if (type == MessageTypes.ADD_VPS) {
      var tid = must_have_idx(message, 'tid')
      vp_changes[tid] += must_have_idx(message, 'amount');
    } else {
      invariant_violation('Unable to handle message with type ' + type);
    }
  }

  var min = _.reduce(
    vp_changes,
    function(min, num) {
      return Math.min(min, num);
    },
    Infinity
  );
  for (var tid in vp_changes) {
    vp_changes[tid] -= min;
  }

  for (var tid in teams) {
    var messages = MessageHub.getMessagesForEntity(tid);
    var team = teams[tid];
    team.update(messages);
    team.addVictoryPoints(vp_changes[tid]);
  }
};

exports.getEntity = function (eid) {
  return entities[eid];
};

exports.getNearbyEntities = function (pos2, range, callback) {
  GetNearbyEntities(pos2, range, function (id) {
    var game_entity = entities[id];
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

  var players = Players.getPlayers();
  for (var pid in players) {
    var player = players[pid];
    var tid = player.getTeamID();

    var team = teams[tid] || new Team(tid);
    team.addPlayer(pid);
    teams[tid] = team;
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
    var id_arr = must_have_idx(input, 'entity');
    for (var j = 0; j < id_arr.length; j++) {
      var entity = entities[id_arr[j]];
      if (!entity) {
        Log('message to unknown entity', id_arr[j]);
        continue;
      }
      invariant(
        must_have_idx(input, 'from_pid') === entity.getPlayerID(),
        'can only recieve input from controlling player'
      );
      entity.handleOrder(input);
    }
  }

  // update entities
  for (var eid in entities) {
    entities[eid].update(dt);
  }


  // TODO(zack): remove this, and replace with 'state creation'
  // when the time comes
  // 'resolve' entities
  var eids_by_player = {};
  for (var eid in entities) {
    var entity = entities[eid];
    var status = entity.resolve(dt);
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

  var new_bodies = Pathing.stepAllForward(entities, dt);
  for (var key in new_bodies) {
    var entity = must_have_idx(entities, key);
    var body = new_bodies[key];
    // HACK ALERT
    entity.pos_ = body.pos;
    entity.angle_ = body.angle;
    entity.path_ = body.path;
  }

  var players = Players.getPlayers();
  for (var pid in players) {
    players[pid].units = must_have_idx(eids_by_player, pid);
  }

  // spawn entities, handle resources, etc
  handleMessages();

  // TODO(zack): check win condition
};

exports.render = function () {
  for (var eid in entities) {
    var game_entity = entities[eid];
    var render_entity = eid_to_render_entity[eid];
    if (!render_entity) {
      render_entity = SpawnRenderEntity();
      eid_to_render_entity[eid] = render_entity;
    }

    var render_id = render_entity.getID();
    render_entity.eid = eid;
    render_entity.setGameID(eid);
    var entity_def = game_entity.getDefinition();
    if (entity_def.model) {
      render_entity.setModel(entity_def.model);
    }
    render_entity.setSize(game_entity.getSize());
    render_entity.setHeight(game_entity.getHeight());
    render_entity.setProperties(game_entity.properties_);
    render_entity.setSight(game_entity.getSight());

    // TODO(zack): have these do interpolation
    //render_entity.setMaxSpeed(game_entity.currentSpeed_);
    render_entity.setPosition2(game_entity.getPosition2());
    render_entity.setAngle(game_entity.getAngle());

    var ui_info = game_entity.getUIInfo();
    render_entity.setUIInfo(ui_info);
    var actions = game_entity.getActions();
    render_entity.setActions(actions);

    var events = game_entity.getEvents(); 
    game_entity.clearEvents();
    for (var i = 0; i < events.length; i++) {
      var event = events[i];
      event.params.eid = render_id;
      AddEffect(event.name, event.params);
    }
  }

  var vps = _.map(teams, function (team, tid) {
    return {
      tid: tid,
      vps: team.getVictoryPoints(),
    };
  });

  return {
    players: Players.getRequisitionCounts(),
    teams: vps,
  };
};