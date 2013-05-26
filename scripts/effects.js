function __healingAura(radius, amount, entity, dt) {
  var radius = 5.0;
  var amount = 5.0;

  entity.getNearbyEntities(radius, function (nearby_entity) {
    SendMessage({
      to: nearby_entity.getID(),
      from: entity.getID(),
      type: "STAT",
      healing: dt * amount
    });

    return true;
  });

  return true;
}

baseHealingAura = __healingAura.bind(undefined, 5.0, 5.0);

function vpGenEffect(entity, dt) {
  if (!entity.getPlayerID()) {
    return true;
  }

  var amount = 1.0;
  AddVPs(entity.getTeamID(), dt * amount, entity.getID());
  return true;
}

function reqGenEffect(entity, dt) {
  if (!entity.getPlayerID()) {
    return true;
  }

  var amount = 1.0;
  AddRequisition(entity.getPlayerID(), dt * amount, entity.getID());
  return true;
}
