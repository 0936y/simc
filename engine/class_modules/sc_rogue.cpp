// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

using namespace residual_action;

namespace { // UNNAMED NAMESPACE

/* ==========================================================================
  --------- Legion TODO ---------
  Common
    Preparation : RIP
    Recuperate : RIP
    Shiv : RIP
    Poisons : To Remove from Outlaw & Subtlety
    Crimson Tempest : May be ReAdded for Ass/Sub, not finished
    Sprint : CD Reduced by 30s
  Outlaw
   Abilities
    AR : Same
    Ambush : Same
    Between the Eyes : To Implement - Finisher with Scaling Stun + Damage (Currently only Stun on Alpha)
    Blade Flurry : Same (Perks is now baseline)
    Eviscerate : Copy then change to Run Through
    Garrote : To Remove from Outlaw
    Killing Spree : Same but a Talent
    Pistol Shot : To Implement - Builder with an empowered proc from SS (blindside like ?)
    Revealing Strike : RIP
    Run Through : New Eviscerate
    Saber Slash : New Siniter Strike
    Sinister Strike : Replaced by Saber Slash
    Slice and Dice : Same (Only Outlaw now)
   Passive
    Combat Potency : No more +5% haste
    Mastery : Same
    Ruthlessness : CD Reduction & Energy refund removed, only 20% to get 1 CP per CP spent
    Swashbuckler : New Improved Dual Wield -- Same
    Bandit's Guile : RIP
   Talent
    T15
     -WOD
     Nightstalker : Removed from Outlaw
     Subterfuge : Removed from Outlaw
     Shadow Focus : Removed from Outlaw
     -Legion
     Ghotly Strike : To Implement - New Builder with dmg increased
     Swordmaster : To Implement - Increase double SS proc chance
     Quick Draw : To Implement - Better Pistol Shot
    T45
     -Legion
      Deeper Strategem : To Implement - Now a built-in 6th CP (so finisher can consume up to 6)
      Anticipation : Max stacks reduced to 3
      Vigor : To Implement - Energy Max increased by 50 + Energy Regen increased by 10%
    T90
     -WOD
      Shuriken Toss : To Remove from Outlaw
      Marked for Death : Now a T100 talent
      Anticipation : Now a T45 talent
     -Legion
      Cannonball Barrage : To Implement - GTAoE (5-8y not sure) that deal dmg over 1.8s, do not break AA nor is channeled (like Army of the Dead)
      Alacrity : To Implement - 1% haste per finisher during 20s, refreshed each time and stacking to 25
      Killing Spree : No longer baseline, T90 talent
    T100
     -WOD
      Venom Rush : RIP
      Shadow Reflection : RIP ?
      Death from Above : Still here
     -Legion
      Roll the Bones : To Implement - Replacement of SnD that gives random buff, same duration as SnD
      Marked for Death : Still here (T90->T100 talent)
      Death from Above : Still here
  Subtlety
    Everything
  Assassination
    Everything
========================================================================== */

// ==========================================================================
// Custom Combo Point Impl.
// ==========================================================================

struct rogue_t;
namespace actions
{
struct rogue_attack_state_t;
struct residual_damage_state_t;
struct rogue_poison_t;
struct rogue_attack_t;
struct rogue_poison_buff_t;
struct melee_t;
}

namespace buffs
{
struct marked_for_death_debuff_t;
}

enum current_weapon_e
{
  WEAPON_PRIMARY = 0u,
  WEAPON_SECONDARY
};

enum weapon_slot_e
{
  WEAPON_MAIN_HAND = 0u,
  WEAPON_OFF_HAND
};

struct weapon_info_t
{
  // State of the hand, i.e., primary or secondary weapon currently equipped
  current_weapon_e     current_weapon;
  // Pointers to item data
  const item_t*        item_data[ 2 ];
  // Computed weapon data
  weapon_t             weapon_data[ 2 ];
  // Computed stats data
  gear_stats_t         stats_data[ 2 ];
  // Callbacks, associated with special effects on each weapons
  std::vector<dbc_proc_callback_t*> cb_data[ 2 ];

  // Item data storage for secondary weapons
  item_t               secondary_weapon_data;

  // Protect against multiple initialization since the init is done in an action_t object.
  bool                 initialized;

  // Track secondary weapon uptime through a buff
  buff_t*              secondary_weapon_uptime;

  weapon_info_t() :
    current_weapon( WEAPON_PRIMARY ), initialized( false ), secondary_weapon_uptime( nullptr )
  {
    range::fill( item_data, nullptr );
  }

  weapon_slot_e slot() const;
  void initialize();
  void reset();

  // Enable/disable callbacks on the primary/secondary weapons.
  void callback_state( current_weapon_e weapon, bool state );
};

// ==========================================================================
// Rogue
// ==========================================================================

struct rogue_td_t : public actor_target_data_t
{
  struct dots_t
  {
    dot_t* deadly_poison;
    dot_t* garrote;
    dot_t* hemorrhage;
    dot_t* killing_spree; // Strictly speaking, this should probably be on player
    dot_t* rupture;
  } dots;

  struct debuffs_t
  {
    buff_t* vendetta;
    buff_t* wound_poison;
    buff_t* crippling_poison;
    buff_t* leeching_poison;
    buffs::marked_for_death_debuff_t* marked_for_death;
  } debuffs;

  rogue_td_t( player_t* target, rogue_t* source );

  bool poisoned() const
  {
    return dots.deadly_poison -> is_ticking() ||
           debuffs.wound_poison -> check() ||
           debuffs.crippling_poison -> check() ||
           debuffs.leeching_poison -> check();
  }
};

struct rogue_t : public player_t
{
  // Venom Rush poison tracking
  unsigned poisoned_enemies;

  // Premeditation
  event_t* event_premeditation;

  // Active
  attack_t* active_blade_flurry;
  actions::rogue_poison_t* active_lethal_poison;
  actions::rogue_poison_t* active_nonlethal_poison;
  action_t* active_main_gauche;

  // Autoattacks
  action_t* auto_attack;
  actions::melee_t* melee_main_hand;
  actions::melee_t* melee_off_hand;

  // Data collection
  luxurious_sample_data_t* dfa_mh, *dfa_oh;

  // Experimental weapon swapping
  weapon_info_t weapon_data[ 2 ];

  // Tier 18 (WoD 6.2) trinket effects
  const special_effect_t* toxic_mutilator;
  const special_effect_t* eviscerating_blade;
  const special_effect_t* from_the_shadows;

  // Buffs
  struct buffs_t
  {
    buff_t* adrenaline_rush;
    buff_t* blade_flurry;
    buff_t* deadly_poison;
    buff_t* death_from_above;
    buff_t* feint;
    buff_t* killing_spree;
    buff_t* master_of_subtlety;
    buff_t* master_of_subtlety_passive;
    buff_t* nightstalker;
    buff_t* free_pistol_shot; // TODO: Not its real name, need to see in game what it is
    buff_t* shadow_dance;
    buff_t* shadowstep;
    buff_t* sleight_of_hand;
    buff_t* sprint;
    buff_t* stealth;
    buff_t* subterfuge;
    buff_t* t16_2pc_melee;
    buff_t* tier13_2pc;
    buff_t* tot_trigger;
    buff_t* vanish;
    buff_t* wound_poison;

    // Ticking buffs
    buff_t* envenom;
    buff_t* slice_and_dice;

    // Legendary buffs
    buff_t* fof_fod; // Fangs of the Destroyer
    stat_buff_t* fof_p1; // Phase 1
    stat_buff_t* fof_p2;
    stat_buff_t* fof_p3;
    stat_buff_t* toxicologist;

    buff_t* anticipation;
    buff_t* deceit;
    buff_t* shadow_strikes;
    buff_t* deathly_shadows;
  } buffs;

  // Cooldowns
  struct cooldowns_t
  {
    cooldown_t* adrenaline_rush;
    cooldown_t* killing_spree;
    cooldown_t* sprint;
    cooldown_t* vanish;
  } cooldowns;

  // Gains
  struct gains_t
  {
    gain_t* adrenaline_rush;
    gain_t* combat_potency;
    gain_t* deceit;
    gain_t* energetic_recovery;
    gain_t* energy_refund;
    gain_t* murderous_intent;
    gain_t* overkill;
    gain_t* relentless_strikes;
    gain_t* shadow_strikes;
    gain_t* t17_2pc_assassination;
    gain_t* t17_4pc_assassination;
    gain_t* t17_2pc_subtlety;
    gain_t* t17_4pc_subtlety;
    gain_t* venomous_wounds;
    gain_t* vitality;

    // CP Gains
    gain_t* empowered_fan_of_knives;
    gain_t* premeditation;
    gain_t* seal_fate;
    gain_t* legendary_daggers;
  } gains;

  // Spec passives
  struct spec_t
  {
    // Shared
    const spell_data_t* shadowstep;

    // Assassination
    const spell_data_t* assassins_resolve;
    const spell_data_t* cut_to_the_chase;
    const spell_data_t* improved_poisons;
    const spell_data_t* seal_fate;
    const spell_data_t* venomous_wounds;

    // Outlaw
    const spell_data_t* blade_flurry;
    const spell_data_t* combat_potency;
    const spell_data_t* ruthlessness;
    const spell_data_t* saber_slash;
    const spell_data_t* swashbuckler; // Ambidexterity
    const spell_data_t* vitality;

    // Subtlety
    const spell_data_t* energetic_recovery;
    const spell_data_t* master_of_shadows;
    const spell_data_t* shadow_dance;
    const spell_data_t* shadow_techniques;
  } spec;

  // Spell Data
  struct spells_t
  {
    const spell_data_t* critical_strikes;
    const spell_data_t* death_from_above;
    const spell_data_t* fan_of_knives;
    const spell_data_t* fleet_footed;
    const spell_data_t* sprint;
    const spell_data_t* relentless_strikes;
    const spell_data_t* ruthlessness_cp_driver;
    const spell_data_t* ruthlessness_driver;
    const spell_data_t* ruthlessness_cp;
    const spell_data_t* shadow_focus;
    const spell_data_t* tier13_2pc;
    const spell_data_t* tier13_4pc;
    const spell_data_t* tier15_4pc;
    const spell_data_t* tier18_2pc_combat_ar;
  } spell;

  // Talents
  struct talents_t
  {
    // Shared - Level 45
    const spell_data_t* deeper_strategem;
    const spell_data_t* anticipation;
    const spell_data_t* vigor;

    // Shared - Level 90
    const spell_data_t* alacrity;

    // Shared - Level 100
    const spell_data_t* marked_for_death;
    const spell_data_t* death_from_above;

    // Assassination/Subtlety - Level 30
    const spell_data_t* nightstalker;
    const spell_data_t* subterfuge;
    const spell_data_t* shadow_focus;

    // Assassination - Level 15
    const spell_data_t* master_poisoner;
    const spell_data_t* elaborate_planning;
    const spell_data_t* hemorrhage;

    // Assassination - Level 90
    const spell_data_t* numbing_poison;
    const spell_data_t* blood_sweat;

    // Assassination - Level 100
    const spell_data_t* venom_rush;

    // Outlaw - Level 15
    const spell_data_t* ghostly_strike;
    const spell_data_t* swordmaster;
    const spell_data_t* quick_draw;

    // Outlaw - Level 30 (NYI)

    // Outlaw - Level 90
    const spell_data_t* cannonball_barrage;
    const spell_data_t* killing_spree;

    // Outlaw - Level 100
    const spell_data_t* roll_the_bones;

    // Subtlety - Level 15
    const spell_data_t* master_of_subtlety;
    const spell_data_t* weaponmaster;
    const spell_data_t* gloomblade;

    // Subtlety - Level 90
    const spell_data_t* premeditation;
    const spell_data_t* enveloping_shadows;

    // Subtlety - Level 100
    const spell_data_t* relentless_strikes;
  } talent;

  // Masteries
  struct masteries_t
  {
    const spell_data_t* executioner;
    const spell_data_t* main_gauche;
    const spell_data_t* potent_poisons;
  } mastery;

  // Procs
  struct procs_t
  {
    proc_t* seal_fate;
    proc_t* t16_2pc_melee;
    proc_t* t18_2pc_combat;

    proc_t* anticipation_wasted;
  } procs;

  player_t* tot_target;
  action_callback_t* virtual_hat_callback;

  // Options
  uint32_t fof_p1, fof_p2, fof_p3;
  int initial_combo_points;

  rogue_t( sim_t* sim, const std::string& name, race_e r = RACE_NIGHT_ELF ) :
    player_t( sim, ROGUE, name, r ),
    poisoned_enemies( 0 ),
    event_premeditation( nullptr ),
    active_blade_flurry( nullptr ),
    active_lethal_poison( nullptr ),
    active_nonlethal_poison( nullptr ),
    active_main_gauche( nullptr ),
    auto_attack( nullptr ), melee_main_hand( nullptr ), melee_off_hand( nullptr ),
    dfa_mh( nullptr ), dfa_oh( nullptr ),
    toxic_mutilator( nullptr ),
    eviscerating_blade( nullptr ),
    from_the_shadows( nullptr ),
    buffs( buffs_t() ),
    cooldowns( cooldowns_t() ),
    gains( gains_t() ),
    spec( spec_t() ),
    spell( spells_t() ),
    talent( talents_t() ),
    mastery( masteries_t() ),
    procs( procs_t() ),
    tot_target( nullptr ),
    virtual_hat_callback( nullptr ),
    fof_p1( 0 ), fof_p2( 0 ), fof_p3( 0 ),
    initial_combo_points( 0 )
  {
    // Cooldowns
    cooldowns.adrenaline_rush     = get_cooldown( "adrenaline_rush"     );
    cooldowns.killing_spree       = get_cooldown( "killing_spree"       );
    cooldowns.sprint              = get_cooldown( "sprint"              );
    cooldowns.vanish              = get_cooldown( "vanish"              );

    base.distance = 3;
    regen_type = REGEN_DYNAMIC;
    regen_caches[CACHE_HASTE] = true;
    regen_caches[CACHE_ATTACK_HASTE] = true;
  }

  // Character Definition
  void      init_spells() override;
  void      init_base_stats() override;
  void      init_gains() override;
  void      init_procs() override;
  void      init_scaling() override;
  void      init_resources( bool force ) override;
  bool      init_items() override;
  void      init_special_effects() override;
  bool      init_finished() override;
  void      create_buffs() override;
  void      create_options() override;
  void      copy_from( player_t* source ) override;
  std::string      create_profile( save_e stype ) override;
  void      init_action_list() override;
  void      register_callbacks() override;
  void      reset() override;
  void      arise() override;
  void      regen( timespan_t periodicity ) override;
  timespan_t available() const override;
  action_t* create_action( const std::string& name, const std::string& options ) override;
  expr_t*   create_expression( action_t* a, const std::string& name_str ) override;
  resource_e primary_resource() const override { return RESOURCE_ENERGY; }
  role_e    primary_role() const override  { return ROLE_ATTACK; }
  stat_e    convert_hybrid_stat( stat_e s ) const override;

  double    composite_melee_speed() const override;
  double    composite_melee_crit() const override;
  double    composite_spell_crit() const override;
  double    matching_gear_multiplier( attribute_e attr ) const override;
  double    composite_attack_power_multiplier() const override;
  double    composite_player_multiplier( school_e school ) const override;
  double    energy_regen_per_second() const override;
  double    passive_movement_modifier() const override;
  double    temporary_movement_modifier() const override;
  void combat_begin() override;

  bool poisoned_enemy( player_t* target, bool deadly_fade = false ) const;

  void trigger_auto_attack( const action_state_t* );
  void trigger_seal_fate( const action_state_t* );
  void trigger_main_gauche( const action_state_t* );
  void trigger_combat_potency( const action_state_t* );
  void trigger_energy_refund( const action_state_t* );
  void trigger_venomous_wounds( const action_state_t* );
  void trigger_blade_flurry( const action_state_t* );
  void trigger_combo_point_gain( const action_state_t*, int = -1, gain_t* gain = nullptr, bool allow_anticipation = true );
  void spend_combo_points( const action_state_t* );
  bool trigger_t17_4pc_combat( const action_state_t* );
  void trigger_anticipation_replenish( const action_state_t* );

  target_specific_t<rogue_td_t> target_data;

  virtual rogue_td_t* get_target_data( player_t* target ) const override
  {
    rogue_td_t*& td = target_data[ target ];
    if ( ! td )
    {
      td = new rogue_td_t( target, const_cast<rogue_t*>(this) );
    }
    return td;
  }

  static actions::rogue_attack_t* cast_attack( action_t* action )
  { return debug_cast<actions::rogue_attack_t*>( action ); }

  static const actions::rogue_attack_t* cast_attack( const action_t* action )
  { return debug_cast<const actions::rogue_attack_t*>( action ); }

  void swap_weapon( weapon_slot_e slot, current_weapon_e to_weapon, bool in_combat = true );
};

namespace actions { // namespace actions

// ==========================================================================
// Static Functions
// ==========================================================================

// break_stealth ============================================================

static void break_stealth( rogue_t* p )
{
  if ( p -> buffs.stealth -> check() )
    p -> buffs.stealth -> expire();

  if ( p -> buffs.vanish -> check() )
    p -> buffs.vanish -> expire();
}

// ==========================================================================
// Rogue Attack
// ==========================================================================

struct rogue_attack_state_t : public action_state_t
{
  int cp;

  rogue_attack_state_t( action_t* action, player_t* target ) :
    action_state_t( action, target ), cp( 0 )
  { }

  void initialize() override
  { action_state_t::initialize(); cp = 0; }

  std::ostringstream& debug_str( std::ostringstream& s ) override
  { action_state_t::debug_str( s ) << " cp=" << cp; return s; }

  void copy_state( const action_state_t* o ) override
  {
    action_state_t::copy_state( o );
    const rogue_attack_state_t* st = debug_cast<const rogue_attack_state_t*>( o );
    cp = st -> cp;
  }
};

struct rogue_attack_t : public melee_attack_t
{
  bool         requires_stealth;
  position_e   requires_position;
  int          adds_combo_points;
  weapon_e     requires_weapon;

  // we now track how much combo points we spent on an action
  int              combo_points_spent;

  // Combo point gains
  gain_t* cp_gain;

  // Sinister calling proc action
  rogue_attack_t* sc_action;

  rogue_attack_t( const std::string& token, rogue_t* p,
                  const spell_data_t* s = spell_data_t::nil(),
                  const std::string& options = std::string() ) :
    melee_attack_t( token, p, s ),
    requires_stealth( false ), requires_position( POSITION_NONE ),
    adds_combo_points( 0 ),
    requires_weapon( WEAPON_NONE ),
    combo_points_spent( 0 ),
    sc_action( nullptr )
  {
    parse_options( options );

    may_crit                  = true;
    may_glance                = false;
    special                   = true;
    tick_may_crit             = true;
    hasted_ticks              = false;

    for ( size_t i = 1; i <= s -> effect_count(); i++ )
    {
      const spelleffect_data_t& effect = s -> effectN( i );

      switch ( effect.type() )
      {
        case E_ADD_COMBO_POINTS:
          adds_combo_points = effect.base_value();
          break;
        default:
          break;
      }

      if ( effect.type() == E_APPLY_AURA && effect.subtype() == A_PERIODIC_DAMAGE )
        base_ta_adder = effect.bonus( player );
      else if ( effect.type() == E_SCHOOL_DAMAGE )
        base_dd_adder = effect.bonus( player );
    }
  }

  void init() override
  {
    melee_attack_t::init();

    if ( adds_combo_points )
      cp_gain = player -> get_gain( name_str );
  }

  virtual void snapshot_state( action_state_t* state, dmg_e rt ) override
  {
    melee_attack_t::snapshot_state( state, rt );

    if ( base_costs[ RESOURCE_COMBO_POINT ] > 0 )
      cast_state( state ) -> cp = static_cast<int>( player -> resources.current[RESOURCE_COMBO_POINT] );
  }

  bool stealthed()
  {
    return p() -> buffs.vanish -> check() || p() -> buffs.stealth -> check() || player -> buffs.shadowmeld -> check();
  }

  virtual bool procs_poison() const
  { return weapon != nullptr; }

  // Generic rules for proccing Main Gauche, used by rogue_t::trigger_main_gauche()
  virtual bool procs_main_gauche() const
  { return callbacks && ! proc && weapon != nullptr && weapon -> slot == SLOT_MAIN_HAND; }

  // Adjust poison proc chance
  virtual double composite_poison_flat_modifier( const action_state_t* ) const
  { return 0.0; }

  action_state_t* new_state() override
  { return new rogue_attack_state_t( this, target ); }

  static const rogue_attack_state_t* cast_state( const action_state_t* st )
  { return debug_cast< const rogue_attack_state_t* >( st ); }

  static rogue_attack_state_t* cast_state( action_state_t* st )
  { return debug_cast< rogue_attack_state_t* >( st ); }

  rogue_t* cast()
  { return p(); }
  const rogue_t* cast() const
  { return p(); }

  rogue_t* p()
  { return debug_cast< rogue_t* >( player ); }
  const rogue_t* p() const
  { return debug_cast< rogue_t* >( player ); }

  rogue_td_t* td( player_t* t ) const
  { return p() -> get_target_data( t ); }

  virtual double cost() const override;
  virtual void   execute() override;
  virtual void   consume_resource() override;
  virtual bool   ready() override;
  virtual void   impact( action_state_t* state ) override;

  virtual double attack_direct_power_coefficient( const action_state_t* s ) const override
  {
    if ( base_costs[ RESOURCE_COMBO_POINT ] )
      return attack_power_mod.direct * cast_state( s ) -> cp;
    return melee_attack_t::attack_direct_power_coefficient( s );
  }

  virtual double attack_tick_power_coefficient( const action_state_t* s ) const override
  {
    if ( base_costs[ RESOURCE_COMBO_POINT ] )
      return attack_power_mod.tick * cast_state( s ) -> cp;
    return melee_attack_t::attack_tick_power_coefficient( s );
  }

