function testEffect(entity, dt) {
  var radius = 5.0;

  entity.getNearbyEntities(radius, function (nearby_entity) {
    Log('Entity', nearby_entity.getID(), 'is near', entity.getID());
    return true;
  });

  return true;
}

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
