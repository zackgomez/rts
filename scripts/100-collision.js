var Collision = function () {
  var exports = {};
  var binding = runtime.binding('collision');

  // function pointInOBB2(p, center, size, angle): bool
  exports.pointInOBB2 = binding.pointInOBB2;

  return exports;
}();

var Pathing = function () {
  var exports = {}

  // HACK ALERT
  _.mixin({ mapValues: function (obj, f_val) {
      return _.object(_.keys(obj), _.map(obj, f_val));
  }});

  var binding = runtime.binding('pathing');

  var update_body = function (body, dt) {
    var movement_intent = body.getMovementIntent();
    var pos = body.getPosition2();
    var angle = body.getAngle();
    if (!movement_intent) {
      return {
        pos: pos,
        angle: angle,
        vel: [0, 0],
      };
    }

    if (movement_intent.look_at) {
      angle = vecAngleBetween(vecSub(movement_intent.look_at, pos), [1, 0]);
    }

    if (movement_intent.warp) {
      return {
        pos: movement_intent.warp,
        angle: angle,
        vel: [0, 0],
      };
    }
    var vel = [0, 0];
    var target_pos = movement_intent.move_towards;
    if (target_pos) {
      vel = vecMul(
        vecNormalize(vecSub(target_pos, pos)),
        body.getSpeed()
      );
    }
    return {
      pos: vecAdd(pos, vecMul(vel, dt)),
      angle: angle,
      vel: vel,
    };
  };

  // each body has
  // getPosition2()
  // getSize()
  // getAngle()
  // getSpeed()
  // getMovementIntent()
  exports.stepAllForward = function (bodies, dt) {
    return _.mapValues(bodies, function (body) { return update_body(body, dt); });

    // TODO collision detection
    // TODO make warp's not interpolate
  };

  // function locationVisible(int player, vec2 pos): bool
  exports.locationVisible = binding.locationVisible;

  return exports;
}();
