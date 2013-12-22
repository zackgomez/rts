var invariant = require('invariant').invariant;

var DamageTypes = require('constants').DamageTypes;
var IDConst = require('constants').IDConst;
var MessageTypes = require('constants').MessageTypes;
var MessageHub = require('MessageHub');

var definitions = {
  rifle: {
    part: 'left',
    range: 8.0,
    damage: 15.0,
    damage_type: 'ranged',
    health_target: DamageTypes.HEALTH_TARGET_RANDOM,
    cooldown_name: 'rifle_weapon',
    cooldowns: {
      rifle_weapon: 1.0,
    },
    on_hit_cooldowns: {},
  },
  advanced_rifle: {
    part: 'left',
    part_upgrade: 'advanced_rifle',
    range: 9.0,
    damage: 15.0,
    damage_type: 'ranged',
    health_target: DamageTypes.HEALTH_TARGET_RANDOM,
    cooldown_name: 'rifle_weapon',
    cooldowns: {
      rifle_weapon: 0.6,
    },
    on_hit_cooldowns: {},
  },
  basic_melee: {
    range: 1.0,
    damage: 5.0,
    damage_type: 'melee',
    health_target: DamageTypes.HEALTH_TARGET_RANDOM,
    cooldown_name: 'melee_weapon',
    cooldowns: {
      melee_weapon: 0.7,
      melee_leash: 1.1,
    },
    on_hit_cooldowns: {
      melee_leash: 1.1,
    },
  },
  advanced_melee: {
    range: 1.0,
    damage: 10.0,
    damage_type: 'melee',
    health_target: DamageTypes.HEALTH_TARGET_RANDOM,
    cooldown_name: 'melee_weapon',
    cooldowns: {
      melee_weapon: 0.5,
      melee_leash: 1.1,
    },
    on_hit_cooldowns: {
      melee_leash: 1.1,
    },
  },
  tanky_melee: {
    range: 1.0,
    damage: 10.0,
    damage_type: 'melee',
    health_target: DamageTypes.HEALTH_TARGET_AOE,
    cooldown_name: 'melee_weapon',
    cooldowns: {
      melee_weapon: 1.0,
      melee_leash: 1.1,
    },
    on_hit_cooldowns: {
      melee_leash: 1.1,
    },
  },
  cc_bot_fist: {
    range: 1.0,
    damage: 10.0,
    damage_type: 'melee',
    health_target: DamageTypes.HEALTH_TARGET_RANDOM,
    cooldown_name: 'melee_weapon',
    cooldowns: {
      melee_weapon: 1.5,
      melee_leash: 1.6,
    },
    on_hit_cooldowns: {
      melee_leash: 1.6,
    },
  },
  cc_bot_powerfist: {
    range: 1.0,
    damage: 15.0,
    damage_type: 'melee',
    health_target: DamageTypes.HEALTH_TARGET_RANDOM,
    cooldown_name: 'melee_weapon',
    cooldowns: {
      melee_weapon: 1.5,
      melee_leash: 1.6,
    },
    on_hit_cooldowns: {
      melee_leash: 1.6,
    },
  },
};

function Weapon(def) {
  this.params = def;

  this.getRange = function (entity) {
    return this.params.range;
  };

  this.isEnabled = function (entity) {
    if (this.params.part) {
      var part = entity.getPart(this.params.part);
      if (!part.isAlive()) {
        return false;
      }
      if (this.params.part_upgrade &&
          !part.hasUpgrade(this.params.part_upgrade)) {
        return false;
      }
    }
    if (this.params.damage_type === 'ranged') {
      return !entity.hasCooldown('melee_leash');
    }
    return true;
  };

  this.isReady = function (entity) {
    return !entity.hasCooldown(this.params.cooldown_name);
  };

  this.fire = function (entity, target_id) {
    for (var cd_name in this.params.cooldowns) {
      entity.addCooldown(cd_name, this.params.cooldowns[cd_name]);
    }
    var damage_factor = entity.deltas.damage_factor;
    invariant(
      typeof damage_factor == 'number' && damage_factor > 0,
      'missing damage_factor delta'
    );
    var damage = damage_factor * this.params.damage;
    if (this.params.damage_type == 'ranged') {
      var params = {
        pid: entity.getPlayerID(),
        pos: entity.getPosition2(),
        target_id: target_id,
        damage: damage,
        damage_type: this.params.damage_type,
        health_target: this.params.health_target,
        on_hit_cooldowns: this.params.on_hit_cooldowns,
      };
      MessageHub.sendMessage({
        to: IDConst.GAME_ID,
        from: entity.getID(),
        type: MessageTypes.SPAWN,
        name: 'projectile',
        params: params,
      });
    } else {
      MessageHub.sendMessage({
        to: target_id,
        from: entity.getID(),
        type: MessageTypes.ATTACK,
        damage: damage,
        damage_type: this.params.damage_type,
        health_target: this.params.health_target,
        on_hit_cooldowns: this.params.on_hit_cooldowns,
      });
    }
  };
}

exports.newWeapon = function (name) {
  invariant(
    definitions[name] !== undefined,
    'weapon \'' + name + '\' not found'
  );
  var def = definitions[name];
  return new Weapon(def);
};
