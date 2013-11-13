var IDConst = require('constants').IDConst;
var MessageTypes = require('constants').MessageTypes;
var MessageHub = require('MessageHub');
var Players = require('Players');
var Vector = require('Vector');

// This file contains entity effects.  Effects are run once per frame and
// should set intent (send messages, adjust deltas) but not actually change
// values on the entity
module.exports = {
  makeProductionEffect: function (params) {
    var cooldown_name = params.cooldown_name;
    var prod_name = params.prod_name;

    return function (entity) {
      if (entity.hasCooldown(cooldown_name)) {
        return true;
      }

      var player = Players.getPlayer(entity.getPlayerID());
      if (player.units[prod_name]) {
        Log('WTF producing a unit that already exists?!');
      }

      Log('spawning', prod_name);

      // Spawn
      MessageHub.sendMessage({
        to: IDConst.GAME_ID,
        from: entity.getID(),
        type: MessageTypes.SPAWN,
        name: prod_name,
        params: {
          pid: entity.getPlayerID(),
          pos: Vector.add(entity.getPosition2(), entity.getDirection()),
          angle: entity.getAngle(),
        },
      });

      return false;
    };
  },

  makeHealingAura: function (radius, amount) {
    return function(entity) {
      entity.getNearbyEntities(radius, function (nearby_entity) {
        if (nearby_entity.getPlayerID() == entity.getPlayerID()) {
          MessageHub.sendMessage({
            to: nearby_entity.getID(),
            from: entity.getID(),
            type: MessageTypes.ADD_DELTA,
            deltas: {
              healing_rate: amount,
            },
          });
        }

        return true;
      });

      return true;
    };
  },

  makeDamageFactorEffect: function (params) {
    return function (entity) {
      entity.deltas.damage_factor *= params.factor;
      return entity.hasCooldown(params.cooldown_name);
    };
  },

  makeDamageBuffAura: function (params) {
    return function(entity) {
      if (params.alive_fun && !params.alive_func(entity)) {
        return false;
      }
      if (params.active_func && !params.active_func(entity)) {
        return true;
      }
      var cooldown_name = params.name + '_cd';
      entity.getNearbyEntities(params.radius, function (nearby_entity) {
        if (nearby_entity.getTeamID() == entity.getTeamID()) {
          nearby_entity.addEffect(
            params.name,
            makeDamageFactorEffect({
              factor: 1 + params.amount,
              cooldown_name: cooldown_name,
            })
          );
          nearby_entity.addCooldown(cooldown_name, 0.1);
        }
        // Next entity
        return true;
      });

      return true;
    };
  },

  makeManaRegenEffect: function (amount) {
    return function (entity) {
      if (entity.maxMana_) {
        entity.deltas.mana_regen_rate += amount;
      }
      return true;
    };
  },

  makeVpGenEffect: function (amount) {
    return function(entity) {
      if (entity.getTeamID() == IDConst.NO_TEAM) {
        return true;
      }

      entity.deltas.vp_rate += amount;
      return true;
    };
  },

  makeReqGenEffect: function (amount) {
    return function(entity) {
      if (entity.getPlayerID() == IDConst.NO_PLAYER) {
        return true;
      }

      entity.deltas.req_rate += amount;
      return true;
    };
  },
};
