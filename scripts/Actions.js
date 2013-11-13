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

var ActionStates = require('constants').ActionStates;
var DamageTypes = require('constants').DamageTypes;
var EntityProperties = require('constants').EntityProperties;
var TargetingTypes = require('constants').TargetingTypes;

var Players = require('Players');

var ActionPrototype = {
  getState: function (entity) {
    if (!this.isAvailable(entity)) {
      return ActionStates.UNAVAILABLE;
    } else if (this.params.cooldown_name &&
        entity.hasCooldown(this.params.cooldown_name)) {
      return ActionStates.COOLDOWN;
    } else if (entity.retreat_) {
      return ActionStates.DISABLED;
    } else if (!this.isEnabled(entity)) {
      return ActionStates.DISABLED;
    }
    return ActionStates.ENABLED;
  },

  isAvailable: function (entity) {
    if (this.params.part) {
      var part = entity.getPart(this.params.part);
      if (!part.isAlive()) {
        return false;
      }
      if (this.params.part_upgrade) {
        if (!part.hasUpgrade(this.params.part_upgrade)) {
          return false;
        }
      }
    }
    return true;
  },

  // Override this if your ability has a resource cost that can disable the
  // ability.
  isEnabled: function (entity) {
    return true;
  },

  getIcon: function (entity) {
    return this.params.icon;
  },

  getRange: function (entity) {
    return this.params.range ? this.params.range : 0.0;
  },

  getRadius: function (entity) {
    return this.params.radius ? this.params.radius : 0.0;
  },
};

var Actions = {};

Actions.ReinforceAction = function (params) {
  this.targeting = TargetingTypes.NONE;
  this.params = params;
  this.params.hotkey = 'g';

  this.getTooltip = function (entity) {
    return 'Reinforce' +
      '\nreq: ' + this.params.req_cost;
  };

  this.isEnabled = function (entity) {
    var near_base = false;
    // TODO(zack): unhardcode this radius
    entity.getNearbyEntities(5.0, function (e) {
      if (e.getName() == 'base' && entity.getPlayerID() == e.getPlayerID()) {
        near_base = true;
        return false;
      }
      return true;
    });
    var disabled_part = false;
    for (var i = 0; i < entity.parts_.length; i++) {
      if (entity.parts_[i].getHealth() <= 0.0) {
        disabled_part = true;
        break;
      }
    }
    var req = Players.getPlayer(entity.getPlayerID()).getRequisition();
    return req >= this.params.req_cost && disabled_part && near_base;
  };

  this.exec = function (entity, target) {
    Log(entity.getID(), 'repairing');
    if (!entity.parts_) {
      throw new Error('Trying to repair entity without parts');
    }

    for (var i = 0; i < entity.parts_.length; i++) {
      if (entity.parts_[i].getHealth() <= 0) {
        entity.parts_[i].setHealth(entity.parts_[i].getMaxHealth());
        break;
      }
    }
    var player = Players.getPlayer(entity.getPlayerID());
    player.addRequisition(-this.params.req_cost);
    entity.addCooldown(this.params.cooldown_name, this.params.cooldown);
  };
}
Actions.ReinforceAction.prototype = ActionPrototype;

Actions.ProductionAction = function (params) {
  this.targeting = TargetingTypes.NONE;
  this.params = params;
  this.params.cooldown_name = 'production';
  this.params.cooldown = this.params.time_cost;

  this.getTooltip = function (entity) {
    return this.params.prod_name +
      '\nreq: ' + this.params.req_cost +
      '\ntime: ' + this.params.time_cost;
  };

  this.isEnabled = function (entity) {
    var owner = Players.getPlayer(entity.getPlayerID());
    var unit_constructed = this.params.prod_name in owner.units;

    // Always disabled if unit is already produced
    if (unit_constructed) {
      return false;
    }

    // Always enough resources when production is on cooldown to show
    // progress
    if (entity.hasCooldown('production')) {
      return true;
    }
    var req = Players.getPlayer(entity.getPlayerID()).getRequisition();
    return req > this.params.req_cost;
  };

  this.exec = function (entity, target) {
    entity.addCooldown(this.params.cooldown_name, this.params.cooldown);
    Players.getPlayer(entity.getPlayerID()).addRequisition(
      -this.params.req_cost
    );

    entity.effects_['production'] = makeProductionEffect({
      prod_name: this.params.prod_name,
      cooldown_name: this.params.cooldown_name,
    });
  };
}
Actions.ProductionAction.prototype = ActionPrototype;

