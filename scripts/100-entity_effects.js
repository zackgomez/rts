// This file contains entity effects.  Effects are run once per frame and
// should set intent (send messages, adjust deltas) but not actually change
// values on the entity


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
