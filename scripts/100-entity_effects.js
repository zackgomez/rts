// This file contains entity effects.  Effects are run once per frame and
// should set intent (send messages, adjust deltas) but not actually change
// values on the entity

function makeProductionEffect(params) {
  var cooldown_name = params.cooldown_name;
  var prod_name = params.prod_name;

  return function (entity) {
    if (entity.hasCooldown(cooldown_name)) {
      return true;
    }

    var player = Players.getPlayerInfo(entity.getPlayerID());
    if (player.units[prod_name]) {
      Log('WTF producing a unit that already exists?!');
    }

    // Spawn
    var eid = SpawnEntity(
      prod_name,
      {
        pid: entity.getPlayerID(),
        pos: vecAdd(entity.getPosition2(), entity.getDirection()),
        angle: entity.getAngle(),
      });

    // Record
    player.units[prod_name] = eid;
    return false;
  };
}

function makeHealingAura(radius, amount) {
  return function(entity) {
    entity.getNearbyEntities(radius, function (nearby_entity) {
      if (nearby_entity.getPlayerID() == entity.getPlayerID()) {
        SendMessage({
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
  }
}

function makeManaRegenEffect(amount) {
  return function (entity) {
    if (entity.maxMana_) {
      entity.deltas.mana_regen_rate += amount;
    }
    return true;
  }
}

function makeVpGenEffect(amount) {
  return function(entity) {
    if (entity.getTeamID() == NO_TEAM) {
      return true;
    }

    entity.deltas.vp_rate += amount;
    return true;
  }
}

function makeReqGenEffect(amount) {
  return function(entity) {
    if (entity.getPlayerID() == NO_PLAYER) {
      return true;
    }

    entity.deltas.req_rate += amount;
    return true;
  }
}