  virtual double spell_direct_power_coefficient( const action_state_t* s ) const override
  {
    if ( base_costs[ RESOURCE_COMBO_POINT ] )
      return spell_power_mod.direct * cast_state( s ) -> cp;
    return melee_attack_t::spell_direct_power_coefficient( s );
  }

  virtual double spell_tick_power_coefficient( const action_state_t* s ) const override
  {
    if ( base_costs[ RESOURCE_COMBO_POINT ] )
      return spell_power_mod.tick * cast_state( s ) -> cp;
    return melee_attack_t::spell_tick_power_coefficient( s );
  }

  virtual double bonus_da( const action_state_t* s ) const override
  {
    if ( base_costs[ RESOURCE_COMBO_POINT ] )
      return base_dd_adder * cast_state( s ) -> cp;
    return melee_attack_t::bonus_da( s );
  }

  virtual double bonus_ta( const action_state_t* s ) const override
  {
    if ( base_costs[ RESOURCE_COMBO_POINT ] )
      return base_ta_adder * cast_state( s ) -> cp;
    return melee_attack_t::bonus_ta( s );
  }

  virtual double composite_da_multiplier( const action_state_t* state ) const override
  {
    double m = melee_attack_t::composite_da_multiplier( state );

    if ( base_costs[ RESOURCE_COMBO_POINT ] && p() -> mastery.executioner -> ok() )
      m *= 1.0 + p() -> cache.mastery_value();

    return m;
  }

  virtual double composite_ta_multiplier( const action_state_t* state ) const override
  {
    double m = melee_attack_t::composite_ta_multiplier( state );

    if ( base_costs[ RESOURCE_COMBO_POINT ] && p() -> mastery.executioner -> ok() )
      m *= 1.0 + p() -> cache.mastery_value();

    return m;
  }

  virtual double composite_target_multiplier( player_t* target ) const override
  {
    double m = melee_attack_t::composite_target_multiplier( target );

    rogue_td_t* tdata = td( target );

    m *= 1.0 + tdata -> debuffs.vendetta -> value();

    return m;
  }

  virtual double action_multiplier() const override
  {
    double m = melee_attack_t::action_multiplier();

    if ( p() -> talent.nightstalker -> ok() && p() -> buffs.stealth -> check() )
      m += p() -> talent.nightstalker -> effectN( 2 ).percent();

    return m;
  }
};

// ==========================================================================
// Poisons
// ==========================================================================

struct rogue_poison_t : public rogue_attack_t
{
  double proc_chance_;

  rogue_poison_t( const std::string& token, rogue_t* p,
                  const spell_data_t* s = spell_data_t::nil() ) :
    actions::rogue_attack_t( token, p, s ),
    proc_chance_( 0 )
  {
    proc              = true;
    background        = true;
    trigger_gcd       = timespan_t::zero();
    may_dodge         = false;
    may_parry         = false;
    may_block         = false;
    callbacks         = false;

    weapon_multiplier = 0;

    proc_chance_  = data().proc_chance();
    proc_chance_ += p -> spec.improved_poisons -> effectN( 1 ).percent();
  }

  timespan_t execute_time() const override
  { return timespan_t::zero(); }

  virtual double proc_chance( const action_state_t* source_state )
  {
    double chance = proc_chance_;

    if ( p() -> buffs.envenom -> up() )
      chance += p() -> buffs.envenom -> data().effectN( 2 ).percent();

    const rogue_attack_t* attack = debug_cast< const rogue_attack_t* >( source_state -> action );
    chance += attack -> composite_poison_flat_modifier( source_state );

    return chance;
  }

  virtual void trigger( const action_state_t* source_state )
  {
    bool result = rng().roll( proc_chance( source_state ) );

    if ( sim -> debug )
      sim -> out_debug.printf( "%s attempts to proc %s, target=%s source_action=%s proc_chance=%.3f: %d",
          player -> name(), name(), source_state -> target -> name(), source_state -> action -> name(), proc_chance( source_state ), result );

    if ( ! result )
      return;

    target = source_state -> target;
    execute();
  }

  virtual double action_da_multiplier() const override
  {
    double m = rogue_attack_t::action_da_multiplier();

    if ( p() -> mastery.potent_poisons -> ok() )
      m *= 1.0 + p() -> cache.mastery_value();

    return m;
  }

  virtual double action_ta_multiplier() const override
  {
    double m = rogue_attack_t::action_ta_multiplier();

    if ( p() -> mastery.potent_poisons -> ok() )
      m *= 1.0 + p() -> cache.mastery_value();

    return m;
  }
};

// Venomous Wound ===========================================================

struct venomous_wound_t : public rogue_poison_t
{
  venomous_wound_t( rogue_t* p ) :
    rogue_poison_t( "venomous_wound", p, p -> find_spell( 79136 ) )
  {
    background       = true;
    proc             = true;
  }

  double composite_da_multiplier( const action_state_t* state ) const override
  {
    double m = rogue_poison_t::composite_da_multiplier( state );

    m *= 1.0 + p() -> sets.set( SET_MELEE, T14, B2 ) -> effectN( 1 ).percent();

    return m;
  }
};

// Deadly Poison ============================================================

struct deadly_poison_t : public rogue_poison_t
{
  struct deadly_poison_dd_t : public rogue_poison_t
  {
    deadly_poison_dd_t( rogue_t* p ) :
      rogue_poison_t( "deadly_poison_instant", p, p -> find_spell( 113780 ) )
    {
      harmful          = true;
    }
  };

  struct deadly_poison_dot_t : public rogue_poison_t
  {
    deadly_poison_dot_t( rogue_t* p ) :
      rogue_poison_t( "deadly_poison_dot", p, p -> find_specialization_spell( "Deadly Poison" ) -> effectN( 1 ).trigger() )
    {
      may_crit       = false;
      harmful        = true;
    }

    void impact( action_state_t* state ) override
    {
      if ( ! p() -> poisoned_enemy( state -> target ) && result_is_hit( state -> result ) )
      {
        p() -> poisoned_enemies++;
      }

      rogue_poison_t::impact( state );
    }

    void last_tick( dot_t* d ) override
    {
      player_t* t = d -> state -> target;

      rogue_poison_t::last_tick( d );

      // Due to DOT system behavior, deliver "Deadly Poison DOT fade event" as
      // a separate parmeter to poisoned_enemy() call.
      if ( ! p() -> poisoned_enemy( t, true ) )
      {
        p() -> poisoned_enemies--;
      }
    }
  };

  deadly_poison_dd_t*  proc_instant;
  deadly_poison_dot_t* proc_dot;

  deadly_poison_t( rogue_t* player ) :
    rogue_poison_t( "deadly_poison", player, player -> find_specialization_spell( "Deadly Poison" ) ),
    proc_instant( nullptr ), proc_dot( nullptr )
  {
    dual = true;
    may_miss = may_crit = false;

    proc_instant = new deadly_poison_dd_t( player );
    proc_dot     = new deadly_poison_dot_t( player );
  }

  virtual void impact( action_state_t* state ) override
  {
    bool is_up = ( td( state -> target ) -> dots.deadly_poison -> is_ticking() != 0 );

    rogue_poison_t::impact( state );

    if ( result_is_hit( state -> result ) )
    {
      proc_dot -> target = state -> target;
      proc_dot -> execute();
      if ( is_up )
      {
        proc_instant -> target = state -> target;
        proc_instant -> execute();
      }
    }
  }
};

// Wound Poison =============================================================

struct wound_poison_t : public rogue_poison_t
{
  struct wound_poison_dd_t : public rogue_poison_t
  {
    wound_poison_dd_t( rogue_t* p ) :
      rogue_poison_t( "wound_poison", p, p -> find_specialization_spell( "Wound Poison" ) -> effectN( 1 ).trigger() )
    {
      harmful          = true;
    }

    void impact( action_state_t* state ) override
    {
      rogue_poison_t::impact( state );

      if ( result_is_hit( state -> result ) )
      {
        td( state -> target ) -> debuffs.wound_poison -> trigger();

        if ( ! sim -> overrides.mortal_wounds )
          state -> target -> debuffs.mortal_wounds -> trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, data().duration() );
      }
    }
  };

  wound_poison_dd_t* proc_dd;

  wound_poison_t( rogue_t* player ) :
    rogue_poison_t( "wound_poison_driver", player, player -> find_specialization_spell( "Wound Poison" ) )
  {
    dual           = true;
    may_miss = may_crit = false;

    proc_dd = new wound_poison_dd_t( player );
  }

  void impact( action_state_t* state ) override
  {
    rogue_poison_t::impact( state );

    proc_dd -> target = state -> target;
    proc_dd -> execute();
  }
};

// Crippling poison =========================================================

struct crippling_poison_t : public rogue_poison_t
{
  struct crippling_poison_proc_t : public rogue_poison_t
  {
    crippling_poison_proc_t( rogue_t* rogue ) :
      rogue_poison_t( "crippling_poison", rogue, rogue -> find_spell( 3409 ) )
    { }

    void impact( action_state_t* state ) override
    {
      rogue_poison_t::impact( state );

      td( state -> target ) -> debuffs.crippling_poison -> trigger();
    }
  };

  crippling_poison_proc_t* proc;

  crippling_poison_t( rogue_t* player ) :
    rogue_poison_t( "crippling_poison_driver", player, player -> find_specialization_spell( "Crippling Poison" ) ),
    proc( new crippling_poison_proc_t( player ) )
  {
    dual = true;
    may_miss = may_crit = false;
  }

  void impact( action_state_t* state ) override
  {
    rogue_poison_t::impact( state );

    proc -> target = state -> target;
    proc -> execute();
  }
};

// Leeching poison =========================================================

struct leeching_poison_t : public rogue_poison_t
{
  struct leeching_poison_proc_t : public rogue_poison_t
  {
    leeching_poison_proc_t( rogue_t* rogue ) :
      rogue_poison_t( "leeching_poison", rogue, rogue -> find_spell( 112961 ) )
    { }

    void impact( action_state_t* state ) override
    {
      rogue_poison_t::impact( state );

      td( state -> target ) -> debuffs.leeching_poison -> trigger();
    }
  };

  leeching_poison_proc_t* proc;

  leeching_poison_t( rogue_t* player ) :
    rogue_poison_t( "leeching_poison_driver", player, player -> find_talent_spell( "Leeching Poison" ) ),
    proc( new leeching_poison_proc_t( player ) )
  {
    dual = true;
    may_miss = may_crit = false;
  }

  void impact( action_state_t* state ) override
  {
    rogue_poison_t::impact( state );

    proc -> target = state -> target;
    proc -> execute();
  }
};

// Apply Poison =============================================================

struct apply_poison_t : public action_t
{
  enum poison_e
  {
    POISON_NONE = 0,
    DEADLY_POISON,
    WOUND_POISON,
    INSTANT_POISON,
    CRIPPLING_POISON,
    LEECHING_POISON,
  };
  poison_e lethal_poison;
  poison_e nonlethal_poison;
  bool executed;

  apply_poison_t( rogue_t* p, const std::string& options_str ) :
    action_t( ACTION_OTHER, "apply_poison", p ),
    lethal_poison( POISON_NONE ), nonlethal_poison( POISON_NONE ),
    executed( false )
  {
    std::string lethal_str;
    std::string nonlethal_str;

    add_option( opt_string( "lethal", lethal_str ) );
    add_option( opt_string( "nonlethal", nonlethal_str ) );
    parse_options( options_str );
    ignore_false_positive = true;

    trigger_gcd = timespan_t::zero();
    harmful = false;

    if ( p -> main_hand_weapon.type != WEAPON_NONE || p -> off_hand_weapon.type != WEAPON_NONE )
    {
      if ( lethal_str == "deadly"    ) lethal_poison = DEADLY_POISON;
      if ( lethal_str == "instant"   ) lethal_poison = INSTANT_POISON;
      if ( lethal_str == "wound"     ) lethal_poison = WOUND_POISON;

      if ( nonlethal_str == "crippling" ) nonlethal_poison = CRIPPLING_POISON;
      if ( nonlethal_str == "leeching"  ) nonlethal_poison = LEECHING_POISON;
    }

    if ( ! p -> active_lethal_poison )
    {
      if ( lethal_poison == DEADLY_POISON  ) p -> active_lethal_poison = new deadly_poison_t( p );
      if ( lethal_poison == WOUND_POISON   ) p -> active_lethal_poison = new wound_poison_t( p );
    }

    if ( ! p -> active_nonlethal_poison )
    {
      if ( nonlethal_poison == CRIPPLING_POISON ) p -> active_nonlethal_poison = new crippling_poison_t( p );
      if ( nonlethal_poison == LEECHING_POISON  ) p -> active_nonlethal_poison = new leeching_poison_t( p );
    }
  }

  void reset() override
  {
    action_t::reset();

    executed = false;
  }

  virtual void execute() override
  {
    executed = true;

    if ( sim -> log )
      sim -> out_log.printf( "%s performs %s", player -> name(), name() );
  }

  virtual bool ready() override
  {
    if ( player -> specialization() != ROGUE_ASSASSINATION )
    {
      return false;
    }

    return ! executed;
  }
};

// ==========================================================================
// Attacks
// ==========================================================================

// rogue_attack_t::impact ===================================================

void rogue_attack_t::impact( action_state_t* state )
{
  melee_attack_t::impact( state );

  if ( adds_combo_points )
    p() -> trigger_seal_fate( state );

  p() -> trigger_main_gauche( state );
  p() -> trigger_combat_potency( state );
  p() -> trigger_blade_flurry( state );

  if ( result_is_hit( state -> result ) )
  {
    if ( procs_poison() && p() -> active_lethal_poison )
      p() -> active_lethal_poison -> trigger( state );

    if ( procs_poison() && p() -> active_nonlethal_poison )
      p() -> active_nonlethal_poison -> trigger( state );

    // Legendary Daggers buff handling
    // Proc rates from: https://github.com/Aldriana/ShadowCraft-Engine/blob/master/shadowcraft/objects/proc_data.py#L504
    // Logic from: https://github.com/simulationcraft/simc/issues/1117
    double fof_chance = ( p() -> specialization() == ROGUE_ASSASSINATION ) ? 0.23139 : ( p() -> specialization() == ROGUE_OUTLAW ) ? 0.09438 : 0.28223;
    if ( state -> target && state -> target -> level() > 88 )
    {
      fof_chance *= ( 1.0 - 0.1 * ( state -> target -> level() - 88 ) );
    }
    if ( rng().roll( fof_chance ) )
    {
      p() -> buffs.fof_p1 -> trigger();
      p() -> buffs.fof_p2 -> trigger();
      p() -> buffs.fof_p3 -> trigger();

      if ( ! p() -> buffs.fof_fod -> check() && p() -> buffs.fof_p3 -> check() > 30 )
      {
        // Trigging FoF and the Stacking Buff are mutually exclusive
        if ( rng().roll( 1.0 / ( 51.0 - p() -> buffs.fof_p3 -> check() ) ) )
        {
          p() -> buffs.fof_fod -> trigger();
          p() -> trigger_combo_point_gain( state, 5, p() -> gains.legendary_daggers );
        }
      }
    }

    // Prevent poisons from proccing Toxicologist
    if ( ! proc && td( state -> target ) -> debuffs.vendetta -> up() )
      p() -> buffs.toxicologist -> trigger();
  }
}

// rogue_attack_t::cost =====================================================

double rogue_attack_t::cost() const
{
  double c = melee_attack_t::cost();

  if ( c <= 0 )
    return 0;

  if ( p() -> talent.shadow_focus -> ok() &&
       ( p() -> buffs.stealth -> check() || p() -> buffs.vanish -> check() ) )
  {
    c *= 1.0 + p() -> spell.shadow_focus -> effectN( 1 ).percent();
  }

  if ( p() -> sets.has_set_bonus( SET_MELEE, T15, B2 ) && p() -> buffs.tier13_2pc -> check() )
    c *= 1.0 + p() -> spell.tier13_2pc -> effectN( 1 ).percent();

  if ( c <= 0 )
    c = 0;

  return c;
}

// rogue_attack_t::consume_resource =========================================

// NOTE NOTE NOTE: Eviscerate / Envenom both override this fully due to Death
// from Above
void rogue_attack_t::consume_resource()
{
  melee_attack_t::consume_resource();

  p() -> spend_combo_points( execute_state );

  if ( result_is_miss( execute_state -> result ) && resource_consumed > 0 )
    p() -> trigger_energy_refund( execute_state );
}

// rogue_attack_t::execute ==================================================

void rogue_attack_t::execute()
{
  melee_attack_t::execute();

  // T17 4PC combat has to occur before combo point gain, so we can get
  // Ruthlessness to function properly with Anticipation
  bool combat_t17_4pc_triggered = p() -> trigger_t17_4pc_combat( execute_state );

  p() -> trigger_auto_attack( execute_state );

  p() -> trigger_combo_point_gain( execute_state );

  // Anticipation only refreshes Combo Points, if the Combat and Subtlety T17
  // 4pc set bonuses are not in effect. Note that currently in game, Shadow
  // Strikes (Sub 4PC) does not prevent the consumption of Anticipation, but
  // presuming here that it is a bug.
  if ( ! combat_t17_4pc_triggered && ! p() -> buffs.shadow_strikes -> check() )
    p() -> trigger_anticipation_replenish( execute_state );

  // Subtlety T17 4PC set bonus processing on the "next finisher"
  if ( result_is_hit( execute_state -> result ) &&
       base_costs[ RESOURCE_COMBO_POINT ] > 0 &&
       p() -> buffs.shadow_strikes -> check() )
  {
    p() -> buffs.shadow_strikes -> expire();
    double cp = player -> resources.max[ RESOURCE_COMBO_POINT ] - player -> resources.current[ RESOURCE_COMBO_POINT ];

    if ( cp > 0 )
      player -> resource_gain( RESOURCE_COMBO_POINT, cp, p() -> gains.t17_4pc_subtlety );
  }

  if ( harmful && stealthed() )
  {
    player -> buffs.shadowmeld -> expire();

    if ( ! p() -> talent.subterfuge -> ok() )
      break_stealth( p() );
    // Check stealthed again after shadowmeld is popped. If we're still
    // stealthed, trigger subterfuge
    else if ( stealthed() && ! p() -> buffs.subterfuge -> check() )
      p() -> buffs.subterfuge -> trigger();
  }
}

// rogue_attack_t::ready() ==================================================

inline bool rogue_attack_t::ready()
{
  rogue_t* p = cast();

  if ( ! melee_attack_t::ready() )
    return false;

  if ( base_costs[ RESOURCE_COMBO_POINT ] > 0 &&
       player -> resources.current[ RESOURCE_COMBO_POINT ] < base_costs[ RESOURCE_COMBO_POINT ] )
    return false;

  if ( requires_stealth )
  {
    if ( ! p -> buffs.shadow_dance -> check() &&
         ! p -> buffs.stealth -> check() &&
         ! player -> buffs.shadowmeld -> check() &&
         ! p -> buffs.vanish -> check() &&
         ! p -> buffs.subterfuge -> check() )
    {
      return false;
    }
  }

  if ( requires_position != POSITION_NONE )
    if ( p -> position() != requires_position )
      return false;

  if ( requires_weapon != WEAPON_NONE )
    if ( ! weapon || weapon -> type != requires_weapon )
      return false;

  return true;
}

// Melee Attack =============================================================

struct melee_t : public rogue_attack_t
{
  int sync_weapons;
  bool first;

  melee_t( const char* name, rogue_t* p, int sw ) :
    rogue_attack_t( name, p ), sync_weapons( sw ), first( true )
  {
    auto_attack     = true;
    school          = SCHOOL_PHYSICAL;
    background      = true;
    repeating       = true;
    trigger_gcd     = timespan_t::zero();
    special         = false;
    may_glance      = true;

    if ( p -> dual_wield() && ! p -> spec.swashbuckler -> ok() )
      base_hit -= 0.19;

    p -> auto_attack = this;
  }

  void reset() override
  {
    rogue_attack_t::reset();

    first = true;
  }

  virtual timespan_t execute_time() const override
  {
    timespan_t t = rogue_attack_t::execute_time();
    if ( first )
    {
      return ( weapon -> slot == SLOT_OFF_HAND ) ? ( sync_weapons ? std::min( t / 2, timespan_t::zero() ) : t / 2 ) : timespan_t::zero();
    }
    return t;
  }

  virtual void execute() override
  {
    if ( first )
    {
      first = false;
    }
    rogue_attack_t::execute();
  }
};

// Auto Attack ==============================================================

struct auto_melee_attack_t : public action_t
{
  int sync_weapons;

  auto_melee_attack_t( rogue_t* p, const std::string& options_str ) :
    action_t( ACTION_OTHER, "auto_attack", p ),
    sync_weapons( 0 )
  {
    trigger_gcd = timespan_t::zero();

    add_option( opt_bool( "sync_weapons", sync_weapons ) );
    parse_options( options_str );

    if ( p -> main_hand_weapon.type == WEAPON_NONE )
    {
      background = true;
      return;
    }

    p -> melee_main_hand = debug_cast<melee_t*>( p -> find_action( "auto_attack_mh" ) );
    if ( ! p -> melee_main_hand )
      p -> melee_main_hand = new melee_t( "auto_attack_mh", p, sync_weapons );

    p -> main_hand_attack = p -> melee_main_hand;
    p -> main_hand_attack -> weapon = &( p -> main_hand_weapon );
    p -> main_hand_attack -> base_execute_time = p -> main_hand_weapon.swing_time;

    if ( p -> off_hand_weapon.type != WEAPON_NONE )
    {
      p -> melee_off_hand = debug_cast<melee_t*>( p -> find_action( "auto_attack_oh" ) );
      if ( ! p -> melee_off_hand )
        p -> melee_off_hand = new melee_t( "auto_attack_oh", p, sync_weapons );

      p -> off_hand_attack = p -> melee_off_hand;
      p -> off_hand_attack -> weapon = &( p -> off_hand_weapon );
      p -> off_hand_attack -> base_execute_time = p -> off_hand_weapon.swing_time;
      p -> off_hand_attack -> id = 1;
    }
  }

