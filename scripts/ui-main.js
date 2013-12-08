/*
var invariant = require('invariant').invariant;
var VisibilityMap = require('Visibility').VisibilityMap;

var NativeUI = runtime.binding('native_ui');

var UI = function () {
  this.initialized = false;
  this.visibilityMap = null;

  this.init = function (params) {
    invariant(this.initialized === false, 'ui already initialized');
    this.initialized = true;
    this.visibilityMap = new VisibilityMap(
      must_have_idx(params, 'map_size'),
      must_have_idx(params, 'num_players')
    );

    NativeUI.setVisibilityBuffer(
      this.visibilityMap.getGridDim(),
      this.visibilityMap.getRawData()
    );
  };

  this.update = function (entities) {
    this.visibilityMap.updateMap(entities);
  };
};

var main = function () {
  return new UI();
}
*/

var main = function () {
  return undefined;
}

module.exports = main;
