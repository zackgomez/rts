// This file is for constants and global utility functions
exports.EntityProperties = {
  P_CAPPABLE: 815586235,
  P_TARGETABLE: 463132888,
  P_ACTOR: 913794634,
  P_COLLIDABLE: 983556954,
  P_MOBILE: 1122719651,
  P_UNIT: 118468328,
};

exports.IDConst = {
  NO_PLAYER: 0,
  NO_ENTITY: 0,
  NO_TEAM: 0,

  GAME_ID: 1,

  STARTING_PID: 100,
  STARTING_TID: 200,
  STARTING_EID: 3000,
};

exports.EntityConsts = {
  retreat_speed: 1.5,
};

exports.DamageTypes = {
  HEALTH_TARGET_AOE: -1,
  HEALTH_TARGET_RANDOM: -2,
  HEALTH_TARGET_LOWEST: -3,
  HEALTH_TARGET_HIGHEST: -4,
};

exports.MessageTypes = {
  ATTACK: 'ATTACK',
  HEAL: 'HEAL',
  CAPTURE: 'CAPTURE',
  ADD_DELTA: 'DELTA',

  // For the game
  SPAWN: 'SPAWN',

  // For teams
  ADD_VPS: 'ADD_VPS',
  ADD_REQUISITION: 'ADD_REQUISITION',
  ADD_POWER: 'ADD_POWER',
};

exports.TargetingTypes = {
  NONE: 0,
  LOCATION: 1,
  ENEMY: 2,
  ALLY: 3,
  PATHABLE: 4,
};

exports.ActionStates = {
  DISABLED: 0,
  ENABLED: 1,
  COOLDOWN: 2,
  UNAVAILABLE: 3,
};

exports.EntityStatus = {
  NORMAL: 0,
  DEAD: 1,
};
