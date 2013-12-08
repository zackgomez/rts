// This function is called with the native runtime object to start up the game
// engine
//
// This file largely ripped from the Node.js minimal module system and startup
// process
(function (runtime, starting_module) {
  this.global = this;

  function bootstrap() {
    bootstrap.globalVariables();
    var invariant = NativeModule.require('invariant').invariant;

    invariant(starting_module, 'must have starting module');
    var main = NativeModule.require(starting_module);
    invariant(
      typeof main === 'function',
      'main module must export a function \'main\''
    );

    return main();
  };

  bootstrap.globalVariables = function () {
    global.global = global;
    global.root = global;
    global.runtime = runtime;
  };

  function NativeModule(id) {
    this.filename = id + '.js';
    this.id = id;
    this.exports = {};
    this.loaded = false;
  };

  NativeModule._source = runtime.source_map;
  NativeModule._cache = {};

  NativeModule.require = function(id) {
    if (id === 'native_module') {
      return NativeModule;
    }

    var cached = NativeModule.getCached(id);
    if (cached) {
      return cached.exports;
    }

    if (!NativeModule.exists(id)) {
      throw new Error('No such native module ' + id);
    }

    var nativeModule = new NativeModule(id);

    nativeModule.cache();
    nativeModule.compile();

    return nativeModule.exports;
  };

  NativeModule.getCached = function(id) {
    return NativeModule._cache[id];
  }

  NativeModule.exists = function(id) {
    return NativeModule._source.hasOwnProperty(id);
  }

  NativeModule.getSource = function(id) {
    return NativeModule._source[id];
  }

  NativeModule.wrap = function(script) {
    return NativeModule.wrapper[0] + script + NativeModule.wrapper[1];
  };

  NativeModule.wrapper = [
    '(function (exports, require, module, __filename) {',
    '\n});'
  ];

  NativeModule.prototype.compile = function() {
    var source = NativeModule.getSource(this.id);
    source = NativeModule.wrap(source);

    var fn = runtime.eval(source, {filename: this.filename});
    var args = [this.exports, NativeModule.require, this, this.filename];
    fn.apply(this.exports, args);

    this.loaded = true;
  };

  NativeModule.prototype.cache = function() {
    NativeModule._cache[this.id] = this;
  };

  return bootstrap();
});
