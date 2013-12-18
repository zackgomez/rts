var _ = require('underscore');
var must_have_idx = require('must_have_idx');
var invariant = require('invariant').invariant;
var invariant_violation = require('invariant').invariant_violation;

var ActionStates = require('constants').ActionStates;
var DamageTypes = require('constants').DamageTypes;
var EntityProperties = require('constants').EntityProperties;
var EntityStatus = require('constants').EntityStatus;
var IDConst = require('constants').IDConst;
var MessageTypes = require('constants').MessageTypes;
var TargetingTypes = require('constants').TargetingTypes;

var Collision = require('Collision');
var EntityDefs = require('EntityDefs');
var EntityStates = require('EntityStates');
var MessageHub = require('MessageHub');
var Game = require('game');
var Vector = require('Vector');
var Weapons = require('Weapons');

var Entity = function (id, name, params) {
  var def = must_have_idx(EntityDefs, name);
  var entity = this;

  entity.id_ = id;
  entity.name_ = name;
  entity.def_ = def;
  entity.defaultState_ = EntityStates.NullState;
  entity.cooldowns_ = {};
  entity.retreat_ = false;
  entity.pid_ = params.pid || IDConst.NO_PLAYER;
  entity.properties_ = def.properties || [];
  entity.maxSpeed_ = def.speed || 0;
  entity.sight_ = def.sight || 0;
  entity.size_ = def.size || [0, 0];
  entity.height_ = def.height || 0;
  entity.pos_ = params.pos || [0, 0];
  entity.angle_ = params.angle || 0;
  entity.currentSpeed_ = 0;
  entity.visibilitySet_ = [];

  entity.resetDeltas();

  // TODO(zack): some kind of copy properties or something, this sucks
  if (def.default_state) {
    entity.defaultState_ = def.default_state;
  }
  if (def.getParts) {
    entity.parts_ = def.getParts();
  }
  if (def.mana) {
    entity.maxMana_ = def.mana;
    entity.mana_ = entity.maxMana_;
  }
  if (def.cap_time) {
    entity.capTime_ = def.cap_time;
    entity.cappingPlayerID_ = null;
    entity.capAmount_ = 0.0;
    entity.capResetDelay_ = 0;
  }
  if (def.capture_range) {
    entity.captureRange_ = def.capture_range;
  }
  entity.weapons_ = [];
  if (def.weapons) {
    for (var i = 0; i < def.weapons.length; i++) {
      entity.weapons_.push(Weapons.newWeapon(def.weapons[i]));
    }
  }
  entity.effects_ = {};
  if (def.getEffects) {
    entity.effects_ = def.getEffects(entity);
  }
  if (def.actions) {
    entity.actions_ = def.actions;
  }
  if (def.minimap_icon) {
    entity.minimap_icon_ = def.minimap_icon;
  }
  if (def.hotkey) {
    entity.hotkey_ = def.hotkey;
  }

  entity.state_ = new entity.defaultState_(params);
  // Holds the 'intent' of the entity w.r.t. movement for this frame
  entity.movementIntent_ = null;

  // Set some functions on the entity
  entity.getID = function () {
    return this.id_;
  };
  entity.getName = function () {
    return this.name_;
  };
  entity.getDefinition = function () {
    return this.def_;
  };
  entity.hasProperty = function (property) {
    // TODO(zack): optimize this...
    for (var i = 0; i < this.properties_.length; i++) {
      if (this.properties_[i] === property) {
        return true;
      }
    }
    return false;
  };
  entity.hasCooldown = function (name) {
    return name in this.cooldowns_;
  };
  entity.addCooldown = function (name, t) {
    if (this.cooldowns_[name]) {
      this.cooldowns_[name] = {
        t: Math.max(this.cooldowns_[name].t, t),
        maxt: Math.max(this.cooldowns_[name].maxt, t),
      };
    } else {
      this.cooldowns_[name] = {
        t: t,
        maxt: t,
      };
    }
  };
  entity.getCooldownPercent = function (name) {
    if (!(name in this.cooldowns_)) {
      return 0;
    }
    var cd = this.cooldowns_[name];
    return cd.t / cd.maxt;
  };

  entity.addEffect = function (name, effect) {
    this.effects_[name] = effect;
  };

  entity.getPlayerID = function () {
    return this.pid_;
  };
  entity.setPlayerID = function (pid) {
    this.pid_ = pid;
    return this;
  };
  entity.getTeamID = function () {
    var player_id = this.getPlayerID();
    var player = player_id ? Game.getPlayer(player_id) : null;
    return player ? player.getTeamID() : IDConst.NO_TEAM;
  };
  entity.getPosition2 = function () {
    return this.pos_;
  };
  entity.getSize = function () {
    return this.size_;
  };
  entity.getDirection = function () {
    return [
      Math.cos(this.angle_ * Math.PI / 180),
      Math.sin(this.angle_ * Math.PI / 180),
    ];
  };
  entity.getMovementIntent = function () {
    return this.movementIntent_;
  }
  entity.getSpeed = function () {
    return this.currentSpeed_;
  };
  entity.getHeight = function () {
    return this.height_;
  };
  entity.getAngle = function () {
    return this.angle_;
  };
  entity.getSight = function () {
    return this.sight_;
  };

  entity.getPart = function (name) {
    for (var i = 0; i < this.parts_.length; i++) {
      if (this.parts_[i].getName() === name) {
        return this.parts_[i];
      }
    }
    throw new Error('couldn\'t find part '+ name);
  };
  entity.startUpgrade = function (part_name, upgrade_name) {
    var player = Game.getPlayer(this.getPlayerID());
    var part = this.getPart(part_name);
    var upgrade = part.getAvailableUpgrades()[upgrade_name];
    if (!upgrade) {
      Log('asked to upgrade nonexistent upgrade:', upgrade_name);
      return;
    }
    var req = player.getRequisition();
    if (upgrade.req_cost < req) {
      player.addRequisition(-upgrade.req_cost);
      part.purchaseUpgrade(upgrade_name);
    } else {
      Log('not enough req to build', upgrade, 'on part', part);
    }
  };

  // Basic movement intent setters
  entity.remainStationary = function () {
    this.movementIntent_ = null;
    return this;
  };
  entity.turnTowards = function (pt) {
    this.movementIntent_ = {
      look_at: pt,
    };
    return this;
  };
  entity.moveTowards = function (pt) {
    this.movementIntent_ = {
      look_at: pt,
      move_towards: pt,
    };
    return this;
  };
  entity.warpPosition = function (pt) {
    this.movementIntent_ = {
      warp: pt,
    };
    return this;
  };

  // Find entities near the current one.
  // Calls the passed callback for each entity in range.
  // If the callback returns true, it will continue passing entities until
  // there are no more
  entity.getNearbyEntities = function (range, callback) {
    return Game.getNearbyVisibleEntities(
      entity.getPosition2(),
      range,
      entity.getPlayerID(),
      callback
    );
  };

  // @return Entity object or null if no target
  entity.findTarget = function (previous_target_id) {
    // Only looking for targetable entities belonging to enemy teams
    var is_viable_target = function (target) {
      return target.getPlayerID() != IDConst.NO_PLAYER &&
        target.getTeamID() != this.getTeamID() &&
        target.hasProperty(EntityProperties.P_TARGETABLE);
    }.bind(this);

    // Default to previous target
    if (previous_target_id) {
      var previous_target = Game.getVisibleEntity(previous_target_id);
      if (previous_target && is_viable_target(previous_target)) {
        return previous_target;
      }
    }

    // Search for closest entity in sight range
    var new_target = null;
    var best_dist = Infinity;
    this.getNearbyEntities(this.sight_*2, function (e) {
      if (!is_viable_target(e)) {
        return true;
      }
      var dist = this.distanceToEntity(e);
      if (dist < best_dist) {
        new_target = e;
        best_dist = dist;
      }
      return true;
    }.bind(this));

    return new_target;
  };


  // Chases after a target, attacking it whenever possible
  entity.pursue = function (target) {
    if (this.attack(target)) {
      this.remainStationary();
    } else {
      this.moveTowards(target.getPosition2());
    }
  };

  // entity.attack attacks a target if it can.  Checks range and weapon
  // cooldowns
  entity.attack = function (target) {
    if (!this.weapons_) {
      Log(this.getID(), 'Told to attack without weapon');
      return false;
    }
    var dist = this.distanceToEntity(target);

    for (var i = 0; i < this.weapons_.length; i++) {
      var weapon = this.weapons_[i];
      if (weapon.isEnabled(this) && weapon.getRange() > dist) {
        if (weapon.isReady(this)) {
          weapon.fire(entity, target.getID());
        }
        return true;
      }
    }

    return false;
  };

  entity.updatePartHealth = function (health_target, amount) {
    var modified_parts = [];
    if (health_target == DamageTypes.HEALTH_TARGET_AOE) {
      var i = 0;
      this.parts_.forEach(function (part) {
        if (part.getHealth() > 0) {
          part.addHealth(amount);
          modified_parts.push(i);
        }
        i++;
      });
    } else if (health_target == DamageTypes.HEALTH_TARGET_RANDOM) {
      // Pick the last part with health
      var candidate_parts = [];
      this.parts_.forEach(function (part) {
        if (part.getHealth() > 0) {
          candidate_parts.push(part);
        }
      });
      if (candidate_parts.length) {
        var part_idx = Math.floor(Math.random() * candidate_parts.length);
        var part = candidate_parts[part_idx];
        part.addHealth(amount);
        modified_parts.push(part_idx);
      }
    } else if (health_target == DamageTypes.HEALTH_TARGET_LOWEST) {
      var best_part = null;
      var best_idx = null;
      for (var i = 0; i < entity.parts_.length; i++) {
        var part = entity.parts_[i];
        var health = part.getHealth();
        if (health > 0 && (!best_part || health < best_part.getHealth())) {
          best_part = part;
          best_idx = i;
        }
      }
      if (best_part) {
        best_part.addHealth(amount);
        modified_parts.push(best_idx);
      }
    } else {
      invariant_violation('Unknown health target ' + health_target);
    }
    return modified_parts;
  };

  entity.containsPoint = function (p) {
    return Collision.pointInOBB2(
      p,
      this.getPosition2(),
      this.getSize(),
      this.getAngle()
    );
  };
  entity.distanceToPoint = function (pt) {
    return Vector.distance(pt, this.getPosition2());
  };
  entity.distanceToEntity = function (entity) {
    // TODO(zack): upgrade to point - obb test
    return Vector.distance(entity.getPosition2(), this.getPosition2());
  };

  entity.onEvent = function (name, params) {
    this.events_.push({
      name: name,
      params: params,
    });
  };
  entity.getEvents = function () {
    return this.events_ || [];
  };
  entity.clearEvents = function () {
    this.events_ = [];
  }

  return entity;
}

