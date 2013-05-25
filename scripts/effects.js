function testEffect(entity, dt) {
  var radius = 5.0;

  entity.getNearbyEntities(radius, function (nearby_entity) {
    Log('Entity', nearby_entity.getID(), 'is near', entity.getID());
    return true;
  });

  return true;
}
