
function Part (params) {
  if (!params.health) {
    throw new Error("Missing part health");
  }
  this.maxHealth_ = params.health;
  this.health_ = this.maxHealth_;

  this.getHealth = function () {
    return this.health_;
  }
  this.getMaxHealth = function () {
    return this.maxHealth_;
  }
  this.setHealth = function (delta) {
    this.health_ = this.maxHealth_;
  }
  this.addHealth = function (delta) {
    this.health_ = Math.min(
      this.health_ + delta,
      this.maxHealth_
    );
    return this.health_;
  }
}

function makePart(params) {
  return new Part(params);
}