Entity.prototype.setVisibilitySet = function (set) {
  this.visibilitySet_ = set;
  return this;
}
Entity.prototype.getVisibilitySet = function () {
  return this.visibilitySet_;
}

// Helper function that clears out the deltas at the end of the resolve.
Entity.prototype.resetDeltas = function () {
  this.deltas = {
    should_destroy: false,
    capture: {},
    damage_list: [],
    healing_list: [],
    healing_rate: 0,
    vp_rate: 0,
    req_rate: 0,
    mana_regen_rate: 0,
    max_speed_percent: 1,
    damage_factor: 1,
  };
}

// Called once per tick.  Should not do any direct updates, should only set
// intents (like moveTowards, attack, etc) or send messages.
Entity.prototype.update = function (dt) {
  this.movementIntent_ = null;

  for (var ename in this.effects_) {
    var res = this.effects_[ename](this);
    if (!res) {
      delete this.effects_[ename];
    }
  }
  
  var new_state = this.state_.update(this);
  if (new_state) {
    this.state_ = new_state;
  }
}

// Called in a second round after all entities have been updated.  Should
// actually change positions, update values etc.  Do not send messages or
// interact with other entities.
Entity.prototype.resolve = function (dt) {
  // First handle messages this entity has received
  var messages = MessageHub.getMessagesForEntity(this.getID());
  for (var i = 0; i < messages.length; i++) {
    this.handleMessage(messages[i]);
  }

  if (this.deltas.should_destroy) {
    return EntityStatus.DEAD;
  }

  for (var cd in this.cooldowns_) {
    this.cooldowns_[cd].t -= dt;
    if (this.cooldowns_[cd].t < 0.0) {
      delete this.cooldowns_[cd];
    }
  }

  // Resources
  if (this.deltas.req_rate) {
    var req = dt * this.deltas.req_rate;
    MessageHub.sendMessage({
      to: this.getTeamID(),
      from: this.getID(),
      type: MessageTypes.ADD_REQUISITION,
      amount: req,
    });
  }
  if (this.deltas.vp_rate) {
    var vps = dt * this.deltas.vp_rate;
    MessageHub.sendMessage({
      to: IDConst.GAME_ID,
      from: this.getID(),
      tid: this.getTeamID(),
      type: MessageTypes.ADD_VPS,
      amount: vps,
    });
  }

  // Healing
  if (this.deltas.healing_rate) {
    this.deltas.healing_list.push({
      healing: dt * this.deltas.healing_rate,
      health_target: DamageTypes.HEALTH_TARGET_AOE,
    });
  }
  if (this.parts_) {
    this.deltas.healing_list.forEach(function (healing_obj) {
      this.updatePartHealth(healing_obj.health_target, healing_obj.healing);
    }.bind(this));
  }

  var ndamages = this.deltas.damage_list.length;
  if (this.parts_ && ndamages) {
    var nparts = this.parts_.length;
    for (var j = 0; j < ndamages; j++) {
      var damage_obj = this.deltas.damage_list[j];
      if (damage_obj.damage <= 0) {
        continue;
      }

      for (var cd_name in damage_obj.on_hit_cooldowns) {
        this.addCooldown(cd_name, damage_obj.on_hit_cooldowns[cd_name]);
      }

      var modified_parts = this.updatePartHealth(
        damage_obj.health_target,
        -damage_obj.damage
      );
      this.onEvent('on_damage', {
        amount: damage_obj.damage,
        parts: modified_parts
      });
    }

    var alive = false;
    this.parts_.forEach(function (part) {
      if (part.isAlive() > 0) {
        alive = true;
      }
    });
    if (!alive) {
      return EntityStatus.DEAD;
    }
  }

  // Mana
  var mana_delta = dt * this.deltas.mana_regen_rate;
  this.mana_ = Math.min(this.mana_ + mana_delta, this.maxMana_);

  // Capture
  var capture_values = this.deltas.capture;
  for (var pid in capture_values) {
    // object keys are strings, this value is an id (int)
    pid = Math.floor(pid);
    if (!this.cappingPlayerID_) {
      this.cappingPlayerID_ = pid;
    }
    if (this.cappingPlayerID_ == pid) {
      this.capAmount_ += dt * capture_values[pid];
      if (this.capAmount_ >= this.capTime_) {
        this.setPlayerID(this.cappingPlayerID_);
        this.cappingPlayerID_ = null;
      }
    }
  }
  if (Object.keys(capture_values).length === 0) {
    this.cappingPlayerID_ = null;
    this.capAmount_ = 0;
  }

  // Attributes
  var speed_modifier = this.deltas.max_speed_percent;
  this.currentSpeed_ = speed_modifier * this.maxSpeed_;

  // Resolved!
  this.resetDeltas();

  return EntityStatus.NORMAL;
}