Actions.TeleportAction = function (params) {
  this.targeting = TargetingTypes.PATHABLE;
  this.params = params;

  this.getTooltip = function (entity) {
    return 'Teleport' +
      '\nCooldown:'+this.params.cooldown;
  };

  this.isEnabled = function (entity) {
    return entity.mana_ > this.params.mana_cost;
  };

  this.exec = function (entity, target) {
    entity.addCooldown(this.params.cooldown_name, this.params.cooldown);
    entity.mana_ -= this.params.mana_cost;
    entity.warpPosition(target);
    entity.onEffect('teleport', {
      start: entity.getPosition2(),
      end: target,
    });
  };
}
Actions.TeleportAction.prototype = ActionPrototype;

Actions.SnipeAction = function (params) {
  this.targeting = TargetingTypes.ENEMY;
  this.params = params;

  this.getTooltip = function (entity) {
    return 'Snipe' +
      '\nDamage:' + this.params.damage +
      '\nCooldown:' + this.params.cooldown;
  };

  this.isEnabled = function (entity) {
    return entity.mana_ > this.params.mana_cost;
  };

  this.exec = function (entity, target) {
    var target_entity = Game.getEntity(target);
    if (!target_entity || target_entity.getTeamID() == entity.getTeamID()) {
      Log('not sniping', target, target_entity);
      return;
    }

    entity.mana_ -= this.params.mana_cost;
    entity.addCooldown(this.params.cooldown_name, this.params.cooldown);
    entity.turnTowards(target_entity.getPosition2());

    Log('Sniping', target, 'for', this.params.damage);
    MessageHub.sendMessage({
      to: target,
      from: entity.getID(),
      type: MessageTypes.ATTACK,
      damage: this.params.damage,
      damage_type: 'ranged',
      health_target: DamageTypes.HEALTH_TARGET_RANDOM,
    });
    entity.onEvent('snipe', {});
  };
}
Actions.SnipeAction.prototype = ActionPrototype;

Actions.CenteredAOEAction = function (params) {
  this.targeting = TargetingTypes.NONE;
  this.params = params;

  this.getTooltip = function (entity) {
    return 'Spinning Storm' +
      '\nDeals AOE damage in a radius around the unit' +
      '\nDamage: ' + this.params.damage +
      '\nRadius: ' + this.params.radius +
      '\nMana Cost: ' + this.params.mana_cost +
      '\nCooldown: ' + this.params.cooldown;
  };

  this.isEnabled = function (entity) {
    return entity.mana_ > this.params.mana_cost;
  };

  this.exec = function (entity, target) {
    entity.mana_ -= this.params.mana_cost;
    entity.addCooldown(this.params.cooldown_name, this.params.cooldown);

    Log('AOE blast at', entity.getPosition2(), 'for', this.params.damage);
    var damage = this.params.damage;
    var damage_type = this.params.damage_type;
    entity.getNearbyEntities(
      this.params.radius,
      function (candidate) {
        if (candidate.getTeamID() != entity.getTeamID() &&
            candidate.hasProperty(EntityProperties.P_TARGETABLE)) {
          MessageHub.sendMessage({
            to: candidate.getID(),
            from: entity.getID(),
            type: MessageTypes.ATTACK,
            damage: damage,
            damage_type: damage_type,
            health_target: DamageTypes.HEALTH_TARGET_AOE,
          });
        }
        return true;
      });
  };
}
Actions.CenteredAOEAction.prototype = ActionPrototype;

Actions.HealAction = function (params) {
  this.targeting = TargetingTypes.ALLY;
  this.params = params;

  this.getTooltip = function (entity) {
    return 'Heal' +
      '\nAmount:' + this.params.healing +
      '\nCooldown:' + this.params.cooldown;
  };

  this.isEnabled = function (entity) {
    return entity.mana_ > this.params.mana_cost;
  };

  this.exec = function (entity, target) {
    var target_entity = Game.getEntity(target);
    if (!target_entity || target_entity.getTeamID() != entity.getTeamID()) {
      Log('not healing', target, target_entity);
      return;
    }

    entity.mana_ -= this.params.mana_cost;
    entity.addCooldown(this.params.cooldown_name, this.params.cooldown);

    Log('Healing', target, 'for', this.params.healing);
    MessageHub.sendMessage({
      to: target,
      from: entity.getID(),
      type: MessageTypes.HEAL,
      healing: this.params.healing,
      health_target: this.params.health_target,
    });

    target_entity.onEvent('heal_target', {});
  };
}
Actions.HealAction.prototype = ActionPrototype;

module.exports = Actions;
