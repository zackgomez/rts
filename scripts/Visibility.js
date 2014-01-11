var _ = require('underscore');
var must_have_idx = require('must_have_idx');
var invariant = require('invariant').invariant;

var IDConst = require('constants').IDConst;
var EntityProperties = require('constants').EntityProperties;
var Vector = require('Vector');

var cell_size = 0.25;

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
VisibilityMap.prototype.pointToCell = function (pt) {
  var e = 0.0001;
  if (pt[0] < this.mapOrigin[0] - this.mapSize[0] / 2 || pt[0] + e >= this.mapOrigin[0] + this.mapSize[0] / 2) {
    return null;
  }
  if (pt[1] < this.mapOrigin[1] - this.mapSize[1] / 2 || pt[1] + e >= this.mapOrigin[1] + this.mapSize[1] / 2) {
    return null;
  }
  var cell_pos = Vector.compMul(
    Vector.clamp(Vector.add(Vector.compDiv(Vector.sub(pt, this.mapOrigin), this.mapSize),
      [0.5, 0.5]
    ), 0, 1),
    this.cellDims
  );

  cell_pos = Vector.floor(Vector.add(cell_pos, 0.0001));
  invariant(cell_pos[0] >= 0, 'invalid cell position');
  invariant(cell_pos[1] >= 0, 'invalid cell position');
  invariant(cell_pos[0] < this.cellDims[0], 'invalid cell position');
  invariant(cell_pos[1] < this.cellDims[1], 'invalid cell position ' + cell_pos);
  return cell_pos;
}

VisibilityMap.prototype.cellToIndex = function (pid, cell) {
  var player_offset = pid - IDConst.STARTING_PID;
  return this.numPlayers * (
    cell[1] * this.cellDims[0] + cell[0]
  ) + player_offset;
}

VisibilityMap.prototype.pointToIndex = function (pid, pt) {
  return this.cellToIndex(pid, this.pointToCell(pt));
}


VisibilityMap.prototype.clearCells = function () {
  for (var i = 0; i < this.data.length; i++) {
    this.data[i] = 0;
  }
}

VisibilityMap.prototype.getGridDim = function () {
  return this.cellDims;
};

VisibilityMap.prototype.getRawData = function () {
  return this.data;
}

// returns true if the player given by pid can see the passed point
// id pid, vec2 pt
VisibilityMap.prototype.isPointVisible = function (pid, pt) {
  var cell_i = this.pointToIndex(pid, pt);
  return this.data[cell_i] !== 0;
}

// Takes in integer cell positions
VisibilityMap.prototype.fillHorizontalLine = function (pid, x0, x1, y) {
  if (x0 < 0) x0 = 0;
  if (x0 >= this.cellDims[0]) x0 = this.cellDims[0] - 1;
  if (x1 < 0) x1 = 0;
  if (x1 >= this.cellDims[0]) x1 = this.cellDims[0] - 1;
  if (y < 0) y = 0;
  if (y >= this.cellDims[1]) y = this.cellDims[1] - 1;

  for (var cell = this.cellToIndex(pid, [x0, y]); x0 <= x1; x0++, cell += this.numPlayers) {
    this.data[cell] = 255;
  }
}

VisibilityMap.prototype.updateVisibilityFor = function (pid, pos, sight) {
  var center = this.pointToCell(pos);
  var r = Math.floor(sight / cell_size);
  var x = r;
  var y = 0;
  var re = 1 - x;
  while (x >= y) {
    this.fillHorizontalLine(pid, center[0] - x, center[0] + x, center[1] + y);
    this.fillHorizontalLine(pid, center[0] - x, center[0] + x, center[1] - y);

    this.fillHorizontalLine(pid, center[0] - y, center[0] + y, center[1] + x);
    this.fillHorizontalLine(pid, center[0] - y, center[0] + y, center[1] - x);

    y++;
    if (re < 0) {
      re += 2 * y + 1;
    } else {
      x--;
      re += 2 * (y - x + 1);
    }
  }
}

VisibilityMap.prototype.updateMap_UI = function (sight_positions) {
  this.clearCells();
  _.each(sight_positions, function (e) {
    var pos = e.pos;
    var sight = e.sight;
    // HACK only one play for GUI usage
    this.updateVisibilityFor(IDConst.STARTING_PID, pos, sight);
 }, this);
}

// takes js entities
VisibilityMap.prototype.updateMap = function (entities) {
  this.clearCells();
  _.each(entities, function (entity) {
    var pid = entity.getPlayerID();
    if (pid === IDConst.NO_PLAYER) {
      return;
    }

    this.updateVisibilityFor(pid, entity.getPosition2(), entity.getSight());
  }, this);
}

VisibilityMap.prototype.updateEntityVisibilities = function (entities) {
  var all_players = _.range(
    IDConst.STARTING_PID,
    IDConst.STARTING_PID + this.numPlayers
  );
  var get_visibility = function (entity) {
    if (entity.hasProperty(EntityProperties.P_CAPPABLE)) {
      return all_players;
    }
    var ret = _.filter(all_players, function (pid) {
      return this.isPointVisible(pid, entity.getPosition2());
    }, this);
    return ret;
  }.bind(this);

  _.each(entities, function (entity) {
    entity.setVisibilitySet(get_visibility(entity));
  }, this);
}

module.exports = {
  VisibilityMap: function (map_def, num_player) {
    return new VisibilityMap(map_def, num_player);
  },
};
