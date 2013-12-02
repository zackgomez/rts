var _ = require('underscore');
var must_have_idx = require('must_have_idx');

var Pathing = require('Pathing');

var VisibilityMap = function (map_def) {
  this.mapOrigin = map_def.origin || [0, 0];
  this.mapSize = must_have_idx(map_def, 'size');
};

// returns true if the player given by pid can see the passed point
// id pid, vec2 pt
VisibilityMap.prototype.isPointVisible = function (pid, pt) {
  return Pathing.locationVisible(pid, pt);
  return true;
}

VisibilityMap.prototype.update = function (entities) {
  _.each(entities, function (entity) {
  });
}

module.exports = {
  VisibilityMap: function (map_def) {
    return new VisibilityMap(map_def);
  },
};
