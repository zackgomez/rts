// This file contains the different states an Entity can be in
// A state is basically a behavior.  It only sets intent/sends messages.  It
// should not directly modify the passed entity.
//
// States have one required function
//
// state update(entity)
// it should return a new state if the state is to transition, and null
// otherwise


function NullState(params) {
  this.update = function (entity) {
    return null;
  }
}

function UnitIdleState() {
  this.targetID = null;

  this.update = function (entity) {
    entity.remainStationary();

    var target = entity.findTarget(this.targetID);
    this.targetID = target ? target.getID() : null;
    if (target) {
      entity.pursue(target);
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

    entity.pursue(target);
  }
}

function UnitAttackMoveState(params) {
  this.targetPos = params.target;
  this.targetID = null;

  this.update = function (entity) {
    var targetEnemy = entity.findTarget(this.targetID);
    this.targetID = targetEnemy ? targetEnemy.getID() : null;

    if (!targetEnemy) {
      // If no viable enemy, move towards target position
      if (entity.containsPoint(this.targetPos)) {
        return new UnitIdleState();
      }
      entity.moveTowards(this.targetPos);
    } else {
      // Pursue a target enemy
      entity.pursue(targetEnemy);
    }

    return null;
  }
}

function HoldPositionState() {
  this.targetID = null;

  this.update = function (entity) {
    entity.remainStationary();

    var targetEnemy = entity.findTarget(this.targetID);
    this.targetID = targetEnemy ? targetEnemy.getID() : null;
    if (targetEnemy) {
      entity.attack(targetEnemy);
    }
  }
}

function UntargetedAbilityState(params) {
  this.next_state = params.state;
  this.action = params.action;
  this.update = function (entity) {
    this.action.exec(entity);
    var next_state = this.next_state.update(entity);
    return next_state ? next_state : this.next_state;
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

function TargetedAbilityState(params) {
  this.target_id = params.target_id;
  this.action = params.action;
  this.update = function (entity) {
    var target = GetEntity(this.target_id);
    if (!target) {
      return new entity.defaultState_;
    }
    // TODO(zack): some checking of visibility here
    if (entity.distanceToEntity(target) < this.action.range) {
      this.action.exec(entity, this.target_id);
      return new entity.defaultState_;
    }

    entity.moveTowards(target.getPosition3()); return null;
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