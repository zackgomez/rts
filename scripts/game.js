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
var Visibility = require('Visibility');

var vps_to_win = null;

var elapsed_time = 0;
var running = false;
var entities = {};
var players = {};
var teams = {};
var dead_entities = [];
var last_id = IDConst.STARTING_EID;
var visibility_map = null;
var chats = [];
var extra_renders = [];

var last_update_dt = 0;

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

exports.init = function (game_def) {
  vps_to_win = must_have_idx(game_def, 'vps_to_win');
  var map_def = must_have_idx(game_def, 'map_def');
  // Spawn map entities
  for (var i = 0; i < map_def.entities.length; i++) {
    var entity = map_def.entities[i];
    spawnEntity(
      must_have_idx(entity, 'type'),
      must_have_idx(entity, 'params')
    );
  }

  // initialize players and teams
  var player_defs = must_have_idx(game_def, 'player_defs');
  for (var i = 0; i < player_defs.length; i++) {
    var player_def = player_defs[i];
    player_def.starting_location = map_def.starting_locations[i];
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

  extra_renders.push({
    type: 'start',
  });
  running = true;
};

var handle_player_input = function (input) {
  var type = must_have_idx(input, 'type');
  var from_pid = must_have_idx(input, 'from_pid');
  if (type == 'ORDER') {
    var order = must_have_idx(input, 'order');
    if (order.target) {
      order.target = order.target.slice(0, 2);
    }

    // send to each ordered entity
    var id_arr = must_have_idx(order, 'entity');
    for (var j = 0; j < id_arr.length; j++) {
      var entity = entities[id_arr[j]];
      if (!entity) {
        Log('message to unknown entity', id_arr[j]);
        continue;
      }
      invariant(
        from_pid === entity.getPlayerID(),
        'can only recieve input from controlling player'
      );
      entity.handleOrder(order);
    }
  } else if (type == 'LEAVE_GAME') {
    if (running) {
      extra_renders.push({
        type: "game_over",
        winning_team: IDConst.NO_TEAM,
      });
    }
    running = false;
  } else if (type == 'CHAT') {
    chats.push({
      pid: from_pid,
      msg: must_have_idx(input, 'msg'),
    });
  } else {
    Log('Warning got action of unknown type:', type);
  }
};

exports.update = function (player_inputs, dt) {
  last_update_dt = dt;
  if (!running) {
    return false;
  }
  // reset state
  MessageHub.clearMessages();

  // issue player input
  _.each(player_inputs, handle_player_input);

  // update entities
  for (var eid in entities) {
    entities[eid].update(dt);
  }


  // TODO(zack): remove this, and replace with 'state creation'
  // when the time comes
  // 'resolve' entities
  var eids_by_player = {};
  _.each(_.keys(players), function (pid) {
    eids_by_player[pid] = {};
  });
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

  // check win condition
  _.some(teams, function (team) {
    // TODO(zack): vp victory amount is hardcoded here
    if (team.getVictoryPoints() > 500) {
      Log('Team ', team.getID(), ' has won');
      running = false;
      extra_renders.push({
        type: "game_over",
        winning_team: team.getID(),
      });
      return true;
    }
    return false;
  });

  elapsed_time += dt;

  return running;
};

var name_to_diff_func = {
  __default: function (t, prev, next) {
    if (prev === next) {
      return null;
    }
    return [[t, next]];
  },
};
var simplify_entity_renders = function (t, previous_renders, current_renders) {
  var simple_renders = {};
  _.each(current_renders, function (entity, id) {
    var previous_render = previous_renders[id];
    var simple_render = {};
    _.each(entity, function (value, name) {
      var differ = name_to_diff_func[name] || must_have_idx(name_to_diff_func, '__default');
      var diff = differ(
        t,
        previous_render ? previous_render[name] : null,
        value
      );
      // skip if value has no diff
      if (diff) {
        simple_render[name] = diff;
      }
    });
    simple_renders[id] = simple_render;
  });

  return simple_renders;
}

var previous_entity_renders = {};
exports.render = function () {
  var t = elapsed_time;
  var entity_renders = {};
  var events = [];

  _.each(dead_entities, function (entity) {
    entity_renders[entity.getID()] = {
      alive: false,
    };
  });
  dead_entities = [];

  for (var eid in entities) {
    var game_entity = entities[eid];
    var render = {
      alive: true,
    };

    var entity_def = game_entity.getDefinition();

    render.model = entity_def.model;
    render.properties = game_entity.getProperties();
    render.pid = game_entity.getPlayerID();
    render.tid = game_entity.getTeamID();

    var size2 = game_entity.getSize();
    var size3 = [size2[0], size2[1], game_entity.getHeight()];
    render.size = size3;

    render.sight = game_entity.getSight();

    render.pos = game_entity.getPosition2();
    render.angle = game_entity.getAngle();
    render.ui_info = game_entity.getUIInfo();
    render.actions = game_entity.getActions();
    render.visible = game_entity.getVisibilitySet();

    entity_renders[game_entity.getID()] = render;

    var entity_events = game_entity.getEvents();
    game_entity.clearEvents();
    for (var i = 0; i < entity_events.length; i++) {
      var event = entity_events[i];
      event.params.eid = eid;
      events.push(event);
    }
  }

  var vps = _.map(teams, function (team, tid) {
    return {
      tid: +tid,
      vps: team.getVictoryPoints(),
    };
  });
  var player_render = _.map(players, function (player, pid) {
    return {
      pid: +pid,
      req: player.getRequisition(),
      power: player.getPower(),
    };
  });

  var final_entity_renders = simplify_entity_renders(
    t,
    previous_entity_renders,
    entity_renders
  );
  previous_entity_renders = entity_renders;

  var full_render = {
    type: 'render',
    t: t,
    dt: last_update_dt,
    entities: final_entity_renders,
    events: events,
    players: player_render,
    teams: vps,
    chats: chats,
  };
  chats = [];

  var renders = extra_renders;
  extra_renders = [];
  renders.push(full_render);
  return JSON.stringify(renders);
};
