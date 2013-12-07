var _ = require('underscore');
var must_have_idx = require('must_have_idx');
var invariant = require('invariant').invariant;

var IDConst = require('constants').IDConst;
var EntityProperties = require('constants').EntityProperties;
var Pathing = require('Pathing');
var Vector = require('Vector');

var cell_size = 0.5;

var VisibilityMap = function (map_def, num_players) {
  this.mapOrigin = map_def.origin || [0, 0];
  this.mapSize = must_have_idx(map_def, 'size');
  this.numPlayers = num_players;

  this.cellDims = Vector.floor(Vector.div(this.mapSize, cell_size));
  invariant(
    this.cellDims[0] > 0 && this.cellDims[1] > 0,
    'invalid cell dims'
  );
  // allocate one uint8 per player per cell
  var buf_size = num_players * this.cellDims[0] * this.cellDims[1];
  this.data = new Uint8Array(buf_size);
  this.clearCells();
};

// returns index of first player's flags in the cell
VisibilityMap.prototype.pointToCell = function (pid, pt) {
  var cell_pos = Vector.compMul(
    Vector.add(
      Vector.compDiv(Vector.sub(pt, this.mapOrigin), this.mapSize),
      [0.5, 0.5]
    ),
    this.cellDims
  );
  cell_pos = Vector.floor(Vector.clamp(cell_pos, 0, 1));
  invariant(cell_pos[0] >= 0, 'invalid cell position');
  invariant(cell_pos[1] >= 0, 'invalid cell position');
  invariant(cell_pos[0] < this.cellDims[0], 'invalid cell position');
  invariant(cell_pos[1] < this.cellDims[1], 'invalid cell position');
  var player_offset = pid - IDConst.STARTING_PID;
  return 4 * (
    cell_pos[1] * this.cellDims[0] + cell_pos[0]
  ) + player_offset;
}


VisibilityMap.prototype.clearCells = function () {
  for (var i = 0; i < this.data.length; i++) {
    this.data[i] = 0;
  }
}

VisibilityMap.prototype.getRawData = function () {
  return this.data;
}

// returns true if the player given by pid can see the passed point
// id pid, vec2 pt
VisibilityMap.prototype.isPointVisible = function (pid, pt) {
  var cell_i = this.pointToCell(pid, pt);
  return this.data[cell_i] !== 0;
}


// entity needs getPlayerID(), getPosition2(), getSight()
VisibilityMap.prototype.updateMap = function (entities) {
  this.clearCells();
  _.each(entities, function (entity) {
    var pid = entity.getPlayerID();
    if (pid === IDConst.NO_PLAYER) {
      return;
    }

    // TODO(zack): fill out grid here
    var cell_i = this.pointToCell(pid, entity.getPosition2());
    this.data[cell_i] = 1;
  }, this);
}

VisibilityMap.prototype.updateEntityVisibilities = function (entities) {
  var all_players = _.range(
    IDConst.STARTING_PID,
    IDConst.STARTING_PID + this.numPlayers
  );
  _.each(entities, function (entity) {
    entity.setVisibilitySet(all_players);
    return;
    /*
    if (entity.hasProperty(EntityProperties.P_CAPPABLE)) {
      entity.setVisibilitySet(all_players);
      return;
    }
    var pid = entity.getPlayerID();
    if (pid === IDConst.NO_PLAYER) {
      entity.setVisibilitySet([])
      return;
    }
    entity.setVisibilitySet([pid]);
    */
  }, this);
}

module.exports = {
  VisibilityMap: function (map_def, num_player) {
    return new VisibilityMap(map_def, num_player);
  },
};
