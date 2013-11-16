var invariant = require('invariant').invariant;
var Vector = require('Vector');

var IDConst = require('constants').IDConst;
var MessageTypes = require('constants').MessageTypes;

var MessageHub = require('MessageHub');

// Create a new player.
// @param start_def dictionary with keys for `pos` and `angle`
var Player = function (def) {
  this.pid = def.pid;
  this.tid = def.tid;
  this.units = {};
  this.requisition = def.starting_requisition;
  this.spawned = false;

  this.base_location = def.starting_location;

  var base_dir = [
    Math.cos(this.base_location.angle * Math.PI / 180),
    Math.sin(this.base_location.angle * Math.PI / 180),
  ];

  this.retreat_location = Vector.add(
    this.base_location.pos,
    Vector.mul(base_dir, 3)
  );
};

Player.prototype.spawn = function () {
  invariant(!this.spawned, 'player already spawned');
  this.spawned = true;

  MessageHub.sendMessage({
    to: IDConst.GAME_ID,
    from: this.pid,
    type: MessageTypes.SPAWN,
    name: 'base',
    params: {
      pid: this.pid,
      pos: this.base_location.pos,
      angle: this.base_location.angle,
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
      angle: this.base_location.angle,
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