// Called each time an entity receives a mesage from another entity.  You should
// not directly update values, only set intents and update deltas.
Entity.prototype.handleMessage = function (msg) {
  if (msg.type == MessageTypes.CAPTURE) {
    if (!this.capTime_) {
      Log('Uncappable entity received capture message');
      return;
    }
    var from_entity = Game.getEntity(msg.from);
    if (!from_entity) {
      Log('Received capture message from missing entity', msg.from);
      return;
    }
    var from_pid = from_entity.getPlayerID();
    var cur = this.deltas.capture[from_pid];
    if (cur) {
      this.deltas.capture[from_pid] += msg.cap;
    } else {
      this.deltas.capture[from_pid] = msg.cap;
    }
  } else if (msg.type == MessageTypes.ATTACK) {
    invariant(this.parts_, 'Entity without parts received attack message');
    invariant(
      msg.damage && msg.damage > 0,
      'expected positive damage in attack message'
    );
    invariant(msg.damage_type, 'missing damage type in attack message');
    invariant(msg.health_target, 'missing health target in attack message');
    this.deltas.damage_list.push({
      damage: msg.damage,
      health_target: msg.health_target,
      damage_type: msg.damage_type,
      on_hit_cooldowns: msg.on_hit_cooldowns,
    });
  } else if (msg.type == MessageTypes.HEAL) {
    invariant(
      msg.healing && msg.healing > 0,
      'entity without parts received heal message'
    );
    invariant(msg.health_target, 'missing health target in heal message');
    this.deltas.healing_list.push({
      healing: msg.healing,
      health_target: msg.health_target,
    });
  } else if (msg.type == MessageTypes.ADD_DELTA) {
    for (var name in msg.deltas) {
      this.deltas[name] += msg.deltas[name];
    }
  } else {
    Log('Unknown message of type', msg.type);
  }
}

