// This file is for constants and global utility functions

// --
// -- Constants --
// --
var P_CAPPABLE = 815586235;
var P_TARGETABLE = 463132888;
var P_ACTOR = 913794634;
var P_RENDERABLE = 565038773;
var P_COLLIDABLE = 983556954;
var P_MOBILE = 1122719651;
var P_UNIT = 118468328;

var NO_PLAYER = 0;
var NO_ENTITY = 0;
var NO_TEAM = 0;

var GAME_ID = 1;

var STARTING_PID = 100;
var STARTING_TID = 200;
var STARTING_EID = 300;

// --
// -- Damage Types --
// --
var HEALTH_TARGET_AOE = -1;
var HEALTH_TARGET_RANDOM = -2;
var HEALTH_TARGET_LOWEST = -3;
var HEALTH_TARGET_HIGHEST = -4;

var MessageTypes = {
  ATTACK: 'ATTACK',
  HEAL: 'HEAL',
  CAPTURE: 'CAPTURE',
  ADD_DELTA: 'DELTA',

  // For the game
  SPAWN: 'SPAWN',

  // For teams
  ADD_VPS: 'ADD_VPS',
  ADD_REQUISITION: 'ADD_REQUISITION',
};

var TargetingTypes = {
  NONE: 0,
  LOCATION: 1,
  ENEMY: 2,
  ALLY: 3,
  PATHABLE: 4,
};

var ActionStates = {
  DISABLED: 0,
  ENABLED: 1,
  COOLDOWN: 2,
  UNAVAILABLE: 3,
};

var EntityStatus = {
  NORMAL: 0,
  DEAD: 1,
};

// --
// -- Utility --
// --
function vecAdd(v1, v2) {
  if (v1.length != v2.length) return undefined;

  var ret = [];
  for (var i = 0; i < v1.length; i++) {
    ret.push(v1[i] + v2[i]);
  }
  return ret;
}

function vecMul(v, s) {
  var ret = [];
  for (var i = 0; i < v.length; i++) {
    ret.push(v[i] * s);
  }
  return ret;
}

function invariant(condition, message) {
  if (!condition) {
    invariant_violation(message);
  }
}

function invariant_violation(message) {
  throw new Error('invariant_violation: ' + message);
}

function must_have_idx(obj, index) {
  if (!obj.hasOwnProperty(index)) {
    throw new Error('Missing index \'' + index + '\'');
  }
  return obj[index];
}
