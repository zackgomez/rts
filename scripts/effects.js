// -- defines --
P_CAPPABLE = 815586235;
P_TARGETABLE = 463132888;
P_ACTOR = 913794634;
NO_PLAYER = 0;
NO_ENTITY = 0;
NO_TEAM = 0;

MessageTypes = {
  ORDER: 'ORDER',
  ATTACK: 'ATTACK',
  CAPTURE: 'CAPTURE',
  ADD_STAT: 'STAT',
};

// -- utility --
function vecAdd(v1, v2) {
  if (v1.length != v2.length) return undefined;

  var ret = [];
  for (var i = 0; i < v1.length; i++) {
    ret.push(v1[i] + v2[i]);
  }
  return ret;
}

// ------------- effects ----------------
function healingAura(radius, amount, entity, dt) {
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

function vpGenEffect(entity, dt) {
  if (entity.getTeamID() == NO_TEAM) {
    return true;
  }

  var amount = 1.0;
  AddVPs(entity.getTeamID(), dt * amount, entity.getID());
  return true;
}

function reqGenEffect(entity, dt) {
  if (entity.getPlayerID() == NO_PLAYER) {
    return true;
  }

  var amount = 1.0;
  AddRequisition(entity.getPlayerID(), dt * amount, entity.getID());
  return true;
}

// -- Entity Definitions --
var EntityDefs =
{
  unit:
  {
    health: 100.0,
  },
  melee_unit:
  {
    health: 50.0,
  },
  building:
  {
    health: 700.0,
    effects:
    {
      req_gen: reqGenEffect,
      base_healing: healingAura.bind(undefined, 5.0, 5.0),
    },
  },
  victory_point:
  {
    cap_time: 5.0,
    effects:
    {
      vp_gen: vpGenEffect,
    },
  },
  req_point:
  {
    cap_time: 5.0,
    effects:
    {
      req_gen: reqGenEffect,
    },
  },
};

// -- Entity Functions --
function entityInit(entity) {
  entity.prodQueue_ = [];

  var name = entity.getName();
  var def = EntityDefs[name];
  if (def) {
    if (def.effects) {
      entity.effects_ = EntityDefs[name].effects;
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

function entityHandleAction(entity, action) {
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
