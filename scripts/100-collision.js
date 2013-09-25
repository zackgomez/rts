var Collision = function () {
  var exports = {};
  var binding = runtime.binding('collision');

  // function pointInOBB2(p, center, size, angle): bool
  exports.pointInOBB2 = binding.pointInOBB2;

  return exports;
}();

var Pathing = function () {
  var exports = {}

  var binding = runtime.binding('pathing');

  // function locationVisible(int player, vec2 pos): bool
  exports.locationVisible = binding.locationVisible;

  return exports;
}();
