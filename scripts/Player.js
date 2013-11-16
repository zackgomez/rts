var invariant = require('invariant').invariant;
var Vector = require('Vector');

var IDConst = require('constants').IDConst;
var MessageTypes = require('constants').MessageTypes;

var MessageHub = require('MessageHub');

// Create a new player.
// @param pid the player ID
// @param start_def dictionary with keys for `pos` and `angle`
var Player = function (def) {
  this.pid = def.pid;
  this.tid = def.tid;
  this.units = {};
  this.requisition = def.starting_requisition;

  var starting_def = def.starting_location;

  var base_dir = [
    Math.cos(starting_def.angle * Math.PI / 180),
    Math.sin(starting_def.angle * Math.PI / 180),
  ];

  this.retreat_location = Vector.add(
    starting_def.pos,
    Vector.mul(base_dir, 3)
  );

  MessageHub.sendMessage({
    to: IDConst.GAME_ID,
    from: this.pid,
    type: MessageTypes.SPAWN,
    name: 'base',
    params: {
      pid: this.pid,
      pos: starting_def.pos,
      angle: starting_def.angle,
    },
  })

  MessageHub.sendMessage({
    to: IDConst.GAME_ID,
    from: this.pid,
    type: MessageTypes.SPAWN,
    name: 'melee_unit',
    params: {
      pid: this.pid,
      pos: this.retreat_location,
      angle: starting_def.angle,
    },
  });
};

Player.prototype.getPlayerID = function () {
  return this.pid;
};
Player.prototype.getTeamID = function () {
  return this.tid;
};
Player.prototype.getRetreatLocation = function () {
  return this.retreat_location;
};
Player.prototype.getRequisition = function () {
  return this.requisition;
};
Player.prototype.addRequisition = function (amount) {
  this.requisition += amount;
};

module.exports = Player;
