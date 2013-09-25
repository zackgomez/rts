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
  invariant(v1.length === v2.length, "vectors must have same length");

  var ret = [];
  for (var i = 0; i < v1.length; i++) {
    ret.push(v1[i] + v2[i]);
  }
  return ret;
}

// returns v1 - v2
function vecSub(v1, v2) {
  invariant(v1.length === v2.length, "vectors must have same length");

  var ret = [];
  for (var i = 0; i < v1.length; i++) {
    ret.push(v1[i] - v2[i]);
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

function vecDot(v1, v2) {
  invariant(
    v1.length === v2.length,
    "must have same sized vectors to dot"
  );

  var dot = 0;
  for (var i = 0; i < v1.length; i++) {
    dot += v1[i] * v2[i];
  }
  return dot;
};

function vecLength2(v) {
  return vecDot(v, v);
};

function vecLength(v) {
  return Math.sqrt(vecLength2(v));
}

function vecNormalize(v) {
  return vecMul(v, 1 / vecLength(v));
}

function vecDistance(v1, v2) {
  return vecLength(vecSub(v1, v2));
}

// Returns angle between vector and [1, 0] in degrees
function vecAngle(v) {
  invariant(v.length === 2, 'can only get angle between 2d vectors');
  return 180 / Math.PI * Math.atan2(v[1], v[0]);
}

function vecAngleBetween(v1, v2) {
  invariant(v1.length === 2, 'can only get angle between 2d vectors');
  var diff = vecSub(v1, v2);
  return 180 / Math.PI * Math.atan2(diff[1], diff[0]);
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
