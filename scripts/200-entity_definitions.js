var EntityConsts = {
  retreat_speed: 1.5,
};

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
    capture_range: 1.0,
    mana: 100,
    weapons: ['rifle'],
    getParts: function (entity) {
      return [
        makePart({
          health: 100,
        }),
        makePart({
          health: 100,
        }),
        makePart({
          health: 100,
        }),
        makePart({
          health: 100,
        }),
        makePart({
          health: 100,
        }),
      ];
    },
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
        healing: 50.0,
        health_target: HEALTH_TARGET_AOE,
        mana_cost: 40,
        icon: 'heal_icon',
        hotkey: 'w',
      }),
      reinforce: new ReinforceAction({
        req_cost: 10,
        cooldown_name: 'reinforce',
        cooldown: 7.0,
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
    capture_range: 1.0,
    mana: 100,
    hotkey: '2',
    weapons: ['advanced_melee'],
    getParts: function (entity) {
      return [
        makePart({
          health: 50,
        }),
        makePart({
          health: 50,
        }),
        makePart({
          health: 50,
        }),
        makePart({
          health: 50,
        }),
      ];
    },
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
        cooldown: 5.0,
        icon: 'repair_icon',
      }),
    },
  },
  tanky_melee_unit: {
    properties: [
      P_ACTOR,
      P_UNIT,
      P_TARGETABLE,
      P_RENDERABLE,
      P_COLLIDABLE,
      P_MOBILE,
    ],
    default_state: UnitIdleState,
    size: [1.6, 1.6, 1.0],
    speed: 2.8,
    sight: 6.0,
    capture_range: 1.0,
    mana: 80,
    hotkey: '3',
    weapons: ['tanky_melee'],
    getParts: function (entity) {
      return [
        makePart({
          health: 150,
        }),
        makePart({
          health: 150,
        }),
        makePart({
          health: 150,
        }),
        makePart({
          health: 150,
        }),
      ];
    },
    getEffects: function (entity) {
      return {
        mana_regen: makeManaRegenEffect(2.5),
      };
    },
    actions: {
      blast: new CenteredAOEAction({
        mana_cost: 60,
        radius: 3.0,
        damage: 35,
        damage_type: 'melee',
        cooldown: 15.0,
        cooldown_name: 'centered_aoe_blast',
        icon: 'melee_icon',
        hotkey: 'q',
      }),
      reinforce: new ReinforceAction({
        req_cost: 5,
        cooldown_name: 'reinforce',
        cooldown: 5.0,
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
    getParts: function (entity) {
      return [
        makePart({
          health: 700,
        }),
      ];
    },
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
      prod_tanky_melee: new ProductionAction({
        prod_name: 'tanky_melee_unit',
        req_cost: 190,
        time_cost: 6.0,
        icon: 'melee_icon',
        hotkey: 'e',
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
};