  virtual void execute() override
  {
    player -> main_hand_attack -> schedule_execute();

    if ( player -> off_hand_attack )
      player -> off_hand_attack -> schedule_execute();
  }

  virtual bool ready() override
  {
    if ( player -> is_moving() )
      return false;

    return ( player -> main_hand_attack -> execute_event == nullptr ); // not swinging
  }
};

// Adrenaline Rush ==========================================================

struct adrenaline_rush_t : public rogue_attack_t
{
  adrenaline_rush_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "adrenaline_rush", p, p -> find_specialization_spell( "Adrenaline Rush" ), options_str )
  {
    harmful = may_miss = may_crit = false;
  }

  void execute() override
  {
    rogue_attack_t::execute();

    p() -> buffs.adrenaline_rush -> trigger();
  }
};

// Ambush ===================================================================

struct ambush_t : public rogue_attack_t
{
  ambush_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "ambush", p, p -> find_specialization_spell( "Ambush" ), options_str )
  {
    requires_stealth  = true;

    // Tier 18 (WoD 6.2) Subtlety trinket effect
    if ( p -> from_the_shadows )
    {
      const spell_data_t* data = p -> find_spell( p -> from_the_shadows -> spell_id );
      base_multiplier *= 1.0 + ( data -> effectN( 1 ).average( p -> from_the_shadows -> item ) / 100.0 );
    }
  }

  double action_multiplier() const override
  {
    double m = rogue_attack_t::action_multiplier();

    if ( weapon -> type == WEAPON_DAGGER )
      m *= 1.40;

    return m;
  }

  virtual double cost() const override
  {
    double c = rogue_attack_t::cost();

    if ( p() -> buffs.shadow_dance -> check() )
      c += p() -> spec.shadow_dance -> effectN( 2 ).base_value();

    c -= 2 * p() -> buffs.t16_2pc_melee -> stack();

    if ( c < 0 )
      return 0;

    return c;
  }

  void execute() override
  {
    rogue_attack_t::execute();

    p() -> buffs.t16_2pc_melee -> expire();
    p() -> buffs.sleight_of_hand -> expire();
  }

  bool ready() override
  {
    bool rd;

    if ( p() -> buffs.sleight_of_hand -> check() )
    {
      // Sigh ....
      requires_stealth = false;
      rd = rogue_attack_t::ready();
      requires_stealth = true;
    }
    else
      rd = rogue_attack_t::ready();

    return rd;
  }
};

// Backstab =================================================================

struct backstab_t : public rogue_attack_t
{
  backstab_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "backstab", p, p -> find_specialization_spell( "Backstab" ), options_str )
  {
    requires_weapon   = WEAPON_DAGGER;
    requires_position = POSITION_BACK;
  }

  virtual double cost() const override
  {
    double c = rogue_attack_t::cost();
    c -= 2 * p() -> buffs.t16_2pc_melee -> stack();
    if ( c < 0 )
      c = 0;
    return c;
  }

  void execute() override
  {
    rogue_attack_t::execute();

    p() -> buffs.t16_2pc_melee -> expire();

    if ( result_is_hit( execute_state -> result ) && p() -> sets.has_set_bonus( SET_MELEE, T16, B4 ) )
      p() -> buffs.sleight_of_hand -> trigger();
  }

  double composite_da_multiplier( const action_state_t* state ) const override
  {
    double m = rogue_attack_t::composite_da_multiplier( state );

    m *= 1.0 + p() -> sets.set( SET_MELEE, T14, B2 ) -> effectN( 2 ).percent();

    return m;
  }
};

// Blade Flurry =============================================================

struct blade_flurry_t : public rogue_attack_t
{
  blade_flurry_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "blade_flurry", p, p -> find_specialization_spell( "Blade Flurry" ), options_str )
  {
    harmful = may_miss = may_crit = false;
    ignore_false_positive = true;
  }

  void execute() override
  {
    rogue_attack_t::execute();

    if ( ! p() -> buffs.blade_flurry -> check() )
      p() -> buffs.blade_flurry -> trigger();
    else
      p() -> buffs.blade_flurry -> expire();
  }
};

// Envenom ==================================================================

struct envenom_t : public rogue_attack_t
{
  envenom_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "envenom", p, p -> find_specialization_spell( "Envenom" ), options_str )
  {
    weapon = &( p -> main_hand_weapon );
    base_costs[ RESOURCE_COMBO_POINT ] = 1;
    attack_power_mod.direct = 0.417;
    dot_duration = timespan_t::zero();
    weapon_multiplier = weapon_power_mod = 0.0;
    base_multiplier *= 1.05; // Hard-coded tooltip.
    base_dd_min = base_dd_max = 0;
  }

  void consume_resource() override
  {
    melee_attack_t::consume_resource();

    if ( ! p() -> buffs.death_from_above -> check() )
      p() -> spend_combo_points( execute_state );

    if ( p() -> sets.has_set_bonus( ROGUE_ASSASSINATION, T17, B4 ) )
      p() -> trigger_combo_point_gain( execute_state, 1, p() -> gains.t17_4pc_assassination );

    if ( result_is_miss( execute_state -> result ) && resource_consumed > 0 )
      p() -> trigger_energy_refund( execute_state );
  }

  double action_multiplier() const override
  {
    double m = rogue_attack_t::action_multiplier();

    if ( p() -> buffs.death_from_above -> up() )
      m *= 1.0 + p() -> buffs.death_from_above -> data().effectN( 2 ).percent();

    return m;
  }

  double cost() const override
  {
    double c = rogue_attack_t::cost();

    if ( p() -> buffs.death_from_above -> check() )
      c *= 1.0 + p() -> buffs.death_from_above -> data().effectN( 1 ).percent();

    if ( c < 0 )
      c = 0;

    return c;
  }

  virtual void execute() override
  {
    rogue_attack_t::execute();

    timespan_t envenom_duration = p() -> buffs.envenom -> data().duration() * ( 1 + cast_state( execute_state ) -> cp );

    if ( p() -> sets.has_set_bonus( SET_MELEE, T15, B2 ) )
      envenom_duration += p() -> buffs.envenom -> data().duration();

    p() -> buffs.envenom -> trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, envenom_duration );

    if ( p() -> buffs.death_from_above -> check() )
    {
      timespan_t extend_increase = p() -> buffs.envenom -> remains() * p() -> buffs.death_from_above -> data().effectN( 4 ).percent();
      p() -> buffs.envenom -> extend_duration( player, extend_increase );
    }
  }

  virtual double action_da_multiplier() const override
  {
    double m = rogue_attack_t::action_da_multiplier();

    if ( p() -> mastery.potent_poisons -> ok() )
      m *= 1.0 + p() -> cache.mastery_value();

    return m;
  }

  virtual void impact( action_state_t* state ) override
  {
    rogue_attack_t::impact( state );

    if ( p() -> spec.cut_to_the_chase -> ok() &&
         p() -> buffs.slice_and_dice -> check() )
    {
      double snd = p() -> buffs.slice_and_dice -> data().effectN( 1 ).percent();
      if ( p() -> mastery.executioner -> ok() )
        snd *= 1.0 + p() -> cache.mastery_value();
      timespan_t snd_duration = 6 * p() -> buffs.slice_and_dice -> data().duration();

      p() -> buffs.slice_and_dice -> trigger( 1, snd, -1.0, snd_duration );
    }
  }
};

// Eviscerate ===============================================================

struct eviscerate_t : public rogue_attack_t
{
  eviscerate_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "eviscerate", p, p -> find_specialization_spell( "Eviscerate" ), options_str )
  {
    base_costs[ RESOURCE_COMBO_POINT ] = 1;
    weapon = &( player -> main_hand_weapon );
    weapon_multiplier = weapon_power_mod = 0;

    attack_power_mod.direct = 0.559;
    // Hard-coded tooltip.
    attack_power_mod.direct *= 0.88;

    // Tier 18 (WoD 6.2) Combat trinket effect
    // TODO: Eviscerate actually changes spells to 185187
    if ( p -> eviscerating_blade )
    {
      const spell_data_t* data = p -> find_spell( p -> eviscerating_blade -> spell_id );
      base_multiplier *= 1.0 + data -> effectN( 2 ).average( p -> eviscerating_blade -> item ) / 100.0;

      range += data -> effectN( 1 ).base_value();
    }
  }

  timespan_t gcd() const override
  {
    timespan_t t = rogue_attack_t::gcd();

    if ( t != timespan_t::zero() && p() -> buffs.adrenaline_rush -> check() )
      t += p() -> buffs.adrenaline_rush -> data().effectN( 3 ).time_value();

    return t;
  }

  double action_multiplier() const override
  {
    double m = rogue_attack_t::action_multiplier();

    if ( p() -> buffs.death_from_above -> up() )
      m *= 1.0 + p() -> buffs.death_from_above -> data().effectN( 2 ).percent();

    return m;
  }

  double cost() const override
  {
    double c = rogue_attack_t::cost();

    if ( p() -> buffs.death_from_above -> check() )
      c *= 1.0 + p() -> buffs.death_from_above -> data().effectN( 1 ).percent();

    if ( p() -> buffs.deceit -> check() )
      c *= 1.0 + p() -> buffs.deceit -> data().effectN( 1 ).percent();

    if ( c < 0 )
      c = 0;

    return c;
  }

  void consume_resource() override
  {
    melee_attack_t::consume_resource();

    if ( ! p() -> buffs.death_from_above -> check() )
    {
      p() -> spend_combo_points( execute_state );
      p() -> buffs.deceit -> expire();
    }

    if ( result_is_miss( execute_state -> result ) && resource_consumed > 0 )
      p() -> trigger_energy_refund( execute_state );
  }

  void execute() override
  {
    rogue_attack_t::execute();

    if ( p() -> sets.has_set_bonus( ROGUE_SUBTLETY, T18, B4 ) )
    {
      timespan_t v = timespan_t::from_seconds( -p() -> sets.set( ROGUE_SUBTLETY, T18, B4 ) -> effectN( 1 ).base_value() );
      v *= cast_state( execute_state ) -> cp;
      p() -> cooldowns.vanish -> adjust( v, false );
    }
  }

  virtual void impact( action_state_t* state ) override
  {
    rogue_attack_t::impact( state );

    if ( result_is_hit( state -> result ) &&
         p() -> spec.cut_to_the_chase -> ok() && p() -> buffs.slice_and_dice -> check() )
    {
      double snd = p() -> buffs.slice_and_dice -> data().effectN( 1 ).percent();
      if ( p() -> mastery.executioner -> ok() )
        snd *= 1.0 + p() -> cache.mastery_value();
      timespan_t snd_duration = 3 * 6 * p() -> buffs.slice_and_dice -> buff_period;

      p() -> buffs.slice_and_dice -> trigger( 1, snd, -1.0, snd_duration );
    }

  }
};

// Fan of Knives ============================================================

struct fan_of_knives_t: public rogue_attack_t
{
  fan_of_knives_t( rogue_t* p, const std::string& options_str ):
    rogue_attack_t( "fan_of_knives", p, p -> find_specialization_spell( "Fan of Knives" ), options_str )
  {
    weapon = &( player -> main_hand_weapon );
    weapon_multiplier = 0;
    aoe = -1;
    adds_combo_points = 1;
  }

  void impact( action_state_t* state ) override
  {
    rogue_attack_t::impact( state );
    // Don't generate a combo point on the first target hit, since that's
    // already covered by the action execution logic.
    if ( state -> chain_target > 0 &&
         result_is_hit( state -> result ) )
      p() -> trigger_combo_point_gain( state, 1, p() -> gains.empowered_fan_of_knives );
  }
};

// Feint ====================================================================

struct feint_t : public rogue_attack_t
{
  feint_t( rogue_t* p, const std::string& options_str ):
  rogue_attack_t( "feint", p, p -> find_class_spell( "Feint" ), options_str )
  {
  }

  void execute() override
  {
    rogue_attack_t::execute();
    p() -> buffs.feint -> trigger();
  }
};

// Garrote ==================================================================

struct garrote_t : public rogue_attack_t
{
  garrote_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "garrote", p, p -> find_specialization_spell( "Garrote" ), options_str )
  {
    may_crit          = false;
    requires_stealth  = true;

    // Tier 18 (WoD 6.2) Subtlety trinket effect
    if ( p -> from_the_shadows )
    {
      const spell_data_t* data = p -> find_spell( p -> from_the_shadows -> spell_id );
      base_multiplier *= 1.0 + ( data -> effectN( 1 ).average( p -> from_the_shadows -> item ) / 100.0 );
    }
  }
};

// Hemorrhage ===============================================================

struct hemorrhage_t : public rogue_attack_t
{
  hemorrhage_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "hemorrhage", p, p -> talent.hemorrhage, options_str )
  {
    weapon = &( p -> main_hand_weapon );
  }

  double action_da_multiplier() const override
  {
    double m = rogue_attack_t::action_da_multiplier();

    if ( weapon -> type == WEAPON_DAGGER )
      m *= 1.4;

    return m;
  }
};

// Kick =====================================================================

struct kick_t : public rogue_attack_t
{
  kick_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "kick", p, p -> find_class_spell( "Kick" ), options_str )
  {
    may_miss = may_glance = may_block = may_dodge = may_parry = may_crit = false;
    ignore_false_positive = true;
  }

  virtual bool ready() override
  {
    if ( ! target -> debuffs.casting -> check() )
      return false;

    return rogue_attack_t::ready();
  }
};

// Killing Spree ============================================================

struct killing_spree_tick_t : public rogue_attack_t
{
  killing_spree_tick_t( rogue_t* p, const char* name, const spell_data_t* s ) :
    rogue_attack_t( name, p, s )
  {
    school      = SCHOOL_PHYSICAL;
    background  = true;
    may_crit    = true;
    direct_tick = true;
  }

  bool procs_main_gauche() const override
  { return true; }
};

struct killing_spree_t : public rogue_attack_t
{
  melee_attack_t* attack_mh;
  melee_attack_t* attack_oh;

  killing_spree_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "killing_spree", p, p -> talent.killing_spree, options_str ),
    attack_mh( nullptr ), attack_oh( nullptr )
  {
    may_miss  = false;
    may_crit  = false;
    channeled = true;
    tick_zero = true;

    attack_mh = new killing_spree_tick_t( p, "killing_spree_mh", p -> find_spell( 57841 ) );
    attack_mh -> weapon = &( player -> main_hand_weapon );
    add_child( attack_mh );

    if ( player -> off_hand_weapon.type != WEAPON_NONE )
    {
      attack_oh = new killing_spree_tick_t( p, "killing_spree_oh", p -> find_spell( 57841 ) -> effectN( 2 ).trigger() );
      attack_oh -> weapon = &( player -> off_hand_weapon );
      add_child( attack_oh );
    }
  }

  double composite_target_da_multiplier( player_t* target ) const override
  {
    double m = rogue_attack_t::composite_target_da_multiplier( target );

    rogue_td_t* td = this -> td( target );
    if ( td -> dots.killing_spree -> current_tick >= 0 )
        m *= std::pow( 1.0 + p() -> sets.set( SET_MELEE, T16, B4 ) -> effectN( 1 ).percent(),
                       td -> dots.killing_spree -> current_tick + 1 );

    return m;
  }

  timespan_t tick_time( double ) const override
  { return base_tick_time; }

  virtual void execute() override
  {
    p() -> buffs.killing_spree -> trigger();

    rogue_attack_t::execute();
  }

  virtual void tick( dot_t* d ) override
  {
    rogue_attack_t::tick( d );

    attack_mh -> pre_execute_state = attack_mh -> get_state( d -> state );
    attack_mh -> execute();

    if ( attack_oh && result_is_hit( attack_mh -> execute_state -> result ) )
    {
      attack_oh -> pre_execute_state = attack_oh -> get_state( d -> state );
      attack_oh -> execute();
    }
  }
};

// Pistol Shot =========================================================

struct pistol_shot_t : public rogue_attack_t
{
    pistol_shot_t( rogue_t* p, const std::string& options_str ) :
        rogue_attack_t( "pistol_shot", p, p -> spec.saber_slash, options_str )
    { }

    double cost() const override
    {
      if ( p() -> buffs.free_pistol_shot -> check() )
      {
        return 0;
      }

      return rogue_attack_t::cost();
    }
};

// Marked for Death =========================================================

struct marked_for_death_t : public rogue_attack_t
{
  marked_for_death_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "marked_for_death", p, p -> find_talent_spell( "Marked for Death" ), options_str )
  {
    may_miss = may_crit = harmful = callbacks = false;
    adds_combo_points = data().effectN( 1 ).base_value();
  }

  // Defined after marked_for_death_debuff_t. Sigh.
  void impact( action_state_t* state ) override;
};



// Mutilate =================================================================

struct mutilate_strike_t : public rogue_attack_t
{
  mutilate_strike_t( rogue_t* p, const char* name, const spell_data_t* s ) :
    rogue_attack_t( name, p, s )
  {
    background  = true;
    may_miss = may_dodge = may_parry = false;
  }

  void impact( action_state_t* state ) override
  {
    rogue_attack_t::impact( state );

    p() -> trigger_seal_fate( state );

    if ( p() -> sets.has_set_bonus( ROGUE_ASSASSINATION, T17, B2 ) && state -> result == RESULT_CRIT )
      p() -> resource_gain( RESOURCE_ENERGY,
                            p() -> sets.set( ROGUE_ASSASSINATION, T17, B2 ) -> effectN( 1 ).base_value(),
                            p() -> gains.t17_2pc_assassination,
                            this );
  }
};

struct mutilate_t : public rogue_attack_t
{
  rogue_attack_t* mh_strike;
  rogue_attack_t* oh_strike;
  double toxic_mutilator_crit_chance;

  mutilate_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "mutilate", p, p -> find_specialization_spell( "Mutilate" ), options_str ),
    mh_strike( nullptr ), oh_strike( nullptr ), toxic_mutilator_crit_chance( 0 )
  {
    may_crit = false;
    snapshot_flags |= STATE_MUL_DA;

    // Tier 18 (WoD 6.2) trinket effect for Assassination
    if ( p -> toxic_mutilator )
    {
      const spell_data_t* data = p -> find_spell( p -> toxic_mutilator -> spell_id );
      toxic_mutilator_crit_chance = data -> effectN( 1 ).average( p -> toxic_mutilator -> item );
      toxic_mutilator_crit_chance /= 100.0;
    }

    if ( p -> main_hand_weapon.type != WEAPON_DAGGER ||
         p ->  off_hand_weapon.type != WEAPON_DAGGER )
    {
      sim -> errorf( "Player %s attempting to execute Mutilate without two daggers equipped.", p -> name() );
      background = true;
    }

    mh_strike = new mutilate_strike_t( p, "mutilate_mh", data().effectN( 2 ).trigger() );
    mh_strike -> weapon = &( p -> main_hand_weapon );
    add_child( mh_strike );

    oh_strike = new mutilate_strike_t( p, "mutilate_oh", data().effectN( 3 ).trigger() );
    oh_strike -> weapon = &( p -> off_hand_weapon );
    add_child( oh_strike );
  }

  double cost() const override
  {
    double c = rogue_attack_t::cost();
    if ( p() -> buffs.t16_2pc_melee -> up() )
      c -= 6 * p() -> buffs.t16_2pc_melee -> check();

    if ( c < 0 )
      c = 0;

    return c;
  }

  double composite_crit() const override
  {
    double c = rogue_attack_t::composite_crit();

    if ( p() -> buffs.envenom -> check() )
    {
      c += toxic_mutilator_crit_chance;
    }

    return c;
  }

  void execute() override
  {
    rogue_attack_t::execute();

    p() -> buffs.t16_2pc_melee -> expire();

    if ( result_is_hit( execute_state -> result ) )
    {
      action_state_t* s = mh_strike -> get_state( execute_state );
      mh_strike -> target = execute_state -> target;
      mh_strike -> schedule_execute( s );

      s = oh_strike -> get_state( execute_state );
      oh_strike -> target = execute_state -> target;
      oh_strike -> schedule_execute( s );
    }
  }
};

// Premeditation ============================================================

struct premeditation_t : public rogue_attack_t
{
  struct premeditation_event_t : public player_event_t
  {
    int combo_points;
    player_t* target;

    premeditation_event_t( rogue_t& p, player_t* t, timespan_t duration, int cp ) :
      player_event_t( p ),
      combo_points( cp ), target( t )
    {
      add_event( duration );
    }
    virtual const char* name() const override
    { return "premeditation"; }
    void execute() override
    {
      rogue_t* p = static_cast< rogue_t* >( player() );

      p -> resources.current[ RESOURCE_COMBO_POINT ] -= combo_points;
      if ( sim().log )
      {
        sim().out_log.printf( "%s loses %d temporary combo_points from premeditation (%d)",
                    player() -> name(), combo_points, p -> resources.current[ RESOURCE_COMBO_POINT ] );
      }

      assert( p -> resources.current[ RESOURCE_COMBO_POINT ] >= 0 );
    }
  };

  premeditation_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "premeditation", p, p -> find_specialization_spell( "Premeditation" ), options_str )
  {
    harmful = may_crit = may_miss = false;
    requires_stealth = true;
    // We need special combo points handling here
    adds_combo_points = 0;
  }

  void impact( action_state_t* state ) override
  {
    rogue_attack_t::impact( state );

    double add_points = data().effectN( 1 ).base_value();

    add_points = std::min( add_points, player -> resources.max[ RESOURCE_COMBO_POINT ] - player -> resources.current[ RESOURCE_COMBO_POINT ] );

    if ( add_points > 0 )
      p() -> trigger_combo_point_gain( nullptr, static_cast<int>( add_points ), p() -> gains.premeditation );

    p() -> event_premeditation = new ( *sim ) premeditation_event_t( *p(), state -> target, data().duration(), static_cast<int>( add_points ) );
  }
};

