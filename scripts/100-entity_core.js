// This file contains the core entity logic

// --
// -- Entity Functions --
// --

// This function is called on an entity when it is created.
function entityInit(entity, params) {
  entity.defaultState_ = NullState;
  entity.cooldowns_ = {};
  entityResetDeltas(entity);
  var name = entity.getName();
  var def = EntityDefs[name];
  if (!def) {
    throw new Error('No def for ' + name);
  }
  // TODO(zack): some kind of copy properties or something, this sucks
  if (def.properties) {
    for (var i = 0; i < def.properties.length; i++) {
      entity.setProperty(def.properties[i], true);
    }
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
  if (def.weapon) {
    entity.weapon_ = Weapons[def.weapon];
  }
  entity.effects_ = {};
  if (def.getEffects) {
    entity.effects_ = def.getEffects(entity);
  }
  if (def.actions) {
    entity.actions_ = def.actions;
  }
  if (def.hotkey) {
    registerEntityHotkey(entity.getID(), def.hotkey);
    entity.hotkey_ = def.hotkey;
  }

  entity.state_ = new entity.defaultState_(params);

  // Set some functions on the entity
  entity.hasCooldown = function (name) {
    return name in this.cooldowns_;
  }
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
  }
  entity.getCooldownPercent = function (name) {
    if (!(name in this.cooldowns_)) {
      return 0;
    }
    var cd = this.cooldowns_[name];
    return cd.t / cd.maxt;
  }

  // Find entities near the current one.
  // Calls the passed callback for each entity in range.
  // If the callback returns true, it will continue passing entities until
  // there are no more
  entity.getNearbyEntities = function (range, callback) {
    GetNearbyEntities(entity.getPosition2(), range, callback);
  }

  // @return Entity object or null if no target
  entity.findTarget = function (previous_target_id) {
    // Only looking for targetable entities belonging to enemy teams
    var is_viable_target = function (target) {
      // TODO(zack): only consider 'visible' enemies
      return target.getPlayerID() != NO_PLAYER
        && target.getTeamID() != this.getTeamID()
        && target.hasProperty(P_TARGETABLE)
        && target.isVisibleTo(entity.getPlayerID());
    }.bind(this);

    // Default to previous target
    if (previous_target_id) {
      var previous_target = GetEntity(previous_target_id);
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
  }


  // Chases after a target, attacking it whenever possible
  entity.pursue = function (target) {
    if (this.attack(target)) {
      this.remainStationary();
    } else {
      this.moveTowards(target.getPosition2());
    }
  }

  // entity.attack attacks a target if it can.  Checks range and weapon
  // cooldowns
  entity.attack = function (target) {
    if (!this.weapon_) {
      Log(this.getID(), 'Told to attack without weapon');
      return false;
    }
    var weapon = this.weapon_;

    var dist = this.distanceToEntity(target);
    if (dist > weapon.range) {
      return false;
    }

    if (!this.hasCooldown(weapon.cooldown_name)) {
      this.addCooldown(weapon.cooldown_name, weapon.cooldown);

      if (weapon.damage_type == 'ranged') {
        var params = {
          pid: this.getPlayerID(),
          pos: this.getPosition2(),
          target_id: target.getID(),
          damage: weapon.damage,
          damage_type: weapon.damage_type,
          health_target: weapon.health_target,
        };
        SpawnEntity('projectile', params);
      } else {
        SendMessage({
          to: target.getID(),
          from: this.getID(),
          type: MessageTypes.ATTACK,
          damage: weapon.damage,
          damage_type: weapon.damage_type,
          health_target: weapon.health_target,
        });
      }
    }
    return true;
  }

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
  }
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
  };
}

// Called once per tick.  Should not do any direct updates, should only set
// intents (like moveTowards, attack, etc) or send messages.
function entityUpdate(entity, dt) {
  var new_state = entity.state_.update(entity);
  if (new_state) {
    entity.state_ = new_state;
  }

  for (var ename in entity.effects_) {
    var res = entity.effects_[ename](entity);
    if (!res) {
      delete entity.effects_[ename];
    }
  }
}

