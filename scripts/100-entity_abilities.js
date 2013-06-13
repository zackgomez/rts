// This file contains the actions/abilties that entities can perform.
// These are not actions like attack/move/hold position but instead more
// advanced abilities like teleports or heals.
//
// An ability has a few required attributes
// targeting - a value in TargetingTypes
// icon - the name of a texture in config.json
// tooltip - a string describing the action, a newline acts as a line break
// range - the casting range of the ability (should be 0 for TargetingType.NONE)
//
// getState(entity) - returns the state of the action, see ActionStates
// TODO(zack): make this (entity, params)
// exec(entity, target) - runs the action

function ProductionAction(params) {
  this.prod_name = params.prod_name;
  this.time_cost = params.time_cost;
  this.req_cost = params.req_cost;
  this.icon = params.icon;
  this.targeting = TargetingTypes.NONE;
  this.range = 0;

  this.tooltip =  this.prod_name +
    '\nreq: ' + this.req_cost +
    '\ntime: ' + this.time_cost;

  this.getState = function (entity) {
    if (GetRequisition(entity.getPlayerID()) > this.req_cost) {
      return ActionStates.ENABLED;
    } else {
      return ActionStates.DISABLED;
    }
  }

  this.exec = function (entity, target) {
    if (this.getState(entity) != ActionStates.ENABLED) {
      return;
    }
    var prod = {
      name: this.prod_name,
      t: 0.0,
      endt: this.time_cost
    };
    entity.prodQueue_.push(prod);
    Log('Started production of', prod.name, 'for', prod.endt);
    AddRequisition(entity.getPlayerID(), -this.req_cost, entity.getID());
  }
}

function TeleportAction(params) {
  this.range = params.range;
  this.cooldown = params.cooldown;
  this.icon = 'teleport_icon';
  this.tooltip = 'Teleport\nCooldown:'+this.cooldown;
  this.cooldownName = 'teleport';
  this.targeting = TargetingTypes.LOCATION;

  this.getState = function (entity) {
    if (entity.hasCooldown(this.cooldownName)) {
      return ActionStates.COOLDOWN;
    }
    return ActionStates.ENABLED;
  }

  this.exec = function (entity, target) {
    if (this.getState(entity) != ActionStates.ENABLED) {
      return;
    }
    Log('Teleporting to', target, 'Range:', this.range);
    entity.addCooldown(this.cooldownName, this.cooldown);
    entity.warpPosition(target);
  }
}

function SnipeAction(params) {
  this.range = params.range;
  this.cooldown = params.cooldown;
  this.damage = params.damage;

  this.icon = 'ranged_icon';
  this.tooltip = 'Snipe\nDamage:'+this.damage+'\nCooldown:'+this.cooldown;
  this.cooldownName = 'snipe';
  this.targeting = TargetingTypes.ENEMY;

  this.getState = function (entity) {
    // TODO(zack): add mana cost
    if (entity.hasCooldown(this.cooldownName)) {
      return ActionStates.COOLDOWN;
    }
    return ActionStates.ENABLED;
  }

  this.exec = function (entity, target) {
    if (this.getState(entity) != ActionStates.ENABLED) {
      return;
    }
    var target_entity = GetEntity(target);
    if (!target_entity || target_entity.getTeamID() == entity.getTeamID()) {
      Log('not sniping', target, target_entity);
      return;
    }

    entity.addCooldown(this.cooldownName, this.cooldown);

    Log('Sniping', target, 'for', this.damage);
    SendMessage({
      to: target,
      from: entity.getID(),
      type: MessageTypes.ATTACK,
      damage: this.damage,
    });
  }
}

