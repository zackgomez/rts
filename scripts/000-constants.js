// This file is for constants and global utility functions

// --
// -- Constants --
// --
P_CAPPABLE = 815586235;
P_TARGETABLE = 463132888;
P_ACTOR = 913794634;
P_RENDERABLE = 565038773;
P_COLLIDABLE = 983556954;
P_MOBILE = 1122719651;
P_UNIT = 118468328;

NO_PLAYER = 0;
NO_ENTITY = 0;
NO_TEAM = 0;

MessageTypes = {
  ORDER: 'ORDER',
  ATTACK: 'ATTACK',
  CAPTURE: 'CAPTURE',
  ADD_STAT: 'STAT',
};

TargetingTypes = {
  NONE: 0,
  LOCATION: 1,
  ENTITY: 2,
};

ActionStates = {
  DISABLED: 0,
  ENABLED: 1,
  COOLDOWN: 2,
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