// Handles an order from the player.  Called before update/resolve.
// Intentions only.
Entity.prototype.handleOrder = function (order) {
  var type = order.type;
  // Ignore orders when retreating
  if (this.retreat_) {
    Log(this.getID(), 'is ignoring order', type, 'in retreat');
    return;
  }

  if (type == 'ACTION') {
    var action_name = must_have_idx(order, 'action');
    var action_args = {}
    if (order.target) {
      action_args.target = order.target;
    }
    if (order.target_id) {
      action_args.target_id = order.target_id;
    }
    this.handleAction(action_name, action_args);
  } else if (type == 'MOVE' && this.hasProperty(EntityProperties.P_MOBILE)) {
    this.state_ = new EntityStates.UnitMoveState({
      target: order.target,
    });
  } else if (type == 'RETREAT' && this.hasProperty(EntityProperties.P_MOBILE)) {
    this.retreat_ = true;
    this.state_ = new EntityStates.RetreatState();
  } else if (type == 'STOP') {
    this.state_ = new EntityStates.UnitIdleState();
  } else if (type == 'CAPTURE' && this.hasProperty(EntityProperties.P_UNIT)) {
    if (!this.captureRange_) {
      Log('Entity without capturing ability told to cap!');
      return;
    }
    this.state_ = new EntityStates.UnitCaptureState({
      target_id: order.target_id,
    });
  } else if (type == 'HOLD') {
    this.state_ = new EntityStates.HoldPositionState();
  } else if (type == 'ATTACK') {
    if (order.target_id) {
      this.state_ = new EntityStates.UnitAttackState({
        target_id: order.target_id,
      });
    } else if (this.hasProperty(EntityProperties.P_MOBILE)) {
      this.state_ = new EntityStates.UnitAttackMoveState({
        target: order.target,
      });
    }
  } else if (type == 'UPGRADE') {
    this.startUpgrade(order.part, order.upgrade);
  } else {
    Log('Unknown order of type', type);
  }
}

