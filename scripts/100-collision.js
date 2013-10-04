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
  var resolveCollisions = binding.resolveCollisions;

  var update_body = function (body, dt) {
    var movement_intent = body.getMovementIntent();
    var pos = body.getPosition2();
    var size = body.getSize();
    var angle = body.getAngle();
    if (!movement_intent) {
      return {
        pos: pos,
        size: size,
        angle: angle,
        vel: [0, 0],
      };
    }

    if (movement_intent.look_at) {
      angle = vecAngle(vecSub(movement_intent.look_at, pos));
    }

    if (movement_intent.warp) {
      return {
        pos: movement_intent.warp,
        size: size,
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
      pos: pos,
      size: size,
      angle: angle,
      vel: vel,
    };
  };

  var collide = function (a, b, dt) {
  };

  // each body has
  // getPosition2()
  // getSize()
  // getAngle()
  // getSpeed()
  // getMovementIntent()
  exports.stepAllForward = function (bodies, dt) {
    var new_bodies = _.mapValues(
      bodies,
      function (body) { return update_body(body, dt); }
    );
    var collisions = [];
    resolveCollisions(new_bodies, dt, function (a, b, t) {
      collisions.push({
        a: a,
        b: b,
        t: t,
      });
    });

    for (var i = 0; i < collisions.length; i++) {
      var collision = collisions[i];
      var a = collision.a;
      var b = collision.b;

      if (vecNonZero(new_bodies[a].vel) || vecNonZero(new_bodies[b].vel)) {
        continue;
      }
      if (bodies[a].getPlayerID() !== bodies[b].getPlayerID()) {
        continue;
      }

      var diff = vecSub(new_bodies[b].pos, new_bodies[a].pos);
      var dist = vecLength(diff);
      var dir = dist > 0.00001 ? vecMul(diff, 1 / dist) : vecRandDir2();
      if (bodies[a].hasProperty(P_MOBILE)) {
        new_bodies[a].vel = vecSub(new_bodies[a].vel, dir);
      }
      if (bodies[b].hasProperty(P_MOBILE)) {
        new_bodies[b].vel = vecAdd(new_bodies[b].vel, dir);
      }
    }

    _.each(new_bodies, function (body) {
      body.pos = vecAdd(body.pos, vecMul(body.vel, dt));
    });

    return new_bodies;

    // TODO make warp's not interpolate
  };

  // function locationVisible(int player, vec2 pos): bool
  exports.locationVisible = binding.locationVisible;

  return exports;
}();
