// 
var players = {}

function getPlayerInfo(pid) {
  return players[pid];
}

function playerInit(pid, starting_def) {
  if (players[pid]) {
    throw new Error('player '+pid+' already exists!');
  }

  var base_id = SpawnEntity(
    'base',
    {
      pid: pid,
      pos: starting_def.pos,
      angle: starting_def.angle,
    });
  var base_entity = GetEntity(base_id);

  var hero_id = SpawnEntity(
    'unit',
    {
      pid: pid,
      pos: vecAdd(base_entity.getPosition2(), base_entity.getDirection()),
      angle: base_entity.getAngle(),
    });
  var hero_entity = GetEntity(hero_id);

  players[pid] = {
    base_id: base_id,
    hero_id: hero_id,
  };
}
