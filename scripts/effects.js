// -- Defines/Constants --
P_CAPPABLE = 815586235;
P_TARGETABLE = 463132888;
P_ACTOR = 913794634;
P_RENDERABLE = 565038773;
P_COLLIDABLE = 983556954;
P_MOBILE = 1122719651;

NO_PLAYER = 0;
NO_ENTITY = 0;
NO_TEAM = 0;

MessageTypes = {
  ORDER: 'ORDER',
  ATTACK: 'ATTACK',
  CAPTURE: 'CAPTURE',
  ADD_STAT: 'STAT',
};

// -- Utility --
function vecAdd(v1, v2) {
  if (v1.length != v2.length) return undefined;

  var ret = [];
  for (var i = 0; i < v1.length; i++) {
    ret.push(v1[i] + v2[i]);
  }
  return ret;
}

// -- Effects --
function makeHealingAura(radius, amount) {
  return function(entity, dt) {
    entity.getNearbyEntities(radius, function (nearby_entity) {
      if (nearby_entity.getPlayerID() == entity.getPlayerID()) {
        SendMessage({
          to: nearby_entity.getID(),
          from: entity.getID(),
          type: "STAT",
          healing: dt * amount
        });
      }

      return true;
    });

    return true;
  }
}

function makeVpGenEffect(amount) {
  return function(entity, dt) {
    if (entity.getTeamID() == NO_TEAM) {
      return true;
    }

    AddVPs(entity.getTeamID(), dt * amount, entity.getID());
    return true;
  }
}

function makeReqGenEffect(amount) {
  return function(entity, dt) {
    if (entity.getPlayerID() == NO_PLAYER) {
      return true;
    }

    var amount = 1.0;
    AddRequisition(entity.getPlayerID(), dt * amount, entity.getID());
    return true;
  }
}

// -- Entity Definitions --
var EntityDefs =
{
  unit:
  {
    properties:
    [
      P_ACTOR,
      P_TARGETABLE,
      P_RENDERABLE,
      P_COLLIDABLE,
      P_MOBILE,
    ],
    health: 100.0,
  },
  melee_unit:
  {
    properties:
    [
      P_ACTOR,
      P_TARGETABLE,
      P_RENDERABLE,
      P_COLLIDABLE,
      P_MOBILE,
    ],
    health: 50.0,
  },
  building:
  {
    properties:
    [
      P_ACTOR,
      P_TARGETABLE,
      P_RENDERABLE,
      P_COLLIDABLE,
    ],
    health: 700.0,
    effects:
    {
      req_gen: makeReqGenEffect(1.0),
      base_healing: makeHealingAura(5.0, 5.0),
    },
    actions:
    {
      prod_ranged:
      {
        type: 'production',
        prod_name: 'unit',
        req_cost: 100,
        time_cost: 5.0,
        icon: 'ranged_icon',
      },
      prod_melee:
      {
        type: 'production',
        prod_name: 'melee_unit',
        req_cost: 70,
        time_cost: 2.5,
        icon: 'melee_icon',
      },
    },
  },
  victory_point:
  {
    properties:
    [
      P_ACTOR,
      P_CAPPABLE,
      P_RENDERABLE,
      P_COLLIDABLE,
    ],
    cap_time: 5.0,
    effects:
    {
      vp_gen: makeVpGenEffect(1.0),
    },
  },
  req_point:
  {
    properties:
    [
      P_ACTOR,
      P_CAPPABLE,
      P_RENDERABLE,
      P_COLLIDABLE,
    ],
    cap_time: 5.0,
    effects:
    {
      req_gen: makeReqGenEffect(1.0),
    },
  },
};

// -- Entity Helper Functions --
function getActionTooltip(action) {
  if (action.type == 'production') {
    return action.prod_name +
      '\nreq: ' + action.req_cost +
      '\ntime: ' + action.time_cost;
  } else {
    Log('No tooltip for action type', action.type);
    throw new Error('No tooltip for action type ' + action.type);
  }
}