// A special case of an order.  Actions are like teleport, or production
Entity.prototype.handleAction = function (action_name, args) {
  var action = this.actions_[action_name];
  if (!action) {
    Log(this.getID(), 'got unknown action', action_name);
    return;
  }
  if (action.getState(this) !== ActionStates.ENABLED) {
    Log(this.getID(), 'told to run unenabled action');
    return;
  }
  if (this.retreat_) {
    Log(this.getID(), 'told to execute action while in retreat');
    return;
  }
  if (action.targeting == TargetingTypes.NONE) {
    this.state_ = new EntityStates.UntargetedAbilityState({
      action: action,
      state: this.state_,
    });
  } else if (action.targeting == TargetingTypes.LOCATION) {
    this.state_ = new EntityStates.LocationAbilityState({
      target: args.target,
      action: action,
    });
  } else if (action.targeting == TargetingTypes.ENEMY) {
    this.state_ = new EntityStates.TargetedAbilityState({
      target_id: args.target_id,
      action: action,
    });
  } else if (action.targeting == TargetingTypes.ALLY) {
    this.state_ = new EntityStates.TargetedAbilityState({
      target_id: args.target_id,
      action: action,
    });
  } else if (action.targeting == TargetingTypes.PATHABLE) {
    this.state_ = new EntityStates.LocationAbilityState({
      target: args.target,
      action: action,
    });
  } else {
    Log(this.getID(), 'unknown action type', action.type);
  }
}


