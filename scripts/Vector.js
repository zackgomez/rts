var _ = require('underscore');
var invariant = require('invariant').invariant;

module.exports = {
  add: function (v1, v2) {
    invariant(v1.length === v2.length, "vectors must have same length");

    var ret = [];
    for (var i = 0; i < v1.length; i++) {
      ret.push(v1[i] + v2[i]);
    }
    return ret;
  },

  // returns v1 - v2
  sub: function (v1, v2) {
    invariant(v1.length === v2.length, "vectors must have same length");

    var ret = [];
    for (var i = 0; i < v1.length; i++) {
      ret.push(v1[i] - v2[i]);
    }
    return ret;
  },

  compMul: function (v1, v2) {
    invariant(v1.length === v2.length, "vectors must have same length");

    var ret = [];
    for (var i = 0; i < v1.length; i++) {
      ret.push(v1[i] * v2[i]);
    }
    return ret;
  },

  compDiv: function (v1, v2) {
    invariant(v1.length === v2.length, "vectors must have same length");

    var ret = [];
    for (var i = 0; i < v1.length; i++) {
      ret.push(v1[i] / v2[i]);
    }
    return ret;
  },

  mul: function (v, s) {
    var ret = [];
    for (var i = 0; i < v.length; i++) {
      ret.push(v[i] * s);
    }
    return ret;
  },

  div: function (v, s) {
    var ret = [];
    for (var i = 0; i < v.length; i++) {
      ret.push(v[i] / s);
    }
    return ret;
  },

  dot: function (v1, v2) {
    invariant(
      v1.length === v2.length,
      "must have same sized vectors to dot"
    );

    var dot = 0;
    for (var i = 0; i < v1.length; i++) {
      dot += v1[i] * v2[i];
    }
    return dot;
  },

  length2: function (v) {
    return this.dot(v, v);
  },

  length: function (v) {
    return Math.sqrt(this.length2(v));
  },

  normalize: function (v) {
    return this.mul(v, 1 / this.length(v));
  },

  distance: function (v1, v2) {
    return this.length(this.sub(v1, v2));
  },

  // Returns angle between vector and [1, 0] in degrees
  angle: function (v) {
    invariant(v.length === 2, 'can only get angle between 2d vectors');
    return 180 / Math.PI * Math.atan2(v[1], v[0]);
  },

  angleBetween: function (v1, v2) {
    invariant(v1.length === 2, 'can only get angle between 2d vectors');
    var diff = this.sub(v1, v2);
    return 180 / Math.PI * Math.atan2(diff[1], diff[0]);
  },

  isNonZero: function (v) {
    for (var i = 0; i < v.length; i++) {
      if (v[i] !== 0) {
        return true;
      }
    }
    return false;
  },

  randDir2: function () {
    var vec = [
      (Math.random() - 0.5) * 2,
      (Math.random() - 0.5) * 2,
    ];
    return this.normalize(vec);
  },

  min: function (v, s) {
    return _.map(v, function (x) {
      return Math.min(s, x);
    });
  },

  max: function (v, s) {
    return _.map(v, function (x) {
      return Math.max(s, x);
    });
  },

  clamp: function (v, min, max) {
    return _.map(v, function (x) {
      return Math.min(Math.max(x, min), max);
    });
  },

  floor: function (v) {
    return _.map(v, Math.floor);
  },

  ceil: function (v) {
    return _.map(v, Math.ceil);
  },
};