// Rupture ==================================================================

struct rupture_t : public rogue_attack_t
{
  rupture_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "rupture", p, p -> find_specialization_spell( "Rupture" ), options_str )
  {
    may_crit              = false;
    base_costs[ RESOURCE_COMBO_POINT ] = 1;
  }

  timespan_t gcd() const override
  {
    timespan_t t = rogue_attack_t::gcd();

    if ( t != timespan_t::zero() && p() -> buffs.adrenaline_rush -> check() )
      t += p() -> buffs.adrenaline_rush -> data().effectN( 3 ).time_value();

    return t;
  }

  void execute() override
  {
    rogue_attack_t::execute();

    if ( p() -> sets.has_set_bonus( ROGUE_SUBTLETY, T18, B4 ) )
    {
      timespan_t v = timespan_t::from_seconds( -p() -> sets.set( ROGUE_SUBTLETY, T18, B4 ) -> effectN( 1 ).base_value() );
      v *= cast_state( execute_state ) -> cp;
      p() -> cooldowns.vanish -> adjust( v, false );
    }
  }

  virtual timespan_t composite_dot_duration( const action_state_t* s ) const override
  {
    timespan_t duration = data().duration();

    duration += duration * cast_state( s ) -> cp;
    if ( p() -> sets.has_set_bonus( SET_MELEE, T15, B2 ) )
      duration += data().duration();

    return duration;
  }

  virtual void tick( dot_t* d ) override
  {
    rogue_attack_t::tick( d );

    p() -> trigger_venomous_wounds( d -> state );
  }
};

// Saber Slash ==========================================================

struct saber_slash_t : public rogue_attack_t
{
  struct saberslash_proc_event_t : public player_event_t
  {
    saber_slash_t* spell;
    player_t* target;

    saberslash_proc_event_t( rogue_t* p, saber_slash_t* s, player_t* t ) :
      player_event_t( *p ), spell( s ), target( t )
    {
      add_event( spell -> delay );
    }

    const char* name() const override
    { return "saberslash_proc_execute"; }

    void execute() override
    {
      spell -> target = target;
      spell -> execute();
      spell -> saberslash_proc_event = nullptr;
    }
  };

  saberslash_proc_event_t* saberslash_proc_event;
  timespan_t delay;

  saber_slash_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "saber_slash", p, p -> find_specialization_spell( "Saber Slash" ), options_str ),
    saberslash_proc_event( nullptr ), delay( data().duration() )
  { }

  void reset() override
  {
    rogue_attack_t::reset();
    saberslash_proc_event = nullptr;
  }

  void impact( action_state_t* state ) override
  {
    rogue_attack_t::impact( state );

    if ( result_is_hit( state -> result ) && ! saberslash_proc_event )
    {
      if ( p() -> buffs.free_pistol_shot -> trigger() )
      {
        saberslash_proc_event = new ( *sim ) saberslash_proc_event_t( p(), this, state -> target );
      }
    }
  }
};

// Shadowstep ===============================================================

struct shadowstep_t : public rogue_attack_t
{
  shadowstep_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "shadowstep", p, p -> spec.shadowstep, options_str )
  {
    harmful = false;
    dot_duration = timespan_t::zero();
    base_teleport_distance = data().max_range();
    movement_directionality = MOVEMENT_OMNI;
  }

  void execute() override
  {
    rogue_attack_t::execute();
    p() -> buffs.shadowstep -> trigger();
  }
};

// Shuriken Toss ============================================================

struct shuriken_toss_t : public rogue_attack_t
{
  shuriken_toss_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "shuriken_toss", p, p -> find_talent_spell( "Shuriken Toss" ), options_str )
  {
    adds_combo_points = 1; // it has an effect but with no base value :rollseyes:
  }

  bool procs_poison() const override
  { return true; }
};

// Slice and Dice ===========================================================

struct slice_and_dice_t : public rogue_attack_t
{
  slice_and_dice_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "slice_and_dice", p, p -> find_specialization_spell( "Slice and Dice" ), options_str )
  {
    base_costs[ RESOURCE_COMBO_POINT ] = 1;
    harmful = false;
    dot_duration = timespan_t::zero();
  }

  timespan_t gcd() const override
  {
    timespan_t t = rogue_attack_t::gcd();

    if ( t != timespan_t::zero() && p() -> buffs.adrenaline_rush -> check() )
      t += p() -> buffs.adrenaline_rush -> data().effectN( 3 ).time_value();

    return t;
  }

  virtual void execute() override
  {
    rogue_attack_t::execute();

    double snd = p() -> buffs.slice_and_dice -> data().effectN( 1 ).percent();
    timespan_t snd_duration = ( cast_state( execute_state ) -> cp + 1 ) * p() -> buffs.slice_and_dice -> data().duration();

    if ( p() -> sets.has_set_bonus( SET_MELEE, T15, B2 ) )
      snd_duration += p() -> buffs.slice_and_dice -> data().duration();

    p() -> buffs.slice_and_dice -> trigger( 1, snd, -1.0, snd_duration );
  }
};

// Sprint ===================================================================

struct sprint_t: public rogue_attack_t
{
  sprint_t( rogue_t* p, const std::string& options_str ):
    rogue_attack_t( "sprint", p, p -> spell.sprint, options_str )
  {
    harmful = callbacks = false;
    cooldown = p -> cooldowns.sprint;
    ignore_false_positive = true;
  }

  void execute() override
  {
    rogue_attack_t::execute();

    p() -> buffs.sprint -> trigger();
  }
};

// Vanish ===================================================================

struct vanish_t : public rogue_attack_t
{
  vanish_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "vanish", p, p -> find_class_spell( "Vanish" ), options_str )
  {
    may_miss = may_crit = harmful = false;
    ignore_false_positive = true;
  }

  void init() override
  {
    rogue_attack_t::init();

    if ( p() -> sets.has_set_bonus( ROGUE_SUBTLETY, T18, B2 ) )
    {
      cp_gain = player -> get_gain( name_str );
    }
  }

  void execute() override
  {
    rogue_attack_t::execute();

    p() -> buffs.vanish -> trigger();

    // Vanish stops autoattacks
    if ( p() -> main_hand_attack && p() -> main_hand_attack -> execute_event )
      event_t::cancel( p() -> main_hand_attack -> execute_event );

    if ( p() -> off_hand_attack && p() -> off_hand_attack -> execute_event )
      event_t::cancel( p() -> off_hand_attack -> execute_event );

    p() -> buffs.deathly_shadows -> trigger();

    if ( p() -> sets.has_set_bonus( ROGUE_SUBTLETY, T18, B2 ) )
    {
      p() -> trigger_combo_point_gain( execute_state,
                                       p() -> sets.set( ROGUE_SUBTLETY, T18, B2 ) -> effectN( 1 ).base_value(),
                                       cp_gain );
    }
  }
};

// Vendetta =================================================================

struct vendetta_t : public rogue_attack_t
{
  vendetta_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "vendetta", p, p -> find_specialization_spell( "Vendetta" ), options_str )
  {
    harmful = may_miss = may_crit = false;
  }

  void execute() override
  {
    rogue_attack_t::execute();

    rogue_td_t* td = this -> td( execute_state -> target );

    td -> debuffs.vendetta -> trigger();
  }
};

// Death From Above

struct death_from_above_driver_t : public rogue_attack_t
{
  envenom_t* envenom;
  eviscerate_t* eviscerate;

  death_from_above_driver_t( rogue_t* p ) :
    rogue_attack_t( "death_from_above_driver", p, p -> talent.death_from_above ),
    envenom( p -> specialization() == ROGUE_ASSASSINATION ? new envenom_t( p, "" ) : nullptr ),
    eviscerate( p -> specialization() != ROGUE_ASSASSINATION ? new eviscerate_t( p, "" ) : nullptr )
  {
    callbacks = tick_may_crit = false;
    quiet = dual = background = harmful = true;
    attack_power_mod.direct = 0;
    base_dd_min = base_dd_max = 0;
    base_costs[ RESOURCE_ENERGY ] = 0;
  }

  void tick( dot_t* d ) override
  {
    rogue_attack_t::tick( d );

    if ( envenom )
    {
      // DFA is a finisher, so copy CP state (number of CPs used on DFA) from
      // the DFA dot
      action_state_t* env_state = envenom -> get_state();
      envenom -> target = d -> target;
      envenom -> snapshot_state( env_state, DMG_DIRECT );
      cast_state( env_state ) -> cp = cast_state( d -> state ) -> cp;

      envenom -> pre_execute_state = env_state;
      envenom -> execute();
    }
    else if ( eviscerate )
    {
      // DFA is a finisher, so copy CP state (number of CPs used on DFA) from
      // the DFA dot
      action_state_t* evis_state = eviscerate -> get_state();
      eviscerate -> target = d -> target;
      eviscerate -> snapshot_state( evis_state, DMG_DIRECT );
      cast_state( evis_state ) -> cp = cast_state( d -> state ) -> cp;

      eviscerate -> pre_execute_state = evis_state;
      eviscerate -> execute();
    }
    else
    {
      assert( 0 );
    }

    p() -> buffs.death_from_above -> expire();
  }
};

struct death_from_above_t : public rogue_attack_t
{
  death_from_above_driver_t* driver;

  death_from_above_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "death_from_above", p, p -> talent.death_from_above, options_str ),
    driver( new death_from_above_driver_t( p ) )
  {
    weapon = &( p -> main_hand_weapon );
    weapon_multiplier = 0;
    attack_power_mod.direct /= 5;
    base_costs[ RESOURCE_COMBO_POINT ] = 1;

    base_tick_time = timespan_t::zero();
    dot_duration = timespan_t::zero();

    aoe = -1;
  }

  void adjust_attack( attack_t* attack, const timespan_t& oor_delay )
  {
    if ( ! attack || ! attack -> execute_event )
    {
      return;
    }

    if ( attack -> execute_event -> remains() >= oor_delay )
    {
      return;
    }

    timespan_t next_swing = attack -> execute_event -> remains();
    timespan_t initial_next_swing = next_swing;
    // Fit the next autoattack swing into a set of increasing 500ms values,
    // which seems to be what is occurring with OOR+autoattacks in game.
    while ( next_swing <= oor_delay )
    {
      next_swing += timespan_t::from_millis( 500 );
    }

    if ( attack == player -> main_hand_attack )
    {
      p() -> dfa_mh -> add( ( next_swing - oor_delay ).total_seconds() );
    }
    else if ( attack == player -> off_hand_attack )
    {
      p() -> dfa_oh -> add( ( next_swing - oor_delay ).total_seconds() );
    }

    attack -> execute_event -> reschedule( next_swing );
    if ( sim -> debug )
    {
      sim -> out_debug.printf( "%s %s swing pushback: oor_time=%.3f orig_next=%.3f next=%.3f lands=%.3f",
          player -> name(), name(), oor_delay.total_seconds(), initial_next_swing.total_seconds(),
          next_swing.total_seconds(),
          attack -> execute_event -> occurs().total_seconds() );
    }
  }

  void execute() override
  {
    rogue_attack_t::execute();

    p() -> buffs.death_from_above -> trigger();

    timespan_t oor_delay = timespan_t::from_seconds( rng().gauss( 1.3, 0.025 ) );

    adjust_attack( player -> main_hand_attack, oor_delay );
    adjust_attack( player -> off_hand_attack, oor_delay );
/*
    // Apparently DfA is out of range for ~0.8 seconds during the "attack", so
    // ensure that we have a swing timer of at least 800ms on both hands. Note
    // that this can sync autoattacks which also happens in game.
    if ( player -> main_hand_attack && player -> main_hand_attack -> execute_event )
    {
      if ( player -> main_hand_attack -> execute_event -> remains() < timespan_t::from_seconds( 0.8 ) )
        player -> main_hand_attack -> execute_event -> reschedule( timespan_t::from_seconds( 0.8 ) );
    }

    if ( player -> off_hand_attack && player -> off_hand_attack -> execute_event )
    {
      if ( player -> off_hand_attack -> execute_event -> remains() < timespan_t::from_seconds( 0.8 ) )
        player -> off_hand_attack -> execute_event -> reschedule( timespan_t::from_seconds( 0.8 ) );
    }
*/
    action_state_t* driver_state = driver -> get_state( execute_state );
    driver_state -> target = target;
    driver -> schedule_execute( driver_state );
  }
};

// ==========================================================================
// Stealth
// ==========================================================================

struct stealth_t : public spell_t
{
  bool used;

  stealth_t( rogue_t* p, const std::string& options_str ) :
    spell_t( "stealth", p, p -> find_class_spell( "Stealth" ) ), used( false )
  {
    harmful = false;
    ignore_false_positive = true;

    parse_options( options_str );
  }

  virtual void execute() override
  {
    rogue_t* p = debug_cast< rogue_t* >( player );

    if ( sim -> log )
      sim -> out_log.printf( "%s performs %s", p -> name(), name() );

    p -> buffs.stealth -> trigger();
    used = true;
  }

  virtual bool ready() override
  {
    return ! used;
  }

  virtual void reset() override
  {
    spell_t::reset();
    used = false;
  }
};

// ==========================================================================
// Experimental weapon swapping
// ==========================================================================

struct weapon_swap_t : public action_t
{
  enum swap_slot_e
  {
    SWAP_MAIN_HAND,
    SWAP_OFF_HAND,
    SWAP_BOTH
  };

  std::string slot_str, swap_to_str;

  swap_slot_e swap_type;
  current_weapon_e swap_to_type;
  rogue_t* rogue;

  weapon_swap_t( rogue_t* rogue_, const std::string& options_str ) :
    action_t( ACTION_OTHER, "weapon_swap", rogue_ ),
    swap_type( SWAP_MAIN_HAND ), swap_to_type( WEAPON_SECONDARY ),
    rogue( rogue_ )
  {
    may_miss = may_crit = may_dodge = may_parry = may_glance = callbacks = harmful = false;

    add_option( opt_string( "slot", slot_str ) );
    add_option( opt_string( "swap_to", swap_to_str ) );

    parse_options( options_str );

    if ( slot_str.empty() )
    {
      background = true;
    }
    else if ( util::str_compare_ci( slot_str, "main" ) ||
              util::str_compare_ci( slot_str, "main_hand" ) )
    {
      swap_type = SWAP_MAIN_HAND;
    }
    else if ( util::str_compare_ci( slot_str, "off" ) ||
              util::str_compare_ci( slot_str, "off_hand" ) )
    {
      swap_type = SWAP_OFF_HAND;
    }
    else if ( util::str_compare_ci( slot_str, "both" ) )
    {
      swap_type = SWAP_BOTH;
    }

    if ( util::str_compare_ci( swap_to_str, "primary" ) )
    {
      swap_to_type = WEAPON_PRIMARY;
    }
    else if ( util::str_compare_ci( swap_to_str, "secondary" ) )
    {
      swap_to_type = WEAPON_SECONDARY;
    }

    if ( swap_type != SWAP_BOTH )
    {
      if ( ! rogue -> weapon_data[ swap_type ].item_data[ swap_to_type ] )
      {
        background = true;
        sim -> errorf( "Player %s weapon_swap: No weapon info for %s/%s",
            player -> name(), slot_str.c_str(), swap_to_str.c_str() );
      }
    }
    else
    {
      if ( ! rogue -> weapon_data[ WEAPON_MAIN_HAND ].item_data[ swap_to_type ] ||
           ! rogue -> weapon_data[ WEAPON_OFF_HAND ].item_data[ swap_to_type ] )
      {
        background = true;
        sim -> errorf( "Player %s weapon_swap: No weapon info for %s/%s",
            player -> name(), slot_str.c_str(), swap_to_str.c_str() );
      }
    }
  }

  result_e calculate_result( action_state_t* ) const override
  { return RESULT_HIT; }

  block_result_e calculate_block_result( action_state_t* ) const override
  { return BLOCK_RESULT_UNBLOCKED; }

  void execute() override
  {
    action_t::execute();

    if ( swap_type == SWAP_MAIN_HAND )
    {
      rogue -> swap_weapon( WEAPON_MAIN_HAND, swap_to_type );
    }
    else if ( swap_type == SWAP_OFF_HAND )
    {
      rogue -> swap_weapon( WEAPON_OFF_HAND, swap_to_type );
    }
    else if ( swap_type == SWAP_BOTH )
    {
      rogue -> swap_weapon( WEAPON_MAIN_HAND, swap_to_type );
      rogue -> swap_weapon( WEAPON_OFF_HAND, swap_to_type );
    }
  }

  bool ready() override
  {
    if ( swap_type == SWAP_MAIN_HAND &&
         rogue -> weapon_data[ WEAPON_MAIN_HAND ].current_weapon == swap_to_type )
    {
      return false;
    }
    else if ( swap_type == SWAP_OFF_HAND &&
              rogue -> weapon_data[ WEAPON_OFF_HAND ].current_weapon == swap_to_type )
    {
      return false;
    }
    else if ( swap_type == SWAP_BOTH && 
              rogue -> weapon_data[ WEAPON_MAIN_HAND ].current_weapon == swap_to_type &&
              rogue -> weapon_data[ WEAPON_OFF_HAND ].current_weapon == swap_to_type )
    {
      return false;
    }

    return action_t::ready();
  }

};

// ==========================================================================
// Rogue Secondary Abilities
// ==========================================================================

struct main_gauche_t : public rogue_attack_t
{
  main_gauche_t( rogue_t* p ) :
    rogue_attack_t( "main_gauche", p, p -> find_spell( 86392 ) )
  {
    weapon          = &( p -> off_hand_weapon );
    special         = true;
    background      = true;
    may_crit        = true;
    proc = true; // it's proc; therefore it cannot trigger main_gauche for chain-procs
  }

  bool procs_poison() const override
  { return false; }
};

struct blade_flurry_attack_t : public rogue_attack_t
{
  blade_flurry_attack_t( rogue_t* p ) :
    rogue_attack_t( "blade_flurry_attack", p, p -> find_spell( 22482 ) )
  {
    may_miss = may_crit = proc = callbacks = may_dodge = may_parry = may_block = false;
    background = true;
    aoe = -1;
    weapon = &p -> main_hand_weapon;
    weapon_multiplier = 0;
    radius = 5;
    range = -1.0;

    snapshot_flags |= STATE_MUL_DA;
  }

  bool procs_main_gauche() const override
  { return false; }

  double composite_da_multiplier( const action_state_t* ) const override
  {
    return p() -> spec.blade_flurry -> effectN( 3 ).percent();
  }

  size_t available_targets( std::vector< player_t* >& tl ) const override
  {
    rogue_attack_t::available_targets( tl );

    for ( size_t i = 0; i < tl.size(); i++ )
    {
      if ( tl[i] == target ) // Cannot hit the original target.
      {
        tl.erase( tl.begin() + i );
        break;
      }
    }
    return tl.size();
  }
};

// Sinister Calling proc for subtlety

struct sinister_calling_proc_t : public rogue_attack_t
{
  sinister_calling_proc_t( rogue_t* r, const std::string& name, unsigned spell_id ) :
    rogue_attack_t( name + "_sc", r, r -> find_spell( spell_id ) )
  {
    background = proc = true;
    callbacks = false;
    weapon_multiplier = 0;
    weapon_power_mod = 0;
  }

  void init() override
  {
    rogue_attack_t::init();

    // Raw damage value comes from the source action, nothing else is applied here. Crit is
    // snapshotted to figure out a result.
    snapshot_flags = STATE_CRIT | STATE_TGT_CRIT;
    update_flags = 0;
  }

  double target_armor( player_t* ) const override
  { return 0; }
};

} // end namespace actions

weapon_slot_e weapon_info_t::slot() const
{
  if ( item_data[ WEAPON_PRIMARY ] -> slot == SLOT_MAIN_HAND )
  {
    return WEAPON_MAIN_HAND;
  }
  else
  {
    return WEAPON_OFF_HAND;
  }
}

void weapon_info_t::callback_state( current_weapon_e weapon, bool state )
{
  sim_t* sim = item_data[ WEAPON_PRIMARY ] -> sim;

  for ( size_t i = 0, end = cb_data[ weapon ].size(); i < end; ++i )
  {
    if ( state )
    {
      cb_data[ weapon ][ i ] -> activate();
      if ( cb_data[ weapon ][ i ] -> effect.rppm() > 0 )
      {
        cb_data[ weapon ][ i ] -> rppm.set_last_trigger_success( sim -> current_time() );
        cb_data[ weapon ][ i ] -> rppm.set_last_trigger_attempt( sim -> current_time() );
      }

      if ( sim -> debug )
      {
        sim -> out_debug.printf( "%s enabling callback %s on item %s",
            item_data[ WEAPON_PRIMARY ] -> player -> name(),
            cb_data[ weapon ][ i ] -> effect.name().c_str(),
            item_data[ weapon ] -> name() );
      }
    }
    else
    {
      cb_data[ weapon ][ i ] -> deactivate();
      if ( sim -> debug )
      {
        sim -> out_debug.printf( "%s disabling callback %s on item %s",
            item_data[ WEAPON_PRIMARY ] -> player -> name(),
            cb_data[ weapon ][ i ] -> effect.name().c_str(),
            item_data[ weapon ] -> name() );
      }
    }
  }
}

