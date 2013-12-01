var _ = require('underscore');
var must_have_idx = require('must_have_idx');

var VisibilityMap = function (map_def) {
  Log('visibility map', JSON.stringify(_.keys(map_def)));
  this.mapOrigin = map_def.origin || [0, 0];
  this.mapSize = must_have_idx(map_def, 'size');
}

VisibilityMap.prototype.isPointVisible = function (pt) {
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