// -- Entity Functions --
function entityInit(entity) {
  entity.prodQueue_ = [];

  var name = entity.getName();
  var def = EntityDefs[name];
  if (def) {
    if (def.properties) {
      for (var i = 0; i < def.properties.length; i++) {
        entity.setProperty(def.properties[i], true);
      }
    }
    if (def.health) {
      entity.maxHealth_ = def.health;
      entity.health_ = entity.maxHealth_;
    }
    if (def.cap_time) {
      entity.capTime_ = def.cap_time;
      entity.cappingPlayerID_ = null;
      entity.capAmount_ = 0.0;
      entity.capResetDelay_ = 0;
    }
    if (def.effects) {
      entity.effects_ = EntityDefs[name].effects;
    }
    if (def.actions) {
      entity.actions_ = EntityDefs[name].actions;
    }
  }
}

function entityUpdate(entity, dt) {
  if (entity.prodQueue_.length) {
    var prod = entity.prodQueue_[0];
    prod.t += dt;
    if (prod.t >= prod.endt) {
      SpawnEntity(
        prod.name,
        {
          entity_pid: entity.getPlayerID(),
          entity_pos: vecAdd(entity.getPosition2(), entity.getDirection()),
          entity_angle: entity.getAngle()
        });
      entity.prodQueue_.pop();
    }
  }

  entity.capResetDelay_ += 1;
  if (entity.getPlayerID() !== NO_PLAYER || entity.capResetDelay_ > 1) {
    entity.capAmount_ = 0.0;
    entity.cappingPlayerID_ = null;
  }

  for (var ename in entity.effects_) {
    var res = entity.effects_[ename](entity, dt);
    if (!res) {
      delete entity.effects_[ename];
    }
  }
}

function entityHandleMessage(entity, msg) {
  if (msg.type == MessageTypes.CAPTURE) {
    if (!entity.capTime_) {
      Log('Uncappable entity received capture message');
      return;
    }
    if (!entity.cappingPlayerID_ || entity.cappingPlayerID_ === msg.pid) {
      entity.capResetDelay_ = 0;
      entity.cappingPlayerID_ = msg.pid;
      entity.capAmount_ += msg.cap;
      if (entity.capAmount_ >= entity.capTime_) {
        entity.setPlayerID(entity.cappingPlayerID_);
      }
    }
  } else if (msg.type == MessageTypes.ATTACK) {
    if (!entity.maxHealth_) {
      Log('Entity without health received attack message');
      return;
    }
    entity.health_ -= msg.damage;
    if (entity.health_ <= 0.0) {
      entity.destroy();
    }
    // TODO(zack): melee timer
    // TODO(zack): Compute this in UIInfo/update
    entity.onTookDamage();
  } else if (msg.type == MessageTypes.ADD_STAT) {
    if (msg.healing) {
      entity.health_ = Math.min(
        entity.health_ + msg.healing,
        entity.maxHealth_);
    }
  } else {
    Log('Unknown message of type', msg.type);
  }
}

function entityHandleAction(entity, action_name) {
  action = entity.actions_[action_name];
  if (!action) {
    Log(entity.getID(), 'got unknown action', action_name);
  }
  if (action.type == 'production') {
    if (GetRequisition(entity.getPlayerID()) > action.req_cost) {
      var prod = {
        name: action.prod_name,
        t: 0.0,
        endt: action.time_cost
      };
      entity.prodQueue_.push(prod);
      Log('Started production of', prod.name, 'for', prod.endt);
      AddRequisition(entity.getPlayerID(), -action.req_cost, entity.getID());
    }
  } else {
    Log(entity.getID(), 'unknown action type', action.type);
  }
}

function entityGetActions(entity) {
  if (!entity.actions_) {
    return [];
  }
  var actions = [];
  for (var action_name in entity.actions_) {
    var action = entity.actions_[action_name];
    actions.push({
      name: action_name,
      icon: action.icon,
      tooltip: getActionTooltip(action),
    });
  }

  return actions;
}

function entityGetUIInfo(entity) {
  var ui_info = {};
  if (entity.hasProperty(P_CAPPABLE)) {
    if (entity.cappingPlayerID_) {
      ui_info.capture = [entity.capAmount_, 5.0];
    }
    return ui_info;
  }

  if (entity.prodQueue_.length) {
    var prod = entity.prodQueue_[0];
    ui_info.production = [prod.t, prod.endt];
  }

  if (entity.maxHealth_) {
    ui_info.health = [entity.health_, entity.maxHealth_];
  }

  return ui_info;
}
