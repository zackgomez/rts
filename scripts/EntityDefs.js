var Actions = require('Actions');
var DamageTypes = require('constants').DamageTypes;
var Effects = require('Effects');
var EntityProperties = require('constants').EntityProperties;
var EntityStates = require('EntityStates');
var makePart = require('Parts').makePart;

// --
// -- Entity Definitions --
// --
var EntityDefs = {
  unit: {
    properties: [
      EntityProperties.P_ACTOR,
      EntityProperties.P_UNIT,
      EntityProperties.P_TARGETABLE,
      EntityProperties.P_RENDERABLE,
      EntityProperties.P_COLLIDABLE,
      EntityProperties.P_MOBILE,
    ],
    model: 'ranged_unit',
    default_state: EntityStates.UnitIdleState,
    size: [0.6, 0.6],
    height: 1.25,
    speed: 3.0,
    sight: 6.0,
    capture_range: 1.0,
    mana: 100,
    weapons: ['advanced_rifle', 'rifle', 'basic_melee'],
    getParts: function (entity) {
      return [
        makePart({
          name: 'head',
          health: 50,
          description: 'Just Health',
          upgrades: {
            'heal': {
              health: 20,
              req_cost: 50,
              tooltip: 'improves health, and adds heal ability',
            },
          },
        }),
        makePart({
          name: 'left',
          health: 100,
          description: 'provides rifle weapon',
          upgrades: {
            'advanced_rifle': {
              health: 20,
              req_cost: 50,
              tooltip: 'improves health, range, dps and adds snipe ability',
            },
          },
        }),
        makePart({
          name: 'right',
          health: 100,
          description: 'Just Health',
        }),
        makePart({
          name: 'body',
          health: 100,
          description: 'Just Health',
        }),
        makePart({
          name: 'legs',
          health: 100,
          description: 'Just Health',
        }),
      ];
    },
    hotkey: '2',
    getEffects: function (entity) {
      return {
        mana_regen: Effects.makeManaRegenEffect(2.5),
      };
    },
    actions: {
      snipe: new Actions.SnipeAction({
        range: 7.0,
        cooldown_name: 'snipe',
        cooldown: 10.0,
        damage: 100.0,
        mana_cost: 50,
        icon: 'snipe_icon',
        hotkey: 'q',
        part: 'left',
        part_upgrade: 'advanced_rifle',
      }),
      heal: new Actions.HealAction({
        range: 5.0,
        cooldown_name: 'heal',
        cooldown: 8.0,
        healing: 25.0,
        health_target: DamageTypes.HEALTH_TARGET_AOE,
        mana_cost: 40,
        icon: 'heal_icon',
        hotkey: 'w',
        part: 'head',
        part_upgrade: 'heal',
      }),
      reinforce: new Actions.ReinforceAction({
        req_cost: 10,
        cooldown_name: 'reinforce',
        cooldown: 7.0,
        icon: 'repair_icon',
      }),
    },
  },
  melee_unit: {
    properties: [
      EntityProperties.P_ACTOR,
      EntityProperties.P_UNIT,
      EntityProperties.P_TARGETABLE,
      EntityProperties.P_RENDERABLE,
      EntityProperties.P_COLLIDABLE,
      EntityProperties.P_MOBILE,
    ],
    model: 'melee_unit',
    default_state: EntityStates.UnitIdleState,
    size: [0.6, 0.6],
    height: 1.0,
    speed: 4.0,
    sight: 9.0,
    capture_range: 1.0,
    mana: 100,
    hotkey: '1',
    weapons: ['advanced_melee'],
    getParts: function (entity) {
      return [
        makePart({
          name: 'head',
          health: 50,
          description: 'Just Health',
        }),
        makePart({
          name: 'left',
          health: 50,
          description: 'Just Health',
        }),
        makePart({
          name: 'right',
          health: 50,
          description: 'Just Health',
        }),
        makePart({
          name: 'body',
          health: 50,
          description: 'Just Health',
          upgrades: {
            'teleport': {
              health: 50,
              req_cost: 30,
              tooltip: 'Adds health and teleport ability',
            },
          },
        }),
        makePart({
          name: 'legs',
          health: 50,
          description: 'Just Health',
        }),
      ];
    },
    getEffects: function (entity) {
      return {
        mana_regen: Effects.makeManaRegenEffect(5.0),
      };
    },
    actions: {
      teleport: new Actions.TeleportAction({
        range: 8.0,
        cooldown_name: 'teleport',
        cooldown: 2.0,
        mana_cost: 5,
        icon: 'teleport_icon',
        hotkey: 'q',
        part: 'body',
        part_upgrade: 'teleport',
      }),

      reinforce: new Actions.ReinforceAction({
        req_cost: 5,
        cooldown_name: 'reinforce',
        cooldown: 5.0,
        icon: 'repair_icon',
      }),
    },
  },
  tanky_melee_unit: {
    properties: [
      EntityProperties.P_ACTOR,
      EntityProperties.P_UNIT,
      EntityProperties.P_TARGETABLE,
      EntityProperties.P_RENDERABLE,
      EntityProperties.P_COLLIDABLE,
      EntityProperties.P_MOBILE,
    ],
    model: 'tanky_melee_unit',
    default_state: EntityStates.UnitIdleState,
    size: [1.6, 1.6],
    height: 1.0,
    speed: 2.8,
    sight: 6.0,
    capture_range: 1.0,
    mana: 80,
    hotkey: '3',
    weapons: ['tanky_melee'],
    getParts: function (entity) {
      return [
        makePart({
          name: 'right',
          health: 150,
          description: 'Just Health',
        }),
        makePart({
          name: 'left',
          health: 150,
          description: 'Just Health',
        }),
        makePart({
          name: 'body',
          health: 150,
          description: 'Just Health',
          upgrades: {
            'damage_aura': {
              health: 50,
              req_cost: 70,
              tooltip: 'improves health, and grants a damage buff aura',
            },
          },
        }),
        makePart({
          name: 'legs',
          health: 150,
          description: 'Just Health',
        }),
      ];
    },
    getEffects: function (entity) {
      return {
        mana_regen: Effects.makeManaRegenEffect(2.5),
        damage_buff_aura: Effects.makeDamageBuffAura({
          radius: 10,
          amount: 0.20,
          active_func: function (entity) {
            var part = entity.getPart('body');
            return part.isAlive() && part.hasUpgrade('damage_aura');
          },
        }),
      };
    },
    actions: {
      blast: new Actions.CenteredAOEAction({
        mana_cost: 60,
        radius: 3.0,
        damage: 35,
        damage_type: 'melee',
        cooldown: 15.0,
        cooldown_name: 'centered_aoe_blast',
        icon: 'melee_icon',
        hotkey: 'q',
      }),
      reinforce: new Actions.ReinforceAction({
        req_cost: 5,
        cooldown_name: 'reinforce',
        cooldown: 5.0,
        icon: 'repair_icon',
      }),
    },
  },
  base: {
    properties: [
      EntityProperties.P_ACTOR,
      EntityProperties.P_TARGETABLE,
      EntityProperties.P_RENDERABLE,
      EntityProperties.P_COLLIDABLE,
    ],
    model: 'building',
    size: [2.5, 2.5],
    height: 1.65,
    sight: 5.0,
    getParts: function (entity) {
      return [
        makePart({
          health: 700,
          description: 'Base model',
        }),
      ];
    },
    hotkey: '`',
    getEffects: function (entity) {
      return {
        req_gen: Effects.makeReqGenEffect(1.0),
        base_healing: Effects.makeHealingAura(5.0, 5.0),
      };
    },
    actions: {
      prod_melee: new Actions.ProductionAction({
        prod_name: 'melee_unit',
        req_cost: 70,
        time_cost: 2.5,
        icon: 'melee_icon',
        hotkey: 'q',
      }),
      prod_ranged: new Actions.ProductionAction({
        prod_name: 'unit',
        req_cost: 100,
        time_cost: 5.0,
        icon: 'ranged_icon',
        hotkey: 'w',
      }),
      prod_tanky_melee: new Actions.ProductionAction({
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
      EntityProperties.P_ACTOR,
      EntityProperties.P_CAPPABLE,
      EntityProperties.P_RENDERABLE,
      EntityProperties.P_COLLIDABLE,
    ],
    model: 'victory_point',
    minimap_icon: 'vp_minimap_icon',
    size: [1.4, 1.4],
    height: 0.5,
    sight: 2.0,
    cap_time: 5.0,
    getEffects: function (entity) {
      return {
        vp_gen: Effects.makeVpGenEffect(1.0),
      };
    },
  },
  req_point: {
    properties: [
      EntityProperties.P_ACTOR,
      EntityProperties.P_CAPPABLE,
      EntityProperties.P_RENDERABLE,
      EntityProperties.P_COLLIDABLE,
    ],
    model: 'req_point',
    minimap_icon: 'req_minimap_icon',
    size: [0.9, 0.9],
    height: 0.25,
    sight: 2.0,
    cap_time: 5.0,
    getEffects: function (entity) {
      return {
        req_gen: Effects.makeReqGenEffect(1.0),
      };
    },
  },
  projectile: {
    properties: [
      EntityProperties.P_RENDERABLE,
      EntityProperties.P_MOBILE,
    ],
    model: 'basic_bullet',
    speed: 10.0,
    size: [0.6, 0.6],
    height: 0.3,
    default_state: EntityStates.ProjectileState,
  },
};


module.exports = EntityDefs;
