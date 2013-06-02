// -- Defines/Constants --
P_CAPPABLE = 815586235;
P_TARGETABLE = 463132888;
P_ACTOR = 913794634;
P_RENDERABLE = 565038773;
P_COLLIDABLE = 983556954;
P_MOBILE = 1122719651;
P_UNIT = 118468328;

NO_PLAYER = 0;
NO_ENTITY = 0;
NO_TEAM = 0;

MessageTypes = {
  ORDER: 'ORDER',
  ATTACK: 'ATTACK',
  CAPTURE: 'CAPTURE',
  ADD_STAT: 'STAT',
};

TargetingTypes = {
  NONE: 0,
  LOCATION: 1,
  ENTITY: 2,
};

ActionStates = {
  DISABLED: 0,
  ENABLED: 1,
  COOLDOWN: 2,
};

// -- Utility --
function vecAdd(v1, v2) {
  if (v1.length != v2.length) return undefined;

  var ret = [];
  for (var i = 0; i < v1.length; i++) {
    ret.push(v1[i] + v2[i]);
  }
  return ret;
}

// -- Entity Helpers --
// @return Entity object or null if no target
function entityFindTarget(entity, previous_target_id) {
  // Only looking for targetable entities belonging to enemy teams
  var is_viable_target = function (target) {
    // TODO(zack): only consider 'visible' enemies
    return target.getPlayerID() != NO_PLAYER
      && target.getTeamID() != entity.getTeamID()
      && target.hasProperty(P_TARGETABLE);
  };

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
  entity.getNearbyEntities(entity.sight_, function (e) {
    if (!is_viable_target(e)) {
      return true;
    }
    var dist = entity.distanceToEntity(e);
    if (dist < best_dist) {
      new_target = e;
      best_dist = dist;
    }
    return true;
  });

  return new_target;
}

// -- Entity Effects --
function makeHealingAura(radius, amount) {
  return function(entity, dt) {
    entity.getNearbyEntities(radius, function (nearby_entity) {
      if (nearby_entity.getPlayerID() == entity.getPlayerID()) {
        SendMessage({
          to: nearby_entity.getID(),
          from: entity.getID(),
          type: "STAT",
          healing: dt * amount
        });
      }

      return true;
    });

    return true;
  }
}

function makeVpGenEffect(amount) {
  return function(entity, dt) {
    if (entity.getTeamID() == NO_TEAM) {
      return true;
    }

    AddVPs(entity.getTeamID(), dt * amount, entity.getID());
    return true;
  }
}

function makeReqGenEffect(amount) {
  return function(entity, dt) {
    if (entity.getPlayerID() == NO_PLAYER) {
      return true;
    }

    var amount = 1.0;
    AddRequisition(entity.getPlayerID(), dt * amount, entity.getID());
    return true;
  }
}

// -- Entity Actions --
function ProductionAction(params) {
  this.prod_name = params.prod_name;
  this.time_cost = params.time_cost;
  this.req_cost = params.req_cost;
  this.icon = params.icon;
  this.targeting = TargetingTypes.NONE;
  this.range = 0;

  this.tooltip =  this.prod_name +
    '\nreq: ' + this.req_cost +
    '\ntime: ' + this.time_cost;

  this.getState = function (entity) {
    if (GetRequisition(entity.getPlayerID()) > this.req_cost) {
      return ActionStates.ENABLED;
    } else {
      return ActionStates.DISABLED;
    }
  }

  this.exec = function (entity, target) {
    if (this.getState(entity) != ActionStates.ENABLED) {
      return;
    }
    var prod = {
      name: this.prod_name,
      t: 0.0,
      endt: this.time_cost
    };
    entity.prodQueue_.push(prod);
    Log('Started production of', prod.name, 'for', prod.endt);
    AddRequisition(entity.getPlayerID(), -this.req_cost, entity.getID());
  }
}