void weapon_info_t::initialize()
{
  if ( initialized )
  {
    return;
  }

  rogue_t* rogue = debug_cast<rogue_t*>( item_data[ WEAPON_PRIMARY ] -> player );

  // Compute stats and initialize the callback data for the weapon. This needs to be done
  // reasonably late (currently in weapon_swap_t action init) to ensure that everything has been
  // initialized.
  if ( item_data[ WEAPON_PRIMARY ] )
  {
    // Find primary weapon callbacks from the actor list of all callbacks
    for ( size_t i = 0; i < item_data[ WEAPON_PRIMARY ] -> parsed.special_effects.size(); ++i )
    {
      special_effect_t* effect = item_data[ WEAPON_PRIMARY ] -> parsed.special_effects[ i ];

      for ( size_t j = 0; j < rogue -> callbacks.all_callbacks.size(); ++j )
      {
        dbc_proc_callback_t* cb = debug_cast<dbc_proc_callback_t*>( rogue -> callbacks.all_callbacks[ j ] );

        if ( &( cb -> effect ) == effect )
        {
          cb_data[ WEAPON_PRIMARY ].push_back( cb );
        }
      }
    }

    // Pre-compute primary weapon stats
    for ( stat_e i = STAT_NONE; i < STAT_MAX; i++ )
    {
      stats_data[ WEAPON_PRIMARY ].add_stat( rogue -> convert_hybrid_stat( i ),
                                             item_data[ WEAPON_PRIMARY ] -> stats.get_stat( i ) );
    }
  }

  if ( item_data[ WEAPON_SECONDARY ] )
  {
    // Find secondary weapon callbacks from the actor list of all callbacks
    for ( size_t i = 0; i < item_data[ WEAPON_SECONDARY ] -> parsed.special_effects.size(); ++i )
    {
      special_effect_t* effect = item_data[ WEAPON_SECONDARY ] -> parsed.special_effects[ i ];

      for ( size_t j = 0; j < rogue -> callbacks.all_callbacks.size(); ++j )
      {
        dbc_proc_callback_t* cb = debug_cast<dbc_proc_callback_t*>( rogue -> callbacks.all_callbacks[ j ] );

        if ( &( cb -> effect ) == effect )
        {
          cb_data[ WEAPON_SECONDARY ].push_back( cb );
        }
      }
    }

    // Pre-compute secondary weapon stats
    for ( stat_e i = STAT_NONE; i < STAT_MAX; i++ )
    {
      stats_data[ WEAPON_SECONDARY ].add_stat( rogue -> convert_hybrid_stat( i ),
                                               item_data[ WEAPON_SECONDARY ] -> stats.get_stat( i ) );
    }

    if ( item_data[ WEAPON_SECONDARY ] )
    {
      std::string prefix = slot() == WEAPON_MAIN_HAND ? "_mh" : "_oh";

      secondary_weapon_uptime = buff_creator_t( rogue, "secondary_weapon" + prefix );
    }
  }

  initialized = true;
}

void weapon_info_t::reset()
{
  rogue_t* rogue = debug_cast<rogue_t*>( item_data[ WEAPON_PRIMARY ] -> player );

  // Reset swaps back to primary weapon for the slot
  rogue -> swap_weapon( slot(), WEAPON_PRIMARY, false );

  // .. and always deactivates secondary weapon callback(s).
  callback_state( WEAPON_SECONDARY, false );
}

// Due to how our DOT system functions, at the time when last_tick() is called
// for Deadly Poison, is_ticking() for the dot object will still return true.
// This breaks the is_ticking() check below, creating an inconsistent state in
// the sim, if Deadly Poison was the only poison up on the target. As a
// workaround, deliver the "Deadly Poison fade event" as an extra parameter.
inline bool rogue_t::poisoned_enemy( player_t* target, bool deadly_fade ) const
{
  const rogue_td_t* td = get_target_data( target );

  if ( ! deadly_fade && td -> dots.deadly_poison -> is_ticking() )
    return true;

  if ( td -> debuffs.wound_poison -> check() )
    return true;

  if ( td -> debuffs.crippling_poison -> check() )
    return true;

  if ( td -> debuffs.leeching_poison -> check() )
    return true;

  return false;
}

// ==========================================================================
// Rogue Triggers
// ==========================================================================

void rogue_t::trigger_auto_attack( const action_state_t* state )
{
  if ( main_hand_attack -> execute_event || ! off_hand_attack || off_hand_attack -> execute_event )
    return;

  if ( ! state -> action -> harmful )
    return;

  melee_main_hand -> first = true;
  if ( melee_off_hand )
    melee_off_hand -> first = true;

  auto_attack -> execute();
}

void rogue_t::trigger_seal_fate( const action_state_t* state )
{
  if ( ! spec.seal_fate -> ok() )
    return;

  if ( state -> result != RESULT_CRIT )
    return;

  trigger_combo_point_gain( state, 1, gains.seal_fate );

  procs.seal_fate -> occur();

  if ( buffs.t16_2pc_melee -> trigger() )
    procs.t16_2pc_melee -> occur();
}

void rogue_t::trigger_main_gauche( const action_state_t* state )
{
  if ( ! mastery.main_gauche -> ok() )
    return;

  if ( state -> result_total <= 0 )
    return;

  if ( ! state -> action -> result_is_hit( state -> result ) )
    return;

  actions::rogue_attack_t* attack = debug_cast<actions::rogue_attack_t*>( state -> action );
  if ( ! attack -> procs_main_gauche() )
    return;

  if ( ! rng().roll( cache.mastery_value() ) )
    return;

  active_main_gauche -> target = state -> target;
  active_main_gauche -> schedule_execute();
}

void rogue_t::trigger_combat_potency( const action_state_t* state )
{
  if ( ! spec.combat_potency -> ok() )
    return;

  if ( ! state -> action -> result_is_hit( state -> result ) )
    return;

  if ( ! state -> action -> weapon )
    return;

  if ( state -> action -> weapon -> slot != SLOT_OFF_HAND )
    return;

  double chance = 0.2;
  if ( state -> action != active_main_gauche )
    chance *= state -> action -> weapon -> swing_time.total_seconds() / 1.4;

  if ( ! rng().roll( chance ) )
    return;

  // energy gain value is in the proc trigger spell
  resource_gain( RESOURCE_ENERGY,
                 spec.combat_potency -> effectN( 1 ).trigger() -> effectN( 1 ).resource( RESOURCE_ENERGY ),
                 gains.combat_potency );
}

void rogue_t::trigger_energy_refund( const action_state_t* state )
{
  double energy_restored = state -> action -> resource_consumed * 0.80;

  resource_gain( RESOURCE_ENERGY, energy_restored, gains.energy_refund );
}

void rogue_t::trigger_venomous_wounds( const action_state_t* state )
{
  if ( ! spec.venomous_wounds -> ok() )
    return;

  if ( ! get_target_data( state -> target ) -> poisoned() )
    return;

  if ( ! state -> action -> result_is_hit( state -> result ) )
    return;

  double chance = spec.venomous_wounds -> proc_chance();

  if ( ! rng().roll( chance ) )
    return;

  resource_gain( RESOURCE_ENERGY,
                 spec.venomous_wounds -> effectN( 2 ).base_value(),
                 gains.venomous_wounds );
}

void rogue_t::trigger_blade_flurry( const action_state_t* state )
{
  if ( state -> result_total <= 0 )
    return;

  if ( !buffs.blade_flurry -> check() )
    return;

  if ( !state -> action -> weapon )
    return;

  if ( !state -> action -> result_is_hit( state -> result ) )
    return;

  if ( sim -> active_enemies == 1 )
    return;

  if ( state -> action -> n_targets() != 0 )
    return;

  // Invalidate target cache if target changes
  if ( active_blade_flurry -> target != state -> target )
    active_blade_flurry -> target_cache.is_valid = false;
  active_blade_flurry -> target = state -> target;

  // Note, unmitigated damage
  active_blade_flurry -> base_dd_min = state -> result_total;
  active_blade_flurry -> base_dd_max = state -> result_total;
  active_blade_flurry -> schedule_execute();
}

void rogue_t::trigger_combo_point_gain( const action_state_t* state,
                                        int                   cp_override,
                                        gain_t*               gain,
                                        bool                  allow_anticipation )
{
  using namespace actions;

  assert( state || cp_override > 0 );

  rogue_attack_t* attack = state ? debug_cast<rogue_attack_t*>( state -> action ) : nullptr;
  int n_cp = 0;
  if ( cp_override == -1 )
  {
    if ( ! attack -> adds_combo_points )
      return;

    n_cp = attack -> adds_combo_points;
  }
  else
    n_cp = cp_override;

  int fill = static_cast<int>( resources.max[RESOURCE_COMBO_POINT] - resources.current[RESOURCE_COMBO_POINT] );
  int added = std::min( fill, n_cp );
  int overflow = n_cp - added;
  int anticipation_added = 0;
  int anticipation_overflow = 0;
  if ( overflow > 0 && talent.anticipation -> ok() && allow_anticipation )
  {
    int anticipation_fill =  buffs.anticipation -> max_stack() - buffs.anticipation -> check();
    anticipation_added = std::min( anticipation_fill, overflow );
    anticipation_overflow = overflow - anticipation_added;
    if ( anticipation_added > 0 || anticipation_overflow > 0 )
      overflow = 0; // this is never used further???
  }

  gain_t* gain_obj = gain;
  if ( gain_obj == nullptr && attack && attack -> cp_gain )
    gain_obj = attack -> cp_gain;

  if ( ! talent.anticipation -> ok() || ! allow_anticipation )
  {
    resource_gain( RESOURCE_COMBO_POINT, n_cp, gain_obj, state ? state -> action : nullptr );
  }
  else
  {
    if ( added > 0 )
    {
      resource_gain( RESOURCE_COMBO_POINT, added, gain_obj, state ? state -> action : nullptr );
    }

    if ( anticipation_added + anticipation_overflow > 0 )
    {
      buffs.anticipation -> trigger( anticipation_added + anticipation_overflow );
      if ( gain_obj )
        gain_obj -> add( RESOURCE_COMBO_POINT, anticipation_added, anticipation_overflow );
    }
  }

  if ( sim -> log )
  {
    std::string cp_name = "unknown";
    if ( gain )
      cp_name = gain -> name_str;
    else if ( state && state -> action )
    {
      cp_name = state -> action -> name();
    }

    if ( anticipation_added > 0 || anticipation_overflow > 0 )
      sim -> out_log.printf( "%s gains %d (%d) anticipation charges from %s (%d)",
          name(), anticipation_added, anticipation_overflow, cp_name.c_str(), buffs.anticipation -> check() );
  }

  if ( event_premeditation )
    event_t::cancel( event_premeditation );

  assert( resources.current[ RESOURCE_COMBO_POINT ] <= 5 );
}

void rogue_t::trigger_anticipation_replenish( const action_state_t* state )
{
  if ( ! buffs.anticipation -> check() )
    return;

  if ( state -> action -> base_costs[ RESOURCE_COMBO_POINT ] == 0 )
    return;

  if ( ! state -> action -> result_is_hit( state -> result ) )
    return;

  if ( ! state -> action -> harmful )
    return;

  if ( sim -> log )
    sim -> out_log.printf( "%s replenishes %d combo_points through anticipation",
        name(), buffs.anticipation -> check() );

  int cp_left = static_cast<int>( resources.max[ RESOURCE_COMBO_POINT ] - resources.current[ RESOURCE_COMBO_POINT ] );
  int n_overflow = buffs.anticipation -> check() - cp_left;
  for ( int i = 0; i < n_overflow; i++ )
    procs.anticipation_wasted -> occur();

  resource_gain( RESOURCE_COMBO_POINT, buffs.anticipation -> check(), nullptr, state ? state -> action : nullptr );
  buffs.anticipation -> expire();
}

void rogue_t::spend_combo_points( const action_state_t* state )
{
  if ( state -> action -> base_costs[ RESOURCE_COMBO_POINT ] == 0 )
    return;

  if ( ! state -> action -> result_is_hit( state -> result ) )
    return;

  if ( buffs.fof_fod -> up() )
    return;

  state -> action -> stats -> consume_resource( RESOURCE_COMBO_POINT, resources.current[ RESOURCE_COMBO_POINT ] );
  resource_loss( RESOURCE_COMBO_POINT, resources.current[ RESOURCE_COMBO_POINT ], nullptr, state ? state -> action : nullptr );

  if ( event_premeditation )
    event_t::cancel( event_premeditation );
}

bool rogue_t::trigger_t17_4pc_combat( const action_state_t* state )
{
  using namespace actions;

  if ( ! sets.has_set_bonus( ROGUE_OUTLAW, T17, B4 ) )
    return false;

  if ( state -> action -> base_costs[ RESOURCE_COMBO_POINT ] == 0 )
    return false;

  if ( ! state -> action -> harmful )
    return false;

  if ( ! state -> action -> result_is_hit( state -> result ) )
    return false;

  const rogue_attack_state_t* rs = debug_cast<const rogue_attack_state_t*>( state );
  if ( ! rng().roll( sets.set( ROGUE_OUTLAW, T17, B4 ) -> proc_chance() / 5.0 * rs -> cp ) )
    return false;

  trigger_combo_point_gain( state, buffs.deceit -> data().effectN( 2 ).base_value(), gains.deceit );
  buffs.deceit -> trigger();
  return true;
}

namespace buffs {
// ==========================================================================
// Buffs
// ==========================================================================

struct shadow_dance_t : public buff_t
{
  shadow_dance_t( rogue_t* p ) :
    buff_t( buff_creator_t( p, "shadow_dance", p -> find_specialization_spell( "Shadow Dance" ) )
            .cd( timespan_t::zero() )
            .duration( p -> find_specialization_spell( "Shadow Dance" ) -> duration() +
                       p -> sets.set( SET_MELEE, T13, B4 ) -> effectN( 1 ).time_value() ) )
  { }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    buff_t::expire_override( expiration_stacks, remaining_duration );

    rogue_t* rogue = debug_cast<rogue_t*>( player );
    rogue -> buffs.shadow_strikes -> trigger();
  }
};

struct fof_fod_t : public buff_t
{
  fof_fod_t( rogue_t* p ) :
    buff_t( buff_creator_t( p, "legendary_daggers" ).duration( timespan_t::from_seconds( 6.0 ) ).cd( timespan_t::zero() ) )
  { }

  virtual void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    buff_t::expire_override( expiration_stacks, remaining_duration );

    rogue_t* p = debug_cast< rogue_t* >( player );
    p -> buffs.fof_p3 -> expire();
  }
};

struct subterfuge_t : public buff_t
{
  rogue_t* rogue;

  subterfuge_t( rogue_t* r ) :
    buff_t( buff_creator_t( r, "subterfuge", r -> find_spell( 115192 ) ) ),
    rogue( r )
  { }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    buff_t::expire_override( expiration_stacks, remaining_duration );
    // The Glyph of Vanish bug is back, so if Vanish is still up when
    // Subterfuge fades, don't cancel stealth. Instead, the next offensive
    // action in the sim will trigger a new (3 seconds) of Suberfuge.
    if ( ( rogue -> bugs && (
            rogue -> buffs.vanish -> remains() == timespan_t::zero() ||
            rogue -> buffs.vanish -> check() == 0 ) ) ||
        ! rogue -> bugs )
    {
      actions::break_stealth( rogue );
    }
  }
};

struct vanish_t : public buff_t
{
  rogue_t* rogue;

  vanish_t( rogue_t* r ) :
    buff_t( buff_creator_t( r, "vanish", r -> find_spell( 11327 ) ) ),
    rogue( r )
  { }

  void execute( int stacks, double value, timespan_t duration ) override
  {
    buff_t::execute( stacks, value, duration );

    rogue -> buffs.stealth -> trigger();
  }
};

// Note, stealth buff is set a max time of half the nominal fight duration, so it can be forced to
// show in sample sequence tables.
struct stealth_t : public buff_t
{
  rogue_t* rogue;

  stealth_t( rogue_t* r ) :
    buff_t( buff_creator_t( r, "stealth", r -> find_spell( 1784 ) )
        .duration( r -> sim -> max_time / 2 ) ),
    rogue( r )
  { }

  void execute( int stacks, double value, timespan_t duration ) override
  {
    buff_t::execute( stacks, value, duration );

    rogue -> buffs.master_of_subtlety -> expire();
    rogue -> buffs.master_of_subtlety_passive -> trigger();
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    buff_t::expire_override( expiration_stacks, remaining_duration );

    rogue -> buffs.master_of_subtlety_passive -> expire();
    rogue -> buffs.master_of_subtlety -> trigger();
  }
};

struct rogue_poison_buff_t : public buff_t
{
  rogue_poison_buff_t( rogue_td_t& r, const std::string& name, const spell_data_t* spell ) :
    buff_t( buff_creator_t( r, name, spell ) )
  { }

  void execute( int stacks, double value, timespan_t duration ) override
  {
    rogue_t* rogue = debug_cast< rogue_t* >( source );
    if ( ! rogue -> poisoned_enemy( player ) )
      rogue -> poisoned_enemies++;

    buff_t::execute( stacks, value, duration );
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    buff_t::expire_override( expiration_stacks, remaining_duration );

    rogue_t* rogue = debug_cast< rogue_t* >( source );
    if ( ! rogue -> poisoned_enemy( player ) )
      rogue -> poisoned_enemies--;
  }
};

struct wound_poison_t : public rogue_poison_buff_t
{
  wound_poison_t( rogue_td_t& r ) :
    rogue_poison_buff_t( r, "wound_poison", r.source -> find_spell( 8680 ) )
  { }
};

struct crippling_poison_t : public rogue_poison_buff_t
{
  crippling_poison_t( rogue_td_t& r ) :
    rogue_poison_buff_t( r, "crippling_poison", r.source -> find_spell( 3409 ) )
  { }
};

struct leeching_poison_t : public rogue_poison_buff_t
{
  leeching_poison_t( rogue_td_t& r ) :
    rogue_poison_buff_t( r, "leeching_poison", r.source -> find_spell( 112961 ) )
  { }
};

struct marked_for_death_debuff_t : public debuff_t
{
  cooldown_t* mod_cd;

  marked_for_death_debuff_t( rogue_td_t& r ) :
    debuff_t( buff_creator_t( r, "marked_for_death", r.source -> find_talent_spell( "Marked for Death" ) ).cd( timespan_t::zero() ) ),
    mod_cd( r.source -> get_cooldown( "marked_for_death" ) )
  { }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    if ( remaining_duration > timespan_t::zero() )
    {
      if ( sim -> debug )
      {
        sim -> out_debug.printf("%s marked_for_death cooldown reset", player -> name() );
      }

      mod_cd -> reset( false );
    }

    debuff_t::expire_override( expiration_stacks, remaining_duration );
  }
};

} // end namespace buffs

inline void actions::marked_for_death_t::impact( action_state_t* state )
{
  rogue_attack_t::impact( state );

  td( state -> target ) -> debuffs.marked_for_death -> trigger();
}

// ==========================================================================
// Rogue Targetdata Definitions
// ==========================================================================

rogue_td_t::rogue_td_t( player_t* target, rogue_t* source ) :
  actor_target_data_t( target, source ),
  dots( dots_t() ),
  debuffs( debuffs_t() )
{

  dots.deadly_poison    = target -> get_dot( "deadly_poison_dot", source );
  dots.garrote          = target -> get_dot( "garrote", source );
  dots.rupture          = target -> get_dot( "rupture", source );
  dots.hemorrhage       = target -> get_dot( "hemorrhage", source );
  dots.killing_spree    = target -> get_dot( "killing_spree", source );

  const spell_data_t* vd = source -> find_specialization_spell( "Vendetta" );
  debuffs.vendetta =           buff_creator_t( *this, "vendetta", vd )
                               .cd( timespan_t::zero() )
                               .duration ( vd -> duration() +
                                           source -> sets.set( SET_MELEE, T13, B4 ) -> effectN( 3 ).time_value() )
                               .default_value( vd -> effectN( 1 ).percent() );

  debuffs.wound_poison = new buffs::wound_poison_t( *this );
  debuffs.crippling_poison = new buffs::crippling_poison_t( *this );
  debuffs.leeching_poison = new buffs::leeching_poison_t( *this );
  debuffs.marked_for_death = new buffs::marked_for_death_debuff_t( *this );
}

// ==========================================================================
// Rogue Character Definition
// ==========================================================================

// rogue_t::composite_attack_speed ==========================================

double rogue_t::composite_melee_speed() const
{
  double h = player_t::composite_melee_speed();

  if ( buffs.slice_and_dice -> check() )
    h *= 1.0 / ( 1.0 + buffs.slice_and_dice -> value() );

  if ( buffs.adrenaline_rush -> check() )
    h *= 1.0 / ( 1.0 + buffs.adrenaline_rush -> value() );

  return h;
}

// rogue_t::composite_melee_crit =========================================

double rogue_t::composite_melee_crit() const
{
  double crit = player_t::composite_melee_crit();

  crit += spell.critical_strikes -> effectN( 1 ).percent();

  return crit;
}

// rogue_t::composite_spell_crit =========================================

double rogue_t::composite_spell_crit() const
{
  double crit = player_t::composite_spell_crit();

  crit += spell.critical_strikes -> effectN( 1 ).percent();

  return crit;
}

// rogue_t::matching_gear_multiplier ========================================

double rogue_t::matching_gear_multiplier( attribute_e attr ) const
{
  if ( attr == ATTR_AGILITY )
    return 0.05;

  return 0.0;
}

// rogue_t::composite_attack_power_multiplier ===============================

double rogue_t::composite_attack_power_multiplier() const
{
  double m = player_t::composite_attack_power_multiplier();

  if ( spec.vitality -> ok() )
  {
    m *= 1.0 + spec.vitality -> effectN( 2 ).percent();
  }

  return m;
}

// rogue_t::composite_player_multiplier =====================================

double rogue_t::composite_player_multiplier( school_e school ) const
{
  double m = player_t::composite_player_multiplier( school );

  if ( buffs.master_of_subtlety -> check() || buffs.master_of_subtlety_passive -> check() )
  {
    m *= 1.0 + talent.master_of_subtlety -> effectN( 1 ).percent();
  }

  if ( main_hand_weapon.type == WEAPON_DAGGER && off_hand_weapon.type == WEAPON_DAGGER && spec.assassins_resolve -> ok() )
  {
    m *= 1.0 + spec.assassins_resolve -> effectN( 2 ).percent();
  }

  if ( sets.has_set_bonus( ROGUE_OUTLAW, T18, B4 ) )
  {
    if ( buffs.adrenaline_rush -> up() )
    {
      m *= 1.0 + sets.set( ROGUE_OUTLAW, T18, B4 ) -> effectN( 1 ).percent();
    }
  }

  if ( buffs.deathly_shadows -> up() )
  {
    m *= 1.0 + buffs.deathly_shadows -> data().effectN( 1 ).percent();
  }

  return m;
}

