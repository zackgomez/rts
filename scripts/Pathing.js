var _ = require('underscore');
var invariant = require('invariant').invariant;
var Vector = require('Vector');

var EntityProperties = require('constants').EntityProperties;

// HACK ALERT
_.mixin({ mapValues: function (obj, f_val) {
  return _.object(_.keys(obj), _.map(obj, f_val));
}});

var binding = runtime.binding('pathing');
var resolveCollisions = binding.resolveCollisions;
var computePath = function (a, b) { return [b] }

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
    angle = Vector.angle(Vector.sub(movement_intent.look_at, pos));
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
  var path = [];
  if (target_pos) {
    path = computePath(pos, target_pos);
    invariant(path.length >= 1, 'path must have at least one node');
    var node = path[0];
    angle = Vector.angle(Vector.sub(node, pos));
    vel = Vector.mul(
      Vector.normalize(Vector.sub(node, pos)),
      body.getSpeed()
    );
  }
  return {
    pos: pos,
    size: size,
    angle: angle,
    vel: vel,
    path: path,
  };
};

// each body has
// getPosition2()
// getSize()
// getAngle()
// getSpeed()
// getMovementIntent()
// getPlayerID()
// hasProperty()
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

    if (Vector.isNonZero(new_bodies[a].vel) || Vector.isNonZero(new_bodies[b].vel)) {
      continue;
    }
    if (bodies[a].getPlayerID() !== bodies[b].getPlayerID()) {
      continue;
    }

    var diff = Vector.sub(new_bodies[b].pos, new_bodies[a].pos);
    var dist = Vector.length(diff);
    var dir = dist > 0.00001 ? Vector.mul(diff, 1 / dist) : Vector.randDir2();
    // TODO(zack): remove this dependency on EntityProperties, instead use
    // can collide on 'body'
    if (bodies[a].hasProperty(EntityProperties.P_MOBILE)) {
      new_bodies[a].vel = Vector.sub(new_bodies[a].vel, dir);
    }
    if (bodies[b].hasProperty(EntityProperties.P_MOBILE)) {
      new_bodies[b].vel = Vector.add(new_bodies[b].vel, dir);
    }
  }

  _.each(new_bodies, function (body) {
    body.pos = Vector.add(body.pos, Vector.mul(body.vel, dt));
  });

  return new_bodies;
};
