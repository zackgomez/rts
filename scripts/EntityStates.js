// This file contains the different states an Entity can be in
// A state is basically a behavior.  It only sets intent/sends messages.  It
// should not directly modify the passed entity.
//
// States have one required function
//
// state update(entity)
// it should return a new state if the state is to transition, and null
// otherwise
var invariant = require('invariant').invariant;
var EntityConst = require('EntityConst');
var EntityProperties = require('constants').EntityProperties;
var Game = require('game');
var MessageHub = require('MessageHub');
var MessageTypes = require('constants').MessageTypes;

function NullState(params) {
  this.update = function (entity) {
    return null;
  };
};
exports.NullState = NullState;

function UnitIdleState(params) {
  this.targetID = null;

  this.update = function (entity) {
    entity.remainStationary();

    var target = entity.findTarget(this.targetID);
    if (target) {
      entity.attack(target);
    }
    this.targetID = target ? target.getID() : null;

    return null;
  };
};
exports.UnitIdleState = UnitIdleState;

function UnitMoveState(params) {
  this.target = params.target;
  this.update = function (entity) {
    if (entity.containsPoint(this.target)) {
      return new UnitIdleState({});
    }

    entity.moveTowards(this.target);
    return null;
  };
};
exports.UnitMoveState = UnitMoveState;

function UnitCaptureState(params) {
  this.targetID = params.target_id;

  this.update = function (entity) {
    var target = Game.getEntity(this.targetID);
    if (!target ||
        !target.hasProperty(EntityProperties.P_CAPPABLE) ||
        target.getPlayerID() == entity.getPlayerID()) {
      return new UnitIdleState();
    }

    var dist = entity.distanceToEntity(target);
    if (dist < entity.captureRange_) {
      entity.remainStationary();
      MessageHub.sendMessage({
        to: target.getID(),
        from: entity.getID(),
        type: MessageTypes.CAPTURE,
        cap: 1,
      });
    } else {
      entity.moveTowards(target.getPosition2());
    }
    return null;
  };
};
exports.UnitCaptureState = UnitCaptureState;

function UnitAttackState(params) {
  this.targetID = params.target_id;

  this.update = function (entity) {
    var target = Game.getVisibleEntity(entity.getPlayerID(), this.targetID);
    // TODO(zack): or if the target isn't visible
    if (!target ||
        !target.hasProperty(EntityProperties.P_TARGETABLE) ||
        target.getTeamID() == entity.getTeamID()) {
      return new UnitIdleState();
    }

    entity.pursue(target);
  };
};
exports.UnitAttackState = UnitAttackState;

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
  };
};
exports.UnitAttackMoveState = UnitAttackMoveState;

function HoldPositionState() {
  this.targetID = null;

  this.update = function (entity) {
    entity.remainStationary();

    var targetEnemy = entity.findTarget(this.targetID);
    this.targetID = targetEnemy ? targetEnemy.getID() : null;
    if (targetEnemy) {
      entity.attack(targetEnemy);
    }
  };
};
exports.HoldPositionState = HoldPositionState;

function UntargetedAbilityState(params) {
  this.next_state = params.state;
  this.action = params.action;
  this.update = function (entity) {
    this.action.exec(entity);
    var next_state = this.next_state.update(entity);
    return next_state ? next_state : this.next_state;
  };
};
exports.UntargetedAbilityState = UntargetedAbilityState;

function LocationAbilityState(params) {
  this.target = params.target;
  this.action = params.action;
  this.update = function (entity) {
    if (entity.distanceToPoint(this.target) < this.action.getRange(entity)) {
      this.action.exec(entity, this.target);
      return new entity.defaultState_();
    }

    entity.moveTowards(this.target);
    return null;
  };
};
exports.LocationAbilityState = LocationAbilityState;

function TargetedAbilityState(params) {
  this.target_id = params.target_id;
  this.action = params.action;
  this.update = function (entity) {
    var target = Game.getVisibleEntity(entity.getPlayerID(), this.target_id);
    if (!target) {
      return new entity.defaultState_();
    }
    // TODO(zack): some checking of visibility here
    if (entity.distanceToEntity(target) < this.action.getRange(entity)) {
      this.action.exec(entity, this.target_id);
      return new entity.defaultState_();
    }

    entity.moveTowards(target.getPosition2()); return null;
  };
}
exports.TargetedAbilityState = TargetedAbilityState;

function RetreatState(params) {
  this.update = function (entity) {
    var player = Game.getPlayer(entity.getPlayerID());
    var retreat_point = player.getRetreatLocation();

    invariant(entity.retreat_, "Must be retreating while in RetreatState");

    var threshold = 0.1;
    if (entity.distanceToPoint(retreat_point) > threshold) {
      entity.deltas.max_speed_percent *= EntityConst.RetreatSpeedModifier;
      entity.deltas.damage_reduction *= EntityConst.RetreatDamageReduction;
      entity.moveTowards(retreat_point);
    } else {
      entity.retreat_ = false;
      return new entity.defaultState_();
    }

    return null;
  };
};
exports.RetreatState = RetreatState;

function ProjectileState(params) {
  this.targetID = params.target_id;
  this.damage = params.damage;
  this.damage_type = params.damage_type;
  this.health_target = params.health_target;
  this.on_hit_cooldowns = params.on_hit_cooldowns;

  this.update = function (entity) {
    // Entity doesn't need to be visible
    var target = Game.getEntity(this.targetID);
    if (!target) {
      entity.deltas.should_destroy = true;
      return null;
    }

    var threshold = 1.0;
    if (entity.distanceToEntity(target) < threshold) {
      MessageHub.sendMessage({
        to: this.targetID,
        from: entity.getID(),
        type: MessageTypes.ATTACK,
        damage: this.damage,
        damage_type: this.damage_type,
        health_target: this.health_target,
        on_hit_cooldowns: this.on_hit_cooldowns,
      });
      entity.deltas.should_destroy = true;
    }

    entity.moveTowards(target.getPosition2());
    return null;
  };
};
exports.ProjectileState = ProjectileState;
