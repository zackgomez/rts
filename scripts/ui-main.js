var must_have_idx = require('must_have_idx');
var invariant = require('invariant').invariant;
var VisibilityMap = require('Visibility').VisibilityMap;

var NativeUI = runtime.binding('nativeui');

var UI = function () {
  this.initialized = false;
  this.visibilityMap = null;

  this.init = function (params) {
    invariant(this.initialized === false, 'ui already initialized');
    this.initialized = true;
    var map_def = {
      origin: [0, 0],
      size: must_have_idx(params, 'map_size'),
    };
    this.visibilityMap = new VisibilityMap(
      map_def,
      must_have_idx(params, 'num_players')
    );

    NativeUI.setVisibilityBuffer(
      this.visibilityMap.getGridDim(),
      this.visibilityMap.getRawData().buffer
    );
  };

  this.update = function (entities) {
    this.visibilityMap.updateMap(entities);
  };
};

var main = function () {
  return new UI();
}

module.exports = main;