// rogue_t::init_actions ====================================================

void rogue_t::init_action_list()
{
  if ( main_hand_weapon.type == WEAPON_NONE )
  {
    if ( !quiet )
      sim -> errorf( "Player %s has no weapon equipped at the Main-Hand slot.", name() );
    quiet = true;
    return;
  }

  if ( !action_list_str.empty() )
  {
    player_t::init_action_list();
    return;
  }

  action_priority_list_t* precombat = get_action_priority_list( "precombat" );
  action_priority_list_t* def       = get_action_priority_list( "default" );

  std::vector<std::string> item_actions = get_item_actions();
  std::vector<std::string> profession_actions = get_profession_actions();
  std::vector<std::string> racial_actions = get_racial_actions();

  clear_action_priority_lists();

  // Flask
  if ( sim -> allow_flasks && true_level >= 80 )
  {
    std::string flask_action = "flask,type=";
    if ( true_level > 90 )
      flask_action += "greater_draenic_agility_flask";
    else
      flask_action += ( true_level >= 85 ) ? "spring_blossoms" : ( ( true_level >= 80 ) ? "winds" : "" );

    precombat -> add_action( flask_action );
  }

  // Food
  if ( sim -> allow_food && level() >= 80 )
  {
    std::string food_action = "food,type=";
    if ( specialization() == ROGUE_ASSASSINATION )
      food_action += ( ( level() >= 100 ) ? "sleeper_sushi" : ( level() > 85 ) ? "sea_mist_rice_noodles" : ( level() > 80 ) ? "seafood_magnifique_feast" : "" );
    else if ( specialization() == ROGUE_OUTLAW )
      food_action += ( ( level() >= 100 ) ? "felmouth_frenzy" : ( level() > 85 ) ? "sea_mist_rice_noodles" : ( level() > 80 ) ? "seafood_magnifique_feast" : "" );
    else if ( specialization() == ROGUE_SUBTLETY )
      food_action += ( ( level() >= 100 ) ? "salty_squid_roll" : ( level() > 85 ) ? "sea_mist_rice_noodles" : ( level() > 80 ) ? "seafood_magnifique_feast" : "" );

    precombat -> add_action( food_action );
  }

  // Lethal poison
  std::string poison_str = "apply_poison,lethal=deadly";

  precombat -> add_action( poison_str );

  // Snapshot stats
  precombat -> add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );

  std::string potion_name;
  if ( sim -> allow_potions && true_level >= 80 )
  {
    if ( true_level > 90 )
      potion_name = "draenic_agility";
    else if ( true_level > 85 )
      potion_name = "virmens_bite";
    else
      potion_name = "tolvir";

    precombat -> add_action( "potion,name=" + potion_name );
  }

  precombat -> add_action( this, "Stealth" );

  std::string potion_action_str = "potion,name=" + potion_name + ",if=buff.bloodlust.react|target.time_to_die<40";
  if ( specialization() == ROGUE_ASSASSINATION )
  {
    potion_action_str += "|debuff.vendetta.up";
  }
  else if ( specialization() == ROGUE_OUTLAW )
  {
    if ( find_item( "maalus_the_blood_drinker" ) )
      potion_action_str += "|(buff.adrenaline_rush.up&buff.maalus.up&(trinket.proc.any.react|trinket.stacking_proc.any.react|buff.archmages_greater_incandescence_agi.react))";
    else
      potion_action_str += "|(buff.adrenaline_rush.up&(trinket.proc.any.react|trinket.stacking_proc.any.react|buff.archmages_greater_incandescence_agi.react))";
  }
  else if ( specialization() == ROGUE_SUBTLETY )
  {
    potion_action_str += "|(buff.shadow_reflection.up|(!talent.shadow_reflection.enabled&buff.shadow_dance.up))&(trinket.stat.agi.react|buff.archmages_greater_incandescence_agi.react)|((buff.shadow_reflection.up|(!talent.shadow_reflection.enabled&buff.shadow_dance.up))&target.time_to_die<136)";
  }

  // In-combat potion
  if ( sim -> allow_potions )
    def -> add_action( potion_action_str );

  def -> add_action( this, "Kick" );

  if ( specialization() == ROGUE_ASSASSINATION )
  {
    precombat -> add_talent( this, "Marked for Death" );
    precombat -> add_action( this, "Slice and Dice", "if=talent.marked_for_death.enabled" );

    for ( size_t i = 0; i < item_actions.size(); i++ )
      def -> add_action( item_actions[i] + ",if=spell_targets.fan_of_knives>1|(debuff.vendetta.up&spell_targets.fan_of_knives=1)" );

    for ( size_t i = 0; i < racial_actions.size(); i++ )
    {
      if ( racial_actions[i] == "arcane_torrent" )
        def -> add_action( racial_actions[i] + ",if=energy<60" );
      else
        def -> add_action( racial_actions[i] );
    }

    action_priority_list_t* finishers = get_action_priority_list( "finishers" );
    action_priority_list_t* generators = get_action_priority_list( "generators" );

    def -> add_action( this, "Vanish", "if=time>10&energy>13&!buff.stealth.up&buff.blindside.down&energy.time_to_max>gcd*2&((combo_points+anticipation_charges<8)|(!talent.anticipation.enabled&combo_points<=1))" );
    def -> add_action( this, "Mutilate", "if=buff.stealth.up|buff.vanish.up" );
    def -> add_action( this, "Rupture", "if=((combo_points>=4&!talent.anticipation.enabled)|combo_points=5)&ticks_remain<3" );
    def -> add_action( this, "Rupture", "cycle_targets=1,if=spell_targets.fan_of_knives>1&!ticking&combo_points=5" );
    def -> add_talent( this, "Marked for Death", "if=combo_points=0" );
    def -> add_talent( this, "Shadow Reflection", "if=combo_points>4|target.time_to_die<=20" );
    def -> add_action( this, "Vendetta", "if=buff.shadow_reflection.up|!talent.shadow_reflection.enabled|target.time_to_die<=20|(target.time_to_die<=30&glyph.vendetta.enabled)" );
    def -> add_action( this, "Rupture", "cycle_targets=1,if=combo_points=5&remains<=duration*0.3&spell_targets.fan_of_knives>1" );
    def -> add_action( "call_action_list,name=finishers,if=combo_points=5&((!cooldown.death_from_above.remains&talent.death_from_above.enabled)|buff.envenom.down|!talent.anticipation.enabled|anticipation_charges+combo_points>=6)" );
    def -> add_action( "call_action_list,name=finishers,if=dot.rupture.remains<2" );
    def -> add_action( "call_action_list,name=generators" );

    finishers -> add_action( this, "Rupture", "cycle_targets=1,if=(remains<2|(combo_points=5&remains<=(duration*0.3)))" );
    finishers -> add_action( "pool_resource,for_next=1" );
    finishers -> add_talent( this, "Death from Above", "if=(cooldown.vendetta.remains>10|debuff.vendetta.up|target.time_to_die<=25)" );
    finishers -> add_action( this, "Envenom", "cycle_targets=1,if=dot.deadly_poison_dot.remains<4&target.health.pct<=35&(energy+energy.regen*cooldown.vendetta.remains>=105&(buff.envenom.remains<=1.8|energy>45))|buff.bloodlust.up|debuff.vendetta.up" );
    finishers -> add_action( this, "Envenom", "cycle_targets=1,if=dot.deadly_poison_dot.remains<4&target.health.pct>35&(energy+energy.regen*cooldown.vendetta.remains>=105&(buff.envenom.remains<=1.8|energy>55))|buff.bloodlust.up|debuff.vendetta.up" );
    finishers -> add_action( this, "Envenom", "if=target.health.pct<=35&(energy+energy.regen*cooldown.vendetta.remains>=105&(buff.envenom.remains<=1.8|energy>45))|buff.bloodlust.up|debuff.vendetta.up" );
    finishers -> add_action( this, "Envenom", "if=target.health.pct>35&(energy+energy.regen*cooldown.vendetta.remains>=105&(buff.envenom.remains<=1.8|energy>55))|buff.bloodlust.up|debuff.vendetta.up" );

    generators -> add_action( this, "Dispatch", "cycle_targets=1,if=dot.deadly_poison_dot.remains<4&talent.anticipation.enabled&((anticipation_charges<4&set_bonus.tier18_4pc=0)|(anticipation_charges<2&set_bonus.tier18_4pc=1))" );
    generators -> add_action( this, "Dispatch", "cycle_targets=1,if=dot.deadly_poison_dot.remains<4&!talent.anticipation.enabled&combo_points<5&set_bonus.tier18_4pc=0" );
    generators -> add_action( this, "Dispatch", "cycle_targets=1,if=dot.deadly_poison_dot.remains<4&!talent.anticipation.enabled&set_bonus.tier18_4pc=1&(combo_points<2|target.health.pct<35)" );
    generators -> add_action( this, "Dispatch", "if=talent.anticipation.enabled&((anticipation_charges<4&set_bonus.tier18_4pc=0)|(anticipation_charges<2&set_bonus.tier18_4pc=1))" );
    generators -> add_action( this, "Dispatch", "if=!talent.anticipation.enabled&combo_points<5&set_bonus.tier18_4pc=0" );
    generators -> add_action( this, "Dispatch", "if=!talent.anticipation.enabled&set_bonus.tier18_4pc=1&(combo_points<2|target.health.pct<35)" );
    generators -> add_action( this, "Mutilate", "cycle_targets=1,if=dot.deadly_poison_dot.remains<4&target.health.pct>35&(combo_points<5|(talent.anticipation.enabled&anticipation_charges<3))" );
    generators -> add_action( this, "Mutilate", "if=target.health.pct>35&(combo_points<5|(talent.anticipation.enabled&anticipation_charges<3))" );
  }

  else if ( specialization() == ROGUE_OUTLAW )
  {
    precombat -> add_talent( this, "Marked for Death" );
    precombat -> add_action( this, "Slice and Dice", "if=talent.marked_for_death.enabled" );

    for ( size_t i = 0; i < item_actions.size(); i++ )
    {
      if ( find_item( "maalus_the_blood_drinker" ) )
        def -> add_action( item_actions[i] + ",if=buff.adrenaline_rush.up" );
      else if ( find_item( "mirror_of_the_blademaster" ) )
        def -> add_action( item_actions[i] );
      else
        def -> add_action( item_actions[i] + ",if=buff.adrenaline_rush.up" );
    }

    for ( size_t i = 0; i < racial_actions.size(); i++ )
    {
      if ( racial_actions[i] == "arcane_torrent" )
        def -> add_action( racial_actions[i] + ",if=energy<60" );
      else
        def -> add_action( racial_actions[i] );
    }

    action_priority_list_t* ar = get_action_priority_list( "adrenaline_rush", "Adrenaline Rush Handler" );
    action_priority_list_t* ks = get_action_priority_list( "killing_spree", "Killing Spree Handler" );
    action_priority_list_t* finisher = get_action_priority_list( "finisher", "Combo Point Finishers" );
    action_priority_list_t* gen = get_action_priority_list( "generator", "Combo Point Generators" );

    def -> add_action( this, "Blade Flurry", "if=(spell_targets.blade_flurry>=2&!buff.blade_flurry.up)|(spell_targets.blade_flurry<2&buff.blade_flurry.up)" );
    def -> add_talent( this, "Shadow Reflection", "if=(cooldown.killing_spree.remains<10&combo_points>3)|buff.adrenaline_rush.up" );
    def -> add_action( this, find_class_spell( "Ambush" ), "pool_resource", "for_next=1" );
    def -> add_action( this, "Ambush" );
    def -> add_action( this, "Vanish", "if=time>25&(combo_points<4|(talent.anticipation.enabled&anticipation_charges<4))&((talent.shadow_focus.enabled&energy.time_to_max>2.5&energy>=15)|(talent.subterfuge.enabled&energy>=90)|(!talent.shadow_focus.enabled&!talent.subterfuge.enabled&energy>=60))" );
    def -> add_action( "shadowmeld,if=(combo_points<4|(talent.anticipation.enabled&anticipation_charges<4))&energy>=60" );

    // Rotation
    def -> add_action( this, "Slice and Dice", "if=buff.slice_and_dice.remains<2&(dot.revealing_strike.ticking|time>10)|(target.time_to_die>45&combo_points=5&buff.slice_and_dice.remains<12)" );
    def -> add_action( "call_action_list,name=adrenaline_rush,if=dot.revealing_strike.ticking&buff.adrenaline_rush.down" );
    def -> add_action( "call_action_list,name=killing_spree,if=energy.time_to_max>6&(!talent.shadow_reflection.enabled|cooldown.shadow_reflection.remains>30|buff.shadow_reflection.remains>8)" );

    // Adrenaline Rush Handler
    ar -> add_action( this, "Adrenaline Rush", "if=target.time_to_die>=75" );
    if ( find_item( "maalus_the_blood_drinker" ) )
      ar -> add_action( this, "Adrenaline Rush", "if=target.time_to_die<75&(buff.maalus.up|trinket.proc.any.react|trinket.stacking_proc.any.react)" );
    else if ( find_item( "spellbound_runic_band_of_unrelenting_slaughter" ) )
      ar -> add_action( this, "Adrenaline Rush", "if=target.time_to_die<75&(buff.archmages_greater_incandescence_agi.react|trinket.proc.any.react|trinket.stacking_proc.any.react)" );
    else
      ar -> add_action( this, "Adrenaline Rush", "if=target.time_to_die<75&(trinket.proc.any.react|trinket.stacking_proc.any.react)" );
    ar -> add_action( this, "Adrenaline Rush", "if=target.time_to_die<=buff.adrenaline_rush.duration*2" );

    // Killing Spree Handler
    ks -> add_action( this, "Killing Spree", "if=target.time_to_die>=25" );
    if ( find_item( "maalus_the_blood_drinker" ) )
      ks -> add_action( this, "Killing Spree", "if=target.time_to_die<25&(buff.maalus.up&buff.maalus.remains>=buff.killing_spree.duration|trinket.proc.any.react&trinket.proc.any.remains>=buff.killing_spree.duration|trinket.stacking_proc.any.react&trinket.stacking_proc.any.remains>=buff.killing_spree.duration)" );
    else if ( find_item( "spellbound_runic_band_of_unrelenting_slaughter" ) )
      ks -> add_action( this, "Killing Spree", "if=target.time_to_die<25&(buff.archmages_greater_incandescence_agi.react&buff.archmages_greater_incandescence_agi.remains>=buff.killing_spree.duration|trinket.proc.any.react&trinket.proc.any.remains>=buff.killing_spree.duration|trinket.stacking_proc.any.react&trinket.stacking_proc.any.remains>=buff.killing_spree.duration)" );
    else
      ks -> add_action( this, "Killing Spree", "if=target.time_to_die<25&(trinket.proc.any.react&trinket.proc.any.remains>=buff.killing_spree.duration|trinket.stacking_proc.any.react&trinket.stacking_proc.any.remains>=buff.killing_spree.duration)" );
    ks -> add_action( this, "Killing Spree", "if=target.time_to_die<=buff.killing_spree.duration*5" );

    def -> add_talent( this, "Marked for Death", "if=combo_points<=1&dot.revealing_strike.ticking&(!talent.shadow_reflection.enabled|buff.shadow_reflection.up|cooldown.shadow_reflection.remains>30)" );

    // Generate combo points, or use combo points
    if ( true_level >= 3 )
      def -> add_action( "call_action_list,name=finisher,if=combo_points=5&dot.revealing_strike.ticking&(!talent.anticipation.enabled|(talent.anticipation.enabled&anticipation_charges>=3))" );
    def -> add_action( "call_action_list,name=generator,if=combo_points<5|!dot.revealing_strike.ticking|(talent.anticipation.enabled&anticipation_charges<3)" );

    // Combo Point Finishers
    finisher -> add_talent( this, "Death from Above" );
    //finisher -> add_action( this, "Crimson Tempest", "if=active_enemies>6&remains<2" );
    //finisher -> add_action( this, "Crimson Tempest", "if=active_enemies>8" );
    finisher -> add_action( this, "Eviscerate", "if=(!talent.death_from_above.enabled|cooldown.death_from_above.remains)" );

    // Combo Point Generators
    gen -> add_action( this, "Revealing Strike", "if=(combo_points=4&dot.revealing_strike.remains<7.2&(target.time_to_die>dot.revealing_strike.remains+7.2)|(target.time_to_die<dot.revealing_strike.remains+7.2&ticks_remain<2))|!ticking" );
    gen -> add_action( this, "Saber Slash");
  }
  else if ( specialization() == ROGUE_SUBTLETY )
  {
    precombat -> add_talent( this, "Marked for Death" );
    precombat -> add_action( this, "Premeditation", "if=!talent.marked_for_death.enabled" );
    precombat -> add_action( this, "Slice and Dice" );
    precombat -> add_action( this, "Premeditation" );

    for ( size_t i = 0; i < item_actions.size(); i++ )
      def -> add_action( item_actions[i] + ",if=buff.shadow_dance.up" );

    for ( size_t i = 0; i < racial_actions.size(); i++ )
    {
      if ( racial_actions[i] == "arcane_torrent" )
        def -> add_action( racial_actions[i] + ",if=energy<80&buff.shadow_dance.up" );
      else
        def -> add_action( racial_actions[i] + ",if=buff.shadow_dance.up" );
    }

    // Shadow Dancing and Vanishing and Marking for the Deathing
    def -> add_action( this, "Premeditation", "if=combo_points<4" );
    def -> add_action( this, "Vanish","if=set_bonus.tier18_4pc=1&time<1" );
    def -> add_action( "wait,sec=buff.subterfuge.remains-0.1,if=buff.subterfuge.remains>0.5&buff.subterfuge.remains<1.6&time>6" );

    def -> add_action( this, find_class_spell( "Shadow Dance" ), "pool_resource", "if=energy<110&cooldown.shadow_dance.remains<3.5" );
    def -> add_action( this, find_class_spell( "Shadow Dance" ), "pool_resource", "for_next=1,extra_amount=110" );

    if ( find_item( "maalus_the_blood_drinker" ) )
      def -> add_action( this, "Shadow Dance", "if=energy>=110&buff.stealth.down|((buff.bloodlust.up|buff.deathly_shadows.up)&(dot.hemorrhage.ticking|dot.garrote.ticking|dot.rupture.ticking))" );
    else
      def -> add_action( this, "Shadow Dance", "if=energy>=110&buff.stealth.down&buff.vanish.down&debuff.find_weakness.down|(buff.bloodlust.up&(dot.hemorrhage.ticking|dot.garrote.ticking|dot.rupture.ticking))" );

    def -> add_action( this, find_class_spell( "Vanish" ), "pool_resource", "for_next=1,extra_amount=50" );
    def -> add_action( "shadowmeld,if=talent.shadow_focus.enabled&energy>=45&energy<=75&combo_points<4-talent.anticipation.enabled&buff.stealth.down&buff.shadow_dance.down&buff.master_of_subtlety.down&debuff.find_weakness.down" );

    def -> add_action( this, find_class_spell( "Vanish" ), "pool_resource", "for_next=1,extra_amount=50" );
    def -> add_action( this, "Vanish", "if=talent.shadow_focus.enabled&energy>=45&energy<=75&combo_points<4-talent.anticipation.enabled&buff.shadow_dance.down&buff.master_of_subtlety.down&debuff.find_weakness.down" );

    def -> add_action( this, find_class_spell( "Vanish" ), "pool_resource", "for_next=1,extra_amount=115" );
    def -> add_action( "shadowmeld,if=talent.subterfuge.enabled&energy>=115&combo_points<4-talent.anticipation.enabled&buff.stealth.down&buff.shadow_dance.down&buff.master_of_subtlety.down&debuff.find_weakness.down" );

    if ( find_item( "maalus_the_blood_drinker" ) )
      def -> add_action( this, "Vanish", "if=set_bonus.tier18_4pc=1&buff.shadow_reflection.up&combo_points<3");
    def -> add_action( this, find_class_spell( "Vanish" ), "pool_resource", "for_next=1,extra_amount=115" );
    def -> add_action( this, "Vanish", "if=talent.subterfuge.enabled&energy>=115&combo_points<4-talent.anticipation.enabled&buff.shadow_dance.down" );

    def -> add_talent( this, "Marked for Death", "if=combo_points=0" );

    // Rotation
    def -> add_action( "run_action_list,name=finisher,if=combo_points=5&debuff.find_weakness.remains&buff.shadow_reflection.remains" );
    def -> add_action( this, find_class_spell( "Ambush" ), "pool_resource", "for_next=1" );
    def -> add_action( this, "Ambush", "if=talent.anticipation.enabled&combo_points+anticipation_charges<8&time>2" );

    def -> add_action( "run_action_list,name=finisher,if=combo_points=5" );
    def -> add_action( "run_action_list,name=generator,if=combo_points<4|(talent.anticipation.enabled&anticipation_charges<3&debuff.find_weakness.down)" );

    // Combo point generators
    action_priority_list_t* gen = get_action_priority_list( "generator", "Combo point generators" );
    gen -> add_action( this, find_class_spell( "Ambush" ), "pool_resource", "for_next=1" );
    gen -> add_action( this, "Ambush" );
    gen -> add_action( this, "Fan of Knives", "if=spell_targets.fan_of_knives>2", "If simulating AoE, it is recommended to use Anticipation as the level 90 talent." );
    gen -> add_action( this, "Backstab", "if=debuff.find_weakness.up|buff.archmages_greater_incandescence_agi.up|trinket.stat.any.up" );
    gen -> add_talent( this, "Shuriken Toss", "if=energy<65&energy.regen<16" );
    gen -> add_action( this, "Hemorrhage", "if=glyph.hemorrhaging_veins.enabled&((talent.anticipation.enabled&combo_points+anticipation_charges<=2)|combo_points<=2)&!ticking&!dot.rupture.ticking&!dot.crimson_tempest.ticking&!dot.garrote.ticking" );
    gen -> add_action( this, "Backstab", "if=energy.time_to_max<=gcd*2" );
    gen -> add_action( this, "Hemorrhage", "if=energy.time_to_max<=gcd*1.5&position_front" );

    // Combo point finishers
    action_priority_list_t* finisher = get_action_priority_list( "finisher", "Combo point finishers" );
    finisher -> add_action( this, "Rupture", "cycle_targets=1,if=(!ticking|remains<duration*0.3|(buff.shadow_reflection.remains>8&dot.rupture.remains<12&time>20))" );
    finisher -> add_action( this, "Slice and Dice", "if=((buff.slice_and_dice.remains<10.8&debuff.find_weakness.down)|buff.slice_and_dice.remains<6)&buff.slice_and_dice.remains<target.time_to_die" );
    finisher -> add_talent( this, "Death from Above" );
    finisher -> add_action( this, "Crimson Tempest", "if=(spell_targets.crimson_tempest>=2&debuff.find_weakness.down)|spell_targets.crimson_tempest>=3&(cooldown.death_from_above.remains>0|!talent.death_from_above.enabled)" );
    finisher -> add_action( this, "Eviscerate", "if=(energy.time_to_max<=cooldown.death_from_above.remains+action.death_from_above.execute_time)|!talent.death_from_above.enabled" );
  }

  use_default_action_list = true;

  player_t::init_action_list();
}

