// --
// -- Weapon Definitions --
// --
var Weapons = {
  rifle: {
    type: 'ranged',
    range: 8.0,
    damage: 10.0,
    cooldown_name: 'rifle_cd',
    cooldown: 1.0,
  },
  advanced_melee: {
    type: 'melee',
    range: 1.0,
    damage: 6.0,
    cooldown_name: 'advanced_melee_cd',
    cooldown: 0.5,
  },
}

// --
// -- Entity Definitions --
// --
var EntityDefs = {
  unit: {
    properties: [
      P_ACTOR,
      P_UNIT,
      P_TARGETABLE,
      P_RENDERABLE,
      P_COLLIDABLE,
      P_MOBILE,
    ],
    default_state: UnitIdleState,
    size: [0.6, 0.6, 1.25],
    speed: 3.0,
    sight: 6.0,
    health: 100.0,
    health_bars: 5,
    capture_range: 1.0,
    mana: 100,
    weapon: 'rifle',
    hotkey: '1',
    getEffects: function (entity) {
      return {
        mana_regen: makeManaRegenEffect(2.5),
      };
    },
    actions: {
      snipe: new SnipeAction({
        range: 7.0,
        cooldown_name: 'snipe',
        cooldown: 10.0,
        damage: 100.0,
        mana_cost: 50,
        icon: 'snipe_icon',
        hotkey: 'q',
      }),
      heal: new HealAction({
        range: 5.0,
        cooldown_name: 'heal',
        cooldown: 8.0,
        amount: 10.0,
        mana_cost: 40,
        icon: 'heal_icon',
        hotkey: 'w',
      }),
      reinforce: new ReinforceAction({
        req_cost: 10,
        cooldown_name: 'reinforce',
        cooldown: 5.0,
        icon: 'repair_icon',
      }),
    },
  },
  melee_unit: {
    properties: [
      P_ACTOR,
      P_UNIT,
      P_TARGETABLE,
      P_RENDERABLE,
      P_COLLIDABLE,
      P_MOBILE,
    ],
    default_state: UnitIdleState,
    size: [0.6, 0.6, 1.0],
    speed: 4.5,
    sight: 9.0,
    health: 50.0,
    health_bars: 4,
    capture_range: 1.0,
    mana: 100,
    hotkey: '2',
    weapon: 'advanced_melee',
    getEffects: function (entity) {
      return {
        mana_regen: makeManaRegenEffect(5.0),
      };
    },
    actions: {
      teleport: new TeleportAction({
        range: 8.0,
        cooldown_name: 'teleport',
        cooldown: 2.0,
        mana_cost: 55,
        icon: 'teleport_icon',
        hotkey: 'q',
      }),
      reinforce: new ReinforceAction({
        req_cost: 5,
        cooldown_name: 'reinforce',
        cooldown: 4.0,
        icon: 'repair_icon',
      }),
    },
  },
  base: {
    properties: [
      P_ACTOR,
      P_TARGETABLE,
      P_RENDERABLE,
      P_COLLIDABLE,
    ],
    size: [2.5, 2.5, 1.65],
    sight: 5.0,
    health: 700.0,
    hotkey: '`',
    getEffects: function (entity) {
      return {
        req_gen: makeReqGenEffect(1.0),
        base_healing: makeHealingAura(5.0, 5.0),
      };
    },
    actions: {
      prod_ranged: new ProductionAction({
        prod_name: 'unit',
        req_cost: 100,
        time_cost: 5.0,
        icon: 'ranged_icon',
        hotkey: 'q',
      }),
      prod_melee: new ProductionAction({
        prod_name: 'melee_unit',
        req_cost: 70,
        time_cost: 2.5,
        icon: 'melee_icon',
        hotkey: 'w',
      }),
    },
  },
  victory_point: {
    properties: [
      P_ACTOR,
      P_CAPPABLE,
      P_RENDERABLE,
      P_COLLIDABLE,
    ],
    size: [1.4, 1.4, 0.5],
    sight: 2.0,
    cap_time: 5.0,
    getEffects: function (entity) {
      return {
        vp_gen: makeVpGenEffect(1.0),
      };
    },
  },
  req_point: {
    properties: [
      P_ACTOR,
      P_CAPPABLE,
      P_RENDERABLE,
      P_COLLIDABLE,
    ],
    size: [0.9, 0.9, 0.25],
    sight: 2.0,
    cap_time: 5.0,
    getEffects: function (entity) {
      return {
        req_gen: makeReqGenEffect(1.0),
      };
    },
  },
  projectile: {
    properties: [
      P_RENDERABLE,
      P_MOBILE,
    ],
    speed: 10.0,
    size: [0.6, 0.6, 0.3],
    default_state: ProjectileState,
  },
}
