// This file contains the core entity logic

// --
// -- Entity Functions --
// --

// This function is called on an entity when it is created.
function entityInit(entity, name, params) {
  entity.name_ = name;
  entity.defaultState_ = NullState;
  entity.cooldowns_ = {};
  entity.retreat_ = false;
  entityResetDeltas(entity);
  var def = EntityDefs[name];
  if (!def) {
    throw new Error('No def for ' + name);
  }
  entity.pid_ = params.pid || NO_PLAYER;
  // TODO(zack): some kind of copy properties or something, this sucks
  if (def.properties) {
    for (var i = 0; i < def.properties.length; i++) {
      entity.setProperty(def.properties[i], true);
    }
  }
  if (def.model) {
    entity.setModel(def.model);
  }
  if (def.size) {
    entity.setSize(def.size);
  }
  if (def.speed) {
    entity.maxSpeed_ = def.speed;
  }
  if (def.sight) {
    entity.sight_ = def.sight;
  }
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

  // Set some functions on the entity
  entity.getName = function () {
    return this.name_;
  }
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
  }
  entity.setPlayerID = function (pid) {
    this.pid_ = pid;
    return this;
  }

  entity.getPart = function (name) {
    for (var i = 0; i < this.parts_.length; i++) {
      if (this.parts_[i].getName() === name) {
        return this.parts_[i];
      }
    }
    throw new Error('couldn\'t find part '+ name);
  };
  entity.startUpgrade = function (part_name, upgrade_name) {
    var player = Players.getPlayer(this.getPlayerID());
    var part = this.getPart(part_name);
    var upgrade = part.getAvailableUpgrades()[upgrade_name];
    var req = player.getRequisition();
    if (upgrade.req_cost < req) {
      player.addRequisition(-upgrade.req_cost);
      part.purchaseUpgrade(upgrade_name);
    } else {
      Log('not enough req to build', upgrade, 'on part', part);
    }
  };

  // Find entities near the current one.
  // Calls the passed callback for each entity in range.
  // If the callback returns true, it will continue passing entities until
  // there are no more
  entity.getNearbyEntities = function (range, callback) {
    GetNearbyEntities(entity.getPosition2(), range, function (eid) {
      return callback(Game.getEntity(eid));
    });
  };

  // @return Entity object or null if no target
  entity.findTarget = function (previous_target_id) {
    // Only looking for targetable entities belonging to enemy teams
    var is_viable_target = function (target) {
      // TODO(zack): only consider 'visible' enemies
      return target.getPlayerID() != NO_PLAYER &&
        target.getTeamID() != this.getTeamID() &&
        target.hasProperty(P_TARGETABLE) &&
        target.isVisibleTo(entity.getPlayerID());
    }.bind(this);

    // Default to previous target
    if (previous_target_id) {
      var previous_target = Game.getEntity(previous_target_id);
      if (previous_target && is_viable_target(previous_target)) {
        return previous_target;
      }
    }

    // Search for closest entity in sight range
    var new_target = null;
    var best_dist = Infinity;
    this.getNearbyEntities(this.sight_, function (e) {
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
    if (health_target == HEALTH_TARGET_AOE) {
      var i = 0;
      this.parts_.forEach(function (part) {
        if (part.getHealth() > 0) {
          part.addHealth(amount);
          modified_parts.push(i);
        }
        i++;
      });
    } else if (health_target == HEALTH_TARGET_RANDOM) {
      // Pick the last part with health
      var candidate_parts = [];
      this.parts_.forEach(function (part) {
        if (part.getHealth() > 0) {
          candidate_parts.push(part);
        }
      });
      if (candidate_parts.length) {
        var part_idx = Math.floor(GameRandom() * candidate_parts.length);
        var part = candidate_parts[part_idx];
        part.addHealth(amount);
        modified_parts.push(part_idx);
      }
    } else if (health_target == HEALTH_TARGET_LOWEST) {
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
}

// Helper function that clears out the deltas at the end of the resolve.
function entityResetDeltas(entity) {
  entity.deltas = {
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
function entityUpdate(entity, dt) {
  for (var ename in entity.effects_) {
    var res = entity.effects_[ename](entity);
    if (!res) {
      delete entity.effects_[ename];
    }
  }
  
  var new_state = entity.state_.update(entity);
  if (new_state) {
    entity.state_ = new_state;
  }
}

// Called in a second round after all entities have been updated.  Should
// actually change positions, update values etc.  Do not send messages or
// interact with other entities.
function entityResolve(entity, dt) {
  // First handle messages this entity has received
  var messages = MessageHub.getMessagesForEntity(entity.getID());
  for (var i = 0; i < messages.length; i++) {
    entityHandleMessage(entity, messages[i]);
  }

  for (var cd in entity.cooldowns_) {
    entity.cooldowns_[cd].t -= dt;
    if (entity.cooldowns_[cd].t < 0.0) {
      delete entity.cooldowns_[cd];
    }
  }

  // Resources
  if (entity.deltas.req_rate) {
    var req = dt * entity.deltas.req_rate;
    MessageHub.sendMessage({
      to: entity.getTeamID(),
      from: entity.getID(),
      type: MessageTypes.ADD_REQUISITION,
      amount: req,
    });
  }
  if (entity.deltas.vp_rate) {
    var vps = dt * entity.deltas.vp_rate;
    MessageHub.sendMessage({
      to: entity.getTeamID(),
      from: entity.getID(),
      type: MessageTypes.ADD_VPS,
      amount: vps,
    });
  }

  // Healing
  if (entity.deltas.healing_rate) {
    entity.deltas.healing_list.push({
      healing: dt * entity.deltas.healing_rate,
      health_target: HEALTH_TARGET_AOE,
    });
  }
  if (entity.parts_) {
    entity.deltas.healing_list.forEach(function (healing_obj) {
      entity.updatePartHealth(healing_obj.health_target, healing_obj.healing);
    });
  }

  var ndamages = entity.deltas.damage_list.length;
  if (entity.parts_ && ndamages) {
    var nparts = entity.parts_.length;
    for (var j = 0; j < ndamages; j++) {
      var damage_obj = entity.deltas.damage_list[j];
      if (damage_obj.damage <= 0) {
        continue;
      }

      for (var cd_name in damage_obj.on_hit_cooldowns) {
        entity.addCooldown(cd_name, damage_obj.on_hit_cooldowns[cd_name]);
      }

      var modified_parts = entity.updatePartHealth(
        damage_obj.health_target,
        -damage_obj.damage
      );
      entity.onEvent('on_damage', {
        amount: damage_obj.damage,
        parts: modified_parts
      });
    }

    var alive = false;
    entity.parts_.forEach(function (part) {
      if (part.isAlive() > 0) {
        alive = true;
      }
    });
    if (!alive) {
      return EntityStatus.DEAD;
    }
  }

  // Mana
  var mana_delta = dt * entity.deltas.mana_regen_rate;
  entity.mana_ = Math.min(entity.mana_ + mana_delta, entity.maxMana_);

  // Capture
  var capture_values = entity.deltas.capture;
  for (var pid in capture_values) {
    // object keys are strings, this value is an id (int)
    pid = Math.floor(pid);
    if (!entity.cappingPlayerID_) {
      entity.cappingPlayerID_ = pid;
    }
    if (entity.cappingPlayerID_ == pid) {
      entity.capAmount_ += dt * capture_values[pid];
      if (entity.capAmount_ >= entity.capTime_) {
        entity.setPlayerID(entity.cappingPlayerID_);
        entity.cappingPlayerID_ = null;
      }
    }
  }
  if (Object.keys(capture_values).length === 0) {
    entity.cappingPlayerID_ = null;
    entity.capAmount_ = 0;
  }

  // Attributes
  var speed_modifier = entity.deltas.max_speed_percent;
  entity.setMaxSpeed(speed_modifier * entity.maxSpeed_);
  entity.setSight(entity.sight_);

  // Resolved!
  entityResetDeltas(entity);

  return EntityStatus.NORMAL;
}

// Called each time an entity receives a mesage from another entity.  You should
// not directly update values, only set intents and update deltas.
function entityHandleMessage(entity, msg) {
  if (msg.type == MessageTypes.CAPTURE) {
    if (!entity.capTime_) {
      Log('Uncappable entity received capture message');
      return;
    }
    var from_entity = Game.getEntity(msg.from);
    if (!from_entity) {
      Log('Received capture message from missing entity', msg.from);
      return;
    }
    var from_pid = from_entity.getPlayerID();
    var cur = entity.deltas.capture[from_pid];
    if (cur) {
      entity.deltas.capture[from_pid] += msg.cap;
    } else {
      entity.deltas.capture[from_pid] = msg.cap;
    }
  } else if (msg.type == MessageTypes.ATTACK) {
    invariant(entity.parts_, 'Entity without parts received attack message');
    invariant(
      msg.damage && msg.damage > 0,
      'expected positive damage in attack message'
    );
    invariant(msg.damage_type, 'missing damage type in attack message');
    invariant(msg.health_target, 'missing health target in attack message');
    entity.deltas.damage_list.push({
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
    entity.deltas.healing_list.push({
      healing: msg.healing,
      health_target: msg.health_target,
    });
  } else if (msg.type == MessageTypes.ADD_DELTA) {
    for (var name in msg.deltas) {
      entity.deltas[name] += msg.deltas[name];
    }
  } else {
    Log('Unknown message of type', msg.type);
  }
}

// Handles an order from the player.  Called before update/resolve.
// Intentions only.
function entityHandleOrder(entity, order) {
  var type = order.type;
  // Ignore orders when retreating
  if (entity.retreat_) {
    Log(entity.getID(), 'is ignoring order', type, 'in retreat');
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
    entityHandleAction(entity, action_name, action_args);
  } else if (type == 'MOVE' && entity.hasProperty(P_MOBILE)) {
    entity.state_ = new UnitMoveState({
      target: order.target,
    });
  } else if (type == 'RETREAT' && entity.hasProperty(P_MOBILE)) {
    entity.retreat_ = true;
    entity.state_ = new RetreatState();
  } else if (type == 'STOP') {
    entity.state_ = new UnitIdleState();
  } else if (type == 'CAPTURE' && entity.hasProperty(P_UNIT)) {
    if (!entity.captureRange_) {
      Log('Entity without capturing ability told to cap!');
      return;
    }
    entity.state_ = new UnitCaptureState({
      target_id: order.target_id,
    });
  } else if (type == 'HOLD') {
    entity.state_ = new HoldPositionState();
  } else if (type == 'ATTACK') {
    if (order.target_id) {
      entity.state_ = new UnitAttackState({
        target_id: order.target_id,
      });
    } else if (entity.hasProperty(P_MOBILE)) {
      entity.state_ = new UnitAttackMoveState({
        target: order.target,
      });
    }
  } else if (type == 'UPGRADE') {
    entity.startUpgrade(order.part, order.upgrade);
  } else {
    Log('Unknown order of type', type);
  }
}

// A special case of an order.  Actions are like teleport, or production
function entityHandleAction(entity, action_name, args) {
  var action = entity.actions_[action_name];
  if (!action) {
    Log(entity.getID(), 'got unknown action', action_name);
    return;
  }
  if (action.getState(entity) !== ActionStates.ENABLED) {
    Log(entity.getID(), 'told to run unenabled action');
    return;
  }
  if (entity.retreat_) {
    Log(entity.getID(), 'told to execute action while in retreat');
    return;
  }
  if (action.targeting == TargetingTypes.NONE) {
    entity.state_ = new UntargetedAbilityState({
      action: action,
      state: entity.state_,
    });
  } else if (action.targeting == TargetingTypes.LOCATION) {
    entity.state_ = new LocationAbilityState({
      target: args.target,
      action: action,
    });
  } else if (action.targeting == TargetingTypes.ENEMY) {
    entity.state_ = new TargetedAbilityState({
      target_id: args.target_id,
      action: action,
    });
  } else if (action.targeting == TargetingTypes.ALLY) {
    entity.state_ = new TargetedAbilityState({
      target_id: args.target_id,
      action: action,
    });
  } else if (action.targeting == TargetingTypes.PATHABLE) {
    entity.state_ = new LocationAbilityState({
      target: args.target,
      action: action,
    });
  } else {
    Log(entity.getID(), 'unknown action type', action.type);
  }
}


// Accessors


// Returns an array of actions and their associated state.  This is
// used by the UI to display the bottom bar.
function entityGetActions(eid) {
  var entity = Game.getEntity(eid);
  invariant(entity, 'could not find entity for entityGetActions');
  if (!entity.actions_) {
    return [];
  }
  var actions = [];
  for (var action_name in entity.actions_) {
    var action = entity.actions_[action_name];
    actions.push({
      name: action_name,
      hotkey: action.params.hotkey,
      icon: action.getIcon(entity),
      tooltip: action.getTooltip(entity),
      targeting: action.targeting,
      range: action.getRange(entity),
      radius: action.getRadius(entity),
      state: action.getState(entity),
      // TODO(zack): this is hacky, move this into the valued returned by the state
      cooldown: action.getState(entity) == ActionStates.COOLDOWN ?
        1 - entity.getCooldownPercent(action.params.cooldown_name) :
        0.0,
    });
  }

  return actions;
}


// Returns info about this entity, like health or mana.
// It is used by the UI to display this information
function entityGetUIInfo(eid) {
  var entity = Game.getEntity(eid);
  invariant(entity, 'could not find entity for entityGetUIInfo');

  var ui_info = {};
  if (entity.hasProperty(P_CAPPABLE)) {
    if (entity.cappingPlayerID_) {
      ui_info.capture = [entity.capAmount_, 5.0];
      ui_info.cappingPlayerID = entity.cappingPlayerID_;
    }
  }
  ui_info.pid = entity.getPlayerID();
  
  ui_info.parts = [];
  if (entity.parts_) {
    for (var i = 0; i < entity.parts_.length; i++) {
      var upgrades = entity.parts_[i].getAvailableUpgrades();
      upgrades = _.toArray(_.map(upgrades, function (upgrade, name) {
        upgrade.name = name;
        return upgrade;
      }));
      ui_info.parts.push({
        name: entity.parts_[i].getName(),
				health: [
					entity.parts_[i].getHealth(),
					entity.parts_[i].getMaxHealth(),
				],
        tooltip: entity.parts_[i].getTooltip(),
        upgrades: upgrades,
			});
    }
  }

  if (entity.hotkey_) {
    ui_info.hotkey = entity.hotkey_;
  }
  if (entity.minimap_icon_) {
    ui_info.minimap_icon = entity.minimap_icon_;
  }

  ui_info.retreat = entity.retreat_;

  if (entity.maxMana_) {
    ui_info.mana = [entity.mana_, entity.maxMana_];
  }

  // TODO(zack): figure out if there is a better way
  ui_info.extra = {};
  if (entity.getName() === "victory_point") {
    ui_info.extra["vp_status"] = {
      owner: entity.getPlayerID(),
      capper: entity.cappingPlayerID_,
    };
  }

  return ui_info;
}