function TeleportAction(params) {
  this.range = params.range;
  this.cooldown = params.cooldown;
  this.icon = 'melee_icon';
  this.tooltip = 'Teleport\nCooldown:'+this.cooldown;
  this.cooldownName = 'teleport';
  this.targeting = TargetingTypes.LOCATION;

  this.getState = function (entity) {
    if (this.cooldownName in entity.cooldowns_) {
      return ActionStates.COOLDOWN;
    }
    return ActionStates.ENABLED;
  }

  this.exec = function (entity, target) {
    if (this.getState(entity) != ActionStates.ENABLED) {
      return;
    }
    Log('Teleporting to', target, 'Range:', this.range);
    entity.cooldowns_[this.cooldownName] = this.cooldown;
    entity.warpPosition(target);
  }
}

// -- Entity States --
function NullState(params) {
  this.update = function (entity, dt) {
    return null;
  }
}

function UnitIdleState() {
  this.targetID = null;

  this.update = function (entity, dt) {
    entity.remainStationary();

    var target = entityFindTarget(entity, this.targetID);
    this.targetID = target ? target.getID() : null;
    if (target) {
      entity.attack(target, dt);
    }

    return null;
  }
}

function UnitMoveState(params) {
  this.target = params.target;
  this.update = function (entity, dt) {
    if (entity.containsPoint(this.target)) {
      return new UnitIdleState({});
    }

    entity.moveTowards(this.target, dt);
    return null;
  }
}

function UnitCaptureState(params) {
  this.targetID = params.target_id;

  this.update = function (entity, dt) {
    var target = GetEntity(this.targetID);
    if (!target
        || !target.hasProperty(P_CAPPABLE)
        || target.getPlayerID() == entity.getPlayerID()) {
      return new UnitIdleState();
    }

    var dist = entity.distanceToEntity(target);
    if (dist < entity.captureRange_) {
      SendMessage({
        to: target.getID(),
        from: entity.getID(),
        type: MessageTypes.CAPTURE,
        cap: dt,
      });
    } else {
      entity.moveTowards(target.getPosition2(), dt);
    }
    return null;
  }
}

function UnitAttackState(params) {
  this.targetID = params.target_id;

  this.update = function (entity, dt) {
    var target = GetEntity(this.targetID);
    // TODO(zack): or if the target isn't visible
    if (!target
        || !target.hasProperty(P_TARGETABLE)
        || target.getTeamID() == entity.getTeamID()) {
      return new UnitIdleState();
    }

    entity.attack(target, dt);
  }
}

function UnitAttackMoveState(params) {
  this.targetPos = params.target;
  this.targetID = null;

  this.update = function (entity, dt) {
    var targetEnemy = entityFindTarget(entity, this.targetID);
    this.targetID = targetEnemy ? targetEnemy.getID() : null;

    if (!targetEnemy) {
      // If no viable enemy, move towards target position
      if (entity.containsPoint(this.targetPos)) {
        return new UnitIdleState();
      }
      entity.moveTowards(this.targetPos, dt);
    } else {
      // Pursue a target enemy
      entity.attack(targetEnemy, dt);
    }

    return null;
  }
}

function LocationAbilityState(params) {
  this.target = params.target;
  this.action = params.action;
  this.update = function (entity, dt) {
    if (entity.distanceToPoint(this.target) < this.action.range) {
      this.action.exec(entity, this.target);
      return new entity.defaultState_;
    }

    entity.moveTowards(this.target, dt);
    return null;
  }
}

function ProjectileState(params) {
  this.targetID = params.target_id;
  this.damage = params.damage;

  this.update = function (entity, dt) {
    var target = GetEntity(this.targetID);
    if (!target) {
      entity.destroy();
      return null;
    }

    var threshold = 1.0;
    if (entity.distanceToEntity(target) < threshold) {
      SendMessage({
        to: this.targetID,
        from: entity.getID(),
        type: MessageTypes.ATTACK,
        damage: this.damage,
      });
      entity.destroy();
    }

    entity.moveTowards(target.getPosition2(), dt);
    return null;
  }
}

// -- Weapon Definitions --
var Weapons = {
  rifle: {
    type: 'ranged',
    range: 6.0,
    damage: 10.0,
    cooldown_name: 'rifle_cd',
    cooldown: 1.0,
  },
  advanced_melee: {
    type: 'melee',
    range: 1.0,
    damage: 6.0,
    cooldown_name: 'advanced_melee_cd',
    cooldown: 0.5,
  },
}

