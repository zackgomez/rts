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

  var unit_id = SpawnEntity(
    'unit',
    {
      pid: pid,
      pos: vecAdd(base_entity.getPosition2(), base_entity.getDirection()),
      angle: base_entity.getAngle(),
    });

  players[pid] = {
    units: {
      base: base_id,
      unit: unit_id,
    }
  };
}

function playerUpdate(player) {
  for (var entity_name in player.units) {
    var entity = GetEntity(player.units[entity_name]);
    if (!entity) {
      delete player.units[entity_name];
    }
  }
}

function updateAllPlayers() {
  for (var pid in players) {
    playerUpdate(players[pid]);
  }
}
