// This file is for constants and global utility functions

// --
// -- Constants --
// --
exports.Properties = {
  P_CAPPABLE: 815586235,
  P_TARGETABLE: 463132888,
  P_ACTOR: 913794634,
  P_RENDERABLE: 565038773,
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

// --
// -- Damage Types --
// --
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

function vecNonZero(v) {
  for (var i = 0; i < v.length; i++) {
    if (v[i] !== 0) {
      return true;
    }
  }
  return false;
}

function vecRandDir2() {
  return vecNormalize([
    (GameRandom() - 0.5) * 2,
    (GameRandom() - 0.5) * 2,
  ]);
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

function object_fill_keys(keys, value) {
  invariant(Array.isArray(keys), 'keys must be an array');
  var ret = {};
  for (var i = 0; i < keys.length; i++) {
    ret[keys[i]] = value;
  }
  return ret;
}
