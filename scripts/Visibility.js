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
VisibilityMap.prototype.pointToCell = function (pid, pt) {
  if (pt[0] < this.mapOrigin[0] - this.mapSize[0] / 2 || pt[0] >= this.mapOrigin[0] + this.mapSize[0] / 2) {
    return null;
  }
  if (pt[1] < this.mapOrigin[1] - this.mapSize[1] / 2 || pt[1] >= this.mapOrigin[1] + this.mapSize[1] / 2) {
    return null;
  }
  var cell_pos = Vector.compMul(
    Vector.clamp(Vector.add(Vector.compDiv(Vector.sub(pt, this.mapOrigin), this.mapSize),
      [0.5, 0.5]
    ), 0, 1),
    this.cellDims
  );

  cell_pos = Vector.floor(cell_pos);
  invariant(cell_pos[0] >= 0, 'invalid cell position');
  invariant(cell_pos[1] >= 0, 'invalid cell position');
  invariant(cell_pos[0] < this.cellDims[0], 'invalid cell position');
  invariant(cell_pos[1] < this.cellDims[1], 'invalid cell position');
  var player_offset = pid - IDConst.STARTING_PID;
  return this.numPlayers * (
    cell_pos[1] * this.cellDims[0] + cell_pos[0]
  ) + player_offset;
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
  var cell_i = this.pointToCell(pid, pt);
  return this.data[cell_i] !== 0;
}

VisibilityMap.prototype.updateVisibilityFor = function (pid, pos, sight) {
  var y = pos[1] - sight;
  for (; y <= pos[1] + sight; y += cell_size) {
    var x = pos[0] - sight;
    for (; x <= pos[0] + sight; x += cell_size) {
      var curpos = [x, y];
      var dist = Vector.distance2(pos, curpos);
      if (dist > sight * sight) {
        continue;
      }
      var cell = this.pointToCell(pid, curpos);
      if (cell) {
        this.data[cell] = 255;
      }
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

    // TODO(zack): fill out grid here
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
