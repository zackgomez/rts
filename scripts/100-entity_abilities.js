// This file contains the actions/abilties that entities can perform.
// These are not actions like attack/move/hold position but instead more
// advanced abilities like teleports or heals.
//
// An ability has a few required attributes
// targeting - a value in TargetingTypes
// icon - the name of a texture in config.json
// tooltip - a string describing the action, a newline acts as a line break
// range - the casting range of the ability (should be 0 for TargetingType.NONE)
//
// TODO(zack): make this (entity, params)
// exec(entity, target) - runs the action, action.getState is guaranteed
// getState(entity) - returns an ActionState
// getIcon(entity) - returns a string icon name
// getRange(entity) - returns a float range
// getTooltip(entity) - returns a string tooltip (lines separated with '\n')
// to be ENABLED

// This is the list of common boilerplate/helper methods for actions.
// It expects one thing, a this.params object with icon and perhaps
// range/cooldown parameters
var ActionPrototype = {
  getState: function (entity) {
    if (!this.hasResources(entity)) {
      return ActionStates.DISABLED;
    } else if (this.params.cooldown_name &&
               entity.hasCooldown(this.params.cooldown_name)) {
      return ActionStates.COOLDOWN;
    }
    return ActionStates.ENABLED;
  },

  // Override this if your ability has a resource cost that can disable the
  // ability.
  hasResources: function (entity) {
    return true;
  },

  getIcon: function (entity) {
    return this.params.icon;
  },

  getRange: function (entity) {
    return this.params.range ? this.params.range : 0.0;
  },
}

function ProductionAction(params) {
  this.targeting = TargetingTypes.NONE;
  this.params = params;

  this.getTooltip = function (entity) {
    return this.params.prod_name +
      '\nreq: ' + this.params.req_cost +
      '\ntime: ' + this.params.time_cost;
  }

  this.hasResources = function (entity) {
    return GetRequisition(entity.getPlayerID()) > this.params.req_cost;
  }

  this.exec = function (entity, target) {
    var prod = {
      name: this.params.prod_name,
      t: 0.0,
      endt: this.params.time_cost
    };
    entity.prodQueue_.push(prod);
    Log('Started production of', prod.name, 'for', prod.endt);
    AddRequisition(entity.getPlayerID(), -this.params.req_cost, entity.getID());
  }
}
ProductionAction.prototype = ActionPrototype;

function TeleportAction(params) {
  this.targeting = TargetingTypes.LOCATION;
  this.params = params;

  this.getTooltip = function (entity) {
    return 'Teleport' +
      '\nCooldown:'+this.params.cooldown;
  }

  this.hasResources = function (entity) {
    // TODO(zack): add mana cost
    return true;
  }

  this.exec = function (entity, target) {
    entity.addCooldown(this.params.cooldown_name, this.params.cooldown);
    entity.warpPosition(target);
  }
}
TeleportAction.prototype = ActionPrototype;

function SnipeAction(params) {
  this.targeting = TargetingTypes.ENEMY;
  this.params = params;

  this.getTooltip = function (entity) {
    return 'Snipe' +
      '\nDamage:' + this.params.damage +
      '\nCooldown:' + this.params.cooldown;
  }

  this.hasResources = function (entity) {
    // TODO(zack): add mana cost
    return true;
  }

  this.exec = function (entity, target) {
    var target_entity = GetEntity(target);
    if (!target_entity || target_entity.getTeamID() == entity.getTeamID()) {
      Log('not sniping', target, target_entity);
      return;
    }

    entity.addCooldown(this.params.cooldown_name, this.params.cooldown);

    Log('Sniping', target, 'for', this.damage);
    SendMessage({
      to: target,
      from: entity.getID(),
      type: MessageTypes.ATTACK,
      damage: this.params.damage,
    });
  }
}
SnipeAction.prototype = ActionPrototype;

function HealAction(params) {
  this.targeting = TargetingTypes.ALLY;
  this.params = params;

  this.getTooltip = function (entity) {
    return 'Heal' +
      '\nAmount:' + this.params.amount +
      '\nCooldown:' + this.params.cooldown;
  }

  this.hasResources = function (entity) {
    // TODO(zack): add mana cost
    return true;
  }

  this.exec = function (entity, target) {
    var target_entity = GetEntity(target);
    if (!target_entity || target_entity.getTeamID() != entity.getTeamID()) {
      Log('not healing', target, target_entity);
      return;
    }

    entity.addCooldown(this.params.cooldown_name, this.params.cooldown);

    Log('Healing', target, 'for', this.params.amount);
    SendMessage({
      to: target,
      from: entity.getID(),
      type: MessageTypes.ADD_DELTA,
      deltas: {
        healing: this.params.amount,
      },
    });
  }
}
HealAction.prototype = ActionPrototype;
