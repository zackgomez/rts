var _ = require('underscore');
var must_have_idx = require('must_have_idx');
var object_fill_keys = require('object_fill_keys');
var invariant = require('invariant').invariant;
var invariant_violation = require('invariant').invariant_violation;

var constants = require('constants');
var IDConst = constants.IDConst;
var MessageTypes = constants.MessageTypes;
var EntityProperties = constants.EntityProperties;
var EntityStatus = constants.EntityStatus;

var Collision = require('Collision');
var Entity = require('Entity');
var MessageHub = require('MessageHub');
var Pathing = require('Pathing');
var Player = require('Player');
var Team = require('Team');
var Renderer = require('Renderer');
var Visibility = require('Visibility');


var elapsed_time = 0;
var entities = {};
var players = {};
var teams = {};
var dead_entities = [];
var eid_to_render_entity = {};
var last_id = IDConst.STARTING_EID;
var visibility_map = null;

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

exports.getVisibleEntity = function (pid, eid) {
  var e = this.getEntity(eid);
  if (!e) return e;
  return _.contains(e.getVisibilitySet(), pid) ? e : null;
}

exports.getPlayer = function (pid) {
  return must_have_idx(players, pid);
};

exports.getNearbyVisibleEntities = function (pos2, range, pid, callback) {
  this.getNearbyEntities(pos2, range, function (entity) {
    if (_.contains(entity.getVisibilitySet(), pid)) {
      return callback(entity);
    }
    return true;
  });
};

exports.getNearbyEntities = function (pos2, range, callback) {
  // use all to short circuit on the callback returning false
  _.all(entities, function (entity) {
    if (entity.distanceToPoint(pos2) < range) {
      return callback(entity);
    }
    return true;
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
    var player = new Player(player_def);
    players[player.getPlayerID()] = player;
  }

  // Initialize visibility map
  visibility_map = Visibility.VisibilityMap(map_def, player_defs.length);

  for (var pid in players) {
    var player = players[pid];
    var tid = player.getTeamID();

    var team = teams[tid] || new Team(tid);
    team.addPlayer(pid);
    teams[tid] = team;
  }

  _.each(players, function (player) {
    player.spawn();
  });

  handleMessages();

  this.render();
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
  var eids_by_player = object_fill_keys(_.keys(players), {});
  for (var eid in entities) {
    var entity = entities[eid];
    var status = entity.resolve(dt);
    if (status === EntityStatus.DEAD) {
      dead_entities.push(entities[eid]);
      delete entities[eid];
      continue;
    }

    var pid = entity.getPlayerID();
    if (pid !== IDConst.NO_PLAYER) {
      eids_by_player[pid][entity.getName()] = eid;
    }
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

  for (var pid in players) {
    players[pid].units = must_have_idx(eids_by_player, pid);
  }

  // spawn entities, handle resources, etc
  handleMessages();

  visibility_map.updateMap(entities);
  visibility_map.updateEntityVisibilities(entities);

  // TODO(zack): check win condition

  elapsed_time += dt;
};

exports.render = function () {
  _.each(dead_entities, function (entity) {
    var render_entity = eid_to_render_entity[entity.getID()];
    render_entity.setAlive(elapsed_time, false);
  });
  dead_entities = [];
  for (var eid in entities) {
    var game_entity = entities[eid];
    var render_entity = eid_to_render_entity[eid];
    if (!render_entity) {
      render_entity = Renderer.spawnEntity();
      render_entity.setAlive(elapsed_time, true);
      eid_to_render_entity[eid] = render_entity;
    }

    var render_id = render_entity.getID();
    var entity_def = game_entity.getDefinition();
    // non curve data
    render_entity.eid = eid;
    render_entity.setGameID(eid);
    if (entity_def.model) {
      render_entity.setModel(entity_def.model);
    }
    render_entity.setProperties(game_entity.properties_);

    var size2 = game_entity.getSize();
    var size3 = [size2[0], size2[1], game_entity.getHeight()];
    render_entity.setSize(elapsed_time, size3);
    render_entity.setSight(elapsed_time, game_entity.getSight());

    render_entity.setPosition2(elapsed_time, game_entity.getPosition2());
    render_entity.setAngle(elapsed_time, game_entity.getAngle());

    var ui_info = game_entity.getUIInfo();
    render_entity.setUIInfo(elapsed_time, ui_info);
    var actions = game_entity.getActions();
    render_entity.setActions(elapsed_time, actions);

    render_entity.setVisible(elapsed_time, game_entity.getVisibilitySet());

    var events = game_entity.getEvents(); 
    game_entity.clearEvents();
    for (var i = 0; i < events.length; i++) {
      var event = events[i];
      event.params.eid = render_id;
      Renderer.addEffect(event.name, event.params);
    }
  }

  var vps = _.map(teams, function (team, tid) {
    return {
      tid: tid,
      vps: team.getVictoryPoints(),
    };
  });
  var player_render = _.map(players, function (player, pid) {
    return {
      pid: pid,
      req: player.getRequisition(),
    };
  });

  return {
    players: player_render,
    teams: vps,
  };
};