// Accessors


// Returns an array of actions and their associated state.  This is
// used by the UI to display the bottom bar.
Entity.prototype.getActions = function () {
  if (!this.actions_) {
    return [];
  }
  var actions = [];
  for (var action_name in this.actions_) {
    var action = this.actions_[action_name];
    actions.push({
      name: action_name,
      hotkey: action.params.hotkey,
      icon: action.getIcon(this),
      tooltip: action.getTooltip(this),
      targeting: action.targeting,
      range: action.getRange(this),
      radius: action.getRadius(this),
      state: action.getState(this),
      // TODO(zack): this is hacky, move this into the valued returned by the state
      cooldown: action.getState(this) == ActionStates.COOLDOWN ?
        1 - this.getCooldownPercent(action.params.cooldown_name) :
        0.0,
    });
  }

  return actions;
}


// Returns info about this entity, like health or mana.
// It is used by the UI to display this information
Entity.prototype.getUIInfo = function () {
  var ui_info = {};
  if (this.hasProperty(EntityProperties.P_CAPPABLE)) {
    if (this.cappingPlayerID_) {
      ui_info.capture = [this.capAmount_, 5.0];
      ui_info.cappingPlayerID = this.cappingPlayerID_;
    }
  }
  ui_info.pid = this.getPlayerID();
  ui_info.tid = this.getTeamID();
  
  ui_info.parts = [];
  if (this.parts_) {
    for (var i = 0; i < this.parts_.length; i++) {
      var upgrades = this.parts_[i].getAvailableUpgrades();
      upgrades = _.toArray(_.map(upgrades, function (upgrade, name) {
        upgrade.name = name;
        return upgrade;
      }));
      ui_info.parts.push({
        name: this.parts_[i].getName(),
				health: [
					this.parts_[i].getHealth(),
					this.parts_[i].getMaxHealth(),
				],
        tooltip: this.parts_[i].getTooltip(),
        upgrades: upgrades,
			});
    }
  }

  if (this.hotkey_) {
    ui_info.hotkey = this.hotkey_;
  }
  if (this.minimap_icon_) {
    ui_info.minimap_icon = this.minimap_icon_;
  }

  ui_info.retreat = this.retreat_;

  if (this.maxMana_) {
    ui_info.mana = [this.mana_, this.maxMana_];
  }

  ui_info.path = this.path_ || [];

  // TODO(zack): figure out if there is a better way
  ui_info.extra = {};
  if (this.getName() === "victory_point") {
    ui_info.extra["vp_status"] = {
      owner: this.getPlayerID(),
      capper: this.cappingPlayerID_,
    };
  }

  return ui_info;
}

module.exports = Entity;