// rogue_t::create_action  ==================================================

action_t* rogue_t::create_action( const std::string& name,
                                  const std::string& options_str )
{
  using namespace actions;

  if ( name == "adrenaline_rush"     ) return new adrenaline_rush_t    ( this, options_str );
  if ( name == "ambush"              ) return new ambush_t             ( this, options_str );
  if ( name == "apply_poison"        ) return new apply_poison_t       ( this, options_str );
  if ( name == "auto_attack"         ) return new auto_melee_attack_t  ( this, options_str );
  if ( name == "backstab"            ) return new backstab_t           ( this, options_str );
  if ( name == "blade_flurry"        ) return new blade_flurry_t       ( this, options_str );
  if ( name == "death_from_above"    ) return new death_from_above_t   ( this, options_str );
  if ( name == "envenom"             ) return new envenom_t            ( this, options_str );
  if ( name == "eviscerate"          ) return new eviscerate_t         ( this, options_str );
  if ( name == "fan_of_knives"       ) return new fan_of_knives_t      ( this, options_str );
  if ( name == "feint"               ) return new feint_t              ( this, options_str );
  if ( name == "garrote"             ) return new garrote_t            ( this, options_str );
  if ( name == "hemorrhage"          ) return new hemorrhage_t         ( this, options_str );
  if ( name == "kick"                ) return new kick_t               ( this, options_str );
  if ( name == "killing_spree"       ) return new killing_spree_t      ( this, options_str );
  if ( name == "marked_for_death"    ) return new marked_for_death_t   ( this, options_str );
  if ( name == "mutilate"            ) return new mutilate_t           ( this, options_str );
  if ( name == "pistol shot"         ) return new pistol_shot_t        ( this, options_str );
  if ( name == "premeditation"       ) return new premeditation_t      ( this, options_str );
  if ( name == "rupture"             ) return new rupture_t            ( this, options_str );
  if ( name == "saber_slash"         ) return new saber_slash_t        ( this, options_str );
  if ( name == "shadowstep"          ) return new shadowstep_t         ( this, options_str );
  if ( name == "shuriken_toss"       ) return new shuriken_toss_t      ( this, options_str );
  if ( name == "slice_and_dice"      ) return new slice_and_dice_t     ( this, options_str );
  if ( name == "sprint"              ) return new sprint_t             ( this, options_str );
  if ( name == "stealth"             ) return new stealth_t            ( this, options_str );
  if ( name == "vanish"              ) return new vanish_t             ( this, options_str );
  if ( name == "vendetta"            ) return new vendetta_t           ( this, options_str );

  if ( name == "swap_weapon"         ) return new weapon_swap_t        ( this, options_str );

  return player_t::create_action( name, options_str );
}

// rogue_t::create_expression ===============================================

expr_t* rogue_t::create_expression( action_t* a, const std::string& name_str )
{
  if ( name_str == "combo_points" )
    return make_ref_expr( name_str, resources.current[ RESOURCE_COMBO_POINT ] );
  else if ( util::str_compare_ci( name_str, "anticipation_charges" ) )
    return make_ref_expr( name_str, buffs.anticipation -> current_stack );
  else if ( util::str_compare_ci( name_str, "poisoned_enemies" ) )
    return make_ref_expr( name_str, poisoned_enemies );

  return player_t::create_expression( a, name_str );
}

// rogue_t::init_base =======================================================

void rogue_t::init_base_stats()
{
  player_t::init_base_stats();

  base.attack_power_per_strength = 0.0;
  base.attack_power_per_agility  = 1.0;

  resources.base[ RESOURCE_ENERGY ] = 100;
  resources.base[ RESOURCE_COMBO_POINT ] = 5;
  if ( main_hand_weapon.type == WEAPON_DAGGER && off_hand_weapon.type == WEAPON_DAGGER )
    resources.base[ RESOURCE_ENERGY ] += spec.assassins_resolve -> effectN( 1 ).base_value();
  //if ( sets.has_set_bonus( SET_MELEE, PVP, B2 ) )
  //  resources.base[ RESOURCE_ENERGY ] += 10;

  base_energy_regen_per_second = 10 * ( 1.0 + spec.vitality -> effectN( 1 ).percent() );

  base_gcd = timespan_t::from_seconds( 1.0 );
}

// rogue_t::init_spells =====================================================

void rogue_t::init_spells()
{
  player_t::init_spells();

  // Shared
  spec.shadowstep           = find_specialization_spell( "Shadowstep" );

  // Assassination
  spec.assassins_resolve    = find_specialization_spell( "Assassin's Resolve" );
  spec.cut_to_the_chase     = find_specialization_spell( "Cut to the Chase" );
  spec.improved_poisons     = find_specialization_spell( "Improved Poisons" );
  spec.seal_fate            = find_specialization_spell( "Seal Fate" );
  spec.venomous_wounds      = find_specialization_spell( "Venomous Wounds" );

  // Combat
  spec.blade_flurry         = find_specialization_spell( "Blade Flurry" );
  spec.combat_potency       = find_specialization_spell( "Combat Potency" );
  spec.ruthlessness         = find_specialization_spell( "Ruthlessness" );
  spec.swashbuckler         = find_specialization_spell( "Swashbuckler" );
  spec.vitality             = find_specialization_spell( "Vitality" );

  // Subtlety
  spec.energetic_recovery   = find_specialization_spell( "Energetic Recovery" );
  spec.master_of_shadows    = find_specialization_spell( "Master of Shadows" );
  spec.shadow_dance         = find_specialization_spell( "Shadow Dance" );
  spec.shadow_techniques    = find_specialization_spell( "Shadow Techniques" );

  // Masteries
  mastery.potent_poisons    = find_mastery_spell( ROGUE_ASSASSINATION );
  mastery.main_gauche       = find_mastery_spell( ROGUE_OUTLAW );
  mastery.executioner       = find_mastery_spell( ROGUE_SUBTLETY );

  // Misc spells
  spell.critical_strikes    = find_spell( 157442 );
  spell.death_from_above    = find_spell( 163786 );
  spell.fan_of_knives       = find_class_spell( "Fan of Knives" );
  spell.fleet_footed        = find_class_spell( "Fleet Footed" );
  spell.sprint              = find_class_spell( "Sprint" );
  spell.relentless_strikes  = find_spell( 58423 );
  spell.ruthlessness_cp_driver = find_spell( 174597 );
  spell.ruthlessness_driver = find_spell( 14161 );
  spell.ruthlessness_cp     = spec.ruthlessness -> effectN( 1 ).trigger();
  spell.shadow_focus        = find_spell( 112942 );
  spell.tier13_2pc          = find_spell( 105864 );
  spell.tier13_4pc          = find_spell( 105865 );
  spell.tier15_4pc          = find_spell( 138151 );
  spell.tier18_2pc_combat_ar= find_spell( 186286 );

  // Talents
  talent.deeper_strategem   = find_talent_spell( "Deeper Strategem" );
  talent.anticipation       = find_talent_spell( "Anticipation" );
  talent.vigor              = find_talent_spell( "Vigor" );

  talent.alacrity           = find_talent_spell( "Alacrity" );

  talent.marked_for_death   = find_talent_spell( "Marked for Death" );
  talent.death_from_above   = find_talent_spell( "Death from Above" );

  talent.nightstalker       = find_talent_spell( "Nightstalker" );
  talent.subterfuge         = find_talent_spell( "Subterfuge" );
  talent.shadow_focus       = find_talent_spell( "Shadow Focu" );

  talent.master_poisoner    = find_talent_spell( "Master Poisoner" );
  talent.elaborate_planning = find_talent_spell( "Elaborate Planning" );
  talent.hemorrhage         = find_talent_spell( "Hemorrhage" );

  talent.numbing_poison     = find_talent_spell( "Numbing Poison" );
  talent.blood_sweat        = find_talent_spell( "Blood Sweat" );

  talent.venom_rush         = find_talent_spell( "Venom Rush" );

  talent.ghostly_strike     = find_talent_spell( "Ghostly Strike" );
  talent.swordmaster        = find_talent_spell( "Swordmaster" );
  talent.quick_draw         = find_talent_spell( "Quick Draw" );

  talent.cannonball_barrage = find_talent_spell( "Cannonball Barrage" );
  talent.killing_spree      = find_talent_spell( "Killing Spree" );

  talent.roll_the_bones     = find_talent_spell( "Roll the Bones" );

  talent.master_of_subtlety = find_talent_spell( "Master of Subtlety" );
  talent.weaponmaster       = find_talent_spell( "Weaponmaster" );
  talent.gloomblade         = find_talent_spell( "Gloomblade" );

  talent.premeditation      = find_talent_spell( "Premeditation" );
  talent.enveloping_shadows = find_talent_spell( "Enveloping Shadows" );

  talent.relentless_strikes = find_talent_spell( "Relentless Strikes" );

  auto_attack = new actions::auto_melee_attack_t( this, "" );

  if ( mastery.main_gauche -> ok() )
    active_main_gauche = new actions::main_gauche_t( this );

  if ( spec.blade_flurry -> ok() )
    active_blade_flurry = new actions::blade_flurry_attack_t( this );
}

// rogue_t::init_gains ======================================================

void rogue_t::init_gains()
{
  player_t::init_gains();

  gains.adrenaline_rush         = get_gain( "adrenaline_rush"    );
  gains.combat_potency          = get_gain( "combat_potency"     );
  gains.deceit                  = get_gain( "deceit" );
  gains.empowered_fan_of_knives = get_gain( "empowered_fan_of_knives" );
  gains.energetic_recovery      = get_gain( "energetic_recovery" );
  gains.energy_refund           = get_gain( "energy_refund"      );
  gains.legendary_daggers       = get_gain( "legendary_daggers" );
  gains.murderous_intent        = get_gain( "murderous_intent"   );
  gains.overkill                = get_gain( "overkill"           );
  gains.premeditation           = get_gain( "premeditation" );
  gains.relentless_strikes      = get_gain( "relentless_strikes" );
  gains.seal_fate               = get_gain( "seal_fate" );
  gains.shadow_strikes          = get_gain( "shadow_strikes" );
  gains.t17_2pc_assassination   = get_gain( "t17_2pc_assassination" );
  gains.t17_4pc_assassination   = get_gain( "t17_4pc_assassination" );
  gains.t17_2pc_subtlety        = get_gain( "t17_2pc_subtlety" );
  gains.t17_4pc_subtlety        = get_gain( "t17_4pc_subtlety" );
  gains.venomous_wounds         = get_gain( "venomous_vim"       );
}

// rogue_t::init_procs ======================================================

void rogue_t::init_procs()
{
  player_t::init_procs();

  procs.anticipation_wasted      = get_proc( "Anticipation Charges (wasted during replenish)" );
  procs.seal_fate                = get_proc( "Seal Fate"           );
  procs.t16_2pc_melee            = get_proc( "Silent Blades (T16 2PC)" );
  procs.t18_2pc_combat           = get_proc( "Adrenaline Rush (T18 2PC)" );

  if ( talent.death_from_above -> ok() )
  {
    dfa_mh = get_sample_data( "dfa_mh" );
    dfa_oh = get_sample_data( "dfa_oh" );
  }
}

// rogue_t::init_scaling ====================================================

void rogue_t::init_scaling()
{
  player_t::init_scaling();

  scales_with[ STAT_WEAPON_OFFHAND_DPS    ] = items[ SLOT_OFF_HAND ].active();
  scales_with[ STAT_STRENGTH              ] = false;

  // Break out early if scaling is disabled on this player, or there's no
  // scaling stat
  if ( ! scale_player || sim -> scaling -> scale_stat == STAT_NONE )
  {
    return;
  }

  // If weapon swapping is used, adjust the weapon_t object damage values in the weapon state
  // information if this simulator is scaling the corresponding weapon DPS (main or offhand). This
  // is necessary, as weapon swapping overwrites player_t::main_hand_weapon and
  // player_t::ofF_hand_weapon, which is where player_t::init_scaling originally injects the
  // increased scaling value.
  if ( sim -> scaling -> scale_stat == STAT_WEAPON_DPS &&
       weapon_data[ WEAPON_MAIN_HAND ].secondary_weapon_data.active() )
  {
    double v = sim -> scaling -> scale_value;
    double pvalue = weapon_data[ WEAPON_MAIN_HAND ].weapon_data[ WEAPON_PRIMARY ].swing_time.total_seconds() * v;
    double svalue = weapon_data[ WEAPON_MAIN_HAND ].weapon_data[ WEAPON_SECONDARY ].swing_time.total_seconds() * v;

    weapon_data[ WEAPON_MAIN_HAND ].weapon_data[ WEAPON_PRIMARY ].damage += pvalue;
    weapon_data[ WEAPON_MAIN_HAND ].weapon_data[ WEAPON_PRIMARY ].min_dmg += pvalue;
    weapon_data[ WEAPON_MAIN_HAND ].weapon_data[ WEAPON_PRIMARY ].max_dmg += pvalue;

    weapon_data[ WEAPON_MAIN_HAND ].weapon_data[ WEAPON_SECONDARY ].damage += svalue;
    weapon_data[ WEAPON_MAIN_HAND ].weapon_data[ WEAPON_SECONDARY ].min_dmg += svalue;
    weapon_data[ WEAPON_MAIN_HAND ].weapon_data[ WEAPON_SECONDARY ].max_dmg += svalue;
  }

  if ( sim -> scaling -> scale_stat == STAT_WEAPON_OFFHAND_DPS &&
       weapon_data[ WEAPON_OFF_HAND ].secondary_weapon_data.active() )
  {
    double v = sim -> scaling -> scale_value;
    double pvalue = weapon_data[ WEAPON_OFF_HAND ].weapon_data[ WEAPON_PRIMARY ].swing_time.total_seconds() * v;
    double svalue = weapon_data[ WEAPON_OFF_HAND ].weapon_data[ WEAPON_SECONDARY ].swing_time.total_seconds() * v;

    weapon_data[ WEAPON_OFF_HAND ].weapon_data[ WEAPON_PRIMARY ].damage += pvalue;
    weapon_data[ WEAPON_OFF_HAND ].weapon_data[ WEAPON_PRIMARY ].min_dmg += pvalue;
    weapon_data[ WEAPON_OFF_HAND ].weapon_data[ WEAPON_PRIMARY ].max_dmg += pvalue;

    weapon_data[ WEAPON_OFF_HAND ].weapon_data[ WEAPON_SECONDARY ].damage += svalue;
    weapon_data[ WEAPON_OFF_HAND ].weapon_data[ WEAPON_SECONDARY ].min_dmg += svalue;
    weapon_data[ WEAPON_OFF_HAND ].weapon_data[ WEAPON_SECONDARY ].max_dmg += svalue;
  }
}

// rogue_t::init_resources =================================================

void rogue_t::init_resources( bool force )
{
  player_t::init_resources( force );

  resources.current[ RESOURCE_COMBO_POINT ] = 0;
}

// rogue_t::init_buffs ======================================================

void rogue_t::create_buffs()
{
  // Handle the Legendary here, as it's called after init_items()
  if ( find_item( "vengeance" ) && find_item( "fear" ) )
  {
    fof_p1 = 1;
  }
  else if ( find_item( "the_sleeper" ) && find_item( "the_dreamer" ) )
  {
    fof_p2 = 1;
  }
  else if ( find_item( "golad_twilight_of_aspects" ) && find_item( "tiriosh_nightmare_of_ages" ) )
  {
    fof_p3 = 1;
  }

  player_t::create_buffs();

  // buff_t( player, name, max_stack, duration, chance=-1, cd=-1, quiet=false, reverse=false, activated=true )
  // buff_t( player, id, name, chance=-1, cd=-1, quiet=false, reverse=false, activated=true )
  // buff_t( player, name, spellname, chance=-1, cd=-1, quiet=false, reverse=false, activated=true )

  buffs.blade_flurry        = buff_creator_t( this, "blade_flurry", find_spell( 57142 ) );
  buffs.adrenaline_rush     = buff_creator_t( this, "adrenaline_rush", find_class_spell( "Adrenaline Rush" ) )
                              .cd( timespan_t::zero() )
                              .duration( find_class_spell( "Adrenaline Rush" ) -> duration() + sets.set( SET_MELEE, T13, B4 ) -> effectN( 2 ).time_value() )
                              .default_value( find_class_spell( "Adrenaline Rush" ) -> effectN( 2 ).percent() )
                              .affects_regen( true )
                              .add_invalidate( CACHE_ATTACK_SPEED )
                              .add_invalidate( sets.has_set_bonus( ROGUE_OUTLAW, T18, B4 ) ? CACHE_PLAYER_DAMAGE_MULTIPLIER : CACHE_NONE );
  buffs.free_pistol_shot    = buff_creator_t( this, "free_pistol_shot" )
                              .chance( spec.saber_slash -> proc_chance() );
  buffs.feint               = buff_creator_t( this, "feint", find_class_spell( "Feint" ) )
    .duration( find_class_spell( "Feint" ) -> duration() );
  buffs.master_of_subtlety_passive = buff_creator_t( this, "master_of_subtlety_passive", talent.master_of_subtlety )
                                     .duration( sim -> max_time / 2 )
                                     .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  buffs.master_of_subtlety  = buff_creator_t( this, "master_of_subtlety", find_spell( 31666 ) )
                              .default_value( talent.master_of_subtlety -> effectN( 1 ).percent() )
                              .chance( talent.master_of_subtlety -> ok() )
                              .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  // Killing spree buff has only 2 sec duration, main spell has 3, check.
  buffs.killing_spree       = buff_creator_t( this, "killing_spree", talent.killing_spree )
                              .duration( talent.killing_spree -> duration() + timespan_t::from_seconds( 0.001 ) );
  buffs.shadow_dance       = new buffs::shadow_dance_t( this );
  buffs.sleight_of_hand    = buff_creator_t( this, "sleight_of_hand", find_spell( 145211 ) )
                             .chance( sets.set( SET_MELEE, T16, B4 ) -> effectN( 3 ).percent() );
  //buffs.stealth            = buff_creator_t( this, "stealth" ).add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  buffs.stealth            = new buffs::stealth_t( this );
  buffs.vanish             = new buffs::vanish_t( this );
  buffs.subterfuge         = new buffs::subterfuge_t( this );
  buffs.t16_2pc_melee      = buff_creator_t( this, "silent_blades", find_spell( 145193 ) )
                             .chance( sets.has_set_bonus( SET_MELEE, T16, B2 ) );
  buffs.tier13_2pc         = buff_creator_t( this, "tier13_2pc", spell.tier13_2pc )
                             .chance( sets.has_set_bonus( SET_MELEE, T13, B2 ) ? 1.0 : 0 );
  buffs.toxicologist       = stat_buff_creator_t( this, "toxicologist", find_spell( 145249 ) )
                             .chance( sets.has_set_bonus( SET_MELEE, T16, B4 ) );

  buffs.envenom            = buff_creator_t( this, "envenom", find_specialization_spell( "Envenom" ) )
                             .duration( timespan_t::min() )
                             .period( timespan_t::zero() )
                             .refresh_behavior( BUFF_REFRESH_PANDEMIC );

  buff_creator_t snd_creator = buff_creator_t( this, "slice_and_dice", find_class_spell( "Slice and Dice" ) )
                               .tick_behavior( BUFF_TICK_NONE )
                               .period( timespan_t::zero() )
                               .refresh_behavior( BUFF_REFRESH_PANDEMIC )
                               .add_invalidate( CACHE_ATTACK_SPEED );

  if ( spec.energetic_recovery -> ok() )
  {
    snd_creator.period( find_class_spell( "Slice and Dice" ) -> effectN( 2 ).period() );
    snd_creator.tick_behavior( BUFF_TICK_REFRESH );
    snd_creator.tick_callback( [ this ]( buff_t*, int, const timespan_t& ) {
       resource_gain( RESOURCE_ENERGY,
                      spec.energetic_recovery -> effectN( 1 ).base_value(),
                      gains.energetic_recovery );
    } );
  }
  // Presume that combat re-uses the ticker for the T18 2pc set bonus
  else if ( sets.has_set_bonus( ROGUE_OUTLAW, T18, B2 ) )
  {
    snd_creator.period( find_class_spell( "Slice and Dice" ) -> effectN( 2 ).period() );
    snd_creator.tick_behavior( BUFF_TICK_REFRESH );
    snd_creator.tick_callback( [ this ]( buff_t*, int, const timespan_t& ) {
      if ( ! rng().roll( sets.set( ROGUE_OUTLAW, T18, B2 ) -> proc_chance() ) )
        return;

      if ( buffs.adrenaline_rush -> check() )
      {
        buffs.adrenaline_rush -> extend_duration( this, spell.tier18_2pc_combat_ar -> duration() );
      }
      else
      {
        if ( buffs.adrenaline_rush -> trigger( 1, buff_t::DEFAULT_VALUE(), -1.0,
             spell.tier18_2pc_combat_ar -> duration() ) )
        {
          procs.t18_2pc_combat -> occur();
        }
      }
    } );
  }

  buffs.slice_and_dice = snd_creator;

  // Legendary buffs
  buffs.fof_p1            = stat_buff_creator_t( this, "suffering", find_spell( 109959 ) )
                            .add_stat( STAT_AGILITY, find_spell( 109959 ) -> effectN( 1 ).base_value() )
                            .chance( fof_p1 );
  buffs.fof_p2            = stat_buff_creator_t( this, "nightmare", find_spell( 109955 ) )
                            .add_stat( STAT_AGILITY, find_spell( 109955 ) -> effectN( 1 ).base_value() )
                            .chance( fof_p2 );
  buffs.fof_p3            = stat_buff_creator_t( this, "shadows_of_the_destroyer", find_spell( 109939 ) -> effectN( 1 ).trigger() )
                            .add_stat( STAT_AGILITY, find_spell( 109939 ) -> effectN( 1 ).trigger() -> effectN( 1 ).base_value() )
                            .chance( fof_p3 );

  buffs.fof_fod           = new buffs::fof_fod_t( this );

  buffs.death_from_above  = buff_creator_t( this, "death_from_above", spell.death_from_above )
                            .quiet( true );

  buffs.anticipation      = buff_creator_t( this, "anticipation", find_spell( 115189 ) )
                            .chance( talent.anticipation -> ok() );
  buffs.deceit            = buff_creator_t( this, "deceit", sets.set( ROGUE_OUTLAW, T17, B4 ) -> effectN( 1 ).trigger() )
                            .chance( sets.has_set_bonus( ROGUE_OUTLAW, T17, B4 ) );
  buffs.shadow_strikes    = buff_creator_t( this, "shadow_strikes", find_spell( 170107 ) )
                            .chance( sets.has_set_bonus( ROGUE_SUBTLETY, T17, B4 ) );

  buffs.sprint            = buff_creator_t( this, "sprint", spell.sprint )
    .cd( timespan_t::zero() );
  buffs.shadowstep        = buff_creator_t( this, "shadowstep", spec.shadowstep )
    .cd( timespan_t::zero() );
  buffs.deathly_shadows = buff_creator_t( this, "deathly_shadows", find_spell( 188700 ) )
                         .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
                         .chance( sets.has_set_bonus( ROGUE_SUBTLETY, T18, B2 ) );
}

