var Weapons = (function () {
  var exports = {};

  // Weapon Definitions
  var definitions = {
    rifle: {
      range: 8.0,
      damage: 15.0,
      damage_type: 'ranged',
      health_target: HEALTH_TARGET_RANDOM,
      cooldown_name: 'rifle_cd',
      cooldown: 1.0,
    },
    advanced_melee: {
      range: 1.0,
      damage: 10.0,
      damage_type: 'melee',
      health_target: HEALTH_TARGET_RANDOM,
      cooldown_name: 'advanced_melee_cd',
      cooldown: 0.5,
    },
    tanky_melee: {
      range: 1.0,
      damage: 10.0,
      damage_type: 'melee',
      health_target: HEALTH_TARGET_AOE,
      cooldown_name: 'tanky_melee_cd',
      cooldown: 1.5,
    }
  }

  function Weapon(params) {
    this.params = params;

    this.getRange = function (entity) {
      return this.params.range;
    }

    this.ready = function (entity) {
      return !entity.hasCooldown(this.params.cooldown_name);
    }

    this.fire = function (entity, target_id) {
      entity.addCooldown(this.params.cooldown_name, this.params.cooldown);
      if (this.damage_type == 'ranged') {
        var params = {
          pid: entity.getPlayerID(),
          pos: entity.getPosition2(),
          target_id: target.getID(),
          damage: this.params.damage,
          damage_type: this.params.damage_type,
          health_target: this.params.health_target,
        };
        SpawnEntity('projectile', params);
      } else {
        SendMessage({
          to: target_id,
          from: entity.getID(),
          type: MessageTypes.ATTACK,
          damage: this.params.damage,
          damage_type: this.params.damage_type,
          health_target: this.params.health_target,
        });
      }
    }
  }

  exports.newWeapon = function (name) {
    invariant(
      definitions[name] !== undefined,
      'weapon \'' + name + '\' not found'
    );
    var def = definitions[name];
    return new Weapon(def);
  }

  return exports;
})();
