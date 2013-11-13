var _ = require('underscore');
var invariant = require('invariant').invariant;

function Part(params) {
  if (!params.health) {
    throw new Error("Missing part health");
  }
  this.maxHealth_ = params.health;
  this.health_ = this.maxHealth_;
  this.description_ = params.description;
  this.name_ = params.name;
  this.availableUpgrades_ = params.upgrades || {};
  this.completeUpgrades_ = {};


  this.deadUpdate_ = params.dead_update ?
    param.dead_update :
    function (entity) {};

  this.aliveUpdate = params.alive_update ?
    param.alive_update :
    function (entity) {};

  // member functions

  // getters
  this.isAlive = function () {
    return this.health_ > 0;
  };
  this.hasUpgrade = function (name) {
    return name in this.completeUpgrades_ && this.completeUpgrades_[name];
  };
  this.getAvailableUpgrades = function () {
    return this.availableUpgrades_;
  };
  this.getHealth = function () {
    return this.health_;
  };
  this.getMaxHealth = function () {
    return this.maxHealth_;
  };
  this.getName = function () {
    return this.name_;
  };
  this.getTooltip = function () {
    var tooltip = this.name_;
    tooltip += '\n' + this.description_;
    if (!_.isEmpty(this.completeUpgrades_)) {
      tooltip += '\nUPGRADES:'
      _.each(this.completeUpgrades_, function (upgrade, name) {
        tooltip += '\n' + name;
        if (upgrade.health) {
          tooltip += '\nHealth: ' + upgrade.health;
        }
        tooltip += '\n' + upgrade.tooltip;
      });
    }
    if (!_.isEmpty(this.availableUpgrades_)) {
      tooltip += '\nNEW UPGRADES:'
      _.each(this.availableUpgrades_, function (upgrade, name) {
        tooltip += '\n' + name;
        tooltip += '\nReq: ' + upgrade.req_cost;
        if (upgrade.health) {
          tooltip += '\nHealth: ' + upgrade.health;
        }
        tooltip += '\n' + upgrade.tooltip;
      });
    }
    return tooltip;
  };

  // setters
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
  this.purchaseUpgrade = function (name) {
    if (!(name in this.availableUpgrades_)) {
      return;
    }

    var upgrade = this.availableUpgrades_[name];
    if (upgrade.health) {
      invariant(upgrade.health > 0, 'expected positive health');
      this.maxHealth_ += upgrade.health;
      this.health_ += upgrade.health;
    }
    this.completeUpgrades_[name] = upgrade;
    delete this.availableUpgrades_[name];
  };
}

Part.makePart = function (params) {
  return new Part(params);
}

module.exports = Part;
