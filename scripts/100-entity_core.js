// This file contains the core entity logic

// --
// -- Entity Functions --
// --
function entityInit(entity, params) {
  entity.prodQueue_ = [];
  entity.defaultState_ = NullState;
  entity.cooldowns_ = {};

  entity.handleMessage = entityHandleMessage;

  var name = entity.getName();
  // TODO(zack): some kind of copy properties or something, this sucks
  var def = EntityDefs[name];
  if (def) {
    if (def.properties) {
      for (var i = 0; i < def.properties.length; i++) {
        entity.setProperty(def.properties[i], true);
      }
    }
    if (def.size) {
      entity.setSize(def.size);
    }
    if (def.speed) {
      entity.setMaxSpeed(def.speed);
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
  } else {
    throw new Error('No def for ' + name);
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

  entity.attack = function (target, dt) {
    if (!this.weapon_) {
      Log(this.getID(), 'Told to attack without weapon');
      return;
    }
    var weapon = this.weapon_;

    var dist = this.distanceToEntity(target);
    if (dist > weapon.range) {
      this.moveTowards(target.getPosition2(), dt);
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

function entityUpdate(entity, dt) {
  var new_state = entity.state_.update(entity, dt);
  if (new_state) {
    entity.state_ = new_state;
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

  entity.capResetDelay_ += 1;
  if (entity.capResetDelay_ > 1) {
    entity.capAmount_ = 0.0;
    entity.cappingPlayerID_ = null;
  }

  for (var cd in entity.cooldowns_) {
    entity.cooldowns_[cd] -= dt;
    if (entity.cooldowns_[cd] < 0.0) {
      delete entity.cooldowns_[cd];
    }
  }

  for (var ename in entity.effects_) {
    var res = entity.effects_[ename](entity, dt);
    if (!res) {
      delete entity.effects_[ename];
    }
  }
}

function entityHandleMessage(msg) {
  if (msg.type == MessageTypes.CAPTURE) {
    if (!this.capTime_) {
      Log('Uncappable entity received capture message');
      return;
    }
    var from_entity = GetEntity(msg.from);
    if (!from_entity) {
      Log('Received capture message from missing entity', msg.from);
      return;
    }
    if (!this.cappingPlayerID_
        || this.cappingPlayerID_ === from_entity.getPlayerID()) {
      this.capResetDelay_ = 0;
      this.cappingPlayerID_ = from_entity.getPlayerID();
      this.capAmount_ += msg.cap;
      if (this.capAmount_ >= this.capTime_) {
        this.setPlayerID(this.cappingPlayerID_);
      }
    }
  } else if (msg.type == MessageTypes.ATTACK) {
    if (!this.maxHealth_) {
      Log('Entity without health received attack message');
      return;
    }
    this.health_ -= msg.damage;
    if (this.health_ <= 0.0) {
      this.destroy();
    }
    // TODO(zack): Compute this in UIInfo/update
    this.onTookDamage();
  } else if (msg.type == MessageTypes.ADD_STAT) {
    if (msg.healing) {
      this.health_ = Math.min(
        this.health_ + msg.healing,
        this.maxHealth_);
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
    Log(order.target_id);
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