// Called in a second round after all entities have been updated.  Should
// actually change positions, update values etc.  Do not send messages or
// interact with other entities.
function entityResolve(entity, dt) {
  for (var cd in entity.cooldowns_) {
    entity.cooldowns_[cd].t -= dt;
    if (entity.cooldowns_[cd].t < 0.0) {
      delete entity.cooldowns_[cd];
    }
  }

  // Resources
  if (entity.deltas.req_rate) {
    var amount = dt * entity.deltas.req_rate;
    AddRequisition(entity.getPlayerID(), amount, entity.getID());
  }
  if (entity.deltas.vp_rate) {
    var amount = dt * entity.deltas.vp_rate;
    Teams.addVPs(entity.getTeamID(), amount, entity.getID());
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

      var modified_parts = entity.updatePartHealth(
        damage_obj.health_target,
        -damage_obj.damage
      );
      entity.addEffect('on_damage', {
        parts: modified_parts
      });
    }

    var alive = false;
    entity.parts_.forEach(function (part) {
      if (part.getHealth() > 0) {
        alive = true;
      }
    });
    if (!alive) {
      entity.destroy();
    }
  }

  // Mana
  var mana_delta = dt * entity.deltas.mana_regen_rate;
  entity.mana_ = Math.min(entity.mana_ + mana_delta, entity.maxMana_);

  // Capture
  var capture_values = entity.deltas.capture;
  for (var pid in capture_values) {
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
  if (Object.keys(capture_values).length == 0) {
    entity.cappingPlayerID_ = null;
    entity.capAmount_ = 0;
  }

  // Attributes
  entity.setMaxSpeed(entity.maxSpeed_);
  entity.setSight(entity.sight_);

  // Resolved!
  entityResetDeltas(entity);
}

// Called each time an entity receives a mesage from another entity.  You should
// not directly update values, only set intents and update deltas.
function entityHandleMessage(entity, msg) {
  if (msg.type == MessageTypes.CAPTURE) {
    if (!entity.capTime_) {
      Log('Uncappable entity received capture message');
      return;
    }
    var from_entity = GetEntity(msg.from);
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
  // the unit property basically means you can order it around
  if (!entity.hasProperty(P_UNIT)) {
    return;
  }

  // TODO(zack): replace if/else if/else block
  var type = order.type;
  if (type == 'MOVE') {
    entity.state_ = new UnitMoveState({
      target: order.target,
    });
  } else if (type == 'STOP') {
    entity.state_ = new UnitIdleState();
  } else if (type == 'CAPTURE') {
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
    } else {
      entity.state_ = new UnitAttackMoveState({
        target: order.target,
      });
    }
  } else {
    Log('Unknown order of type', type);
  }
}

// A special case of an order.  Actions are like teleport, or production
function entityHandleAction(entity, action_name, args) {
  action = entity.actions_[action_name];
  if (!action) {
    Log(entity.getID(), 'got unknown action', action_name);
    return;
  }
  if (action.getState(entity) !== ActionStates.ENABLED) {
    Log(entity.getID(), 'told to run unenabled action');
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
  } else {
    Log(entity.getID(), 'unknown action type', action.type);
  }
}


// Accessors


// Returns an array of actions and their associated state.  This is
// used by the UI to display the bottom bar.
function entityGetActions(entity) {
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
      state: action.getState(entity),
      // TODO(zack): this is hacky, move this into the valued returned by the state
      cooldown: action.getState(entity) == ActionStates.COOLDOWN
        ? 1 - entity.getCooldownPercent(action.params.cooldown_name)
        : 0.0,
    });
  }

  return actions;
}


// Returns info about this entity, like health or mana.
// It is used by the UI to display this information
function entityGetUIInfo(entity) {
  var ui_info = {};
  if (entity.hasProperty(P_CAPPABLE)) {
    if (entity.cappingPlayerID_) {
      ui_info.capture = [entity.capAmount_, 5.0];
      ui_info.cappingPlayerID = entity.cappingPlayerID_;
    }
    return ui_info;
  }
  
  ui_info.healths = [];
  if (entity.parts_) {
    for (var i = 0; i < entity.parts_.length; i++) {
      ui_info.healths.push([
          entity.parts_[i].getHealth(),
          entity.parts_[i].getMaxHealth(),
        ]);
    }
  }

  if (entity.hotkey_) {
    ui_info.hotkey = entity.hotkey_;
  }

  if (entity.maxMana_) {
    ui_info.mana = [entity.mana_, entity.maxMana_];
  }

  return ui_info;
}