void rogue_t::register_callbacks()
{
  player_t::register_callbacks();
}

// rogue_t::create_options ==================================================

static bool do_parse_secondary_weapon( rogue_t* rogue,
                                       const std::string& value,
                                       slot_e slot )
{
  switch ( slot )
  {
    case SLOT_MAIN_HAND:
      rogue -> weapon_data[ WEAPON_MAIN_HAND ].secondary_weapon_data = item_t( rogue, value );
      rogue -> weapon_data[ WEAPON_MAIN_HAND ].secondary_weapon_data.slot = slot;
      break;
    case SLOT_OFF_HAND:
      rogue -> weapon_data[ WEAPON_OFF_HAND ].secondary_weapon_data = item_t( rogue, value );
      rogue -> weapon_data[ WEAPON_OFF_HAND ].secondary_weapon_data.slot = slot;
      break;
    default:
      break;
  }

  return true;
}

static bool parse_offhand_secondary( sim_t* sim,
                                     const std::string& /* name */,
                                     const std::string& value )
{
  rogue_t* rogue = static_cast<rogue_t*>( sim -> active_player );
  return do_parse_secondary_weapon( rogue, value, SLOT_OFF_HAND );
}

static bool parse_mainhand_secondary( sim_t* sim,
                                      const std::string& /* name */,
                                      const std::string& value )
{
  rogue_t* rogue = static_cast<rogue_t*>( sim -> active_player );
  return do_parse_secondary_weapon( rogue, value, SLOT_MAIN_HAND );
}

void rogue_t::create_options()
{
  add_option( opt_func( "off_hand_secondary", parse_offhand_secondary ) );
  add_option( opt_func( "main_hand_secondary", parse_mainhand_secondary ) );
  add_option( opt_int( "initial_combo_points", initial_combo_points ) );

  player_t::create_options();
}

// rogue_t::copy_from =======================================================

void rogue_t::copy_from( player_t* source )
{
  rogue_t* rogue = static_cast<rogue_t*>( source );
  player_t::copy_from( source );
  if ( ! rogue -> weapon_data[ WEAPON_MAIN_HAND ].secondary_weapon_data.options_str.empty() )
  {
    weapon_data[ WEAPON_MAIN_HAND ].secondary_weapon_data.options_str = \
      rogue -> weapon_data[ WEAPON_MAIN_HAND ].secondary_weapon_data.options_str;
  }

  if ( ! rogue -> weapon_data[ WEAPON_OFF_HAND ].secondary_weapon_data.options_str.empty() )
  {
    weapon_data[ WEAPON_OFF_HAND ].secondary_weapon_data.options_str = \
      rogue -> weapon_data[ WEAPON_OFF_HAND ].secondary_weapon_data.options_str;
  }
}

// rogue_t::create_profile  =================================================

std::string rogue_t::create_profile( save_e stype )
{
  std::string profile_str = player_t::create_profile( stype );

  // Break out early if we are not saving everything, or gear
  if ( stype != SAVE_ALL && stype != SAVE_GEAR )
  {
    return profile_str;
  }

  std::string term = "\n";

  if ( weapon_data[ WEAPON_MAIN_HAND ].secondary_weapon_data.active() ||
       weapon_data[ WEAPON_OFF_HAND ].secondary_weapon_data.active() )
  {
    profile_str += term;
    profile_str += "# Secondary weapons used in conjunction with weapon swapping are defined below.";
    profile_str += term;
  }

  if ( weapon_data[ WEAPON_MAIN_HAND ].secondary_weapon_data.active() )
  {
    profile_str += "main_hand_secondary=";
    profile_str += weapon_data[ WEAPON_MAIN_HAND ].secondary_weapon_data.encoded_item() + term;
    if ( sim -> save_gear_comments &&
         ! weapon_data[ WEAPON_MAIN_HAND ].secondary_weapon_data.encoded_comment().empty() )
    {
      profile_str += "# ";
      profile_str += weapon_data[ WEAPON_MAIN_HAND ].secondary_weapon_data.encoded_comment();
      profile_str += term;
    }
  }

  if ( weapon_data[ WEAPON_OFF_HAND ].secondary_weapon_data.active() )
  {
    profile_str += "off_hand_secondary=";
    profile_str += weapon_data[ WEAPON_OFF_HAND ].secondary_weapon_data.encoded_item() + term;
    if ( sim -> save_gear_comments &&
         ! weapon_data[ WEAPON_OFF_HAND ].secondary_weapon_data.encoded_comment().empty() )
    {
      profile_str += "# ";
      profile_str += weapon_data[ WEAPON_OFF_HAND ].secondary_weapon_data.encoded_comment();
      profile_str += term;
    }
  }

  return profile_str;
}

// rogue_t::init_items ======================================================

bool rogue_t::init_items()
{
  bool ret = player_t::init_items();
  if ( ! ret )
  {
    return ret;
  }

  // Initialize weapon swapping data structures for primary weapons here
  weapon_data[ WEAPON_MAIN_HAND ].weapon_data[ WEAPON_PRIMARY ] = main_hand_weapon;
  weapon_data[ WEAPON_MAIN_HAND ].item_data[ WEAPON_PRIMARY ] = &( items[ SLOT_MAIN_HAND ] );
  weapon_data[ WEAPON_OFF_HAND ].weapon_data[ WEAPON_PRIMARY ] = off_hand_weapon;
  weapon_data[ WEAPON_OFF_HAND ].item_data[ WEAPON_PRIMARY ] = &( items[ SLOT_OFF_HAND ] );

  if ( ! weapon_data[ WEAPON_MAIN_HAND ].secondary_weapon_data.options_str.empty() )
  {
    ret = weapon_data[ WEAPON_MAIN_HAND ].secondary_weapon_data.init();
    if ( ! ret )
    {
      return false;
    }
    weapon_data[ WEAPON_MAIN_HAND ].weapon_data[ WEAPON_SECONDARY ] = main_hand_weapon;
    weapon_data[ WEAPON_MAIN_HAND ].item_data[ WEAPON_SECONDARY ] = &( weapon_data[ WEAPON_MAIN_HAND ].secondary_weapon_data );

    // Restore primary main hand weapon after secondary weapon init
    main_hand_weapon = weapon_data[ WEAPON_MAIN_HAND ].weapon_data[ WEAPON_PRIMARY ];
  }

  if ( ! weapon_data[ WEAPON_OFF_HAND ].secondary_weapon_data.options_str.empty() )
  {
    ret = weapon_data[ WEAPON_OFF_HAND ].secondary_weapon_data.init();
    if ( ! ret )
    {
      return false;
    }
    weapon_data[ WEAPON_OFF_HAND ].weapon_data[ WEAPON_SECONDARY ] = off_hand_weapon;
    weapon_data[ WEAPON_OFF_HAND ].item_data[ WEAPON_SECONDARY ] = &( weapon_data[ WEAPON_OFF_HAND ].secondary_weapon_data );

    // Restore primary off hand weapon after secondary weapon init
    main_hand_weapon = weapon_data[ WEAPON_OFF_HAND ].weapon_data[ WEAPON_PRIMARY ];
  }

  return ret;
}

// rogue_t::init_special_effects ============================================

void rogue_t::init_special_effects()
{
  player_t::init_special_effects();

  if ( weapon_data[ WEAPON_MAIN_HAND ].item_data[ WEAPON_SECONDARY ] )
  {
    for ( size_t i = 0, end = weapon_data[ WEAPON_MAIN_HAND ].item_data[ WEAPON_SECONDARY ] -> parsed.special_effects.size();
          i < end; ++i )
    {
      special_effect_t* effect = weapon_data[ WEAPON_MAIN_HAND ].item_data[ WEAPON_SECONDARY ] -> parsed.special_effects[ i ];
      unique_gear::initialize_special_effect_2( effect );
    }
  }

  if ( weapon_data[ WEAPON_OFF_HAND ].item_data[ WEAPON_SECONDARY ] )
  {
    for ( size_t i = 0, end = weapon_data[ WEAPON_OFF_HAND ].item_data[ WEAPON_SECONDARY ] -> parsed.special_effects.size();
          i < end; ++i )
    {
      special_effect_t* effect = weapon_data[ WEAPON_OFF_HAND ].item_data[ WEAPON_SECONDARY ] -> parsed.special_effects[ i ];
      unique_gear::initialize_special_effect_2( effect );
    }
  }
}

// rogue_t::init_finished ===================================================

bool rogue_t::init_finished()
{
  weapon_data[ WEAPON_MAIN_HAND ].initialize();
  weapon_data[ WEAPON_OFF_HAND ].initialize();

  return player_t::init_finished();
}

// rogue_t::reset ===========================================================

void rogue_t::reset()
{
  player_t::reset();

  poisoned_enemies = 0;

  event_t::cancel( event_premeditation );

  weapon_data[ WEAPON_MAIN_HAND ].reset();
  weapon_data[ WEAPON_OFF_HAND ].reset();
}

void rogue_t::swap_weapon( weapon_slot_e slot, current_weapon_e to_weapon, bool in_combat )
{
  if ( weapon_data[ slot ].current_weapon == to_weapon )
  {
    return;
  }

  if ( ! weapon_data[ slot ].item_data[ to_weapon ] )
  {
    return;
  }

  if ( sim -> debug )
  {
    sim -> out_debug.printf( "%s performing weapon swap from %s to %s",
        name(), weapon_data[ slot ].item_data[ ! to_weapon ] -> name(),
        weapon_data[ slot ].item_data[ to_weapon ] -> name() );
  }

  // First, swap stats on actor, but only if it is in combat. Outside of combat (basically
  // during iteration reset) there is no need to adjust actor stats, as they are always reset to
  // the primary weapons.
  for ( stat_e i = STAT_NONE; in_combat && i < STAT_MAX; i++ )
  {
    stat_loss( i, weapon_data[ slot ].stats_data[ ! to_weapon ].get_stat( i ) );
    stat_gain( i, weapon_data[ slot ].stats_data[ to_weapon ].get_stat( i ) );
  }

  weapon_t* target_weapon = &( slot == WEAPON_MAIN_HAND ? main_hand_weapon : off_hand_weapon );
  action_t* target_action = slot == WEAPON_MAIN_HAND ? main_hand_attack : off_hand_attack;

  // Swap the actor weapon object
  *target_weapon = weapon_data[ slot ].weapon_data[ to_weapon ];
  target_action -> base_execute_time = target_weapon -> swing_time;

  // Enable new weapon callback(s)
  weapon_data[ slot ].callback_state( to_weapon, true );

  // Disable old weapon callback(s)
  weapon_data[ slot ].callback_state( static_cast<current_weapon_e>( ! to_weapon ), false );

  // Reset swing timer of the weapon
  if ( target_action -> execute_event )
  {
    event_t::cancel( target_action -> execute_event );
    target_action -> schedule_execute();
  }

  // Track uptime
  if ( to_weapon == WEAPON_PRIMARY )
  {
    weapon_data[ slot ].secondary_weapon_uptime -> expire();
  }
  else
  {
    weapon_data[ slot ].secondary_weapon_uptime -> trigger();
  }

  // Set the current weapon wielding state for the slot
  weapon_data[ slot ].current_weapon = to_weapon;
}

// rogue_t::arise ===========================================================

void rogue_t::arise()
{
  player_t::arise();

  resources.current[ RESOURCE_COMBO_POINT ] = 0;
}

// rogue_t::energy_regen_per_second =========================================

double rogue_t::energy_regen_per_second() const
{
  double r = player_t::energy_regen_per_second();

  if ( buffs.blade_flurry -> check() )
    r *= 1.0 + spec.blade_flurry -> effectN( 1 ).percent();

  return r;
}

// rogue_t::temporary_movement_modifier ==================================

double rogue_t::temporary_movement_modifier() const
{
  double temporary = player_t::temporary_movement_modifier();

  if ( buffs.sprint -> up() )
    temporary = std::max( buffs.sprint -> data().effectN( 1 ).percent(), temporary );

  if ( buffs.shadowstep -> up() )
    temporary = std::max( buffs.shadowstep -> data().effectN( 2 ).percent(), temporary );

  return temporary;
}

// rogue_t::passive_movement_modifier===================================

double rogue_t::passive_movement_modifier() const
{
  double ms = player_t::passive_movement_modifier();

  ms += spell.fleet_footed -> effectN( 1 ).percent();

  if ( buffs.stealth -> up() ) // Check if nightstalker is temporary or passive.
    ms += talent.nightstalker -> effectN( 1 ).percent();

  return ms;
}

void rogue_t::combat_begin()
{

  player_t::combat_begin();

  if ( initial_combo_points > 0 )
    resources.current[RESOURCE_COMBO_POINT] = initial_combo_points; // User specified Combo Points.
}

// rogue_t::regen ===========================================================

void rogue_t::regen( timespan_t periodicity )
{
  player_t::regen( periodicity );

  if ( buffs.adrenaline_rush -> up() )
  {
    if ( ! resources.is_infinite( RESOURCE_ENERGY ) )
    {
      double energy_regen = periodicity.total_seconds() * energy_regen_per_second() * buffs.adrenaline_rush -> data().effectN( 1 ).percent();

      resource_gain( RESOURCE_ENERGY, energy_regen, gains.adrenaline_rush );
    }
  }
}

// rogue_t::available =======================================================

timespan_t rogue_t::available() const
{
  if ( ready_type != READY_POLL )
  {
    return player_t::available();
  }
  else
  {
    double energy = resources.current[ RESOURCE_ENERGY ];

    if ( energy > 25 )
      return timespan_t::from_seconds( 0.1 );

    return std::max(
             timespan_t::from_seconds( ( 25 - energy ) / energy_regen_per_second() ),
             timespan_t::from_seconds( 0.1 )
           );
  }
}

// rogue_t::convert_hybrid_stat ==============================================

stat_e rogue_t::convert_hybrid_stat( stat_e s ) const
{
  // this converts hybrid stats that either morph based on spec or only work
  // for certain specs into the appropriate "basic" stats
  switch ( s )
  {
  case STAT_STR_AGI_INT:
  case STAT_AGI_INT: 
  case STAT_STR_AGI:
    return STAT_AGILITY; 
  // This is a guess at how STR/INT gear will work for Rogues, TODO: confirm  
  // This should probably never come up since rogues can't equip plate, but....
  case STAT_STR_INT:
    return STAT_NONE;
  case STAT_SPIRIT:
      return STAT_NONE;
  case STAT_BONUS_ARMOR:
      return STAT_NONE;     
  default: return s; 
  }
}

/* Report Extension Class
 * Here you can define class specific report extensions/overrides
 */
class rogue_report_t : public player_report_extension_t
{
public:
  rogue_report_t( rogue_t& player ) :
      p( player )
  {

  }

  virtual void html_customsection( report::sc_html_stream& os ) override
  {
    os << "<div class=\"player-section custom_section\">\n";
    if ( p.talent.death_from_above -> ok() )
    {
      os << "<h3 class=\"toggle open\">Death from Above swing time loss</h3>\n"
         << "<div class=\"toggle-content\">\n";

      os << "<p>";
      os <<
        "Death from Above causes out of range time for the Rogue while the"
        " animation is performing. This out of range time translates to a"
        " potential loss of auto-attack swing time. The following table"
        " represents the total auto-attack swing time loss, when performing Death"
        " from Above during the length of the combat. It is computed as the"
        " interval between the out of range delay (an average of 1.3 seconds in"
        " simc), and the next time the hand swings after the out of range delay"
        " elapsed.";
      os << "</p>";
      os << "<table class=\"sc\" style=\"float: left;margin-right: 10px;\">\n";

      os << "<tr><th></th><th colspan=\"3\">Lost time per iteration (sec)</th></tr>";
      os << "<tr><th>Weapon hand</th><th>Minimum</th><th>Average</th><th>Maximum</th></tr>";

      os << "<tr>";
      os << "<td class=\"left\">Main hand</td>";
      os.format("<td class=\"right\">%.3f</td>", p.dfa_mh -> min() );
      os.format("<td class=\"right\">%.3f</td>", p.dfa_mh -> mean() );
      os.format("<td class=\"right\">%.3f</td>", p.dfa_mh -> max() );
      os << "</tr>";

      os << "<tr>";
      os << "<td class=\"left\">Off hand</td>";
      os.format("<td class=\"right\">%.3f</td>", p.dfa_oh -> min() );
      os.format("<td class=\"right\">%.3f</td>", p.dfa_oh -> mean() );
      os.format("<td class=\"right\">%.3f</td>", p.dfa_oh -> max() );
      os << "</tr>";

      os << "</table>";

      os << "</div>\n";

      os << "<div class=\"clear\"></div>\n";
    }
    os << "</div>\n";
  }
private:
  rogue_t& p;
};

// ROGUE MODULE INTERFACE ===================================================

static void do_trinket_init( rogue_t*                 r,
                             specialization_e         spec,
                             const special_effect_t*& ptr,
                             const special_effect_t&  effect )
{
  if ( ! r -> find_spell( effect.spell_id ) -> ok() || r -> specialization() != spec )
  {
    return;
  }

  ptr = &( effect );
}

static void toxic_mutilator( special_effect_t& effect )
{
  rogue_t* rogue = debug_cast<rogue_t*>( effect.player );
  do_trinket_init( rogue, ROGUE_ASSASSINATION, rogue -> toxic_mutilator, effect );
}

static void eviscerating_blade( special_effect_t& effect )
{
  rogue_t* rogue = debug_cast<rogue_t*>( effect.player );
  do_trinket_init( rogue, ROGUE_OUTLAW, rogue -> eviscerating_blade, effect );
}

static void from_the_shadows( special_effect_t& effect )
{
  rogue_t* rogue = debug_cast<rogue_t*>( effect.player );
  do_trinket_init( rogue, ROGUE_SUBTLETY, rogue -> from_the_shadows, effect );
}

struct rogue_module_t : public module_t
{
  rogue_module_t() : module_t( ROGUE ) {}

  virtual player_t* create_player( sim_t* sim, const std::string& name, race_e r = RACE_NONE ) const override
  {
    auto  p = new rogue_t( sim, name, r );
    p -> report_extension = std::unique_ptr<player_report_extension_t>( new rogue_report_t( *p ) );
    return p;
  }

  virtual bool valid() const override
  { return true; }

  virtual void static_init() const override
  {
    unique_gear::register_special_effect( 184916, toxic_mutilator    );
    unique_gear::register_special_effect( 184917, eviscerating_blade );
    unique_gear::register_special_effect( 184918, from_the_shadows   );
  }

  virtual void register_hotfixes() const override
  {
  }

  virtual void init( player_t* ) const override { }
  virtual void combat_begin( sim_t* ) const override {}
  virtual void combat_end( sim_t* ) const override {}
};

} // UNNAMED NAMESPACE

const module_t* module_t::rogue()
{
  static rogue_module_t m;
  return &m;
}
