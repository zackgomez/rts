// This file contains the core entity logic

// --
// -- Entity Functions --
// --
function entityInit(entity, params) {
  entity.prodQueue_ = [];
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
	if (def.health) {
		entity.maxHealth_ = def.health;
		entity.health_ = entity.maxHealth_;
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
	if (def.effects) {
		entity.effects_ = EntityDefs[name].effects;
	}
	if (def.actions) {
		entity.actions_ = EntityDefs[name].actions;
	}

  entity.state_ = new entity.defaultState_(params);

  entity.hasCooldown = function (name) {
    return name in this.cooldowns_;
  }
  entity.addCooldown = function (name, t) {
    if (this.cooldowns_[name]) {
      this.cooldowns_[name] = Math.max(this.cooldowns_[name], t);
    } else {
      this.cooldowns_[name] = t;
    }
  }

  entity.attack = function (target) {
    if (!this.weapon_) {
      Log(this.getID(), 'Told to attack without weapon');
      return;
    }
    var weapon = this.weapon_;

    var dist = this.distanceToEntity(target);
    if (dist > weapon.range) {
      this.moveTowards(target.getPosition2());
    } else {
      this.remainStationary();
      if (!this.hasCooldown(weapon.cooldown_name)) {
        this.addCooldown(weapon.cooldown_name, weapon.cooldown);

        if (weapon.type == 'ranged') {
          var params = {
            pid: this.getPlayerID(),
            pos: this.getPosition2(),
            target_id: target.getID(),
            damage: weapon.damage,
          };
          SpawnEntity('projectile', params);
        } else {
          SendMessage({
            to: target.getID(),
            from: this.getID(),
            type: MessageTypes.ATTACK,
            damage: weapon.damage,
          });
        }
      }
    }
  }
}

function entityResetDeltas(entity) {
  entity.deltas = {
    capture: {},
    damage: 0,
    healing_rate: 0,
    vp_rate: 0,
    req_rate: 0,
  };
}

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

function entityResolve(entity, dt) {
  for (var cd in entity.cooldowns_) {
    entity.cooldowns_[cd] -= dt;
    if (entity.cooldowns_[cd] < 0.0) {
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
    AddVPs(entity.getTeamID(), amount, entity.getID());
  }

  // Health
  var healing = dt * entity.deltas.healing_rate;
  var health_delta = healing - entity.deltas.damage;
  entity.health_ = Math.min(entity.health_ + health_delta, entity.maxHealth_);
  if (entity.deltas.damage) {
    entity.onTookDamage();
  }
  if (entity.health_ <= 0.0) {
    entity.destroy();
  }

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

  if (entity.prodQueue_.length) {
    var prod = entity.prodQueue_[0];
    prod.t += dt;
    if (prod.t >= prod.endt) {
      SpawnEntity(
        prod.name,
        {
          pid: entity.getPlayerID(),
          pos: vecAdd(entity.getPosition2(), entity.getDirection()),
          angle: entity.getAngle()
        });
      entity.prodQueue_.pop();
    }
  }

  entity.setMaxSpeed(entity.maxSpeed_);
  entity.setSight(entity.sight_);

  // Resolved!
  entityResetDeltas(entity);
}

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
    if (!entity.maxHealth_) {
      Log('Entity without health received attack message');
      return;
    }
    if (msg.damage < 0) {
      Log('Ignoring attack with negative damage');
      return;
    }
    entity.deltas.damage += msg.damage;
  } else if (msg.type == MessageTypes.ADD_DELTA) {
    for (var name in msg.deltas) {
      entity.deltas[name] += msg.deltas[name];
    }
  } else {
    Log('Unknown message of type', msg.type);
  }
}

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

function entityHandleAction(entity, action_name, target) {
  action = entity.actions_[action_name];
  if (!action) {
    Log(entity.getID(), 'got unknown action', action_name);
  }
  if (action.targeting == TargetingTypes.NONE) {
    // TODO(zack): make this also a state
    action.exec(entity);
  } else if (action.targeting == TargetingTypes.LOCATION) {
    entity.state_ = new LocationAbilityState({
      target: target,
      action: action,
    });
  } else {
    Log(entity.getID(), 'unknown action type', action.type);
  }
}

function entityGetActions(entity) {
  if (!entity.actions_) {
    return [];
  }
  var actions = [];
  for (var action_name in entity.actions_) {
    var action = entity.actions_[action_name];
    actions.push({
      name: action_name,
      icon: action.icon,
      tooltip: action.tooltip,
      targeting: action.targeting ? action.targeting : TargetingTypes.NONE,
      range: action.range ? action.range : 0.0,
      state: action.getState(entity),
      // TODO(zack): this is hacky
      cooldown: action.getState(entity) == ActionStates.COOLDOWN
        ? 1 - entity.cooldowns_[action.cooldownName] / action.cooldown
        : 0.0,
    });
  }

  return actions;
}

function entityGetUIInfo(entity) {
  var ui_info = {};
  if (entity.hasProperty(P_CAPPABLE)) {
    if (entity.cappingPlayerID_) {
      ui_info.capture = [entity.capAmount_, 5.0];
    }
    return ui_info;
  }

  if (entity.prodQueue_.length) {
    var prod = entity.prodQueue_[0];
    ui_info.production = [prod.t, prod.endt];
  }

  if (entity.maxHealth_) {
    ui_info.health = [entity.health_, entity.maxHealth_];
  }

  return ui_info;
}