// -- Entity Definitions --
var EntityDefs = {
  unit: {
    properties: [
      P_ACTOR,
      P_UNIT,
      P_TARGETABLE,
      P_RENDERABLE,
      P_COLLIDABLE,
      P_MOBILE,
    ],
    default_state: UnitIdleState,
    sight: 8.0,
    health: 100.0,
    capture_range: 1.0,
    weapon: 'rifle',
  },
  melee_unit: {
    properties: [
      P_ACTOR,
      P_UNIT,
      P_TARGETABLE,
      P_RENDERABLE,
      P_COLLIDABLE,
      P_MOBILE,
    ],
    default_state: UnitIdleState,
    sight: 7.0,
    health: 50.0,
    capture_range: 1.0,
    weapon: 'advanced_melee',
    actions: {
      teleport: new TeleportAction({
        range: 6.0,
        cooldown: 2.0,
      }),
    },
  },
  building: {
    properties: [
      P_ACTOR,
      P_TARGETABLE,
      P_RENDERABLE,
      P_COLLIDABLE,
    ],
    sight: 5.0,
    health: 700.0,
    effects: {
      req_gen: makeReqGenEffect(1.0),
      base_healing: makeHealingAura(5.0, 5.0),
    },
    actions: {
      prod_ranged: new ProductionAction({
        prod_name: 'unit',
        req_cost: 100,
        time_cost: 5.0,
        icon: 'ranged_icon',
      }),
      prod_melee: new ProductionAction({
        prod_name: 'melee_unit',
        req_cost: 70,
        time_cost: 2.5,
        icon: 'melee_icon',
      }),
    },
  },
  victory_point: {
    properties: [
      P_ACTOR,
      P_CAPPABLE,
      P_RENDERABLE,
      P_COLLIDABLE,
    ],
    sight: 2.0,
    cap_time: 5.0,
    effects: {
      vp_gen: makeVpGenEffect(1.0),
    },
  },
  req_point: {
    properties: [
      P_ACTOR,
      P_CAPPABLE,
      P_RENDERABLE,
      P_COLLIDABLE,
    ],
    sight: 2.0,
    cap_time: 5.0,
    effects: {
      req_gen: makeReqGenEffect(1.0),
    },
  },
  projectile: {
    properties: [
      P_RENDERABLE,
      P_MOBILE,
    ],
    default_state: ProjectileState,
  },
}

// -- Entity Functions --
function entityInit(entity, params) {
  entity.prodQueue_ = [];
  entity.defaultState_ = NullState;
  entity.cooldowns_ = {};

  var name = entity.getName();
  // TODO(zack): some kind of copy properties or something, this sucks
  var def = EntityDefs[name];
  if (def) {
    if (def.properties) {
      for (var i = 0; i < def.properties.length; i++) {
        entity.setProperty(def.properties[i], true);
      }
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
    Log('No def for', name);
  }

  entity.state_ = new entity.defaultState_(params);

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
      if (!(weapon.cooldown_name in this.cooldowns_)) {
        this.cooldowns_[weapon.cooldown_name] = weapon.cooldown;

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
  if (entity.getPlayerID() !== NO_PLAYER || entity.capResetDelay_ > 1) {
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
    if (!entity.cappingPlayerID_
        || entity.cappingPlayerID_ === from_entity.getPlayerID()) {
      entity.capResetDelay_ = 0;
      entity.cappingPlayerID_ = from_entity.getPlayerID();
      entity.capAmount_ += msg.cap;
      if (entity.capAmount_ >= entity.capTime_) {
        entity.setPlayerID(entity.cappingPlayerID_);
      }
    }
  } else if (msg.type == MessageTypes.ATTACK) {
    if (!entity.maxHealth_) {
      Log('Entity without health received attack message');
      return;
    }
    entity.health_ -= msg.damage;
    if (entity.health_ <= 0.0) {
      entity.destroy();
    }
    // TODO(zack): Compute this in UIInfo/update
    entity.onTookDamage();
  } else if (msg.type == MessageTypes.ADD_STAT) {
    if (msg.healing) {
      entity.health_ = Math.min(
        entity.health_ + msg.healing,
        entity.maxHealth_);
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
