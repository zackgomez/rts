var invariant = require('invariant').invariant;

function object_fill_keys(keys, value) {
  invariant(Array.isArray(keys), 'keys must be an array');
  var ret = {};
  for (var i = 0; i < keys.length; i++) {
    ret[keys[i]] = value;
  }
  return ret;
};

module.exports = object_fill_keys;
