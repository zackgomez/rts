// -- utility --
function vecAdd(v1, v2) {
  if (v1.length != v2.length) return undefined;

  var ret = [];
  for (var i = 0; i < v1.length; i++) {
    ret.push(v1[i] + v2[i]);
  }
  return ret;
}
// production
function entityUpdate(entity, dt) {
  entity.prodTimer_ -= dt;
  if (entity.prod_ && entity.prodTimer_ <= 0.0) {
    SpawnEntity(
      entity.prod_,
      {
        entity_pid: entity.getPlayerID(),
        entity_pos: vecAdd(entity.getPosition2(), entity.getDirection()),
        entity_angle: entity.getAngle()
      });
    entity.prod_ = null;
  }
}

function entityHandleAction(entity, action) {
  if (action.type == "production") {
    // TODO(zack): allow queueing
    if (entity.prod_) {
      return;
    }
    if (GetRequisition(entity.getPlayerID()) > action.req_cost) {
      entity.prod_ = action.prod_name;
      entity.prodTimer_ = action.time_cost;
      Log('added', entity.prod_, 'for', entity.prodTimer_);

      AddRequisition(entity.getPlayerID(), -action.req_cost, entity.getID());
    }
  } else {
    Log(entity.getID(), 'unknown action type', action.type);
  }
}

function entityInit(entity) {
  entity.prod_ = null;
  entity.prodTimer_ = 0.0;
}


// ------------- effects ----------------
function __healingAura(radius, amount, entity, dt) {
  var radius = 5.0;
  var amount = 5.0;

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

baseHealingAura = __healingAura.bind(undefined, 5.0, 5.0);

function vpGenEffect(entity, dt) {
  if (!entity.getPlayerID()) {
    return true;
  }

  var amount = 1.0;
  AddVPs(entity.getTeamID(), dt * amount, entity.getID());
  return true;
}

function reqGenEffect(entity, dt) {
  if (!entity.getPlayerID()) {
    return true;
  }

  var amount = 1.0;
  AddRequisition(entity.getPlayerID(), dt * amount, entity.getID());
  return true;
}
