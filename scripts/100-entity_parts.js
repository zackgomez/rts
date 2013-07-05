//
// -- Core part class definition
// --
function Part (params) {
  if (!params.health) {
    throw new Error("Missing part health");
  }
  this.maxHealth_ = params.health;
  this.health_ = this.maxHealth_;

  this.deadUpdate_ = params.dead_update ?
    param.dead_update :
    function (entity) {};

  this.aliveUpdate = params.alive_update ?
    param.alive_update :
    function (entity) {};

  // member functions

  this.getHealth = function () {
    return this.health_;
  };
  this.getMaxHealth = function () {
    return this.maxHealth_;
  };
  this.setHealth = function (delta) {
    this.health_ = this.maxHealth_;
  };
  this.addHealth = function (delta) {
    this.health_ = Math.min(
      this.health_ + delta,
      this.maxHealth_
    );
    return this.health_;
  };

  this.update = function (entity) {
    if (this.health_ <= 0) {
      this.deadUpdate_(entity);
    } else {
      this.aliveUpdate_(entity);
    }
  };
}

function makePart(params) {
  return new Part(params);
}

// 
// -- Part update functions
//
