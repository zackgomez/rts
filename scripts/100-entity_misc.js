// This file contains the Effects, Actions, States and any associated
// helper functions.


// --
// -- Entity Helpers --
// --
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

// --
// -- Entity Effects --
// --
function makeHealingAura(radius, amount) {
  return function(entity) {
    entity.getNearbyEntities(radius, function (nearby_entity) {
      if (nearby_entity.getPlayerID() == entity.getPlayerID()) {
        SendMessage({
          to: nearby_entity.getID(),
          from: entity.getID(),
          type: "STAT",
          healing_rate: amount
        });
      }

      return true;
    });

    return true;
  }
}

function makeVpGenEffect(amount) {
  return function(entity) {
    if (entity.getTeamID() == NO_TEAM) {
      return true;
    }

    AddVPs(entity.getTeamID(), amount, entity.getID());
    return true;
  }
}

function makeReqGenEffect(amount) {
  return function(entity) {
    if (entity.getPlayerID() == NO_PLAYER) {
      return true;
    }

    AddRequisition(entity.getPlayerID(), amount, entity.getID());
    return true;
  }
}

// --
// -- Entity Actions --
// --
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
    if (entity.hasCooldown(this.cooldownName)) {
      return ActionStates.COOLDOWN;
    }
    return ActionStates.ENABLED;
  }

  this.exec = function (entity, target) {
    if (this.getState(entity) != ActionStates.ENABLED) {
      return;
    }
    Log('Teleporting to', target, 'Range:', this.range);
    entity.addCooldown(this.cooldownName, this.cooldown);
    entity.warpPosition(target);
  }
}

// --
// -- Entity States --
// --
function NullState(params) {
  this.update = function (entity) {
    return null;
  }
}

function UnitIdleState() {
  this.targetID = null;

  this.update = function (entity) {
    entity.remainStationary();

    var target = entityFindTarget(entity, this.targetID);
    this.targetID = target ? target.getID() : null;
    if (target) {
      entity.attack(target);
    }

    return null;
  }
}

function UnitMoveState(params) {
  this.target = params.target;
  this.update = function (entity) {
    if (entity.containsPoint(this.target)) {
      return new UnitIdleState({});
    }

    entity.moveTowards(this.target);
    return null;
  }
}

function UnitCaptureState(params) {
  this.targetID = params.target_id;

  this.update = function (entity) {
    var target = GetEntity(this.targetID);
    if (!target
        || !target.hasProperty(P_CAPPABLE)
        || target.getPlayerID() == entity.getPlayerID()) {
      return new UnitIdleState();
    }

    var dist = entity.distanceToEntity(target);
    if (dist < entity.captureRange_) {
      entity.remainStationary();
      SendMessage({
        to: target.getID(),
        from: entity.getID(),
        type: MessageTypes.CAPTURE,
        cap: 1,
      });
    } else {
      entity.moveTowards(target.getPosition2());
    }
    return null;
  }
}

function UnitAttackState(params) {
  this.targetID = params.target_id;

  this.update = function (entity) {
    var target = GetEntity(this.targetID);
    // TODO(zack): or if the target isn't visible
    if (!target
        || !target.hasProperty(P_TARGETABLE)
        || target.getTeamID() == entity.getTeamID()) {
      return new UnitIdleState();
    }

    entity.attack(target);
  }
}

function UnitAttackMoveState(params) {
  this.targetPos = params.target;
  this.targetID = null;

  this.update = function (entity) {
    var targetEnemy = entityFindTarget(entity, this.targetID);
    this.targetID = targetEnemy ? targetEnemy.getID() : null;
    Log('amove update');

    if (!targetEnemy) {
      // If no viable enemy, move towards target position
      if (entity.containsPoint(this.targetPos)) {
        return new UnitIdleState();
      }
      entity.moveTowards(this.targetPos);
    } else {
      // Pursue a target enemy
      entity.attack(targetEnemy);
    }

    return null;
  }
}

function LocationAbilityState(params) {
  this.target = params.target;
  this.action = params.action;
  this.update = function (entity) {
    if (entity.distanceToPoint(this.target) < this.action.range) {
      this.action.exec(entity, this.target);
      return new entity.defaultState_;
    }

    entity.moveTowards(this.target);
    return null;
  }
}

function ProjectileState(params) {
  this.targetID = params.target_id;
  this.damage = params.damage;

  this.update = function (entity) {
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

    entity.moveTowards(target.getPosition2());
    return null;
  }
}
