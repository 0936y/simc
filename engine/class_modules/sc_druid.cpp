// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

namespace { // UNNAMED NAMESPACE
// ==========================================================================
// Druid
// ==========================================================================

 /* Legion TODO:

 Astral Influence
 Affinity active components

 Feral ================================
 Predator vs. adds

 Balance ==============================
 Stellar Drift cast while moving
 Starfall positioning
 Utility & NYI artifact perks (see p() -> artifact).
 Celestial alignment damage modifier on correct spells
 Accurate starfall travel time & debuff mechanics ?
 Force of Nature!

 Guardian =============================
 Statistics?
 Incarnation CD modifier rework

 Resto ================================
 All the things

 Needs Documenting ====================
 predator_rppm option
 initial_astral_power option
 initial_moon_stage option
 active_starfalls expression
 moon stage expressions
*/

// Forward declarations
struct druid_t;

// Active actions
struct brambles_t;
struct stalwart_guardian_t;
namespace spells {
  struct moonfire_t;
  struct starshards_t;
}
namespace heals {
  struct cenarion_ward_hot_t;
  struct yseras_tick_t;
}
namespace cat_attacks {
  struct gushing_wound_t;
}

enum form_e {
  CAT_FORM       = 0x1,
  NO_FORM        = 0x2,
  TRAVEL_FORM    = 0x4,
  AQUATIC_FORM   = 0x8, // Legacy
  BEAR_FORM      = 0x10,
  DIRE_BEAR_FORM = 0x40, // Legacy
  MOONKIN_FORM   = 0x40000000,
};

enum moon_stage_e {
  NEW_MOON,
  HALF_MOON,
  FULL_MOON,
};

struct druid_td_t : public actor_target_data_t
{
  struct dots_t
  {
    dot_t* collapsing_stars;
    dot_t* gushing_wound;
    dot_t* lacerate;
    dot_t* lifebloom;
    dot_t* moonfire;
    dot_t* rake;
    dot_t* regrowth;
    dot_t* rejuvenation;
    dot_t* rip;
    dot_t* stellar_flare;
    dot_t* sunfire;
    dot_t* starfall;
    dot_t* thrash_cat;
    dot_t* wild_growth;
  } dots;

  struct buffs_t
  {
    buff_t* lifebloom;
    buff_t* bloodletting;
    buff_t* starfall;
  } buffs;

  int lacerate_stack;

  druid_td_t( player_t& target, druid_t& source );

  bool hot_ticking()
  {
    return dots.regrowth      -> is_ticking() ||
           dots.rejuvenation  -> is_ticking() ||
           dots.lifebloom     -> is_ticking() ||
           dots.wild_growth   -> is_ticking();
  }

  void reset()
  {
    lacerate_stack = 0;
  }
};

struct snapshot_counter_t
{
  const sim_t* sim;
  druid_t* p;
  std::vector<buff_t*> b;
  double exe_up;
  double exe_down;
  double tick_up;
  double tick_down;
  bool is_snapped;
  double wasted_buffs;

  snapshot_counter_t( druid_t* player , buff_t* buff );

  bool check_all()
  {
    double n_up = 0;
    for (auto & elem : b)
    {
      if ( elem -> check() )
        n_up++;
    }
    if ( n_up == 0 )
      return false;

    wasted_buffs += n_up - 1;
    return true;
  }

  void add_buff( buff_t* buff )
  {
    b.push_back( buff );
  }

  void count_execute()
  {
    // Skip iteration 0 for non-debug, non-log sims
    if ( sim -> current_iteration == 0 && sim -> iterations > sim -> threads && ! sim -> debug && ! sim -> log )
      return;

    check_all() ? ( exe_up++ , is_snapped = true ) : ( exe_down++ , is_snapped = false );
  }

  void count_tick()
  {
    // Skip iteration 0 for non-debug, non-log sims
    if ( sim -> current_iteration == 0 && sim -> iterations > sim -> threads && ! sim -> debug && ! sim -> log )
      return;

    is_snapped ? tick_up++ : tick_down++;
  }

  double divisor() const
  {
    if ( ! sim -> debug && ! sim -> log && sim -> iterations > sim -> threads )
      return sim -> iterations - sim -> threads;
    else
      return std::min( sim -> iterations, sim -> threads );
  }

  double mean_exe_up() const
  { return exe_up / divisor(); }

  double mean_exe_down() const
  { return exe_down / divisor(); }

  double mean_tick_up() const
  { return tick_up / divisor(); }

  double mean_tick_down() const
  { return tick_down / divisor(); }

  double mean_exe_total() const
  { return ( exe_up + exe_down ) / divisor(); }

  double mean_tick_total() const
  { return ( tick_up + tick_down ) / divisor(); }

  double mean_waste() const
  { return wasted_buffs / divisor(); }

  void merge( const snapshot_counter_t& other )
  {
    exe_up += other.exe_up;
    exe_down += other.exe_down;
    tick_up += other.tick_up;
    tick_down += other.tick_down;
    wasted_buffs += other.wasted_buffs;
  }
};

struct druid_t : public player_t
{
private:
  form_e form; // Active druid form
public:
  int active_starfalls;
  double max_fb_energy;
  moon_stage_e moon_stage;

  // counters for snapshot tracking
  std::vector<snapshot_counter_t*> counters;

  // Active
  action_t* t16_2pc_starfall_bolt;
  action_t* t16_2pc_sun_bolt;

  // Artifacts
  const special_effect_t* scythe_of_elune;

  // RPPM objects
  real_ppm_t balance_tier18_2pc;
  real_ppm_t predator; // Optional RPPM approximation

  // Options
  double predator_rppm_rate;
  double initial_astral_power;
  int    initial_moon_stage;
  
  struct active_actions_t
  {
    brambles_t*                   brambles;
    stalwart_guardian_t*          stalwart_guardian;
    cat_attacks::gushing_wound_t* gushing_wound;
    heals::cenarion_ward_hot_t*   cenarion_ward_hot;
    heals::yseras_tick_t*         yseras_gift;
    spells::moonfire_t*           galactic_guardian;
    spells::starshards_t*         starshards;
  } active;

  // Pets
  std::array<pet_t*,11> pet_fey_moonwing; // 30 second duration, 3 second internal icd... create 11 to be safe.

  // Auto-attacks
  weapon_t caster_form_weapon;
  weapon_t cat_weapon;
  weapon_t bear_weapon;
  melee_attack_t* caster_melee_attack;
  melee_attack_t* cat_melee_attack;
  melee_attack_t* bear_melee_attack;

  double equipped_weapon_dps;

  // T18 (WoD 6.2) class specific trinket effects
  const special_effect_t* starshards;
  const special_effect_t* wildcat_celerity;
  const special_effect_t* stalwart_guardian;
  const special_effect_t* flourish;

  // Druid Events
  std::vector<event_t*> persistent_buff_delay;

  // Buffs
  struct buffs_t
  {
    // General
    buff_t* bear_form;
    buff_t* cat_form;
    buff_t* dash;
    buff_t* displacer_beast;
    buff_t* cenarion_ward;
    buff_t* dream_of_cenarius;
    buff_t* incarnation;
    buff_t* clearcasting;
    buff_t* prowl;
    buff_t* stampeding_roar;
    buff_t* wild_charge_movement;

    // Balance
    buff_t* blessing_of_anshe;
    buff_t* blessing_of_elune;
    buff_t* celestial_alignment;
    buff_t* collapsing_stars_up; // Tracking buff for APL
    buff_t* lunar_empowerment;
    buff_t* moonkin_form;
    buff_t* owlkin_frenzy;
    buff_t* solar_empowerment;
    buff_t* star_power; // Moon and Stars artifact medal
    buff_t* the_reaping;
    buff_t* warrior_of_elune;
    buff_t* balance_tier18_4pc; // T18 4P Balance

    // Feral
    buff_t* berserk;
    buff_t* bloodtalons;
    buff_t* elunes_guidance;
    buff_t* predatory_swiftness;
    buff_t* savage_roar;
    buff_t* tigers_fury;
    buff_t* feral_tier15_4pc;
    buff_t* feral_tier16_2pc;
    buff_t* feral_tier16_4pc;
    buff_t* feral_tier17_4pc;

    // Guardian
    buff_t* barkskin;
    buff_t* bladed_armor;
    buff_t* bristling_fur;
    buff_t* earthwarden;
    buff_t* earthwarden_driver;
    buff_t* mark_of_ursol;
    buff_t* pulverize;
    buff_t* survival_instincts;
    buff_t* guardian_tier15_2pc;
    buff_t* guardian_tier17_4pc;
    buff_t* ironfur; // proxy buff, manages stack count and mechanical effect
    buff_t* ironfur_stack[9]; // individual buffs, manages timers

    // Restoration
    buff_t* soul_of_the_forest; // needs checking
    buff_t* yseras_gift;
    buff_t* harmony; // NYI
  } buff;

  // Cooldowns
  struct cooldowns_t
  {
    cooldown_t* berserk;
    cooldown_t* celestial_alignment;
    cooldown_t* frenzied_regen_use;
    cooldown_t* growl;
    cooldown_t* mangle;
    cooldown_t* maul;
    cooldown_t* moon_cd; // New / Half / Full Moon
    cooldown_t* wod_pvp_4pc_melee;
    cooldown_t* swiftmend;
    cooldown_t* tigers_fury;
    cooldown_t* warrior_of_elune;
  } cooldown;

  // Gains
  struct gains_t
  {
    // Multiple Specs / Forms
    gain_t* clearcasting;       // Feral & Restoration
    gain_t* primal_fury;        // Feral & Guardian
    gain_t* soul_of_the_forest; // Feral & Guardian

    // Balance
    gain_t* astral_communion;
    gain_t* blessing_of_anshe;
    gain_t* blessing_of_elune;
    gain_t* celestial_alignment;
    gain_t* lunar_strike;
    gain_t* shooting_stars;
    gain_t* solar_wrath;

    // Feral (Cat)
    gain_t* bloody_slash;
    gain_t* energy_refund;
    gain_t* elunes_guidance;
    gain_t* moonfire;
    gain_t* rake;
    gain_t* shred;
    gain_t* swipe;
    gain_t* tigers_fury;
    gain_t* feral_tier15_2pc;
    gain_t* feral_tier16_4pc;
    gain_t* feral_tier17_2pc;
    gain_t* feral_tier18_4pc;

    // Guardian (Bear)
    gain_t* bear_form;
    gain_t* brambles;
    gain_t* bristling_fur;
    gain_t* stalwart_guardian;
    gain_t* rage_refund;
    gain_t* guardian_tier17_2pc;
    gain_t* guardian_tier18_2pc;
  } gain;

  // Glyphs
  struct glyphs_t
  {
    // TOCHECK: Things that may still exist in Legion.
    const spell_data_t* stampeding_roar;
    const spell_data_t* regrowth;
  } glyph;

  // Masteries
  struct masteries_t
  {
    // Done
    const spell_data_t* natures_guardian;
    const spell_data_t* natures_guardian_AP;
    const spell_data_t* razor_claws;
    const spell_data_t* starlight;

    // NYI / TODO!
    const spell_data_t* harmony;
  } mastery;

  // Procs
  struct procs_t
  {
    proc_t* clearcasting;
    proc_t* clearcasting_wasted;
    proc_t* predator;
    proc_t* predator_wasted;
    proc_t* primal_fury;
    proc_t* starshards;
    proc_t* tier15_2pc_melee;
    proc_t* tier17_2pc_melee;
  } proc;

  // Class Specializations
  struct specializations_t
  {
    // Generic
    const spell_data_t* critical_strikes;       // Feral & Guardian
    const spell_data_t* killer_instinct;        // Feral & Guardian
    const spell_data_t* nurturing_instinct;     // Balance & Restoration
    const spell_data_t* leather_specialization; // All Specializations
    const spell_data_t* mana_attunement;        // Feral & Guardian
    const spell_data_t* omen_of_clarity;        // Feral & Restoration
    const spell_data_t* dream_of_cenarius;      // All Specs (TOCHECK: does this still exist?)

    // Feral
    const spell_data_t* feral_passive; // Feral Overrides Passive
    const spell_data_t* sharpened_claws;
    const spell_data_t* predatory_swiftness;
    const spell_data_t* swipe;

    // Balance
    const spell_data_t* balance_passive; // Balance Overrides Passive
    const spell_data_t* celestial_alignment;
    const spell_data_t* moonkin_form;
    const spell_data_t* starfall;
    const spell_data_t* natural_insight;

    // Guardian
    const spell_data_t* bladed_armor;
    const spell_data_t* guardian_passive; // Guardian Overrides Passive
    const spell_data_t* ironfur;
    const spell_data_t* resolve;
  } spec;

  struct spells_t
  {
    // Cat
    const spell_data_t* feline_swiftness; // Feral Affinity passive
    const spell_data_t* ferocious_bite; 
    const spell_data_t* berserk_cat; // Berserk cat resource cost reducer
    const spell_data_t* cat_form; // Cat form hidden effects
    const spell_data_t* cat_form_speed;
    const spell_data_t* primal_fury; // Primal fury gain
    const spell_data_t* gushing_wound; // Feral t17 4pc driver

    // Bear
    const spell_data_t* bear_form_passive; // Bear form passive buff
    const spell_data_t* thick_hide; // Guardian Affinity passive

    // Moonkin
    const spell_data_t* astral_influence; // Balance Affinity passive
    const spell_data_t* blessing_of_anshe;
    const spell_data_t* blessing_of_elune;

    // Resto
    const spell_data_t* yseras_gift; // Restoration Affinity passive
  } spell;

  // Talents
  struct talents_t
  {
    // Multiple Specs
    const spell_data_t* renewal;
    const spell_data_t* displacer_beast;
    const spell_data_t* wild_charge;

    const spell_data_t* balance_affinity;
    const spell_data_t* feral_affinity;
    const spell_data_t* guardian_affinity;
    const spell_data_t* restoration_affinity;

    const spell_data_t* mighty_bash;
    const spell_data_t* mass_entanglement;
    const spell_data_t* typhoon;

    const spell_data_t* soul_of_the_forest;
    const spell_data_t* moment_of_clarity;
    
    // Feral
    const spell_data_t* predator;
    const spell_data_t* blood_scent;
    const spell_data_t* lunar_inspiration;

    const spell_data_t* incarnation_cat;
    const spell_data_t* bloody_slash;

    const spell_data_t* sabertooth;
    const spell_data_t* jagged_wounds;
    const spell_data_t* elunes_guidance;

    const spell_data_t* savage_roar;
    const spell_data_t* bloodtalons;
    
    // Balance
    const spell_data_t* shooting_stars;
    const spell_data_t* warrior_of_elune;
    const spell_data_t* starlord;

    const spell_data_t* incarnation_moonkin;
    const spell_data_t* stellar_flare;
    
    const spell_data_t* stellar_drift;
    const spell_data_t* full_moon;
    const spell_data_t* natures_balance;

    const spell_data_t* collapsing_stars;
    const spell_data_t* astral_communion;
    const spell_data_t* blessing_of_the_ancients;

    // Guardian
    const spell_data_t* brambles;
    const spell_data_t* pulverize;
    const spell_data_t* blood_frenzy;

    const spell_data_t* gutteral_roars;
    
    const spell_data_t* incarnation_bear;
    const spell_data_t* galactic_guardian;
    
    const spell_data_t* earthwarden;
    const spell_data_t* guardian_of_elune;
    const spell_data_t* survival_of_the_fittest;
    
    const spell_data_t* rend_and_tear;
    const spell_data_t* lunar_beam;
    const spell_data_t* bristling_fur;

    // Restoration
    const spell_data_t* verdant_growth;
    const spell_data_t* cenarion_ward;
    const spell_data_t* germination;

    const spell_data_t* incarnation_tree;
    const spell_data_t* cultivation;
    
    const spell_data_t* prosperity;
    const spell_data_t* inner_peace;
    const spell_data_t* profusion;
    
    const spell_data_t* stonebark;
    const spell_data_t* flourish;
  } talent;

  // Artifacts
  struct artifact_spell_data_t
  {
    // Balance -- Scythe of Elune
    artifact_power_t bladed_feathers;
    artifact_power_t dark_side_of_the_moon;
    artifact_power_t empowerment;
    artifact_power_t falling_star;
    artifact_power_t moon_and_stars;
    artifact_power_t new_moon;
    artifact_power_t power_of_goldrinn;
    artifact_power_t scythe_of_the_stars;
    artifact_power_t solar_stabbing;
    artifact_power_t sunfire_burns;
    artifact_power_t twilight_glow;

    // NYI
    artifact_power_t scion_of_the_night_sky;
    artifact_power_t touch_of_the_moon;
    artifact_power_t rejuvenating_innervation;
    artifact_power_t light_of_the_sun;
    artifact_power_t mooncraze; // will be replaced
  } artifact;

  druid_t( sim_t* sim, const std::string& name, race_e r = RACE_NIGHT_ELF ) :
    player_t( sim, DRUID, name, r ),
    form( NO_FORM ),
    active_starfalls( 0 ),
    max_fb_energy( 0 ),
    initial_astral_power( 0 ),
    initial_moon_stage( NEW_MOON ),
    t16_2pc_starfall_bolt( nullptr ),
    t16_2pc_sun_bolt( nullptr ),
    scythe_of_elune(),
    balance_tier18_2pc( *this ),
    predator( *this ),
    active( active_actions_t() ),
    pet_fey_moonwing(),
    caster_form_weapon(),
    starshards(),
    wildcat_celerity(),
    stalwart_guardian(),
    flourish(),
    buff( buffs_t() ),
    cooldown( cooldowns_t() ),
    gain( gains_t() ),
    glyph( glyphs_t() ),
    mastery( masteries_t() ),
    proc( procs_t() ),
    spec( specializations_t() ),
    spell( spells_t() ),
    talent( talents_t() )
  {
    t16_2pc_starfall_bolt = nullptr;
    t16_2pc_sun_bolt      = nullptr;
    
    cooldown.berserk             = get_cooldown( "berserk"             );
    cooldown.celestial_alignment = get_cooldown( "celestial_alignment" );
    cooldown.frenzied_regen_use  = get_cooldown( "frenzied_regen_use"  );
    cooldown.growl               = get_cooldown( "growl"               );
    cooldown.mangle              = get_cooldown( "mangle"              );
    cooldown.maul                = get_cooldown( "maul"                );
    cooldown.moon_cd             = get_cooldown( "moon_cd"             );
    cooldown.wod_pvp_4pc_melee   = get_cooldown( "wod_pvp_4pc_melee"   );
    cooldown.swiftmend           = get_cooldown( "swiftmend"           );
    cooldown.tigers_fury         = get_cooldown( "tigers_fury"         );
    cooldown.warrior_of_elune    = get_cooldown( "warrior_of_elune"    );
    
    cooldown.wod_pvp_4pc_melee -> duration = timespan_t::from_seconds( 30.0 );

    caster_melee_attack = nullptr;
    cat_melee_attack = nullptr;
    bear_melee_attack = nullptr;

    equipped_weapon_dps = 0;

    regen_type = REGEN_DYNAMIC;
    regen_caches[ CACHE_HASTE ] = true;
    regen_caches[ CACHE_ATTACK_HASTE ] = true;
  }

  virtual           ~druid_t();

  // Character Definition
  virtual void      init() override;
  virtual void      init_spells() override;
  virtual void      init_base_stats() override;
  virtual void      create_buffs() override;
  virtual void      init_scaling() override;
  virtual void      init_gains() override;
  virtual void      init_procs() override;
  virtual void      init_resources( bool ) override;
  virtual void      init_rng() override;
  virtual void      init_absorb_priority() override;
  virtual void      invalidate_cache( cache_e ) override;
  virtual void      arise() override;
  virtual void      combat_begin() override;
  virtual void      reset() override;
  virtual void      merge( player_t& other ) override;
  virtual timespan_t available() const override;
  virtual double    composite_armor_multiplier() const override;
  virtual double    composite_attack_power_multiplier() const override;
  virtual double    composite_attribute( attribute_e attr ) const override;
  virtual double    composite_attribute_multiplier( attribute_e attr ) const override;
  virtual double    composite_block() const override { return 0; }
  virtual double    composite_crit_avoidance() const override;
  virtual double    composite_melee_attack_power() const override;
  virtual double    composite_melee_crit() const override;
  virtual double    composite_melee_expertise( const weapon_t* ) const override;
  virtual double    composite_parry() const override { return 0; }
  virtual double    composite_persistent_multiplier( school_e ) const override;
  virtual double    composite_player_multiplier( school_e school ) const override;
  virtual double    composite_spell_crit() const override;
  virtual double    composite_spell_haste() const override;
  virtual double    composite_spell_power( school_e school ) const override;
  virtual double    temporary_movement_modifier() const override;
  virtual double    passive_movement_modifier() const override;
  virtual double    matching_gear_multiplier( attribute_e attr ) const override;
  virtual expr_t*   create_expression( action_t*, const std::string& name ) override;
  virtual action_t* create_action( const std::string& name, const std::string& options ) override;
  virtual pet_t*    create_pet   ( const std::string& name, const std::string& type = std::string() ) override;
  virtual void      create_pets() override;
  virtual resource_e primary_resource() const override;
  virtual role_e    primary_role() const override;
  virtual stat_e    convert_hybrid_stat( stat_e s ) const override;
  virtual double    mana_regen_per_second() const override;
  virtual void      target_mitigation( school_e, dmg_e, action_state_t* ) override;
  virtual void      assess_damage( school_e, dmg_e, action_state_t* ) override;
  virtual void      assess_damage_imminent_pre_absorb( school_e, dmg_e, action_state_t* ) override;
  virtual void      assess_heal( school_e, dmg_e, action_state_t* ) override;
  virtual void      recalculate_resource_max( resource_e ) override;
  virtual void      create_options() override;
  virtual action_t* create_proc_action( const std::string& name, const special_effect_t& ) override;
  virtual std::string      create_profile( save_e type = SAVE_ALL ) override;

  void              apl_precombat();
  void              apl_default();
  void              apl_feral();
  void              apl_balance();
  void              apl_guardian();
  void              apl_restoration();
  virtual void      init_action_list() override;
  virtual bool      has_t18_class_trinket() const override;

  form_e get_form() const
  {
    return form;
  }

  void shapeshift( form_e f )
  {
    if ( form == f )
      return;

    buff.cat_form     -> expire();
    buff.bear_form    -> expire();
    buff.moonkin_form -> expire();

    switch( f )
    {
    case CAT_FORM:
      buff.cat_form -> trigger();
      break;
    case BEAR_FORM:
      buff.bear_form -> trigger();
      break;
    case MOONKIN_FORM:
      buff.moonkin_form -> trigger();
      break;
    default:
      assert( 0 );
      break;
    }

    form = f;
  }

  target_specific_t<druid_td_t> target_data;

  virtual druid_td_t* get_target_data( player_t* target ) const override
  {
    assert( target );
    druid_td_t*& td = target_data[ target ];
    if ( ! td )
    {
      td = new druid_td_t( *target, const_cast<druid_t&>(*this) );
    }
    return td;
  }

  void init_beast_weapon( weapon_t& w, double swing_time )
  {
    w = main_hand_weapon;
    double mod = swing_time /  w.swing_time.total_seconds();
    w.type = WEAPON_BEAST;
    w.school = SCHOOL_PHYSICAL;
    w.min_dmg *= mod;
    w.max_dmg *= mod;
    w.damage *= mod;
    w.swing_time = timespan_t::from_seconds( swing_time );
  }
};

druid_t::~druid_t()
{
  range::dispose( counters );
}

snapshot_counter_t::snapshot_counter_t( druid_t* player , buff_t* buff ) :
  sim( player -> sim ), p( player ), b( 0 ), 
  exe_up( 0 ), exe_down( 0 ), tick_up( 0 ), tick_down( 0 ), is_snapped( false ), wasted_buffs( 0 )
{
  b.push_back( buff );
  p -> counters.push_back( this );
}

// Stalwart Guardian ( 6.2 T18 Guardian Trinket) =========================

struct brambles_t : public absorb_t
{
  struct brambles_reflect_t : public attack_t
  {
    brambles_reflect_t( druid_t* p ) :
      attack_t( "brambles_reflect", p, /* p -> find_spell( 203958 ) FIXME */ spell_data_t::nil() )
    {
      may_block = may_dodge = may_parry = may_miss = true;
      may_crit = true;

      school = SCHOOL_NATURE;
    }
  };

  double incoming_damage;
  double absorb_size;
  player_t* triggering_enemy;
  brambles_reflect_t* reflect;

  brambles_t( druid_t* p ) :
    absorb_t( "brambles_bg", p, p -> find_spell( 185321 ) ),
    incoming_damage( 0 ), absorb_size( 0 ),
    reflect( new brambles_reflect_t( p ) )
  {
    background = quiet = true;
    may_crit = false;
    may_multistrike = 0;
    target = p;
    harmful = false;

    attack_power_mod.direct = 0.2; // FIXME: not in spell data?
  }

  druid_t* p() const
  { return static_cast<druid_t*>( player ); }

  void init() override
  {
    absorb_t::init();

    snapshot_flags &= ~STATE_VERSATILITY; // Is not affected by versatility. TOCHECK
  }

  void execute() override
  {
    absorb_t::execute();

    // Trigger damage reflect
    double resolve = 1.0;
    if ( p() -> resolve_manager.is_started() )
      resolve *= 1.0 + player -> buffs.resolve -> current_value / 100.0;

    // Base damage is equal to the size of the absorb pre-resolve.
    reflect -> base_dd_min = absorb_size / resolve;
    reflect -> target = triggering_enemy;
    reflect -> execute();
  }

  void impact( action_state_t* s ) override
  {
    absorb_size = s -> result_total;
  }
};

// Stalwart Guardian ( 6.2 T18 Guardian Trinket) =========================

struct stalwart_guardian_t : public absorb_t
{
  struct stalwart_guardian_reflect_t : public attack_t
  {
    stalwart_guardian_reflect_t( druid_t* p ) :
      attack_t( "stalwart_guardian_reflect", p, p -> find_spell( 185321 ) )
    {
      may_block = may_dodge = may_parry = may_miss = true;
      may_crit = true;
    }
  };

  double incoming_damage;
  double absorb_limit;
  double absorb_size;
  player_t* triggering_enemy;
  stalwart_guardian_reflect_t* reflect;

  stalwart_guardian_t( druid_t* p ) :
    absorb_t( "stalwart_guardian_bg", p, p -> find_spell( 185321 ) ),
    incoming_damage( 0 ), absorb_limit( 0 ), absorb_size( 0 )
  {
    background = quiet = true;
    may_crit = false;
    may_multistrike = 0;
    target = p;
    harmful = false;

    reflect = new stalwart_guardian_reflect_t( p );
  }

  druid_t* p() const
  { return static_cast<druid_t*>( player ); }

  void init() override
  {
    if ( p() -> stalwart_guardian )
    {
      const spell_data_t* trinket = p() -> stalwart_guardian -> driver();
      attack_power_mod.direct     = trinket -> effectN( 1 ).average( p() -> stalwart_guardian -> item ) / 100.0;
      absorb_limit                = trinket -> effectN( 2 ).percent();
    }

    absorb_t::init();

    snapshot_flags &= ~STATE_VERSATILITY; // Is not affected by versatility.
  }

  void execute() override
  {
    assert( p() -> stalwart_guardian );

    absorb_t::execute();

    // Trigger damage reflect
    double resolve = 1.0;
    if ( p() -> resolve_manager.is_started() )
      resolve *= 1.0 + player -> buffs.resolve -> current_value / 100.0;

    // Base damage is equal to the size of the absorb pre-resolve.
    reflect -> base_dd_min = absorb_size / resolve;
    reflect -> target = triggering_enemy;
    reflect -> execute();
  }

  void impact( action_state_t* s ) override
  {
    s -> result_amount = std::min( s -> result_total, absorb_limit * incoming_damage );
    absorb_size = s -> result_amount;
  }
};

namespace pets {


// ==========================================================================
// Pets and Guardians
// ==========================================================================

// T18 2PC Balance Fairies ==================================================

struct fey_moonwing_t: public pet_t
{
  struct fey_missile_t: public spell_t
  {
    fey_missile_t( fey_moonwing_t* player ):
      spell_t( "fey_missile", player, player -> find_spell( 188046 ) )
    {
      if ( player -> o() -> pet_fey_moonwing[0] )
        stats = player -> o() -> pet_fey_moonwing[0] -> get_stats( "fey_missile" );
      may_crit = true;

      // Casts have a delay that decreases with haste. This is a very rough approximation.
      cooldown -> duration = timespan_t::from_millis( 600 );
    }

    double cooldown_reduction() const override
    {
      return spell_t::cooldown_reduction() * composite_haste();
    }
  };
  druid_t* o() { return static_cast<druid_t*>( owner ); }

  fey_moonwing_t( sim_t* sim, druid_t* owner ):
    pet_t( sim, owner, "fey_moonwing", true /*GUARDIAN*/, true )
  {
    owner_coeff.sp_from_sp = 0.75;
    regen_type = REGEN_DISABLED;
  }

  void init_base_stats() override
  {
    pet_t::init_base_stats();

    resources.base[RESOURCE_HEALTH] = owner -> resources.max[RESOURCE_HEALTH] * 0.4;
    resources.base[RESOURCE_MANA] = 0;

    initial.stats.attribute[ATTR_INTELLECT] = 0;
    initial.spell_power_per_intellect = 0;
    intellect_per_owner = 0;
    stamina_per_owner = 0;
    action_list_str = "fey_missile";
  }

  void summon( timespan_t duration ) override
  {
    pet_t::summon( duration );
    o() -> buff.balance_tier18_4pc -> trigger();
  }

  action_t* create_action( const std::string& name,
    const std::string& options_str ) override
  {
    if ( name == "fey_missile"  ) return new fey_missile_t( this );
    return pet_t::create_action( name, options_str );
  }
};
} // end namespace pets

namespace buffs {

template <typename BuffBase>
struct druid_buff_t : public BuffBase
{
protected:
  typedef druid_buff_t base_t;
  druid_t& druid;

  // Used when shapeshifting to switch to a new attack & schedule it to occur
  // when the current swing timer would have ended.
  void swap_melee( attack_t* new_attack, weapon_t& new_weapon )
  {
    if ( druid.main_hand_attack && druid.main_hand_attack -> execute_event )
    {
      new_attack -> base_execute_time = new_weapon.swing_time;
      new_attack -> execute_event = new_attack -> start_action_execute_event(
                                      druid.main_hand_attack -> execute_event -> remains() );
      druid.main_hand_attack -> cancel();
    }
    new_attack -> weapon = &new_weapon;
    druid.main_hand_attack = new_attack;
    druid.main_hand_weapon = new_weapon;
  }

public:
  druid_buff_t( druid_t& p, const buff_creator_basics_t& params ) :
    BuffBase( params ),
    druid( p )
  { }

  druid_t& p() const { return druid; }
};

// Bear Form ================================================================

struct bear_form_t : public druid_buff_t< buff_t >
{
public:
  bear_form_t( druid_t& p ) :
    base_t( p, buff_creator_t( &p, "bear_form", p.find_class_spell( "Bear Form" ) ) ),
    rage_spell( p.find_spell( 17057 ) )
  {
    add_invalidate( CACHE_AGILITY );
    add_invalidate( CACHE_ATTACK_POWER );
    add_invalidate( CACHE_STAMINA );
    add_invalidate( CACHE_ARMOR );
    add_invalidate( CACHE_EXP );
  }

  virtual void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    base_t::expire_override( expiration_stacks, remaining_duration );

    swap_melee( druid.caster_melee_attack, druid.caster_form_weapon );

    druid.recalculate_resource_max( RESOURCE_HEALTH );

    if ( druid.specialization() == DRUID_GUARDIAN )
      druid.resolve_manager.stop();
  }

  virtual void start( int stacks, double value, timespan_t duration ) override
  {
    druid.buff.moonkin_form -> expire();
    druid.buff.cat_form -> expire();

    druid.buff.tigers_fury -> expire(); // 6/29/2014: Tiger's Fury ends when you enter bear form. Legion TOCHECK

    if ( druid.specialization() == DRUID_GUARDIAN )
      druid.resolve_manager.start();

    swap_melee( druid.bear_melee_attack, druid.bear_weapon );

    // Set rage to 0 and then gain rage to 10
    druid.resource_loss( RESOURCE_RAGE, druid.resources.current[ RESOURCE_RAGE ] );
    druid.resource_gain( RESOURCE_RAGE, rage_spell -> effectN( 1 ).base_value() / 10.0, druid.gain.bear_form );
    // TODO: Clear rage on bear form exit instead of entry.

    base_t::start( stacks, value, duration );

    druid.recalculate_resource_max( RESOURCE_HEALTH );
  }
private:
  const spell_data_t* rage_spell;
};

// Berserk Buff =============================================================

struct berserk_buff_t : public druid_buff_t<buff_t>
{
  double increased_max_energy;
  double cost_modifier;

  berserk_buff_t( druid_t& p ) :
    druid_buff_t<buff_t>( p, buff_creator_t( &p, "berserk", p.find_spell( 106951 ) )
                             .cd( timespan_t::from_seconds( 0.0 ) ) ) // Cooldown handled by ability
  {
    increased_max_energy = data().effectN( 3 ).resource( RESOURCE_ENERGY );
    cost_modifier        = 1 + data().effectN( 1 ).percent();
  }

  virtual bool trigger( int stacks, double value, double chance, timespan_t duration ) override
  {
    bool refresh = druid.buff.berserk -> check() != 0;
    
    /* If Druid Tier 18 (WoD 6.2) trinket effect is in use, adjust Berserk duration
       based on spell data of the special effect. */
    if ( druid.wildcat_celerity )
      duration *= 1.0 + druid.wildcat_celerity -> driver() -> effectN( 1 ).average( druid.wildcat_celerity -> item ) / 100.0;

    bool success = druid_buff_t<buff_t>::trigger( stacks, value, chance, duration );

    if ( ! refresh && success ) {
      player -> resources.max[ RESOURCE_ENERGY ] += increased_max_energy;
      druid.max_fb_energy *= cost_modifier;
    }

    return success;
  }

  virtual void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    druid.max_fb_energy /= cost_modifier;

    player -> resources.max[ RESOURCE_ENERGY ] -= increased_max_energy;
    // Force energy down to cap if it's higher.
    player -> resources.current[ RESOURCE_ENERGY ] = std::min( player -> resources.current[ RESOURCE_ENERGY ], player -> resources.max[ RESOURCE_ENERGY ]);

    druid_buff_t<buff_t>::expire_override( expiration_stacks, remaining_duration );
  }
};

// Cat Form =================================================================

struct cat_form_t : public druid_buff_t< buff_t >
{
  cat_form_t( druid_t& p ) :
    base_t( p, buff_creator_t( &p, "cat_form", p.find_class_spell( "Cat Form" ) ) )
  {
    add_invalidate( CACHE_AGILITY );
    add_invalidate( CACHE_ATTACK_POWER );
  }

  virtual void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    base_t::expire_override( expiration_stacks, remaining_duration );
    
    swap_melee( druid.caster_melee_attack, druid.caster_form_weapon );
  }

  virtual void start( int stacks, double value, timespan_t duration ) override
  {
    druid.buff.bear_form -> expire();
    druid.buff.moonkin_form -> expire();

    swap_melee( druid.cat_melee_attack, druid.cat_weapon );

    base_t::start( stacks, value, duration );
  }
};

// Celestial Alignment Buff ====================================================

struct celestial_alignment_buff_t : public druid_buff_t < buff_t >
{
  druid_t* druid;

  celestial_alignment_buff_t( druid_t& p ) :
    druid_buff_t<buff_t>( p, buff_creator_t( &p, "celestial_alignment", p.spec.celestial_alignment )
               .cd( timespan_t::zero() ) // handled by spell
               .default_value( p.spec.celestial_alignment -> effectN( 1 ).percent() ) ),
    druid( &p )
  {}

  void expire_override( int stacks, timespan_adl_barrier::timespan_t duration )
  {
    druid_buff_t<buff_t>::expire_override( stacks, duration );

    druid -> buff.star_power -> expire();
  }
};

// Ironfur Buff ============================================================

struct ironfur_buff_t : public druid_buff_t < buff_t >
{
  ironfur_buff_t( druid_t& p, const std::string& s ) :
    base_t( p, buff_creator_t( &p, s, p.spec.ironfur )
    .cd( timespan_t::zero() )
    .quiet( true )
    .duration( p.spec.ironfur -> duration() + p.talent.guardian_of_elune -> effectN( 1 ).time_value() ) )
  {}
  
  virtual void start( int stacks, double value, timespan_t duration ) override
  {
    druid_buff_t<buff_t>::start( stacks, value, duration );

    p().buff.ironfur -> trigger();
  }

  virtual void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    druid_buff_t<buff_t>::expire_override( expiration_stacks, remaining_duration );

    p().buff.ironfur -> decrement();
  }
};

// Moonkin Form =============================================================

struct moonkin_form_t : public druid_buff_t< buff_t >
{
  moonkin_form_t( druid_t& p ) :
    base_t( p, buff_creator_t( &p, "moonkin_form", p.spec.moonkin_form )
               .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
               .add_invalidate( CACHE_ARMOR )
               .chance( 1.0 ) )
  {}

  virtual void start( int stacks, double value, timespan_t duration ) override
  {
    druid.buff.bear_form -> expire();
    druid.buff.cat_form  -> expire();

    base_t::start( stacks, value, duration );
  }
};

// Clearcasting Buff =============================================================

struct clearcasting_buff_t : public druid_buff_t<buff_t>
{
  clearcasting_buff_t( druid_t& p ) :
    druid_buff_t<buff_t>( p, buff_creator_t( &p, "clearcasting", p.spec.omen_of_clarity -> effectN( 1 ).trigger() )
      .chance( p.specialization() == DRUID_RESTORATION ? p.find_spell( 113043 ) -> proc_chance()
                                                     : p.find_spell( 16864 ) -> proc_chance() )
      .cd( timespan_t::zero() ) // Cooldown handled by ability
      .max_stack( 1 + p.talent.moment_of_clarity -> effectN( 1 ).base_value() )
    )
  {}

  virtual bool trigger( int stacks, double value, double chance, timespan_t duration ) override
  {
    bool refresh = druid.buff.clearcasting -> check() != 0;

    bool success = druid_buff_t<buff_t>::trigger( stacks, value, chance, duration );

    if ( ! refresh && success )
      druid.max_fb_energy -= druid.spell.ferocious_bite -> powerN( 1 ).cost() * ( 1.0 + druid.buff.berserk -> check() * druid.spell.berserk_cat -> effectN( 1 ).percent() );

    return success;
  }

  virtual void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    druid.max_fb_energy += druid.spell.ferocious_bite -> powerN( 1 ).cost() * ( 1.0 + druid.buff.berserk -> check() * druid.spell.berserk_cat -> effectN( 1 ).percent() );

    druid_buff_t<buff_t>::expire_override( expiration_stacks, remaining_duration );
  }
};

// Warrior of Elune Buff ========================================================

struct warrior_of_elune_buff_t : public druid_buff_t<buff_t>
{
  warrior_of_elune_buff_t( druid_t& p ) :
    druid_buff_t<buff_t>( p, buff_creator_t( &p, "warrior_of_elune", p.talent.warrior_of_elune ) )
  {}

  virtual void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    druid_buff_t<buff_t>::expire_override( expiration_stacks, remaining_duration );

    // disabled for now since they'll probably institute this behavior later.
    // druid.cooldown.warrior_of_elune -> start();
  }
};

} // end namespace buffs

// Template for common druid action code. See priest_action_t.
template <class Base>
struct druid_action_t : public Base
{
  unsigned form_mask; // Restricts use of a spell based on form.
  bool may_autounshift; // Allows a spell that may be cast in NO_FORM but not in current form to be cast by exiting form.
  unsigned autoshift; // Allows a spell that may not be cast in the current form to be cast by automatically changing to the specified form.
private:
  typedef Base ab; // action base, eg. spell_t
public:
  typedef druid_action_t base_t;

  druid_action_t( const std::string& n, druid_t* player,
                  const spell_data_t* s = spell_data_t::nil() ) :
    ab( n, player, s ), 
    form_mask( ab::data().stance_mask() ), may_autounshift( true ), autoshift( 0 )
  {
    ab::may_crit      = true;
    ab::tick_may_crit = true;
  }

  druid_t* p()
  { return static_cast<druid_t*>( ab::player ); }
  const druid_t* p() const
  { return static_cast<druid_t*>( ab::player ); }

  druid_td_t* td( player_t* t ) const
  { return p() -> get_target_data( t ); }

  virtual double composite_target_multiplier( player_t* t ) const
  {
    double tm = ab::composite_target_multiplier( t );

    if ( p() -> talent.rend_and_tear -> ok() )
      tm *= 1.0 + p() -> talent.rend_and_tear -> effectN( 2 ).percent() * td( t ) -> lacerate_stack;

    return tm;
  }

  virtual void impact( action_state_t* s )
  {
    ab::impact( s );

    if ( p() -> buff.feral_tier17_4pc -> check() )
      trigger_gushing_wound( s -> target, s -> result_amount );
  }

  virtual void tick( dot_t* d )
  {
    ab::tick( d );

    if ( p() -> buff.feral_tier17_4pc -> check() )
      trigger_gushing_wound( d -> target, d -> state -> result_amount );
  }

  virtual void multistrike_tick( const action_state_t* src_state, action_state_t* ms_state, double multiplier )
  {
    ab::multistrike_tick( src_state, ms_state, multiplier );

    if ( p() -> buff.feral_tier17_4pc -> check() )
      trigger_gushing_wound( ms_state -> target, ms_state -> result_amount );
  }

  virtual void schedule_execute( action_state_t* s ) override
  {
    if ( ! check_form_restriction() )
    {
      if ( may_autounshift && ( form_mask & NO_FORM ) == NO_FORM )
        p() -> shapeshift( NO_FORM );
      else if ( autoshift )
        p() -> shapeshift( (form_e) autoshift );
      else
        assert( "Action executed in wrong form with no valid form to shift to!" );
    }

    ab::schedule_execute( s );
  }
  
  /* Override this function for temporary effects that change the normal
     form restrictions of the spell. eg: Predatory Swiftness */
  virtual bool check_form_restriction()
  {
    return ! form_mask || ( form_mask & p() -> get_form() ) == p() -> get_form();
  }

  virtual bool ready() override
  {
    if ( ! check_form_restriction() && ! ( ( may_autounshift && ( form_mask & NO_FORM ) == NO_FORM ) || autoshift ) )
    {
      if ( ab::sim -> log )
        ab::sim -> out_log.printf( "%s ready() failed due to wrong form. form=%#.8x form_mask=%#.8x", ab::name(), p() -> get_form(), form_mask );

      return false;
    }
    
    return ab::ready();
  }

  void trigger_gushing_wound( player_t* t, double dmg )
  {
    if ( ! ( ab::special && ab::harmful && dmg > 0 ) )
      return;

    residual_action::trigger(
      p() -> active.gushing_wound, // ignite spell
      t, // target
      p() -> spell.gushing_wound -> effectN( 1 ).percent() * dmg );
  }

  virtual expr_t* create_expression( const std::string& name_str )
  {
    if ( util::str_compare_ci( name_str, "active_dot.starfall" ) )
    {
      return make_ref_expr( "starfall", p() -> active_starfalls );
    }
    else if ( util::str_compare_ci( name_str, "dot.lacerate.stack" ) )
    {
      struct lacerate_stack_expr_t : public expr_t
      {
        druid_t& druid;
        action_t* action;

        lacerate_stack_expr_t( druid_t& p, action_t* a ) :
          expr_t( "stack" ), druid( p ), action( a )
        {}

        virtual double evaluate() override
        { return druid.get_target_data( action -> target ) -> lacerate_stack; }
      };

      return new lacerate_stack_expr_t( *p(), this );
    }

    return ab::create_expression( name_str );
  }
};

// Druid melee attack base for cat_attack_t and bear_attack_t
template <class Base>
struct druid_attack_t : public druid_action_t< Base >
{
protected:
  bool attackHit;
private:
  typedef druid_action_t< Base > ab;
public:
  typedef druid_attack_t base_t;
  
  bool consume_bloodtalons;
  snapshot_counter_t* bt_counter;
  snapshot_counter_t* tf_counter;

  druid_attack_t( const std::string& n, druid_t* player,
                  const spell_data_t* s = spell_data_t::nil() ) :
    ab( n, player, s ), attackHit( false ), consume_bloodtalons( false ),
    bt_counter( nullptr ), tf_counter( nullptr )
  {
    ab::may_glance    = false;
    ab::special       = true;
  }

  virtual void init()
  {
    ab::init();
    
    consume_bloodtalons = ab::harmful && ab::special && ab::trigger_gcd > timespan_t::zero();

    if ( consume_bloodtalons )
    {
      bt_counter = new snapshot_counter_t( ab::p() , ab::p() -> buff.bloodtalons );
      tf_counter = new snapshot_counter_t( ab::p() , ab::p() -> buff.tigers_fury );
    }
  }

  virtual void execute()
  {
    attackHit = false;

    ab::execute();

    if( consume_bloodtalons && attackHit )
    {
      bt_counter -> count_execute();
      tf_counter -> count_execute();

      ab::p() -> buff.bloodtalons -> decrement();
    }
  }

  virtual void impact( action_state_t* s )
  {
    ab::impact( s );

    if ( ab::result_is_hit( s -> result ) )
      attackHit = true;

    if ( ! ab::special )
    {
      if ( ab::result_is_hit( s -> result ) )
        trigger_clearcasting();
    }
  }

  virtual void tick( dot_t* d )
  {
    ab::tick( d );

    if( consume_bloodtalons )
    {
      bt_counter -> count_tick();
      tf_counter -> count_tick();
    }
  }

  virtual double composite_persistent_multiplier( const action_state_t* s ) const
  {
    double pm = ab::composite_persistent_multiplier( s );
    
    if ( ab::p() -> talent.bloodtalons -> ok() && consume_bloodtalons && ab::p() -> buff.bloodtalons -> check() )
      pm *= 1.0 + ab::p() -> buff.bloodtalons -> data().effectN( 1 ).percent();

    if ( ! ab::p() -> buff.bear_form -> check() && dbc::is_school( ab::school, SCHOOL_PHYSICAL ) )
      pm *= 1.0 + ab::p() -> buff.savage_roar -> check() * ab::p() -> buff.savage_roar -> default_value; // Avoid using value() to prevent skewing benefit_pct.

    return pm;
  }
  
  virtual double composite_target_multiplier( player_t* t ) const
  {
    double tm = ab::composite_target_multiplier( t );

    /* Assume that any action that deals physical and applies a dot deals all bleed damage, so
       that it scales direct "bleed" damage. This is a bad assumption if there is an action
       that applies a dot but does plain physical direct damage, but there are none of those. */
    if ( dbc::is_school( ab::school, SCHOOL_PHYSICAL ) && ab::dot_duration > timespan_t::zero() )
      tm *= 1.0 + ab::td( t ) -> buffs.bloodletting -> value();

    return tm;
  }

  void trigger_clearcasting()
  {
    if ( ab::proc )
      return;
    if ( ! ( ab::p() -> specialization() == DRUID_FERAL && ab::p() -> spec.omen_of_clarity -> ok() ) )
      return;

    double chance = ab::weapon -> proc_chance_on_swing( 3.5 );

    if ( ab::p() -> sets.has_set_bonus( DRUID_FERAL, T18, B2 ) )
      chance *= 1.0 + ab::p() -> sets.set( DRUID_FERAL, T18, B2 ) -> effectN( 1 ).percent();

    int active = ab::p() -> buff.clearcasting -> check();

    // 3.5 PPM via https://twitter.com/Celestalon/status/482329896404799488
    if ( ab::p() -> buff.clearcasting -> trigger(
             ab::p() -> buff.clearcasting -> max_stack(),
             buff_t::DEFAULT_VALUE(),
             chance,
             ab::p() -> buff.clearcasting -> buff_duration ) ) {
      ab::p() -> proc.clearcasting -> occur();
      
      if ( active )
        ab::p() -> proc.clearcasting_wasted -> occur();

      if ( ab::p() -> sets.has_set_bonus( SET_MELEE, T16, B2 ) )
        ab::p() -> buff.feral_tier16_2pc -> trigger();
    }
  }

};

// Druid "Spell" Base for druid_spell_t, druid_heal_t ( and potentially druid_absorb_t )
template <class Base>
struct druid_spell_base_t : public druid_action_t< Base >
{
private:
  typedef druid_action_t< Base > ab;
public:
  typedef druid_spell_base_t base_t;

  druid_spell_base_t( const std::string& n, druid_t* player,
                      const spell_data_t* s = spell_data_t::nil() ) :
    ab( n, player, s )
  {}
};

namespace spells {

/* druid_spell_t ============================================================
  Early definition of druid_spell_t. Only spells that MUST for use by other
  actions should go here, otherwise they can go in the second spells
  namespace.
========================================================================== */

struct druid_spell_t : public druid_spell_base_t<spell_t>
{
private:
  bool consumed_owlkin_frenzy;
public:
  double ap_per_hit, ap_per_tick, ap_per_cast;
  bool benefits_from_ca, benefits_from_elune;
  bool consumes_owlkin_frenzy;
  gain_t* ap_gain;

  druid_spell_t( const std::string& token, druid_t* p,
                 const spell_data_t* s = spell_data_t::nil(),
                 const std::string& options = std::string() ) :
    base_t( token, p, s ), ap_per_hit( 0 ), ap_per_tick( 0 ),
    ap_per_cast( 0 ), consumes_owlkin_frenzy( false ),
    benefits_from_ca( false ), benefits_from_elune( false ),
    ap_gain( p -> get_gain( name() ) )
  {
    parse_options( options );
  }

  virtual void reset()
  {
    spell_t::reset();

    // Allows precasted spells, which circumvent schedule_execute(), to consume Owlkin Frenzy.
    consumed_owlkin_frenzy = consumes_owlkin_frenzy;
  }

  virtual void schedule_execute( action_state_t* s ) override
  {
    spell_t::schedule_execute( s );

    consumed_owlkin_frenzy = consumes_owlkin_frenzy && p() -> buff.owlkin_frenzy -> up();
  }

  virtual void execute() override
  {
    // Adjust buffs and cooldowns if we're in precombat.
    if ( ! p() -> in_combat )
    {
      if ( p() -> buff.incarnation -> check() )
      {
        timespan_t time = std::max( std::max( min_gcd, trigger_gcd * composite_haste() ), base_execute_time * composite_haste() );
        p() -> buff.incarnation -> extend_duration( p(), -time );
        p() -> get_cooldown( "incarnation" ) -> adjust( -time );
      }
    }

    if ( harmful && trigger_gcd > timespan_t::zero() )
      p() -> buff.star_power -> up();

    spell_t::execute();

    if ( consumed_owlkin_frenzy )
      p() -> buff.owlkin_frenzy -> decrement();

    trigger_astral_power_gain( ap_per_cast );

    if ( p() -> artifact.moon_and_stars.rank() && p() -> buff.celestial_alignment -> check() && ! background )
      p() -> buff.star_power -> trigger();
  }

  virtual void impact( action_state_t* s ) override
  {
    spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
      trigger_astral_power_gain( ap_per_hit );
  }

  virtual void tick( dot_t* d ) override
  {
    if ( hasted_ticks && d -> state -> result_amount > 0 )
      p() -> buff.star_power -> up();

    spell_t::tick( d );

    if ( result_is_hit( d -> state -> result ) )
      trigger_astral_power_gain( ap_per_tick );
  }

  virtual double action_multiplier() const override
  {
    double am = spell_t::action_multiplier();

    if ( p() -> buff.celestial_alignment -> check() )
      am *= 1.0 + p() -> buff.celestial_alignment -> current_value;

    return am;
  }

  virtual timespan_t execute_time() const
  {
    if ( consumes_owlkin_frenzy && p() -> buff.owlkin_frenzy -> check() )
      return timespan_t::zero();

    return spell_t::execute_time();
  }

  virtual void trigger_astral_power_gain( double base_ap )
  {
    if ( base_ap <= 0 )
      return;

    /*
      Astral power modifiers are multiplicative, so we have to do some shenanigans
      if we want to have seperate gains to track the effectiveness of these modifiers.
    */

    // Calculate the final AP total, and the additive percent bonus to the base.

    double ap = base_ap;
    double bonus_pct = 0;
    
    if ( benefits_from_ca && p() -> buff.celestial_alignment -> check() )
    {
      ap *= 1.0 + p() -> spec.celestial_alignment -> effectN( 3 ).percent();
      bonus_pct += p() -> spec.celestial_alignment -> effectN( 3 ).percent();
    }

    if ( benefits_from_elune && p() -> buff.blessing_of_elune -> check() )
    {
      ap *= 1.0 + p() -> spell.blessing_of_elune -> effectN( 2 ).percent();
      bonus_pct += p() -> spec.celestial_alignment -> effectN( 3 ).percent();
    }

    // Gain the base AP amount and attribute it to the spell cast.
    p() -> resource_gain( RESOURCE_ASTRAL_POWER, base_ap, ap_gain );

    // Subtract the base amount from the total AP gain.
    ap -= base_ap;

    // Divide the remaining AP gain among the buffs based on their modifier / bonus_pct ratio.
    if ( benefits_from_ca && p() -> buff.celestial_alignment -> check() )
      p() -> resource_gain( RESOURCE_ASTRAL_POWER, ap * ( p() -> spec.celestial_alignment -> effectN( 3 ).percent() / bonus_pct ), p() -> gain.celestial_alignment );

    if ( benefits_from_elune && p() -> buff.blessing_of_elune -> check() )
      p() -> resource_gain( RESOURCE_ASTRAL_POWER, ap * ( p() -> spell.blessing_of_elune -> effectN( 2 ).percent() / bonus_pct ), p() -> gain.blessing_of_elune );
  }

  virtual void trigger_balance_tier18_2pc()
  {
    if ( ! p() -> balance_tier18_2pc.trigger() )
      return;

    for ( pet_t* pet : p() -> pet_fey_moonwing )
    {
      if ( pet -> is_sleeping() )
      {
        pet -> summon( timespan_t::from_seconds( 30 ) );
        return;
      }
    }
  }

  virtual void trigger_natures_balance( action_state_t* s, dot_t* d )
  {
    if ( ! p() -> talent.natures_balance -> ok() )
      return;
    if ( ! result_is_hit( s -> result ) )
      return;

    // Nature's Balance may only extend the DoT to 20 seconds remaining.

    timespan_t base_time;
    if ( d == td( d -> target ) -> dots.moonfire )
      base_time = timespan_t::from_seconds( p() -> talent.natures_balance -> effectN( 1 ).base_value() );
    else if ( d == td( d -> target ) -> dots.sunfire )
      base_time = timespan_t::from_seconds( p() -> talent.natures_balance -> effectN( 2 ).base_value() );
    else
      return;

    timespan_t extension_limit = std::max( timespan_t::zero(), timespan_t::from_seconds( 20.0 ) - d -> remains() );
    d -> extend_duration( std::min( base_time, extension_limit ) );
  }
}; // end druid_spell_t

// Shooting Stars ===========================================================

struct shooting_stars_t : public druid_spell_t
{
  double proc_chance;

  shooting_stars_t( druid_t* player ) :
    druid_spell_t( "shooting_stars", player, player -> find_spell( 202497 ) ),
    proc_chance( 0.50 ) // not in spell data, tested Dec 3 2015
  {
    background = true;
    ap_per_cast = data().effectN( 2 ).resource( RESOURCE_ASTRAL_POWER );
  }
};

// Moonfire Spell ===========================================================

struct moonfire_t : public druid_spell_t
{
  shooting_stars_t* shooting_stars;

  moonfire_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "moonfire", player, player -> find_spell( 8921 ) ),
    shooting_stars( new shooting_stars_t( player ) )
  {
    parse_options( options_str );

    const spell_data_t* dmg_spell = player -> find_spell( 164812 );

    dot_duration                  = dmg_spell -> duration(); 
    dot_duration                 += player -> sets.set( SET_CASTER, T14, B4 ) -> effectN( 1 ).time_value();
    dot_duration                 += player -> spec.balance_passive -> effectN( 4 ).time_value();
    base_tick_time                = dmg_spell -> effectN( 2 ).period();
    spell_power_mod.tick          = dmg_spell -> effectN( 2 ).sp_coeff();
    spell_power_mod.direct        = dmg_spell -> effectN( 1 ).sp_coeff();

    base_multiplier *= 1.0 + player -> artifact.twilight_glow.percent();
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double tm = druid_spell_t::composite_target_multiplier( t );

    if ( td( t ) -> buffs.starfall -> up() )
      tm *= 1.0 + td( t ) -> buffs.starfall -> current_value
                + ( p() -> mastery.starlight -> ok() * p() -> cache.mastery_value() );

    return tm;
  }

  void tick( dot_t* d ) override
  {
    druid_spell_t::tick( d );

    if ( result_is_hit( d -> state -> result ) )
    {
      if ( p() -> talent.shooting_stars -> ok() && rng().roll( shooting_stars -> proc_chance ) )
      {
        shooting_stars -> target = d -> target;
        shooting_stars -> execute();
      }

      if ( p() -> sets.has_set_bonus( DRUID_BALANCE, T18, B2 ) )
        trigger_balance_tier18_2pc();
    }
  }
};

}

namespace heals {

struct druid_heal_t : public druid_spell_base_t<heal_t>
{
  action_t* living_seed;
  bool target_self;

  druid_heal_t( const std::string& token, druid_t* p,
                const spell_data_t* s = spell_data_t::nil(),
                const std::string& options_str = std::string() ) :
    base_t( token, p, s ),
    living_seed( nullptr ),
    target_self( 0 )
  {
    add_option( opt_bool( "target_self", target_self ) );
    parse_options( options_str );

    if( target_self )
      target = p;

    may_miss          = false;
    weapon_multiplier = 0;
    harmful           = false;
  }
    
protected:
  void init_living_seed();

public:
  virtual void execute() override
  {
    base_t::execute();

    if ( base_execute_time > timespan_t::zero() )
      p() -> buff.soul_of_the_forest -> expire();

    if ( p() -> mastery.harmony -> ok() && spell_power_mod.direct > 0 && ! background )
      p() -> buff.harmony -> trigger( 1, p() -> mastery.harmony -> ok() ? p() -> cache.mastery_value() : 0.0 );
  }

  virtual double composite_haste() const override
  {
    double h = base_t::composite_haste();

    h *= 1.0 / ( 1.0 + p() -> buff.soul_of_the_forest -> value() );

    return h;
  }

  virtual double action_da_multiplier() const override
  {
    double adm = base_t::action_da_multiplier();

    if ( p() -> buff.incarnation -> up() && p() -> specialization() == DRUID_RESTORATION )
      adm *= 1.0 + p() -> buff.incarnation -> data().effectN( 1 ).percent();

    if ( p() -> mastery.harmony -> ok() )
      adm *= 1.0 + p() -> cache.mastery_value();

    return adm;
  }

  virtual double action_ta_multiplier() const override
  {
    double adm = base_t::action_ta_multiplier();

    if ( p() -> buff.incarnation -> up() && p() -> specialization() == DRUID_RESTORATION )
      adm += p() -> buff.incarnation -> data().effectN( 2 ).percent();

    adm += p() -> buff.harmony -> value();

    return adm;
  }

  void trigger_lifebloom_refresh( action_state_t* s )
  {
    druid_td_t& td = *this -> td( s -> target );

    if ( td.dots.lifebloom -> is_ticking() )
    {
      td.dots.lifebloom -> refresh_duration();

      if ( td.buffs.lifebloom -> check() )
        td.buffs.lifebloom -> refresh();
    }
  }

  void trigger_living_seed( action_state_t* s )
  {
    // Technically this should be a buff on the target, then bloom when they're attacked
    // For simplicity we're going to assume it always heals the target
    if ( living_seed )
    {
      living_seed -> base_dd_min = s -> result_amount;
      living_seed -> base_dd_max = s -> result_amount;
      living_seed -> execute();
    }
  }

  void trigger_clearcasting()
  {
    if ( ! proc && p() -> specialization() == DRUID_RESTORATION && p() -> spec.omen_of_clarity -> ok() )
      p() -> buff.clearcasting -> trigger(); // Proc chance is handled by buff chance
  }
}; // end druid_heal_t

}

namespace caster_attacks {

// Caster Form Melee Attack ========================================================

struct caster_attack_t : public druid_attack_t < melee_attack_t >
{
  caster_attack_t( const std::string& token, druid_t* p,
                const spell_data_t* s = spell_data_t::nil(),
                const std::string& options = std::string() ) :
    base_t( token, p, s )
  {
    parse_options( options );
  }
}; // end druid_caster_attack_t

struct druid_melee_t : public caster_attack_t
{
  druid_melee_t( druid_t* p ) :
    caster_attack_t( "melee", p )
  {
    school      = SCHOOL_PHYSICAL;
    may_glance  = background = repeating = true;
    trigger_gcd = timespan_t::zero();
    special     = false;
  }

  virtual timespan_t execute_time() const override
  {
    if ( ! player -> in_combat )
      return timespan_t::from_seconds( 0.01 );

    return caster_attack_t::execute_time();
  }
};

}

namespace cat_attacks {

// ==========================================================================
// Druid Cat Attack
// ==========================================================================

struct cat_attack_t : public druid_attack_t < melee_attack_t >
{
  bool   requires_stealth;
  int    combo_point_gain;
  double base_dd_bonus;
  double base_td_bonus;
  bool   consume_ooc;
  bool   trigger_tier17_2pc;

  cat_attack_t( const std::string& token, druid_t* p,
                const spell_data_t* s = spell_data_t::nil(),
                const std::string& options = std::string() ) :
    base_t( token, p, s ),
    requires_stealth( false ), combo_point_gain( 0 ),
    base_dd_bonus( 0.0 ), base_td_bonus( 0.0 ), consume_ooc( true ),
    trigger_tier17_2pc( false )
  {
    parse_options( options );

    parse_special_effect_data();

    // Skills that cost combo points can't be cast outside of Cat Form.
    if ( base_costs[ RESOURCE_COMBO_POINT ] > 0 )
      form_mask |= CAT_FORM;
  }

  void parse_special_effect_data()
  {
    for ( size_t i = 1; i <= data().effect_count(); i++ )
    {
      const spelleffect_data_t& ed = data().effectN( i );
      effect_type_t type = ed.type();

      if ( type == E_ADD_COMBO_POINTS )
        combo_point_gain = ed.base_value();
      else if ( type == E_APPLY_AURA && ed.subtype() == A_PERIODIC_DAMAGE )
      {
        snapshot_flags |= STATE_AP;
        base_td_bonus = ed.bonus( player );
      }
      else if ( type == E_SCHOOL_DAMAGE )
      {
        snapshot_flags |= STATE_AP;
        base_dd_bonus = ed.bonus( player );
      }
    }
  }

  virtual double cost() const override
  {
    double c = base_t::cost();

    if ( c == 0 )
      return 0;

    if ( consume_ooc && p() -> buff.clearcasting -> check() )
      return 0;

    if ( p() -> buff.berserk -> check() )
      c *= 1.0 + p() -> spell.berserk_cat -> effectN( 1 ).percent();

    return c;
  }

  virtual bool prowling() const // For effects that specifically trigger only when "prowling."
  {
    // Make sure we call all three methods for accurate benefit tracking.
    bool prowl = p() -> buff.prowl -> up(),
           inc = p() -> buff.incarnation -> up() && p() -> specialization() == DRUID_FERAL;

    return prowl || inc;
  }

  virtual bool stealthed() const // For effects that require any form of stealth.
  {
    // Make sure we call all three methods for accurate benefit tracking.
    bool shadowmeld = p() -> buffs.shadowmeld -> up(),
              prowl = prowling();

    return prowl || shadowmeld;
  }

  virtual void execute() override
  {
    base_t::execute();

    if ( this -> base_costs[ RESOURCE_COMBO_POINT ] > 0 )
    {
      if ( player -> sets.has_set_bonus( SET_MELEE, T15, B2 ) &&
          rng().roll( cost() * 0.15 ) )
      {
        p() -> proc.tier15_2pc_melee -> occur();
        p() -> resource_gain( RESOURCE_COMBO_POINT, 1, p() -> gain.feral_tier15_2pc );
      }

      if ( p() -> buff.feral_tier16_4pc -> up() )
      {
        p() -> resource_gain( RESOURCE_COMBO_POINT, p() -> buff.feral_tier16_4pc -> data().effectN( 1 ).base_value(), p() -> gain.feral_tier16_4pc );
        p() -> buff.feral_tier16_4pc -> expire();
      }
    }

    if ( ! result_is_hit( execute_state -> result ) )
      trigger_energy_refund();

    if ( harmful )
    {
      p() -> buff.prowl -> expire();
      p() -> buffs.shadowmeld -> expire();
      
      // Track benefit of damage buffs
      p() -> buff.tigers_fury -> up();
      p() -> buff.savage_roar -> up();
      if ( special )
        p() -> buff.berserk -> up();
    }
  }

  virtual void impact( action_state_t* s ) override
  {
    base_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      if ( p() -> spec.predatory_swiftness -> ok() && base_costs[ RESOURCE_COMBO_POINT ] )
        p() -> buff.predatory_swiftness -> trigger( 1, 1, p() -> resources.current[ RESOURCE_COMBO_POINT ] * 0.20 );

      // Only manage for single target generators because AoE generators need special logic.
      if ( combo_point_gain && p() -> spell.primal_fury -> ok() &&
           s -> result == RESULT_CRIT && aoe == 0 )
      {
        p() -> proc.primal_fury -> occur();
        p() -> resource_gain( RESOURCE_COMBO_POINT, p() -> spell.primal_fury -> effectN( 1 ).base_value(), p() -> gain.primal_fury );
      }
    }
  }

  virtual void consume_resource() override
  {
    // Treat Omen of Clarity energy savings like an energy gain for tracking purposes.
    if ( base_t::cost() > 0 && consume_ooc && p() -> buff.clearcasting -> up() )
    {
      // Base cost doesn't factor in Berserk, but Omen of Clarity does net us less energy during it, so account for that here.
      double eff_cost = base_t::cost() * ( 1.0 + p() -> buff.berserk -> check() * p() -> spell.berserk_cat -> effectN( 1 ).percent() );
      p() -> gain.clearcasting -> add( RESOURCE_ENERGY, eff_cost );

      // Feral tier18 4pc occurs before the base cost is consumed.
      if ( p() -> sets.has_set_bonus( DRUID_FERAL, T18, B4 ) )
        p() -> resource_gain( RESOURCE_ENERGY, base_t::cost() * p() -> sets.set( DRUID_FERAL, T18, B4 ) -> effectN( 1 ).percent(), p() -> gain.feral_tier18_4pc );
    }

    base_t::consume_resource();

    if ( base_t::cost() > 0 && consume_ooc )
      p() -> buff.clearcasting -> decrement();

    if ( base_costs[ RESOURCE_COMBO_POINT ] && result_is_hit( execute_state -> result ) )
    {
      int consumed = (int) p() -> resources.current[ RESOURCE_COMBO_POINT ];

      p() -> resource_loss( RESOURCE_COMBO_POINT, consumed, nullptr, this );

      if ( sim -> log )
        sim -> out_log.printf( "%s consumes %d %s for %s (%d)",
                               player -> name(),
                               consumed,
                               util::resource_type_string( RESOURCE_COMBO_POINT ),
                               name(),
                               (int) player -> resources.current[ RESOURCE_COMBO_POINT ] );

      if ( p() -> talent.soul_of_the_forest -> ok() && p() -> specialization() == DRUID_FERAL )
        p() -> resource_gain( RESOURCE_ENERGY,
                              consumed * p() -> talent.soul_of_the_forest -> effectN( 1 ).base_value(),
                              p() -> gain.soul_of_the_forest );
    }
  }

  virtual bool ready() override
  {
    if ( ! base_t::ready() )
      return false;

    if ( requires_stealth && ! stealthed() )
      return false;

    if ( p() -> resources.current[ RESOURCE_COMBO_POINT ] < base_costs[ RESOURCE_COMBO_POINT ] )
      return false;

    return true;
  }

  virtual double composite_target_crit( player_t* t ) const override
  {
    double tc = base_t::composite_target_crit( t );

    if ( t -> debuffs.bleeding -> check() )
      tc += p() -> talent.blood_scent -> effectN( 1 ).percent();

    return tc;
  }

  virtual double composite_ta_multiplier( const action_state_t* s ) const override
  {
    double tm = base_t::composite_ta_multiplier( s );

    if ( p() -> mastery.razor_claws -> ok() && dbc::is_school( s -> action -> school, SCHOOL_PHYSICAL ) )
      tm *= 1.0 + p() -> cache.mastery_value();

    return tm;
  }

  void trigger_energy_refund()
  {
    double energy_restored = resource_consumed * 0.80;

    player -> resource_gain( RESOURCE_ENERGY, energy_restored, p() -> gain.energy_refund );
  }

  void tick( dot_t* d ) override
  {
    base_t::tick( d );

    if ( trigger_tier17_2pc )
      p() -> resource_gain( RESOURCE_ENERGY,
                            p() -> sets.set( DRUID_FERAL, T17, B2 ) -> effectN( 1 ).base_value(),
                            p() -> gain.feral_tier17_2pc );

    if ( p() -> predator_rppm_rate && p() -> predator.trigger() )
      trigger_predator();
  }

  void trigger_predator()
  {
    if ( ! p() -> cooldown.tigers_fury -> down() )
      p() -> proc.predator_wasted -> occur();

    p() -> cooldown.tigers_fury -> reset( true );
    p() -> proc.predator -> occur();
  }
}; // end druid_cat_attack_t


// Cat Melee Attack =========================================================

struct cat_melee_t : public cat_attack_t
{
  cat_melee_t( druid_t* player ) :
    cat_attack_t( "cat_melee", player, spell_data_t::nil(), "" )
  {
    form_mask = CAT_FORM;

    school = SCHOOL_PHYSICAL;
    may_glance = background = repeating = true;
    trigger_gcd = timespan_t::zero();
    special = false;
  }

  virtual timespan_t execute_time() const override
  {
    if ( ! player -> in_combat )
      return timespan_t::from_seconds( 0.01 );

    return cat_attack_t::execute_time();
  }

  virtual double action_multiplier() const override
  {
    double cm = cat_attack_t::action_multiplier();

    if ( p() -> buff.cat_form -> check() )
      cm *= 1.0 + p() -> buff.cat_form -> data().effectN( 3 ).percent();

    return cm;
  }
};

// Berserk ==================================================================

struct berserk_t : public cat_attack_t
{
  berserk_t( druid_t* player, const std::string& options_str ) :
    cat_attack_t( "berserk", player, player -> find_spell( 106951 ), options_str )
  {
    harmful = consume_ooc = may_miss = may_parry = may_dodge = may_crit = false;
  }

  void execute() override
  {
    cat_attack_t::execute();

    p() -> buff.berserk -> trigger();

    if ( p() -> sets.has_set_bonus( DRUID_FERAL, T17, B4 ) )
      p() -> buff.feral_tier17_4pc -> trigger();
  }
};

// Bloody Slash =============================================================

struct bloody_slash_t : public cat_attack_t
{
private:
  bool attackCritical;
public:
  bloody_slash_t( druid_t* p, const std::string& options_str ) :
    cat_attack_t( "bloody_slash", p, p -> talent.bloody_slash ),
    attackCritical( false )
  {
    parse_options( options_str );

    aoe = -1;
    cooldown -> charges = 3; // FIXME
    cooldown -> duration = timespan_t::from_seconds( 15.0 ); // FIXME
  }
  
  virtual void impact( action_state_t* s ) override
  {
    cat_attack_t::impact( s );

    if ( s -> result == RESULT_CRIT )
      attackCritical = true;
  }

  virtual void execute() override
  {
    attackCritical = false;

    cat_attack_t::execute();
    
    if ( attackHit )
    {
      p() -> resource_gain( RESOURCE_COMBO_POINT, combo_point_gain, p() -> gain.bloody_slash );
      if ( attackCritical && p() -> spell.primal_fury -> ok() )
      {
        p() -> proc.primal_fury -> occur();
        p() -> resource_gain( RESOURCE_COMBO_POINT, p() -> spell.primal_fury -> effectN( 1 ).base_value(), p() -> gain.primal_fury );
      }
    }
  }
};

// Ferocious Bite ===========================================================

struct ferocious_bite_t : public cat_attack_t
{
  double excess_energy;
  double max_excess_energy;
  timespan_t sabertooth_total;
  timespan_t sabertooth_base;
  bool max_energy;

  ferocious_bite_t( druid_t* p, const std::string& options_str ) :
    cat_attack_t( "ferocious_bite", p, p -> find_class_spell( "Ferocious Bite" ), "" ),
    excess_energy( 0 ), max_excess_energy( 0 ), max_energy( false )
  {
    add_option( opt_bool( "max_energy" , max_energy ) );
    parse_options( options_str );
    base_costs[ RESOURCE_COMBO_POINT ] = 1;
    base_costs[ RESOURCE_ENERGY ] = 25; // FIXME;

    max_excess_energy      = -1 * data().effectN( 2 ).base_value();
    special                = true;
    spell_power_mod.direct = 0;

    if ( p -> talent.sabertooth -> ok() )
      sabertooth_base = timespan_t::from_seconds( p -> talent.sabertooth -> effectN( 1 ).base_value() );
  }

  bool ready() override
  {
    if ( max_energy && p() -> resources.current[ RESOURCE_ENERGY ] < p() -> max_fb_energy )
      return false;

    return cat_attack_t::ready();
  }

  void execute() override
  {
    // Berserk does affect the additional energy consumption.
    if ( p() -> buff.berserk -> check() )
      max_excess_energy *= 1.0 + p() -> spell.berserk_cat -> effectN( 1 ).percent();

    excess_energy = std::min( max_excess_energy,
                              ( p() -> resources.current[ RESOURCE_ENERGY ] - cat_attack_t::cost() ) );

    sabertooth_total = p() -> resources.current[ RESOURCE_COMBO_POINT ] * sabertooth_base;

    cat_attack_t::execute();

    if ( p() -> buff.feral_tier15_4pc -> up() )
      p() -> buff.feral_tier15_4pc -> decrement();

    max_excess_energy = -1 * data().effectN( 2 ).base_value();
  }

  void impact( action_state_t* state ) override
  {
    cat_attack_t::impact( state );

    if ( result_is_hit( state -> result ) )
    {
      if ( td( state -> target ) -> dots.rip -> is_ticking() )
      {
        double blood_in_the_water = p() -> sets.has_set_bonus( SET_MELEE, T13, B2 ) ? p() -> sets.set( SET_MELEE, T13, B2 ) -> effectN( 2 ).base_value() : 25.0;

        if ( state -> target -> health_percentage() <= blood_in_the_water )
            td( state -> target ) -> dots.rip -> refresh_duration( 0 );

        if ( sabertooth_total > timespan_t::zero() )
          td( state -> target ) -> dots.rip -> extend_duration( sabertooth_total ); // TOCHECK: Sabertooth before or after BitW?
      }
    }
  }

  void consume_resource() override
  {
    // Extra energy consumption happens first.
    // In-game it happens before the skill even casts but let's not do that because its dumb.
    if ( result_is_hit( execute_state -> result ) )
    {
      player -> resource_loss( current_resource(), excess_energy );
      stats -> consume_resource( current_resource(), excess_energy );
    }

    cat_attack_t::consume_resource();
  }

  double action_multiplier() const override
  {
    double am = cat_attack_t::action_multiplier();

    am *= p() -> resources.current[ RESOURCE_COMBO_POINT ] / p() -> resources.max[ RESOURCE_COMBO_POINT ];

    am *= 1.0 + excess_energy / max_excess_energy;

    return am;
  }

  double composite_crit() const override
  {
    double c = cat_attack_t::composite_crit();

    if ( p() -> buff.feral_tier15_4pc -> check() )
      c += p() -> buff.feral_tier15_4pc -> data().effectN( 1 ).percent();

    return c;
  }

  double composite_crit_multiplier() const override
  {
    double cm = cat_attack_t::composite_crit_multiplier();

    if ( target -> debuffs.bleeding -> check() )
      cm *= 2.0;

    return cm;
  }
};

// Gushing Wound (tier17_feral_4pc) ========================================================

struct gushing_wound_t : public residual_action::residual_periodic_action_t<cat_attack_t>
{
  gushing_wound_t( druid_t* p ) :
    residual_action::residual_periodic_action_t<cat_attack_t>( "gushing_wound", p, p -> find_spell( 166638 ) )
  {
    background = dual = proc = true;
    consume_ooc = may_miss = may_dodge = may_parry = false;
    trigger_tier17_2pc = p -> sets.has_set_bonus( DRUID_FERAL, T17, B2 );
  }
};

// Maim =====================================================================
// FIXME: Deals totally incorrect damage.

struct maim_t : public cat_attack_t
{
  maim_t( druid_t* player, const std::string& options_str ) :
    cat_attack_t( "maim", player, player -> find_specialization_spell( "Maim" ), options_str )
  {
    base_costs[ RESOURCE_COMBO_POINT ] = 1;
    special          = true;
  }
};

// Rake =====================================================================

struct rake_t : public cat_attack_t
{
  const spell_data_t* bleed_spell;

  rake_t( druid_t* p, const std::string& options_str ) :
    cat_attack_t( "rake", p, p -> find_specialization_spell( "Rake" ), options_str )
  {
    special = true;
    attack_power_mod.direct = data().effectN( 1 ).ap_coeff();

    bleed_spell           = p -> find_spell( 155722 );
    base_td               = bleed_spell -> effectN( 1 ).base_value();
    attack_power_mod.tick = bleed_spell -> effectN( 1 ).ap_coeff();
    dot_duration          = bleed_spell -> duration();
    base_tick_time        = bleed_spell -> effectN( 1 ).period();

    trigger_tier17_2pc = p -> sets.has_set_bonus( DRUID_FERAL, T17, B2 );

    // 2015/09/01: Rake deals 20% less damage in PvP combat.
    if ( sim -> pvp_crit )
      base_multiplier *= 0.8;

    base_tick_time *= 1.0 + p -> talent.jagged_wounds -> effectN( 1 ).percent();
    dot_duration   *= 1.0 + p -> talent.jagged_wounds -> effectN( 2 ).percent();
  }

  /* Treat direct damage as "bleed"
     Must use direct damage because tick_zeroes cannot be blocked, and
     this attack can be blocked if the druid is in front of the target. */
  double composite_da_multiplier( const action_state_t* state ) const override
  {
    double dm = cat_attack_t::composite_da_multiplier( state );

    if ( p() -> mastery.razor_claws -> ok() )
      dm *= 1.0 + p() -> cache.mastery_value();

    return dm;
  }

  // Bleed damage penetrates armor.
  virtual double target_armor( player_t* ) const override
  { return 0.0; }

  virtual void impact( action_state_t* s ) override
  {
    cat_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
      p() -> resource_gain( RESOURCE_COMBO_POINT, combo_point_gain, p() -> gain.rake );
  }
};

// Rip ======================================================================

struct rip_t : public cat_attack_t
{
  struct rip_state_t : public action_state_t
  {
    int combo_points;
    druid_t* druid;

    rip_state_t( druid_t* p, action_t* a, player_t* target ) :
      action_state_t( a, target ), combo_points( 0 ), druid( p )
    { }

    void initialize() override
    {
      action_state_t::initialize();

      combo_points = (int) druid -> resources.current[ RESOURCE_COMBO_POINT ];
    }

    void copy_state( const action_state_t* state ) override
    {
      action_state_t::copy_state( state );
      const rip_state_t* rip_state = debug_cast<const rip_state_t*>( state );
      combo_points = rip_state -> combo_points;
    }

    std::ostringstream& debug_str( std::ostringstream& s ) override
    {
      action_state_t::debug_str( s );

      s << " combo_points=" << combo_points;

      return s;
    }
  };

  double ap_per_point;

  rip_t( druid_t* p, const std::string& options_str ) :
    cat_attack_t( "rip", p, p -> find_specialization_spell( "Rip" ), options_str ),
    ap_per_point( 0.0 )
  {
    base_costs[ RESOURCE_COMBO_POINT ] = 1;
    base_costs[ RESOURCE_ENERGY ] = 30.0; // FIXME

    ap_per_point = data().effectN( 1 ).ap_coeff();
    special      = true;
    may_crit     = false;
    dot_duration += player -> sets.set( SET_MELEE, T14, B4 ) -> effectN( 1 ).time_value();

    trigger_tier17_2pc = p -> sets.has_set_bonus( DRUID_FERAL, T17, B2 );

    // 2015/09/01: Rip deals 20% less damage in PvP combat.
    if ( sim -> pvp_crit )
      base_multiplier *= 0.8;

    base_tick_time *= 1.0 + p -> talent.jagged_wounds -> effectN( 1 ).percent();
    dot_duration   *= 1.0 + p -> talent.jagged_wounds -> effectN( 2 ).percent();
  }

  action_state_t* new_state() override
  { return new rip_state_t( p(), this, target ); }

  double attack_tick_power_coefficient( const action_state_t* s ) const override
  {
    /* FIXME: Does this even work correctly for tick_damage expression?
     probably just uses the CP of the Rip already on the target */
    rip_state_t* rip_state = debug_cast<rip_state_t*>( td( s -> target ) -> dots.rip -> state );

    return ap_per_point * rip_state -> combo_points;
  }
};

// Savage Roar ==============================================================

struct savage_roar_t : public cat_attack_t
{
  savage_roar_t( druid_t* p, const std::string& options_str ) :
    cat_attack_t( "savage_roar", p, p -> talent.savage_roar, options_str )
  {
    base_costs[ RESOURCE_COMBO_POINT ] = 1;
    base_costs[ RESOURCE_ENERGY ] = 25; // FIXME
    may_multistrike = may_crit = may_miss = harmful = false;
    dot_duration  = timespan_t::zero();
    base_tick_time = timespan_t::zero();

    // Does not consume Omen of Clarity. http://us.battle.net/wow/en/blog/16549669/603-patch-notes-10-27-2014
    consume_ooc = false;
  }

  /* We need a custom implementation of Pandemic refresh mechanics since our ready()
     method relies on having the correct final value. */
  timespan_t duration( int combo_points = -1 )
  {
    if ( combo_points == -1 )
      combo_points = (int) p() -> resources.current[ RESOURCE_COMBO_POINT ];
  
    timespan_t d = data().duration() + timespan_t::from_seconds( 6.0 ) * combo_points;

    // Maximum duration is 130% of the raw duration of the new Savage Roar.
    if ( p() -> buff.savage_roar -> check() )
      d += std::min( p() -> buff.savage_roar -> remains(), d * 0.3 );

    return d;
  }

  virtual void execute() override
  {
    // Grab duration before we go and spend all of our combo points.
    timespan_t d = duration();

    cat_attack_t::execute();

    p() -> buff.savage_roar -> trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, d );
  }

  virtual bool ready() override
  {
    // Savage Roar may not be cast if the new duration is less than that of the current
    if ( duration() < p() -> buff.savage_roar -> remains() )
      return false;
    
    return cat_attack_t::ready();
  }
};

// Shred ====================================================================

struct shred_t : public cat_attack_t
{
  shred_t( druid_t* p, const std::string& options_str ) :
    cat_attack_t( "shred", p, p -> find_specialization_spell( "Shred" ), options_str )
  {
    base_multiplier *= 1.0 + player -> sets.set( SET_MELEE, T14, B2 ) -> effectN( 1 ).percent();
    special = true;
  }

  virtual void execute() override
  {
    cat_attack_t::execute();

    if ( p() -> buff.feral_tier15_4pc -> up() )
      p() -> buff.feral_tier15_4pc -> decrement();

    // Track buff benefits
    if ( p() -> specialization() == DRUID_FERAL )
      p() -> buff.incarnation -> up();
  }

  virtual void impact( action_state_t* s ) override
  {
    cat_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      p() -> resource_gain( RESOURCE_COMBO_POINT, combo_point_gain, p() -> gain.shred );

      if ( s -> result == RESULT_CRIT && p() -> sets.has_set_bonus( DRUID_FERAL, PVP, B4 ) )
        td( s -> target ) -> buffs.bloodletting -> trigger(); // Druid module debuff
    }
  }

  virtual double composite_target_multiplier( player_t* t ) const override
  {
    double tm = cat_attack_t::composite_target_multiplier( t );

    if ( t -> debuffs.bleeding -> up() )
      tm *= 1.0 + p() -> spec.swipe -> effectN( 2 ).percent();

    return tm;
  }

  double composite_crit() const override
  {
    double c = cat_attack_t::composite_crit();

    if ( p() -> buff.feral_tier15_4pc -> up() )
      c += p() -> buff.feral_tier15_4pc -> data().effectN( 1 ).percent();

    return c;
  }

  double composite_crit_multiplier() const override
  {
    double cm = cat_attack_t::composite_crit_multiplier();

    if ( stealthed() )
      cm *= 2.0;

    return cm;
  }

  double action_multiplier() const override
  {
    double m = cat_attack_t::action_multiplier();

    if ( p() -> buff.feral_tier16_2pc -> up() )
      m *= 1.0 + p() -> buff.feral_tier16_2pc -> data().effectN( 1 ).percent();

    if ( stealthed() )
      m *= 1.0 + p() -> buff.prowl -> data().effectN( 4 ).percent();

    return m;
  }
};

// Swipe ==============================================================

struct swipe_t : public cat_attack_t
{
private:
  bool attackCritical;
public:
  swipe_t( druid_t* player, const std::string& options_str ) :
    cat_attack_t( "swipe", player, player -> spec.swipe, options_str ),
    attackCritical( false )
  {
    aoe = -1;
    combo_point_gain = data().effectN( 1 ).base_value(); // Effect is not labelled correctly as CP gain
  }

  virtual void impact( action_state_t* s ) override
  {
    cat_attack_t::impact( s );

    if ( s -> result == RESULT_CRIT )
      attackCritical = true;
  }

  virtual void execute() override
  {
    attackCritical = false;

    cat_attack_t::execute();
    
    if ( attackHit )
    {
      p() -> resource_gain( RESOURCE_COMBO_POINT, combo_point_gain, p() -> gain.swipe );
      if ( attackCritical && p() -> spell.primal_fury -> ok() )
      {
        p() -> proc.primal_fury -> occur();
        p() -> resource_gain( RESOURCE_COMBO_POINT, p() -> spell.primal_fury -> effectN( 1 ).base_value(), p() -> gain.primal_fury );
      }
    }

    p() -> buff.feral_tier16_2pc -> up();

    if ( p() -> buff.feral_tier15_4pc -> up() )
      p() -> buff.feral_tier15_4pc -> decrement();
  }

  double action_multiplier() const override
  {
    double m = cat_attack_t::action_multiplier();

    if ( p() -> buff.feral_tier16_2pc -> check() )
      m *= 1.0 + p() -> buff.feral_tier16_2pc -> data().effectN( 1 ).percent();

    return m;
  }

  virtual double composite_target_multiplier( player_t* t ) const override
  {
    double tm = cat_attack_t::composite_target_multiplier( t );

    if ( t -> debuffs.bleeding -> up() )
      tm *= 1.0 + data().effectN( 2 ).percent();

    return tm;
  }

  double composite_crit() const override
  {
    double c = cat_attack_t::composite_crit();

    if ( p() -> buff.feral_tier15_4pc -> check() )
      c += p() -> buff.feral_tier15_4pc -> data().effectN( 1 ).percent();

    return c;
  }

  virtual bool ready() override
  {
    if ( p() -> talent.bloody_slash -> ok() )
      return false;

    return cat_attack_t::ready();
  }
};

// Tiger's Fury =============================================================

struct tigers_fury_t : public cat_attack_t
{
  timespan_t duration;

  tigers_fury_t( druid_t* p, const std::string& options_str ) :
    cat_attack_t( "tigers_fury", p, p -> find_specialization_spell( "Tiger's Fury" ), options_str ),
    duration( p -> buff.tigers_fury -> buff_duration )
  {
    harmful = consume_ooc = may_miss = may_parry = may_dodge = may_crit = false;
    autoshift = form_mask = CAT_FORM;
    
    /* If Druid Tier 18 (WoD 6.2) trinket effect is in use, adjust Tiger's Fury duration
       based on spell data of the special effect. */
    if ( p -> wildcat_celerity )
      duration *= 1.0 + p -> wildcat_celerity -> driver() -> effectN( 1 ).average( p -> wildcat_celerity -> item ) / 100.0;
  }

  void execute() override
  {
    cat_attack_t::execute();

    p() -> buff.tigers_fury -> trigger( 1, buff_t::DEFAULT_VALUE(), 1.0, duration );

    p() -> resource_gain( RESOURCE_ENERGY,
                          data().effectN( 2 ).resource( RESOURCE_ENERGY ),
                          p() -> gain.tigers_fury );

    if ( p() -> sets.has_set_bonus( SET_MELEE, T13, B4 ) )
      p() -> buff.clearcasting -> trigger();

    if ( p() -> sets.has_set_bonus( SET_MELEE, T15, B4 ) )
      p() -> buff.feral_tier15_4pc -> trigger( 3 );

    if ( p() -> sets.has_set_bonus( SET_MELEE, T16, B4 ) )
      p() -> buff.feral_tier16_4pc -> trigger();
  }
};

// Thrash (Cat) =============================================================

struct thrash_cat_t : public cat_attack_t
{
  thrash_cat_t( druid_t* p, const std::string& options_str ) :
    cat_attack_t( "thrash_cat", p, p -> find_spell( 106830 ), options_str )
  {
    aoe                    = -1;
    spell_power_mod.direct = 0;

    trigger_tier17_2pc = p -> sets.has_set_bonus( DRUID_FERAL, T17, B2 );

    base_tick_time *= 1.0 + p -> talent.jagged_wounds -> effectN( 1 ).percent();
    dot_duration   *= 1.0 + p -> talent.jagged_wounds -> effectN( 2 ).percent();
  }

  // Treat direct damage as "bleed"
  double composite_da_multiplier( const action_state_t* state ) const override
  {
    double m = cat_attack_t::composite_da_multiplier( state );

    if ( p() -> mastery.razor_claws -> ok() )
      m *= 1.0 + p() -> cache.mastery_value();

    return m;
  }

  // Treat direct damage as "bleed"
  virtual double target_armor( player_t* ) const override
  { return 0.0; }
};

// Flurry of Xuen (Fen-yu Legendary Cloak proc) =============================

struct flurry_of_xuen_t : public cat_attack_t
{
  flurry_of_xuen_t( druid_t* p ) :
    cat_attack_t( "flurry_of_xuen", p, p -> find_spell( 147891 ) )
  {
    special = may_miss = may_parry = may_block = may_dodge = may_crit = background = true;
    proc = false;
    aoe = 5;
  }
};

// Mark of the Shattered Hand ===============================================

struct shattered_bleed_t : public cat_attack_t
{
  shattered_bleed_t( druid_t* p ):
    cat_attack_t( "shattered_bleed", p, p -> find_spell( 159238 ) )
    {
      hasted_ticks = false; background = true; callbacks = false; special = true;
      may_miss = may_block = may_dodge = may_parry = false; may_crit = true;
      tick_may_crit = false;
    }

    void init() override
    {
      cat_attack_t::init();

      snapshot_flags |= STATE_MUL_TA;
    }

    double target_armor( player_t* ) const override
    { return 0.0; }

    timespan_t calculate_dot_refresh_duration( const dot_t* dot, timespan_t triggered_duration ) const override
    {
      timespan_t new_duration = std::min( triggered_duration * 0.3, dot -> remains() ) + triggered_duration;
      timespan_t period_mod = new_duration % base_tick_time;
      new_duration += ( base_tick_time - period_mod );

      return new_duration;
    }
    
    // Override to prevent benefitting from mastery.
    double composite_ta_multiplier( const action_state_t* /*s*/ ) const override
    { return action_multiplier(); }
};

} // end namespace cat_attacks

namespace bear_attacks {

// ==========================================================================
// Druid Bear Attack
// ==========================================================================

struct bear_attack_t : public druid_attack_t<melee_attack_t>
{
public:
  double rage_amount, rage_tick_amount;

  bear_attack_t( const std::string& n, druid_t* p,
                 const spell_data_t* s = spell_data_t::nil() ) :
    base_t( n, p, s ), rage_amount( 0.0 ), 
    rage_tick_amount( 0.0 ),  rage_gain( p -> get_gain( name() ) )
  {}

  virtual timespan_t gcd() const override
  {
    if ( p() -> specialization() != DRUID_GUARDIAN )
      return base_t::gcd();

    timespan_t t = base_t::gcd();

    if ( t == timespan_t::zero() )
      return timespan_t::zero();

    t *= player -> cache.attack_haste();

    if ( t < min_gcd )
      t = min_gcd;

    return t;
  }

  virtual double cooldown_reduction() const override
  {
    double cdr = base_t::cooldown_reduction();

    cdr *= p() -> cache.attack_haste();

    return cdr;
  }

  virtual void impact( action_state_t* s ) override
  {
    base_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      p() -> resource_gain( RESOURCE_RAGE, rage_amount, rage_gain );

      if ( p() -> spell.primal_fury -> ok() && s -> target == target && s -> result == RESULT_CRIT ) // Only trigger from primary target
      {
        p() -> resource_gain( RESOURCE_RAGE,
                              p() -> spell.primal_fury -> effectN( 1 ).resource( RESOURCE_RAGE ),
                              p() -> gain.primal_fury );
        p() -> proc.primal_fury -> occur();
      }

      if ( p() -> talent.galactic_guardian -> ok() )
        trigger_galactic_guardian( s );
    }
  }

  virtual void tick( dot_t* d ) override
  {
    base_t::tick( d );

    if ( result_is_hit( d -> state -> result ) )
    {
      p() -> resource_gain( RESOURCE_RAGE, rage_tick_amount, rage_gain );

      if ( p() -> talent.galactic_guardian -> ok() )
        trigger_galactic_guardian( d -> state );
    }
  }

  virtual void trigger_galactic_guardian( action_state_t* s ) // TOCHECK: does it proc off of ANY damage event? or just bear attacks
  {
    if ( s -> result_total <= 0 )
      return;

    if ( rng().roll( p() -> talent.galactic_guardian -> effectN( 1 ).percent() ) )
    {
      p() -> active.galactic_guardian -> target = s -> target;
      p() -> active.galactic_guardian -> execute();
    }
  }
private:
  gain_t* rage_gain;
}; // end bear_attack_t

// Bear Melee Attack ========================================================

struct bear_melee_t : public bear_attack_t
{
  bear_melee_t( druid_t* player ) :
    bear_attack_t( "bear_melee", player )
  {
    form_mask = BEAR_FORM;

    school      = SCHOOL_PHYSICAL;
    may_glance  = background = repeating = true;
    trigger_gcd = timespan_t::zero();
    special     = false;

    rage_amount = 5.0;
  }

  virtual timespan_t execute_time() const override
  {
    if ( ! player -> in_combat )
      return timespan_t::from_seconds( 0.01 );

    return bear_attack_t::execute_time();
  }
};

// Growl  ===================================================================

struct growl_t: public bear_attack_t
{
  growl_t( druid_t* player, const std::string& options_str ):
    bear_attack_t( "growl", player, player -> find_class_spell( "Growl" ) )
  {
    parse_options( options_str );

    ignore_false_positive = true;
    may_miss = may_parry = may_dodge = may_block = may_crit = false;
    use_off_gcd = true;
  }

  void update_ready( timespan_t ) override
  {
    timespan_t cd = cooldown -> duration;

    if ( p() -> buff.incarnation -> check() && p() -> specialization() == DRUID_GUARDIAN )
      cd = timespan_t::zero();

    bear_attack_t::update_ready( cd );
  }

  void impact( action_state_t* s ) override
  {
    if ( s -> target -> is_enemy() )
      target -> taunt( player );

    bear_attack_t::impact( s );
  }
};

// Ironfur ==================================================================

struct ironfur_t : public bear_attack_t
{
  ironfur_t( druid_t* p, const std::string& options_str ) :
    bear_attack_t( "ironfur", p, p -> spec.ironfur )
  {
    parse_options( options_str );

    use_off_gcd = true;
    harmful = may_miss = may_parry = may_dodge = may_crit = false;
  }

  virtual void execute() override
  {
    bear_attack_t::execute();

    for ( size_t i = 0; i < 9; i++ )
    {
      if ( ! p() -> buff.ironfur_stack[ i ] -> check() )
      {
        p() -> buff.ironfur_stack[ i ] -> trigger();
        return;
      }
    }

    assert( "No ironfur_stack instance found to trigger!" );
  }
};

// Lacerate DoT =============================================================

struct lacerate_dot_t : public bear_attack_t
{
  double blood_frenzy_amount;

  lacerate_dot_t( druid_t* p ) :
    bear_attack_t( "lacerate_dot", p, p -> find_spell( 192090 ) ),
    blood_frenzy_amount( 0.0 )
  {
    may_miss = may_block = may_dodge = may_parry = may_crit = false;

    if ( p -> talent.blood_frenzy -> ok() )
      blood_frenzy_amount = p -> find_spell( 203961 ) -> effectN( 1 ).resource( RESOURCE_RAGE );
  }

  virtual void impact( action_state_t* state ) override
  {
    bear_attack_t::impact( state );

    if ( result_is_hit( state -> result ) && td( state -> target ) -> lacerate_stack < 3 )
      td( state -> target ) -> lacerate_stack++;
  }

  virtual double composite_target_ta_multiplier( player_t* t ) const override
  {
    double tm = bear_attack_t::composite_target_ta_multiplier( t );

    tm *= td( t ) -> lacerate_stack;

    return tm;
  }

  virtual void tick( dot_t* d ) override
  {
    rage_tick_amount = td( d -> state -> target ) -> lacerate_stack * blood_frenzy_amount;
    
    bear_attack_t::tick( d );
  }

  virtual void last_tick( dot_t* d ) override
  {
    bear_attack_t::last_tick( d );

    td( target ) -> lacerate_stack = 0;
  }
};

// Lacerate =================================================================

struct lacerate_t : public bear_attack_t
{
  lacerate_dot_t* dot;

  lacerate_t( druid_t* p, const std::string& options_str ) :
    bear_attack_t( "lacerate", p, p -> find_specialization_spell( "Lacerate" ) ),
    dot( new lacerate_dot_t( p ) )
  {
    parse_options( options_str );

    add_child( dot );
  }

  virtual void impact( action_state_t* state ) override
  {
    bear_attack_t::impact( state );

    if ( result_is_hit( state -> result ) )
    {
      dot -> target = target;
      dot -> execute();

      if ( rng().roll( 0.25 ) ) // FIXME: Find in spell data.
        p() -> cooldown.mangle -> reset( true );
    }
  }

  // Treat direct damage as "bleed"
  virtual double target_armor( player_t* ) const override
  { return 0.0; }
};

// Mangle ============================================================

struct mangle_t : public bear_attack_t
{
  double bleeding_multiplier;

  mangle_t( druid_t* player, const std::string& options_str ) :
    bear_attack_t( "mangle", player, player -> find_class_spell( "Mangle" ) )
  {
    parse_options( options_str );

    bleeding_multiplier = data().effectN( 3 ).percent();
    rage_amount = data().effectN( 4 ).resource( RESOURCE_RAGE );

    if ( p() -> specialization() == DRUID_GUARDIAN )
    {
      rage_amount += player -> talent.soul_of_the_forest -> effectN( 1 ).resource( RESOURCE_RAGE );
      base_multiplier *= 1.0 + player -> talent.soul_of_the_forest -> effectN( 2 ).percent();
      base_crit += p() -> spec.dream_of_cenarius -> effectN( 3 ).percent();
    }
  }

  void update_ready( timespan_t ) override
  {
    timespan_t cd = cooldown -> duration;

    if ( p() -> buff.incarnation -> check() && p() -> specialization() == DRUID_GUARDIAN )
      cd = timespan_t::zero();

    bear_attack_t::update_ready( cd );
  }

  virtual double composite_target_multiplier( player_t* t ) const
  {
    double tm = bear_attack_t::composite_target_multiplier( t );

    if ( t -> debuffs.bleeding -> check() )
      tm *= 1.0 + bleeding_multiplier;

    return tm;
  }

  virtual void execute() override
  {
    int base_aoe = aoe;
    if ( p() -> buff.incarnation -> up() && p() -> specialization() == DRUID_GUARDIAN )
      aoe = p() -> talent.incarnation_bear -> effectN( 4 ).base_value();

    bear_attack_t::execute();

    aoe = base_aoe;
  }

  virtual void impact( action_state_t* s ) override
  {
    bear_attack_t::impact( s );

    if ( p() -> spec.dream_of_cenarius -> ok() && s -> result == RESULT_CRIT )
      p() -> buff.dream_of_cenarius -> trigger();
  }
};

// Maul =====================================================================

struct maul_t : public bear_attack_t
{ 
  maul_t( druid_t* player, const std::string& options_str ) :
    bear_attack_t( "maul", player, player -> find_specialization_spell( "Maul" ) )
  {
    parse_options( options_str );
    weapon = &( player -> main_hand_weapon );
    normalize_weapon_speed = false; // Legion TOCHECK

    use_off_gcd = true;
  }

  virtual void update_ready( timespan_t ) override
  {
    timespan_t cd = cooldown -> duration;

    if ( p() -> buff.incarnation -> check() && p() -> specialization() == DRUID_GUARDIAN )
      cd = timespan_t::zero();

    bear_attack_t::update_ready( cd );
  }
};

// Pulverize ================================================================

struct pulverize_t : public bear_attack_t
{
  pulverize_t( druid_t* player, const std::string& options_str ) :
    bear_attack_t( "pulverize", player, player -> talent.pulverize )
  {
    parse_options( options_str );

    normalize_weapon_speed = false; // TOCHECK
  }

  virtual void impact( action_state_t* s ) override
  {
    bear_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      // consumes 3 stacks of Lacerate on the target
      s -> target -> find_dot( "lacerate_dot", player ) -> cancel();

      // and reduce damage taken by x% for y sec.
      p() -> buff.pulverize -> trigger();
    }
  }

  virtual bool ready() override
  {
    // Call bear_attack_t::ready() first for proper targeting support.
    if ( bear_attack_t::ready() && td( target ) -> lacerate_stack >= 3 )
      return true;
    else
      return false;
  }
};

// Thrash (Bear) ============================================================

struct thrash_bear_t : public bear_attack_t
{
  lacerate_dot_t* dot;

  thrash_bear_t( druid_t* player, const std::string& options_str ) :
    bear_attack_t( "thrash_bear", player, player -> find_spell( 77758 ) ),
    dot( new lacerate_dot_t( player ) )
  {
    parse_options( options_str );
    aoe                    = -1;
    spell_power_mod.direct = 0;

    // Apply hidden passive damage multiplier
    base_dd_multiplier *= 1.0 + player -> spec.guardian_passive -> effectN( 6 ).percent();
  }

  virtual void impact( action_state_t* s ) override
  {
    bear_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      dot -> target = s -> target;
      dot -> execute();

      td( s -> target ) -> dots.thrash_cat -> cancel(); // Legion TOCHECK
    }
  }

  // Treat direct damage as "bleed"
  virtual double target_armor( player_t* ) const override
  { return 0.0; }
};

} // end namespace bear_attacks

namespace heals {

// ==========================================================================
// Druid Heal
// ==========================================================================

// Cenarion Ward ============================================================

struct cenarion_ward_hot_t : public druid_heal_t
{
  cenarion_ward_hot_t( druid_t* p ) :
    druid_heal_t( "cenarion_ward_hot", p, p -> find_spell( 102352 ) )
  {
    harmful = false;
    background = proc = true;
    target = p;
    hasted_ticks = false;
  }

  virtual void execute() override
  {
    heal_t::execute();

    static_cast<druid_t*>( player ) -> buff.cenarion_ward -> expire();
  }
};

struct cenarion_ward_t : public druid_heal_t
{
  cenarion_ward_t( druid_t* p, const std::string& options_str ) :
    druid_heal_t( "cenarion_ward", p, p -> talent.cenarion_ward,  options_str )
  {}

  virtual void execute() override
  {
    druid_heal_t::execute();

    p() -> buff.cenarion_ward -> trigger();
  }

  virtual bool ready()
  {
    if ( target != player )
    {
      assert( "Cenarion Ward will not trigger on other players!" );
      return false;
    }

    return druid_heal_t::ready();
  }
};

// Living Seed ==============================================================

struct living_seed_t : public druid_heal_t
{
  living_seed_t( druid_t* player ) :
    druid_heal_t( "living_seed", player, player -> find_specialization_spell( "Living Seed" ) )
  {
    background = true;
    may_crit   = false;
    proc       = true;
    school     = SCHOOL_NATURE;
  }

  double composite_da_multiplier( const action_state_t* ) const override
  {
    return data().effectN( 1 ).percent();
  }
};

void druid_heal_t::init_living_seed()
{
  if ( p() -> specialization() == DRUID_RESTORATION )
    living_seed = new living_seed_t( p() );
}

// Frenzied Regeneration ====================================================
// Legion TOCHECK: How do multiple frenzied regenerations interact?

struct frenzied_regeneration_t : public druid_heal_t
{
  double heal_pct;
  timespan_t time_window;
  double min_pct;
  double num_ticks;
  double last_total_healing;

  frenzied_regeneration_t( druid_t* p, const std::string& options_str ) :
    druid_heal_t( "frenzied_regeneration", p, p -> find_specialization_spell( "Frenzied Regeneration"), options_str )
  {
    use_off_gcd = true;
    hasted_ticks = false;
    may_crit = tick_may_crit = false;
    dot_behavior = DOT_CLIP;

    heal_pct = data().effectN( 2 ).percent();
    time_window = timespan_t::from_seconds( data().effectN( 3 ).base_value() );
    min_pct = data().effectN( 4 ).percent();
    num_ticks = dot_duration / base_tick_time;

    p -> cooldown.frenzied_regen_use -> duration = cooldown -> duration;
    cooldown -> charges = 2;
    cooldown -> duration = timespan_t::from_seconds( 20.0 );

    base_multiplier *= 1.0 + p -> talent.guardian_of_elune -> effectN( 2 ).percent();
  }

  virtual void execute()
  {
    double total_healing = p() -> compute_incoming_damage( time_window );

    if ( find_dot( target ) -> is_ticking() )
      total_healing += last_total_healing * ( find_dot( target ) -> ticks_left() / num_ticks );

    last_total_healing = std::max( total_healing, p() -> resources.max[ RESOURCE_HEALTH ] * min_pct );
    base_td = last_total_healing / num_ticks;

    druid_heal_t::execute();

    p() -> cooldown.frenzied_regen_use -> start();
  }

  virtual bool ready()
  {
    if ( p() -> cooldown.frenzied_regen_use -> down() )
      return false;

    return druid_heal_t::ready();
  }
};

// Healing Touch ============================================================

struct healing_touch_t : public druid_heal_t
{
  healing_touch_t( druid_t* p, const std::string& options_str ) :
    druid_heal_t( "healing_touch", p, p -> find_class_spell( "Healing Touch" ), options_str )
  {
    form_mask = NO_FORM | MOONKIN_FORM; // DBC has no mask

    init_living_seed();
    ignore_false_positive = true; // Prevents cat/bear from failing a skill check and going into caster form.

    // redirect to self if not specified
    if ( target -> is_enemy() || ( target -> type == HEALING_ENEMY && p -> specialization() == DRUID_GUARDIAN ) )
      target = p;
    
    if ( p -> spec.dream_of_cenarius -> ok() && p -> specialization() == DRUID_GUARDIAN )
      attack_power_mod.direct = spell_power_mod.direct;
  }

  double spell_direct_power_coefficient( const action_state_t* /* state */ ) const override
  {
    return spell_power_mod.direct * ! p() -> buff.dream_of_cenarius -> check();
  }

  double attack_direct_power_coefficient( const action_state_t* /* state */ ) const override
  {
    return attack_power_mod.direct * p() -> buff.dream_of_cenarius -> check();
  }

  virtual double cost() const override
  {
    if ( p() -> buff.predatory_swiftness -> check() )
      return 0;

    if ( p() -> buff.dream_of_cenarius -> check() )
      return 0;

    return druid_heal_t::cost();
  }

  virtual void consume_resource() override
  {
    // Prevent from consuming Omen of Clarity unnecessarily
    if ( p() -> buff.predatory_swiftness -> check() )
      return;

    druid_heal_t::consume_resource();
  }

  virtual double action_da_multiplier() const override
  {
    double adm = druid_heal_t::action_da_multiplier();

    if ( p() -> spec.dream_of_cenarius -> ok() ) {
      if ( p() -> specialization() == DRUID_FERAL || p() -> specialization() == DRUID_BALANCE )
        adm *= 1.0 + p() -> spec.dream_of_cenarius -> effectN( 1 ).percent();
      else if ( p() -> specialization() == DRUID_GUARDIAN )
        adm *= 1.0 + p() -> spec.dream_of_cenarius -> effectN( 2 ).percent();
    }

    return adm;
  }

  virtual timespan_t execute_time() const override
  {
    if ( p() -> buff.predatory_swiftness -> check() || p() -> buff.dream_of_cenarius -> up() )
      return timespan_t::zero();

    return druid_heal_t::execute_time();
  }

  virtual void impact( action_state_t* state ) override
  {
    druid_heal_t::impact( state );
    
    if ( result_is_hit( state -> result ) )
    {
      trigger_lifebloom_refresh( state );

      if ( state -> result == RESULT_CRIT )
        trigger_living_seed( state );
    }
  }

  virtual bool check_form_restriction() override
  {
    if ( p() -> buff.predatory_swiftness -> check() )
      return true;

    return druid_heal_t::check_form_restriction();
  }

  virtual void execute() override
  {
    druid_heal_t::execute();

    if ( p() -> talent.bloodtalons -> ok() )
      p() -> buff.bloodtalons -> trigger( 2 );

    p() -> buff.predatory_swiftness -> expire();
    p() -> buff.dream_of_cenarius -> expire();
  }

  virtual timespan_t gcd() const override
  {
    const druid_t& p = *this -> p();
    if ( p.buff.cat_form -> check() )
      if ( timespan_t::from_seconds( 1.0 ) < druid_heal_t::gcd() )
        return timespan_t::from_seconds( 1.0 );

    return druid_heal_t::gcd();
  }
};

// Lifebloom ================================================================

struct lifebloom_bloom_t : public druid_heal_t
{
  lifebloom_bloom_t( druid_t* p ) :
    druid_heal_t( "lifebloom_bloom", p, p -> find_class_spell( "Lifebloom" ) )
  {
    background       = true;
    dual             = true;
    dot_duration        = timespan_t::zero();
    base_td          = 0;
    attack_power_mod.tick   = 0;
  }

  virtual double composite_target_multiplier( player_t* target ) const override
  {
    double ctm = druid_heal_t::composite_target_multiplier( target );

    ctm *= td( target ) -> buffs.lifebloom -> check();

    return ctm;
  }
};

struct lifebloom_t : public druid_heal_t
{
  lifebloom_bloom_t* bloom;

  lifebloom_t( druid_t* p, const std::string& options_str ) :
    druid_heal_t( "lifebloom", p, p -> find_class_spell( "Lifebloom" ), options_str ),
    bloom( new lifebloom_bloom_t( p ) )
  {
    may_crit   = false;
    ignore_false_positive = true;
  }

  virtual void impact( action_state_t* state ) override
  {
    // Cancel Dot/td-buff on all targets other than the one we impact on
    for ( size_t i = 0; i < sim -> actor_list.size(); ++i )
    {
      player_t* t = sim -> actor_list[ i ];
      if ( state -> target == t )
        continue;
      get_dot( t ) -> cancel();
      td( t ) -> buffs.lifebloom -> expire();
    }

    druid_heal_t::impact( state );

    if ( result_is_hit( state -> result ) )
      td( state -> target ) -> buffs.lifebloom -> trigger();
  }

  virtual void last_tick( dot_t* d ) override
  {
    if ( ! d -> state -> target -> is_sleeping() ) // Prevent crash at end of simulation
      bloom -> execute();
    td( d -> state -> target ) -> buffs.lifebloom -> expire();

    druid_heal_t::last_tick( d );
  }

  virtual void tick( dot_t* d ) override
  {
    druid_heal_t::tick( d );

    trigger_clearcasting();
  }
};

// Regrowth =================================================================

struct regrowth_t : public druid_heal_t
{
  regrowth_t( druid_t* p, const std::string& options_str ) :
    druid_heal_t( "regrowth", p, p -> find_class_spell( "Regrowth" ), options_str )
  {
    base_crit += 0.60;

    if ( p -> glyph.regrowth -> ok() )
    {
      base_crit += p -> glyph.regrowth -> effectN( 1 ).percent();
      dot_duration  = timespan_t::zero();
    }

    ignore_false_positive = true;
    init_living_seed();
  }

  virtual double cost() const override
  {
    if ( p() -> buff.clearcasting -> check() )
      return 0;

    return druid_heal_t::cost();
  }

  virtual void consume_resource() override
  {
    druid_heal_t::consume_resource();
    double c = druid_heal_t::cost();

    if ( c > 0 && p() -> buff.clearcasting -> up() )
    {
      // Treat the savings like a mana gain for tracking purposes.
      p() -> gain.clearcasting -> add( RESOURCE_MANA, c );
      p() -> buff.clearcasting -> decrement();
    }
  }

  virtual void impact( action_state_t* state ) override
  {
    druid_heal_t::impact( state );

    if ( result_is_hit( state -> result ) )
    {
      trigger_lifebloom_refresh( state );

      if ( state -> result == RESULT_CRIT )
        trigger_living_seed( state );
    }
  }

  virtual timespan_t execute_time() const override
  {
    if ( p() -> buff.incarnation -> check() && p() -> specialization() == DRUID_RESTORATION )
      return timespan_t::zero();

    return druid_heal_t::execute_time();
  }
};

// Rejuvenation =============================================================

struct rejuvenation_t : public druid_heal_t
{
  rejuvenation_t( druid_t* p, const std::string& options_str ) :
    druid_heal_t( "rejuvenation", p, p -> find_class_spell( "Rejuvenation" ), options_str )
  {
    tick_zero = true;
  }

  virtual double action_ta_multiplier() const override
  {
    double atm = base_t::action_ta_multiplier();

    if ( p() -> spec.dream_of_cenarius -> ok() && p() -> specialization() == DRUID_FERAL )
        atm *= 1.0 + p() -> spec.dream_of_cenarius -> effectN( 2 ).percent();

    return atm;
  }
};

// Renewal ============================================================

struct renewal_t : public druid_heal_t
{
  renewal_t( druid_t* p, const std::string& options_str ) :
    druid_heal_t( "renewal", p, p -> find_talent_spell( "Renewal" ), options_str )
  {
    may_crit = false;
    may_multistrike = 0;
  }

  virtual void init() override
  {
    druid_heal_t::init();

    snapshot_flags &= ~STATE_RESOLVE; // Is not affected by resolve.
  }

  virtual void execute() override
  {
    base_dd_min = p() -> resources.max[ RESOURCE_HEALTH ] * data().effectN( 1 ).percent();

    druid_heal_t::execute();
  }
};

// Swiftmend ================================================================

// TODO: in game, you can swiftmend other druids' hots, which is not supported here
struct swiftmend_t : public druid_heal_t
{
  struct swiftmend_aoe_heal_t : public druid_heal_t
  {
    swiftmend_aoe_heal_t( druid_t* p, const spell_data_t* s ) :
      druid_heal_t( "swiftmend_aoe", p, s )
    {
      aoe            = 3;
      background     = true;
      base_tick_time = timespan_t::from_seconds( 1.0 );
      hasted_ticks   = true;
      may_crit       = false;
      proc           = true;
      tick_may_crit  = false;
    }
  };

  swiftmend_aoe_heal_t* aoe_heal;

  swiftmend_t( druid_t* p, const std::string& options_str ) :
    druid_heal_t( "swiftmend", p, p -> find_class_spell( "Swiftmend" ), options_str ),
    aoe_heal( new swiftmend_aoe_heal_t( p, &data() ) )
  {
    init_living_seed();
  }

  virtual void impact( action_state_t* state ) override
  {
    druid_heal_t::impact( state );

    if ( result_is_hit( state -> result ) )
    {
      if ( state -> result == RESULT_CRIT )
        trigger_living_seed( state );

      if ( p() -> talent.soul_of_the_forest -> ok() )
        p() -> buff.soul_of_the_forest -> trigger();

      aoe_heal -> execute();
    }
  }

  virtual bool ready() override
  {
    player_t* t = ( execute_state ) ? execute_state -> target : target;

    // Note: with the glyph you can use other people's regrowth/rejuv
    if ( ! ( td( t ) -> dots.regrowth -> is_ticking() ||
             td( t ) -> dots.rejuvenation -> is_ticking() ) )
      return false;

    return druid_heal_t::ready();
  }
};

// Tranquility ==============================================================

struct tranquility_t : public druid_heal_t
{
  tranquility_t( druid_t* p, const std::string& options_str ) :
    druid_heal_t( "tranquility", p, p -> find_specialization_spell( "Tranquility" ), options_str )
  {
    aoe               = data().effectN( 3 ).base_value(); // Heals 5 targets
    base_execute_time = data().duration();
    channeled         = true;

    // Healing is in spell effect 1
    parse_spell_data( ( *player -> dbc.spell( data().effectN( 1 ).trigger_spell_id() ) ) );
  }
};

// Wild Growth ==============================================================

struct wild_growth_t : public druid_heal_t
{
  wild_growth_t( druid_t* p, const std::string& options_str ) :
    druid_heal_t( "wild_growth", p, p -> find_class_spell( "Wild Growth" ), options_str )
  {
    ignore_false_positive = true;
  }

  virtual void execute() override
  {
    int save = aoe;
    if ( p() -> buff.incarnation -> check() && p() -> specialization() == DRUID_RESTORATION )
      aoe += 2;

    druid_heal_t::execute();

    // Reset AoE
    aoe = save;
  }
};

// Ysera's Gift ==============================================================

struct yseras_tick_t : public druid_heal_t
{
  yseras_tick_t( druid_t* p ) :
    druid_heal_t( "yseras_gift", p, p -> find_spell( 145110 ) )
  {
    may_crit = false;
    background = dual = true;
  }

  virtual void init() override
  {
    druid_heal_t::init();

    snapshot_flags &= ~STATE_RESOLVE; // Is not affected by resolve.
    // TODO: What other multipliers does this not scale with?
  }

  virtual double action_multiplier() const override
  {
    double am = druid_heal_t::action_multiplier();

    if ( p() -> buff.bear_form -> check() )
      am *= 1.0 + p() -> buff.bear_form -> data().effectN( 10 ).percent();

    return am;
  }

  virtual void execute()
  {
    if ( p() -> health_percentage() < 100 )
      target = p();
    else
      target = smart_target();

    druid_heal_t::execute();
  }
};

} // end namespace heals

namespace spells {

// ==========================================================================
// Druid Spells
// ==========================================================================

// Auto Attack ==============================================================

struct auto_attack_t : public melee_attack_t
{
  auto_attack_t( druid_t* player, const std::string& options_str ) :
    melee_attack_t( "auto_attack", player, spell_data_t::nil() )
  {
    parse_options( options_str );

    trigger_gcd = timespan_t::zero();
    ignore_false_positive = true;
    use_off_gcd = true;
  }

  virtual void execute() override
  {
    player -> main_hand_attack -> weapon = &( player -> main_hand_weapon );
    player -> main_hand_attack -> base_execute_time = player -> main_hand_weapon.swing_time;
    player -> main_hand_attack -> schedule_execute();
  }

  virtual bool ready() override
  {
    if ( player -> is_moving() )
      return false;

    if ( ! player -> main_hand_attack )
      return false;

    return( player -> main_hand_attack -> execute_event == nullptr ); // not swinging
  }
};

// Astral Communion =========================================================

struct astral_communion_t : public druid_spell_t
{
  astral_communion_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "astral_communion", player, player -> talent.astral_communion, options_str )
  {
    harmful = false;
    ap_per_cast = data().effectN( 1 ).resource( RESOURCE_ASTRAL_POWER );
  }
};

// Barkskin =================================================================

struct barkskin_t : public druid_spell_t
{
  barkskin_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "barkskin", player, player -> find_specialization_spell( "Barkskin" ), options_str )
  {
    harmful = false;
    use_off_gcd = true;

    cooldown -> duration *= 1.0 + player -> talent.survival_of_the_fittest -> effectN( 1 ).percent();
  }

  void execute() override
  {
    druid_spell_t::execute();

    p() -> buff.barkskin -> trigger();
  }
};

// Bear Form Spell ==========================================================

struct bear_form_t : public druid_spell_t
{
  bear_form_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "bear_form", player, player -> find_class_spell( "Bear Form" ), options_str )
  {
    form_mask = NO_FORM | CAT_FORM | MOONKIN_FORM;
    may_autounshift = false;

    harmful = false;
    min_gcd = timespan_t::from_seconds( 1.5 );
    ignore_false_positive = true;

    if ( ! player -> bear_melee_attack )
    {
      player -> init_beast_weapon( player -> bear_weapon, 2.5 );
      player -> bear_melee_attack = new bear_attacks::bear_melee_t( player );
    }
  }

  void execute() override
  {
    druid_spell_t::execute();

    p() -> shapeshift( BEAR_FORM );
  }
};

// Blessing of An'she =============================================================

struct blessing_of_anshe_t : public druid_spell_t
{
  blessing_of_anshe_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "blessing_of_anshe", player, player -> spell.blessing_of_anshe )
  {
    parse_options( options_str );

    harmful = may_crit = false;
    ignore_false_positive = true;
  }

  void execute() override
  {
    druid_spell_t::execute();

    p() -> buff.blessing_of_elune -> expire();
    p() -> buff.blessing_of_anshe -> start();
  }

  virtual bool ready() override
  {
    if ( ! p() -> talent.blessing_of_the_ancients -> ok() )
      return false;
    if ( p() -> buff.blessing_of_anshe -> check() )
      return false;

    return druid_spell_t::ready();
  }
};

// Blessing of Elune =============================================================

struct blessing_of_elune_t : public druid_spell_t
{
  blessing_of_elune_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "blessing_of_elune", player, player -> spell.blessing_of_elune )
  {
    parse_options( options_str );

    harmful = false;
    ignore_false_positive = true;
  }

  void execute() override
  {
    druid_spell_t::execute();

    p() -> buff.blessing_of_anshe -> expire();
    p() -> buff.blessing_of_elune -> start();
  }

  virtual bool ready() override
  {
    if ( ! p() -> talent.blessing_of_the_ancients -> ok() )
      return false;
    if ( p() -> buff.blessing_of_elune -> check() )
      return false;

    return druid_spell_t::ready();
  }
};

// Bristling Fur Spell ======================================================

struct bristling_fur_t : public druid_spell_t
{
  bristling_fur_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "bristling_fur", player, player -> talent.bristling_fur, options_str  )
  {
    harmful = false;
    use_off_gcd = true;
  }

  void execute() override
  {
    druid_spell_t::execute();

    p() -> buff.bristling_fur -> trigger();
  }
};

// Cat Form Spell ===========================================================

struct cat_form_t : public druid_spell_t
{
  cat_form_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "cat_form", player, player -> find_class_spell( "Cat Form" ), options_str )
  {
    form_mask = NO_FORM | BEAR_FORM | MOONKIN_FORM;
    may_autounshift = false;

    harmful = false;
    min_gcd = timespan_t::from_seconds( 1.5 );
    ignore_false_positive = true;

    if ( ! player -> cat_melee_attack )
    {
      player -> init_beast_weapon( player -> cat_weapon, 1.0 );
      player -> cat_melee_attack = new cat_attacks::cat_melee_t( player );
    }
  }

  void execute() override
  {
    druid_spell_t::execute();

    p() -> shapeshift( CAT_FORM );
  }
};


// Celestial Alignment ======================================================

struct celestial_alignment_t : public druid_spell_t
{
  celestial_alignment_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "celestial_alignment", player, player -> spec.celestial_alignment , options_str )
  {
    parse_options( options_str );

    cooldown = player -> cooldown.celestial_alignment;
    harmful = false;
    dot_duration = timespan_t::zero();
  }

  void execute() override
  {
    druid_spell_t::execute(); // Do not change the order here.

    p() -> buff.celestial_alignment -> trigger();
  }

  virtual bool ready() override
  {
    if ( p() -> talent.collapsing_stars -> ok() )
      return false;

    return druid_spell_t::ready();
  }
};

// Collapsing Stars =========================================================

struct collapsing_stars_t : public druid_spell_t
{
  collapsing_stars_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "collapsing_stars", player, player -> talent.collapsing_stars )
  {
    parse_options( options_str );

    dot_duration = sim -> expected_iteration_time > timespan_t::zero() ?
      2 * sim -> expected_iteration_time :
      2 * sim -> max_time * ( 1.0 + sim -> vary_combat_length ); // "infinite" duration

    // Tick cost is proportional to base tick time.
    base_costs_per_tick[ RESOURCE_ASTRAL_POWER ] *= base_tick_time.total_seconds();
  }

  timespan_t cost_tick_time( const dot_t& d ) const override
  {
    // Consumes cost each time DoT ticks.
    return d.time_to_next_tick();
  }

  void impact( action_state_t* s ) override
  {
    bool refresh = get_dot( s -> target ) -> is_ticking();

    druid_spell_t::impact( s );

    if ( result_is_hit( s -> result ) && ! refresh )
      p() -> buff.collapsing_stars_up -> trigger();
  }

  void cancel() override
  {
    druid_spell_t::cancel();

    if ( dot_t* dot = find_dot( target ) )
      dot -> cancel();

    p() -> buff.collapsing_stars_up -> decrement();
  }
};

// Dash =====================================================================

struct dash_t : public druid_spell_t
{
  dash_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "dash", player, player -> find_class_spell( "Dash" ) )
  {
    parse_options( options_str );
    autoshift = form_mask = CAT_FORM;

    harmful = false;
    ignore_false_positive = true;
  }

  void execute() override
  {
    druid_spell_t::execute();

    p() -> buff.dash -> trigger();
  }
};

// Displacer Beast ==========================================================

struct displacer_beast_t : public druid_spell_t
{
  displacer_beast_t( druid_t* p, const std::string& options_str ) :
    druid_spell_t( "displacer_beast", p, p -> talent.displacer_beast )
  {
    parse_options( options_str );
    autoshift = form_mask = CAT_FORM;

    harmful = may_crit = may_miss = false;
    ignore_false_positive = true;
    base_teleport_distance = radius;
    movement_directionality = MOVEMENT_OMNI;
  }

  void execute() override
  {
    druid_spell_t::execute();

    p() -> buff.displacer_beast -> trigger();
  }
};

// Elune's Guidance =========================================================

struct elunes_guidance_t : public druid_spell_t
{
  int combo_points;

  elunes_guidance_t( druid_t* p, const std::string& options_str ) :
    druid_spell_t( "elunes_guidance", p, p -> talent.elunes_guidance )
  {
    parse_options( options_str );

    combo_points = data().effectN( 1 ).resource( RESOURCE_COMBO_POINT );
    dot_duration = timespan_t::zero();
  }

  void execute() override
  {
    druid_spell_t::execute();

    p() -> resource_gain( RESOURCE_COMBO_POINT, combo_points, p() -> gain.elunes_guidance );
    p() -> buff.elunes_guidance -> trigger();
  }
};

// Full Moon Spell ===========================================================

struct full_moon_t : public druid_spell_t
{
  full_moon_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "full_moon", player, player -> find_spell( 202771 ) )
  {
    parse_options( options_str );

    ap_per_cast = data().effectN( 2 ).resource( RESOURCE_ASTRAL_POWER ); // TOCHECK
  }

  void execute() override
  {
    druid_spell_t::execute();

    p() -> moon_stage = NEW_MOON; // TOCHECK: Requires hit?

    p() -> cooldown.moon_cd -> start( cooldown -> duration );
  }

  bool ready() override
  {
    if ( ! p() -> artifact.new_moon.rank() )
      return false;
    if ( p() -> moon_stage != FULL_MOON )
      return false;
    if ( ! p() -> cooldown.moon_cd -> up() )
      return false;

    return druid_spell_t::ready();
  }
};

// Half Moon Spell ===========================================================

struct half_moon_t : public druid_spell_t
{
  half_moon_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "half_moon", player, player -> find_spell( 202768 ) )
  {
    parse_options( options_str );
  }

  void execute() override
  {
    druid_spell_t::execute();

    p() -> moon_stage++;

    p() -> cooldown.moon_cd -> start( cooldown -> duration );
  }

  bool ready() override
  {
    if ( ! p() -> artifact.new_moon.rank() )
      return false;
    if ( p() -> moon_stage != HALF_MOON )
      return false;
    if ( ! p() -> cooldown.moon_cd -> up() )
      return false;

    return druid_spell_t::ready();
  }
};

// Incarnation ==============================================================

struct incarnation_t : public druid_spell_t
{
  incarnation_t( druid_t* p, const std::string& options_str ) :
    druid_spell_t( "incarnation", p,
      p -> specialization() == DRUID_BALANCE     ? p -> talent.incarnation_moonkin :
      p -> specialization() == DRUID_FERAL       ? p -> talent.incarnation_cat   :
      p -> specialization() == DRUID_GUARDIAN    ? p -> talent.incarnation_bear    :
      p -> specialization() == DRUID_RESTORATION ? p -> talent.incarnation_tree   :
      spell_data_t::nil()
    )
  {
    parse_options( options_str );

    harmful = false;
  }

  void execute() override
  {
    druid_spell_t::execute();

    p() -> buff.incarnation -> trigger();

    if ( ! p() -> in_combat )
    {
      timespan_t time = std::max( min_gcd, trigger_gcd * composite_haste() );
      p() -> buff.incarnation -> extend_duration( p(), -time );
      cooldown -> adjust( -time );
    }

    if ( p() -> specialization() == DRUID_GUARDIAN )
    {
      p() -> cooldown.mangle -> reset( false );
      p() -> cooldown.growl  -> reset( false );
      p() -> cooldown.maul   -> reset( false );
    }
  }
};

// Lunar Beam ===============================================================

struct lunar_beam_t : public druid_spell_t
{
  struct lunar_beam_heal_t : public heals::druid_heal_t
  {
    lunar_beam_heal_t( druid_t* player, const spell_data_t* s ) :
      heals::druid_heal_t( "lunar_beam_heal", player, s )
    {
      target = player;
      spell_power_mod.direct = s -> effectN( 1 ).ap_coeff();
      background = true;
    }
  };

  struct lunar_beam_damage_t : public druid_spell_t
  {
    lunar_beam_damage_t( druid_t* player, const spell_data_t* s ) :
      druid_spell_t( "lunar_beam_damage", player, s )
    {
      spell_power_mod.direct = s -> effectN( 2 ).ap_coeff();
      dual = background = true;
    }
  };

  const spell_data_t* tick_spell;
  lunar_beam_heal_t* heal;
  lunar_beam_damage_t* damage;
  int last_tick;

  lunar_beam_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "lunar_beam", player, player -> talent.lunar_beam ),
    last_tick( 0 )
  {
    parse_options( options_str );

    tick_spell = player -> find_spell( 204069 );
    heal       = new lunar_beam_heal_t( player, tick_spell );
    damage     = new lunar_beam_damage_t( player, tick_spell );

    aoe        = -1;
    ground_aoe = true;
    may_crit = hasted_ticks = false;
    radius = tick_spell -> effectN( 2 ).radius();
    base_tick_time = timespan_t::from_seconds( 1.0 ); // TODO: Find in spell data... somewhere!
    dot_duration = data().duration();

    tick_action = damage;
    add_child( damage );
  }

  virtual void execute()
  {
    druid_spell_t::execute();

    last_tick = 0;
  }

  virtual void tick( dot_t* d ) override
  {
    druid_spell_t::tick( d );

    // Only trigger heal once per tick
    if ( d -> current_tick != last_tick )
    {
      last_tick = d -> current_tick;

      if ( ! sim -> distance_targeting_enabled || p() -> get_ground_aoe_distance( *d -> state ) <= radius )
        heal -> execute();
    }
  }
};

// Lunar Inspiration =======================================

struct lunar_inspiration_t : public druid_spell_t
{
  lunar_inspiration_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "lunar_inspiration", player, player -> find_spell( 155625 ), options_str )
  {}

  virtual void impact( action_state_t* s ) override
  {
    druid_spell_t::impact( s );

    // Grant combo points
    if ( result_is_hit( s -> result ) )
    {
      p() -> resource_gain( RESOURCE_COMBO_POINT, 1, p() -> gain.moonfire );
      if ( p() -> spell.primal_fury -> ok() && s -> result == RESULT_CRIT )
      {
        p() -> proc.primal_fury -> occur();
        p() -> resource_gain( RESOURCE_COMBO_POINT, p() -> spell.primal_fury -> effectN( 1 ).base_value(), p() -> gain.primal_fury );
      }
    }
  }

  virtual bool ready() override
  {
    if ( ! p() -> talent.lunar_inspiration -> ok() )
      return false;

    return druid_spell_t::ready();
  }
};

// Lunar Strike =============================================================

struct lunar_strike_t : public druid_spell_t
{
  lunar_strike_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "lunar_strike", player, player -> find_specialization_spell( "Lunar Strike" ) )
  {
    parse_options( options_str );

    aoe = -1;
    base_aoe_multiplier  = data().effectN( 1 ).percent();

    ap_per_cast = data().effectN( 3 ).resource( RESOURCE_ASTRAL_POWER );
    benefits_from_ca = benefits_from_elune = true;
    consumes_owlkin_frenzy = true;

    base_execute_time *= 1 + player -> sets.set( DRUID_BALANCE, T17, B2 ) -> effectN( 1 ).percent();
    base_crit         += player -> artifact.dark_side_of_the_moon.percent();
  }

  double action_multiplier() const override
  {
    double am = druid_spell_t::action_multiplier();

    if ( p() -> buff.lunar_empowerment -> check() )
      am *= 1.0 + p() -> buff.lunar_empowerment -> current_value
                + ( p() -> mastery.starlight -> ok() * p() -> cache.mastery_value() );

    return am;
  }

  timespan_t execute_time() const override
  {
    timespan_t et = druid_spell_t::execute_time();

    if ( p() -> talent.starlord -> ok() && p() -> buff.lunar_empowerment -> check() )
      et *= 1 - p() -> talent.starlord -> effectN( 1 ).percent();

    if ( p() -> buff.warrior_of_elune -> check() )
      et *= 1 + p() -> talent.warrior_of_elune -> effectN( 1 ).percent();

    return et;
  }

  void execute() override
  {
    p() -> buff.lunar_empowerment -> up();
    p() -> buff.warrior_of_elune -> up();

    druid_spell_t::execute();
    
    // Nature's Balance only extends Moonfire on the primary target.
    trigger_natures_balance( execute_state, td( target ) -> dots.moonfire );

    p() -> buff.lunar_empowerment -> decrement();
    p() -> buff.warrior_of_elune -> decrement();
  }
};

// Mark of the Wild Spell ===================================================

struct mark_of_the_wild_t : public druid_spell_t
{
  mark_of_the_wild_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "mark_of_the_wild", player, player -> find_specialization_spell( "Mark of the Wild" )  )
  {
    parse_options( options_str );

    trigger_gcd = timespan_t::zero();
    harmful     = false;
    ignore_false_positive = true;
  }

  void execute() override
  {
    druid_spell_t::execute();

    if ( sim -> log ) sim -> out_log.printf( "%s performs %s", player -> name(), name() );
  }
};

// Mark of Ursol ============================================================

struct mark_of_ursol_t : public druid_spell_t
{
  mark_of_ursol_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "mark_of_ursol", player, player -> find_specialization_spell( "Mark of Ursol" ) )
  {
    parse_options( options_str );

    harmful = false;
    may_crit = may_miss = false;
  }

  virtual void execute()
  {
    druid_spell_t::execute();

    p() -> buff.mark_of_ursol -> trigger();
  }
};

// New Moon Spell ===========================================================

struct new_moon_t : public druid_spell_t
{
  new_moon_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "new_moon", player, player -> artifact.new_moon.spell_ )
  {
    parse_options( options_str );
  }

  void execute() override
  {
    druid_spell_t::execute();

    p() -> moon_stage++;

    p() -> cooldown.moon_cd -> start( cooldown -> duration );
  }

  bool ready() override
  {
    if ( ! p() -> artifact.new_moon.rank() )
      return false;
    if ( p() -> moon_stage != NEW_MOON )
      return false;
    if ( ! p() -> cooldown.moon_cd -> up() )
      return false;

    return druid_spell_t::ready();
  }
};

// Sunfire Spell ============================================================

struct sunfire_t : public druid_spell_t
{
  shooting_stars_t* shooting_stars;

  sunfire_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "sunfire", player, player -> find_spell( 93402 ) ),
    shooting_stars( new shooting_stars_t( player ) )
  {
    parse_options( options_str );

    const spell_data_t* dmg_spell = player -> find_spell( 164815 );

    dot_duration           = dmg_spell -> duration();
    dot_duration          += player -> sets.set( SET_CASTER, T14, B4 ) -> effectN( 1 ).time_value();
    dot_duration          += player -> spec.balance_passive -> effectN( 4 ).time_value();
    base_tick_time         = dmg_spell -> effectN( 2 ).period();
    spell_power_mod.direct = dmg_spell -> effectN( 1 ).sp_coeff();
    spell_power_mod.tick   = dmg_spell -> effectN( 2 ).sp_coeff();
    aoe                    = -1;

    base_multiplier *= 1.0 + player -> artifact.sunfire_burns.percent();
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double tm = druid_spell_t::composite_target_multiplier( t );

    if ( td( t ) -> buffs.starfall -> up() )
      tm *= 1.0 + td( t ) -> buffs.starfall -> current_value
                + ( p() -> mastery.starlight -> ok() * p() -> cache.mastery_value() );

    return tm;
  }

  void tick( dot_t* d ) override
  {
    druid_spell_t::tick( d );

    if ( result_is_hit( d -> state -> result ) )
    {
      if ( p() -> talent.shooting_stars -> ok() && rng().roll( shooting_stars -> proc_chance ) )
      {
        shooting_stars -> target = d -> target;
        shooting_stars -> execute();
      }

      if ( p() -> sets.has_set_bonus( DRUID_BALANCE, T18, B2 ) )
        trigger_balance_tier18_2pc();
    }
  }
};

// Moonkin Form Spell =======================================================

struct moonkin_form_t : public druid_spell_t
{
  moonkin_form_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "moonkin_form", player, player -> spec.moonkin_form )
  {
    parse_options( options_str );
    form_mask = NO_FORM | CAT_FORM | BEAR_FORM;
    may_autounshift   = false;

    harmful           = false;
    ignore_false_positive = true;
  }

  void execute() override
  {
    druid_spell_t::execute();

    p() -> shapeshift( MOONKIN_FORM );
  }
};

// Prowl ==================================================================

struct prowl_t : public druid_spell_t
{
  prowl_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "prowl", player, player -> find_class_spell( "Prowl" ) )
  {
    parse_options( options_str );
    autoshift = form_mask = CAT_FORM;

    trigger_gcd = timespan_t::zero();
    harmful     = false;
    ignore_false_positive = true;
  }

  void execute() override
  {
    if ( sim -> log )
      sim -> out_log.printf( "%s performs %s", player -> name(), name() );

    p() -> buff.prowl -> trigger();
  }

  bool ready() override
  {
    if ( p() -> buff.prowl -> check() )
      return false;

    if ( p() -> in_combat && ! ( p() -> buff.incarnation -> check() && p() -> specialization() == DRUID_FERAL ) )
      return false;

    return druid_spell_t::ready();
  }
};

// Skull Bash ===============================================================

struct skull_bash_t : public druid_spell_t
{
  skull_bash_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "skull_bash", player, player -> find_specialization_spell( "Skull Bash" ) )
  {
    parse_options( options_str );

    may_miss = may_glance = may_block = may_dodge = may_parry = may_crit = false;
    ignore_false_positive = true;
  }

  void execute() override
  {
    druid_spell_t::execute();

    if ( p() -> sets.has_set_bonus( DRUID_FERAL, PVP, B2 ) )
      p() -> cooldown.tigers_fury -> reset( false );
  }

  bool ready() override
  {
    if ( ! target -> debuffs.casting -> check() )
      return false;

    return druid_spell_t::ready();
  }
};

// Solar Wrath ==============================================================

struct solar_wrath_t : public druid_spell_t
{
  solar_wrath_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "solar_wrath", player, player -> find_specialization_spell( "Solar Wrath" ) )
  {
    parse_options( options_str );

    ap_per_cast = data().effectN( 2 ).resource( RESOURCE_ASTRAL_POWER );
    benefits_from_ca = benefits_from_elune = true;
    consumes_owlkin_frenzy = true;

    base_execute_time *= 1.0 + player -> sets.set( DRUID_BALANCE, T17, B2 ) -> effectN( 1 ).percent();
    base_multiplier   *= 1.0 + player -> sets.set( SET_CASTER, T13, B2 ) -> effectN( 1 ).percent();
    base_multiplier   *= 1.0 + player -> artifact.solar_stabbing.percent();
  }

  double action_multiplier() const override
  {
    double am = druid_spell_t::action_multiplier();

    if ( p() -> buff.solar_empowerment -> check() )
      am *= 1.0 + p() -> buff.solar_empowerment -> current_value
                + ( p() -> mastery.starlight -> ok() * p() -> cache.mastery_value() );

    return am;
  }

  timespan_t execute_time() const override
  {
    timespan_t et = druid_spell_t::execute_time();

    if ( p() -> talent.starlord -> ok() && p() -> buff.solar_empowerment -> check() )
      et *= 1 - p() -> talent.starlord -> effectN( 1 ).percent();

    return et;
  }

  void execute() override
  {
    p() -> buff.solar_empowerment -> up();

    druid_spell_t::execute();

    trigger_natures_balance( execute_state, td( target ) -> dots.sunfire );

    if ( p() -> sets.has_set_bonus( DRUID_BALANCE, T17, B4 ) )
      p() -> cooldown.celestial_alignment -> adjust( -1 * p() -> sets.set( DRUID_BALANCE, T17, B4 ) -> effectN( 1 ).time_value() );

    p() -> buff.solar_empowerment -> decrement();
  }
};

// Stampeding Roar =========================================================

struct stampeding_roar_t : public druid_spell_t
{
  stampeding_roar_t( druid_t* p, const std::string& options_str ) :
    druid_spell_t( "stampeding_roar", p, p -> find_class_spell( "Stampeding Roar" ) )
  {
    parse_options( options_str );
    form_mask = BEAR_FORM | CAT_FORM;
    autoshift = BEAR_FORM;

    harmful = false;
    radius *= 1.0 + p -> talent.gutteral_roars -> effectN( 1 ).percent();
    cooldown -> duration *= 1.0 + p -> talent.gutteral_roars -> effectN( 2 ).percent();
  }

  void execute() override
  {
    druid_spell_t::execute();

    for ( size_t i = 0; i < sim -> player_non_sleeping_list.size(); ++i )
    {
      player_t* p = sim -> player_non_sleeping_list[ i ];
      if ( p -> type == PLAYER_GUARDIAN )
        continue;

      p -> buffs.stampeding_roar -> trigger();
    }
  }
};

// Starfall Spell ===========================================================
// TODO: Ground targeted instead of just hitting everything in the whole sim.

struct starfall_t : public druid_spell_t
{
  struct starfall_pulse_t : public druid_spell_t
  {
    timespan_t remains;

    starfall_pulse_t( druid_t* player, const std::string& name ) :
      druid_spell_t( name, player, player -> find_spell( 191037 ) )
    {
      background = dual = direct_tick = true; // Legion TOCHECK
      callbacks = false;
      
      base_multiplier *= 1.0 + player -> sets.set( SET_CASTER, T14, B2 ) -> effectN( 1 ).percent();
      base_multiplier *= 1.0 + p() -> talent.stellar_drift -> effectN( 2 ).percent();
    }

    timespan_t travel_time() const override
    {
      // Override travel time since sim doesn't understand the missiles don't start from the player.
      return timespan_t::from_seconds( 0.1 );
    }

    double action_multiplier() const override
    {
      double am = druid_spell_t::action_multiplier();

      if ( p() -> mastery.starlight -> ok() )
        am *= 1.0 + p() -> cache.mastery_value();

      return am;
    }

    void impact( action_state_t* s ) override
    {
      // No hit check because if they're in the Starfall they should already have the debuff.
      if ( td( s -> target ) -> buffs.starfall -> remains() < remains )
        td( s -> target ) -> buffs.starfall -> trigger( 1, buff_t::DEFAULT_VALUE(), 1.0, remains );

      druid_spell_t::impact( s );
    }
  };

  struct starfall_dot_event_t : public event_t
  {
    druid_t* druid;
    starfall_t* starfall;
    timespan_t dot_end;
    int ticks;
    int reference_id; // for debug output purposes

    starfall_dot_event_t( druid_t* p, starfall_t* a, int t, int id, timespan_t de ) : 
      event_t( *p ), druid( p ), starfall( a ),
      dot_end( de ), ticks( t ), reference_id( id )
    {
      add_event( tick_time() );
    }

    starfall_dot_event_t( druid_t* p, starfall_t* a ) :
      starfall_dot_event_t( p, a, 0, p -> rng().real() * 16e6, p -> sim -> current_time() + a -> _dot_duration )
    {}

    const char* name() const override
    { return "Starfall Custom Tick"; }

    timespan_t tick_time()
    {
      return starfall -> base_tick_time * druid -> composite_spell_haste();
    }

    void execute() override
    {
      ticks++;
      sim_t& s = sim();

      if ( s.log )
        s.out_log.printf( "%s ticks (%d of %d). id=%X tt=%.3f",
           starfall -> name(),
           ticks,
           ticks + (int) ( ( dot_end - s.current_time() ) / tick_time() ),
           reference_id,
           tick_time().total_seconds() );

      starfall -> pulse -> remains = dot_end - s.current_time();

      for ( size_t i = 0; i < s.actor_list.size(); i++ )
      {
        player_t* target = s.actor_list[ i ];

        if ( target -> is_enemy() && ! target -> is_sleeping() )
        {
          starfall -> pulse -> target = target;
          starfall -> pulse -> execute();
        }
      }

      // If next tick period falls within the duration, schedule another event. (no partial ticks)
      if ( sim().current_time() + tick_time() <= dot_end )
        new ( sim() ) starfall_dot_event_t( druid, starfall, ticks, reference_id, dot_end );
      else
        druid -> active_starfalls--;
    }
  };

  starfall_pulse_t* pulse;
  timespan_t _dot_duration;

  starfall_t( druid_t* player, const std::string& options_str ):
    druid_spell_t( "starfall", player, player -> find_spell( 191034 ) ),
    pulse( new starfall_pulse_t( player, "starfall_pulse" ) )
  {
    parse_options( options_str );
    
    may_crit = false;
    _dot_duration = data().duration();
    base_tick_time = _dot_duration / 9.0; // ticks 9 times (missing from spell data)

    radius     = data().effectN( 1 ).radius();
    radius    *= 1.0 + player -> talent.stellar_drift -> effectN( 1 ).percent();

    add_child( pulse );
  }

  virtual void execute() override
  {
    druid_spell_t::execute();

    p() -> active_starfalls++;
    new ( *sim ) starfall_dot_event_t( p(), this );
  }

  virtual void impact( action_state_t* s ) override
  {
    druid_spell_t::impact( s );

    if ( td( s -> target ) -> buffs.starfall -> remains() < _dot_duration )
      td( s -> target ) -> buffs.starfall -> trigger( 1, buff_t::DEFAULT_VALUE(), 1.0, _dot_duration );
  }
};

struct starshards_t : public starfall_t
{
  starshards_t( druid_t* player ) :
    starfall_t( player, std::string( "" ) )
  {
    background = true;
    target = sim -> target;
    radius = 40;
    cooldown = player -> get_cooldown( "starshards" );
  }
  
  // Allow the action to execute even when Starfall is already active.
  bool ready() override
  { return druid_spell_t::ready(); }
};

// Starsurge Spell ==========================================================

struct starsurge_t : public druid_spell_t
{
  struct goldrinns_fang_t : public druid_spell_t
  {
    goldrinns_fang_t( druid_t* player ) :
      druid_spell_t( "goldrinns_fang", player, player -> find_spell( 203001 ) )
    {
      background = true;
      may_miss = false;
      aoe = -1;
      radius = 5.0; // placeholder radius since the spell data has none
    }
  };

  double starshards_chance;
  goldrinns_fang_t* power_of_goldrinn;

  starsurge_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "starsurge", player, player -> find_specialization_spell( "Starsurge") ),
    starshards_chance( 0.0 )
  {
    parse_options( options_str );

    consumes_owlkin_frenzy = true;

    base_multiplier       *= 1.0 + player -> sets.set( SET_CASTER, T13, B4 ) -> effectN( 2 ).percent();
    base_multiplier       *= 1.0 + p() -> sets.set( SET_CASTER, T13, B2 ) -> effectN( 1 ).percent();
    base_crit             += p() -> sets.set( SET_CASTER, T15, B2 ) -> effectN( 1 ).percent();
    crit_bonus_multiplier *= 1.0 + player -> artifact.scythe_of_the_stars.percent();

    if ( player -> starshards )
      starshards_chance = player -> starshards -> driver() -> effectN( 1 ).average( player -> starshards -> item ) / 100.0;

    if ( player -> artifact.power_of_goldrinn.rank() )
    {
      power_of_goldrinn = new goldrinns_fang_t( player );
      add_child( power_of_goldrinn );
    }
  }

  double action_multiplier() const override
  {
    double am = druid_spell_t::action_multiplier();

    if ( p() -> mastery.starlight -> ok() )
      am *= 1.0 + p() -> cache.mastery_value();

    return am;
  }

  void execute() override
  {
    druid_spell_t::execute();

    if ( result_is_hit( execute_state -> result ) )
    {
      if ( p() -> artifact.power_of_goldrinn.rank() )
      {
        power_of_goldrinn -> target = target;
        power_of_goldrinn -> execute();
      }

      // Dec 3 2015: Starsurge must hit to grant empowerments, but grants them on cast not impact.
      p() -> buff.solar_empowerment -> trigger();
      p() -> buff.lunar_empowerment -> trigger();
    }

    if ( p() -> starshards && rng().roll( starshards_chance ) )
    {
      p() -> proc.starshards -> occur();
      p() -> active.starshards -> execute();
    }
  }
};

// Stellar Flare ==========================================================

struct stellar_flare_t : public druid_spell_t
{
  stellar_flare_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "stellar_flare", player, player -> talent.stellar_flare )
  {
    parse_options( options_str );

    consumes_owlkin_frenzy = true;
  }

  // Dec 3 2015: Empowerments modifiers are multiplicative AND snapshot mastery.
  double composite_persistent_multiplier( const action_state_t* s ) const override
  {
    double pm = druid_spell_t::composite_persistent_multiplier( s );

    if ( p() -> buff.lunar_empowerment -> check() )
      pm *= 1.0 + p() -> buff.lunar_empowerment -> current_value
                + ( p() -> mastery.starlight -> ok() * p() -> cache.mastery_value() );

    if ( p() -> buff.solar_empowerment -> check() )
      pm *= 1.0 + p() -> buff.solar_empowerment -> current_value
                + ( p() -> mastery.starlight -> ok() * p() -> cache.mastery_value() );

    return pm;
  }

  void execute() override
  {
    p() -> buff.lunar_empowerment -> up();
    p() -> buff.solar_empowerment -> up();

    druid_spell_t::execute();

    p() -> buff.lunar_empowerment -> decrement();
    p() -> buff.solar_empowerment -> decrement();
  }
};

// Survival Instincts =======================================================

struct survival_instincts_t : public druid_spell_t
{
  survival_instincts_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "survival_instincts", player, player -> find_specialization_spell( "Survival Instincts" ), options_str )
  {
    harmful = false;
    use_off_gcd = true;

    // Spec-based cooldown modifiers
    cooldown -> duration += player -> spec.feral_passive -> effectN( 6 ).time_value();
    cooldown -> duration += player -> spec.guardian_passive -> effectN( 5 ).time_value();

    cooldown -> charges = 2;
    cooldown -> duration *= 1.0 + player -> talent.survival_of_the_fittest -> effectN( 1 ).percent();
  }

  void execute() override
  {
    druid_spell_t::execute();

    p() -> buff.survival_instincts -> trigger(); // DBC value is 60 for some reason
  }
};

// T16 Balance 2P Bonus =====================================================

struct t16_2pc_starfall_bolt_t : public druid_spell_t
{
  t16_2pc_starfall_bolt_t( druid_t* player ) :
    druid_spell_t( "t16_2pc_starfall_bolt", player, player -> find_spell( 144770 ) )
  {
    background  = true;
  }
};

struct t16_2pc_sun_bolt_t : public druid_spell_t
{
  t16_2pc_sun_bolt_t( druid_t* player ) :
    druid_spell_t( "t16_2pc_sun_bolt", player, player -> find_spell( 144772 ) )
  {
    background  = true;
  }
};

// Typhoon ==================================================================

struct typhoon_t : public druid_spell_t
{
  typhoon_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "typhoon", player, player -> talent.typhoon )
  {
    parse_options( options_str );

    ignore_false_positive = true;
  }
};

// Warrior of Elune =============================================================

struct warrior_of_elune_t : public druid_spell_t
{
  warrior_of_elune_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "warrior_of_elune", player, player -> talent.warrior_of_elune )
  {
    parse_options( options_str );

    harmful = false;
  }

  void execute() override
  {
    druid_spell_t::execute();

    p() -> buff.warrior_of_elune -> trigger( 2 );
  }

  virtual bool ready() override
  {
    if ( p() -> buff.warrior_of_elune -> check() )
      return false;

    return druid_spell_t::ready();
  }
};

// Wild Charge ==============================================================

struct wild_charge_t : public druid_spell_t
{
  double movement_speed_increase;

  wild_charge_t( druid_t* p, const std::string& options_str ) :
    druid_spell_t( "wild_charge", p, p -> talent.wild_charge ),
    movement_speed_increase( 5.0 )
  {
    parse_options( options_str );

    harmful = may_crit = may_miss = false;
    ignore_false_positive = true;
    range = data().max_range();
    movement_directionality = MOVEMENT_OMNI; 
    trigger_gcd = timespan_t::zero();
  }

  void schedule_execute( action_state_t* execute_state ) override
  {
    druid_spell_t::schedule_execute( execute_state );

    /* Since Cat/Bear charge is limited to moving towards a target,
       cancel form if the druid wants to move away.
       Other forms can already move in any direction they want so they're fine. */
    if ( p() -> current.movement_direction == MOVEMENT_AWAY )
      p() -> shapeshift( NO_FORM );
  }

  void execute() override
  {
    if ( p() -> current.distance_to_move > data().min_range() )
    {
      p() -> buff.wild_charge_movement -> trigger( 1, movement_speed_increase, 1,
        timespan_t::from_seconds( p() -> current.distance_to_move / ( p() -> base_movement_speed * ( 1 + p() -> passive_movement_modifier() + movement_speed_increase ) ) ) );
    }

    druid_spell_t::execute();
  }

  bool ready() override
  {
    if ( p() -> current.distance_to_move < data().min_range() ) // Cannot charge if the target is too close.
      return false;

    return druid_spell_t::ready();
  }
};

} // end namespace spells

// ==========================================================================
// Druid Helper Functions & Structures
// ==========================================================================

// Brambles Absorb/Reflect Handler =========================================

double brambles_handler( const action_state_t* s )
{
  druid_t* p = static_cast<druid_t*>( s -> target );
  assert( p -> talent.brambles -> ok() );
  assert( s );

  // Pass incoming damage value so the absorb can be calculated.
  // TOCHECK: Does this use result_amount or result_mitigated?
  p -> active.brambles -> incoming_damage = s -> result_mitigated;
  // Pass the triggering enemy so that the damage reflect has a target;
  p -> active.brambles -> triggering_enemy = s -> action -> player;
  p -> active.brambles -> execute();

  return p -> active.brambles -> absorb_size;
}

// Persistent Buff Delay Event ==============================================

struct persistent_buff_delay_event_t : public event_t
{
  buff_t* buff;

  persistent_buff_delay_event_t( druid_t* p, buff_t* b ) : 
    event_t( *p ), buff( b )
  {
    /* Delay triggering the buff a random amount between 0 and buff_period.
       This prevents fixed-period driver buffs from ticking at the exact same
       times on every iteration. 
       
       Buffs that use the event to activate should implement tick_zero-like
       behavior. */
    add_event( rng().real() * b -> buff_period );
  }

  const char* name() const override
  { return "persistent_buff_delay"; }

  void execute() override
  { buff -> trigger(); }
};

// ==========================================================================
// Druid Character Definition
// ==========================================================================

// druid_t::create_action  ==================================================

action_t* druid_t::create_action( const std::string& name,
                                  const std::string& options_str )
{
  using namespace cat_attacks;
  using namespace bear_attacks;
  using namespace heals;
  using namespace spells;

  if ( name == "astral_communion" || 
       name == "ac")                      return new       astral_communion_t( this, options_str );
  if ( name == "auto_attack"            ) return new            auto_attack_t( this, options_str );
  if ( name == "barkskin"               ) return new               barkskin_t( this, options_str );
  if ( name == "berserk"                ) return new                berserk_t( this, options_str );
  if ( name == "bear_form"              ) return new              bear_form_t( this, options_str );
  if ( name == "blessing_of_anshe"      ) return new      blessing_of_anshe_t( this, options_str );
  if ( name == "blessing_of_elune"      ) return new      blessing_of_elune_t( this, options_str );
  if ( name == "bloody_slash"           ) return new           bloody_slash_t( this, options_str );
  if ( name == "bristling_fur"          ) return new          bristling_fur_t( this, options_str );
  if ( name == "cat_form"               ) return new               cat_form_t( this, options_str );
  if ( name == "celestial_alignment" ||
       name == "ca"                     ) return new    celestial_alignment_t( this, options_str );
  if ( name == "collapsing_stars"       ) return new       collapsing_stars_t( this, options_str );
  if ( name == "cenarion_ward"          ) return new          cenarion_ward_t( this, options_str );
  if ( name == "dash"                   ) return new                   dash_t( this, options_str );
  if ( name == "displacer_beast"        ) return new        displacer_beast_t( this, options_str );
  if ( name == "elunes_guidance"        ) return new        elunes_guidance_t( this, options_str );
  if ( name == "ferocious_bite"         ) return new         ferocious_bite_t( this, options_str );
  if ( name == "frenzied_regeneration"  ) return new  frenzied_regeneration_t( this, options_str );
  if ( name == "full_moon"              ) return new              full_moon_t( this, options_str );
  if ( name == "growl"                  ) return new                  growl_t( this, options_str );
  if ( name == "half_moon"              ) return new              half_moon_t( this, options_str );
  if ( name == "healing_touch"          ) return new          healing_touch_t( this, options_str );
  if ( name == "ironfur"                ) return new                ironfur_t( this, options_str );
  if ( name == "lacerate"               ) return new               lacerate_t( this, options_str );
  if ( name == "lifebloom"              ) return new              lifebloom_t( this, options_str );
  if ( name == "lunar_beam"             ) return new             lunar_beam_t( this, options_str );
  if ( name == "lunar_strike"           ) return new           lunar_strike_t( this, options_str );
  if ( name == "maim"                   ) return new                   maim_t( this, options_str );
  if ( name == "mangle"                 ) return new                 mangle_t( this, options_str );
  if ( name == "mark_of_the_wild"       ) return new       mark_of_the_wild_t( this, options_str );
  if ( name == "mark_of_ursol"          ) return new          mark_of_ursol_t( this, options_str );
  if ( name == "maul"                   ) return new                   maul_t( this, options_str );
  if ( name == "moonfire"               ) return new               moonfire_t( this, options_str );
  if ( name == "moonfire_cat" ||
       name == "lunar_inspiration" )      return new      lunar_inspiration_t( this, options_str );
  if ( name == "new_moon"               ) return new               new_moon_t( this, options_str );
  if ( name == "sunfire"                ) return new                sunfire_t( this, options_str );
  if ( name == "moonkin_form"           ) return new           moonkin_form_t( this, options_str );
  if ( name == "pulverize"              ) return new              pulverize_t( this, options_str );
  if ( name == "rake"                   ) return new                   rake_t( this, options_str );
  if ( name == "renewal"                ) return new                renewal_t( this, options_str );
  if ( name == "regrowth"               ) return new               regrowth_t( this, options_str );
  if ( name == "rejuvenation"           ) return new           rejuvenation_t( this, options_str );
  if ( name == "rip"                    ) return new                    rip_t( this, options_str );
  if ( name == "savage_roar"            ) return new            savage_roar_t( this, options_str );
  if ( name == "shred"                  ) return new                  shred_t( this, options_str );
  if ( name == "skull_bash"             ) return new             skull_bash_t( this, options_str );
  if ( name == "solar_wrath"            ) return new            solar_wrath_t( this, options_str );
  if ( name == "stampeding_roar"        ) return new        stampeding_roar_t( this, options_str );
  if ( name == "starfall"               ) return new               starfall_t( this, options_str );
  if ( name == "starsurge"              ) return new              starsurge_t( this, options_str );
  if ( name == "stellar_flare"          ) return new          stellar_flare_t( this, options_str );
  if ( name == "prowl"                  ) return new                  prowl_t( this, options_str );
  if ( name == "survival_instincts"     ) return new     survival_instincts_t( this, options_str );
  if ( name == "swipe"                  ) return new                  swipe_t( this, options_str );
  if ( name == "swiftmend"              ) return new              swiftmend_t( this, options_str );
  if ( name == "tigers_fury"            ) return new            tigers_fury_t( this, options_str );
  if ( name == "thrash_bear"            ) return new            thrash_bear_t( this, options_str );
  if ( name == "thrash_cat"             ) return new             thrash_cat_t( this, options_str );
  if ( name == "tranquility"            ) return new            tranquility_t( this, options_str );
  if ( name == "typhoon"                ) return new                typhoon_t( this, options_str );
  if ( name == "warrior_of_elune"       ) return new       warrior_of_elune_t( this, options_str );
  if ( name == "wild_charge"            ) return new            wild_charge_t( this, options_str );
  if ( name == "wild_growth"            ) return new            wild_growth_t( this, options_str );
  if ( name == "incarnation"            ) return new            incarnation_t( this, options_str );

  return player_t::create_action( name, options_str );
}

// druid_t::create_pet ======================================================

pet_t* druid_t::create_pet( const std::string& pet_name,
                            const std::string& /* pet_type */ )
{
  pet_t* p = find_pet( pet_name );

  if ( p ) return p;

  using namespace pets;

  return nullptr;
}

// druid_t::create_pets =====================================================

void druid_t::create_pets()
{
  player_t::create_pets();

  if ( sets.has_set_bonus( DRUID_BALANCE, T18, B2 ) )
  {
    for ( pet_t*& pet : pet_fey_moonwing )
      pet = new pets::fey_moonwing_t( sim, this );
  }
}

// druid_t::init_spells =====================================================

void druid_t::init_spells()
{
  player_t::init_spells();

  // Specializations ========================================================

  // Generic / Multiple specs
  spec.critical_strikes        = find_specialization_spell( "Critical Strikes" );
  spec.leather_specialization  = find_specialization_spell( "Leather Specialization" );
  spec.omen_of_clarity         = find_specialization_spell( "Omen of Clarity" );
  spec.killer_instinct         = find_specialization_spell( "Killer Instinct" );
  spec.mana_attunement         = find_specialization_spell( "Mana Attunement" );
  spec.nurturing_instinct      = find_specialization_spell( "Nurturing Instinct" );

  // Boomkin
  spec.balance_passive         = find_specialization_spell( "Balance Overrides Passive" );
  spec.celestial_alignment     = find_specialization_spell( "Celestial Alignment" );
  spec.moonkin_form            = find_specialization_spell( "Moonkin Form" );
  spec.starfall                = find_specialization_spell( "Starfall" );
  spec.natural_insight         = find_specialization_spell( "Natural Insight" );

  // Feral
  spec.feral_passive           = find_specialization_spell( "Feral Overrides Passive" );
  spec.predatory_swiftness     = find_specialization_spell( "Predatory Swiftness" );
  spec.nurturing_instinct      = find_specialization_spell( "Nurturing Instinct" );
  spec.predatory_swiftness     = find_specialization_spell( "Predatory Swiftness" );
  spec.sharpened_claws         = find_specialization_spell( "Sharpened Claws" );
  spec.swipe                   = find_specialization_spell( "Swipe" );

  // Guardian
  spec.bladed_armor            = find_specialization_spell( "Bladed Armor" );
  spec.guardian_passive        = find_specialization_spell( "Guardian Overrides Passive" );
  spec.ironfur                 = find_specialization_spell( "Ironfur" );
  spec.resolve                 = find_specialization_spell( "Resolve" );

  // Talents ================================================================

  // Multiple Specs
  talent.renewal                        = find_talent_spell( "Renewal" );
  talent.displacer_beast                = find_talent_spell( "Displacer Beast" );
  talent.wild_charge                    = find_talent_spell( "Wild Charge" );

  talent.balance_affinity               = find_talent_spell( "Balance Affinity" );
  talent.feral_affinity                 = find_talent_spell( "Feral Affinity" );
  talent.guardian_affinity              = find_talent_spell( "Guardian Affinity" );
  talent.restoration_affinity           = find_talent_spell( "Restoration Affinity" );

  talent.mighty_bash                    = find_talent_spell( "Mighty Bash" );
  talent.mass_entanglement              = find_talent_spell( "Mass Entanglement" );
  talent.typhoon                        = find_talent_spell( "Typhoon" );

  talent.soul_of_the_forest             = find_talent_spell( "Soul of the Forest" );
  talent.moment_of_clarity              = find_talent_spell( "Moment of Clarity" );
    
  // Feral
  talent.predator                       = find_talent_spell( "Predator" );
  talent.blood_scent                    = find_talent_spell( "Blood Scent" );
  talent.lunar_inspiration              = find_talent_spell( "Lunar Inspiration" );

  talent.incarnation_cat                = find_talent_spell( "Incarnation: King of the Jungle" );
  talent.bloody_slash                   = find_talent_spell( "Bloody Slash" );

  talent.sabertooth                     = find_talent_spell( "Sabertooth" );
  talent.jagged_wounds                  = find_talent_spell( "Jagged Wounds" );
  talent.elunes_guidance                = find_talent_spell( "Elune's Guidance" );

  talent.savage_roar                    = find_talent_spell( "Savage Roar" );
  talent.bloodtalons                    = find_talent_spell( "Bloodtalons" );
    
  // Balance
  talent.shooting_stars                 = find_talent_spell( "Shooting Stars" );
  talent.warrior_of_elune               = find_talent_spell( "Warrior of Elune" );
  talent.starlord                       = find_talent_spell( "Starlord" );

  talent.incarnation_moonkin            = find_talent_spell( "Incarnation: Chosen of Elune" );
  talent.stellar_flare                  = find_talent_spell( "Stellar Flare" );
    
  talent.stellar_drift                  = find_talent_spell( "Stellar Drift" );
  talent.full_moon                      = find_talent_spell( "Full Moon" );
  talent.natures_balance                = find_talent_spell( "Nature's Balance" );

  talent.collapsing_stars               = find_talent_spell( "Collapsing Stars" );
  talent.astral_communion               = find_talent_spell( "Astral Communion" );
  talent.blessing_of_the_ancients       = find_talent_spell( "Blessing of the Ancients" );

  // Guardian
  talent.brambles                       = find_talent_spell( "Brambles" );
  talent.pulverize                      = find_talent_spell( "Pulverize" );
  talent.blood_frenzy                   = find_talent_spell( "Blood Frenzy" );

  talent.gutteral_roars                 = find_talent_spell( "Gutteral Roars" );
    
  talent.incarnation_bear               = find_talent_spell( "Incarnation: Guardian of Ursoc" );
  talent.galactic_guardian              = find_talent_spell( "Galactic Guardian" );
    
  talent.earthwarden                    = find_talent_spell( "Earthwarden" );
  talent.guardian_of_elune              = find_talent_spell( "Guardian of Elune" );
  talent.survival_of_the_fittest        = find_talent_spell( "Survival of the Fittest" );
    
  talent.rend_and_tear                  = find_talent_spell( "Rend and Tear" );
  talent.lunar_beam                     = find_talent_spell( "Lunar Beam" );
  talent.bristling_fur                  = find_talent_spell( "Bristling Fur" );

  // Restoration
  talent.verdant_growth                 = find_talent_spell( "Verdant Growth" );
  talent.cenarion_ward                  = find_talent_spell( "Cenarion Ward" );
  talent.germination                    = find_talent_spell( "Germination" );

  talent.incarnation_tree               = find_talent_spell( "Incarnation: Tree of Life" );
  talent.cultivation                    = find_talent_spell( "Cultivation" );
    
  talent.prosperity                     = find_talent_spell( "Prosperity" );
  talent.inner_peace                    = find_talent_spell( "Inner Peace" );
  talent.profusion                      = find_talent_spell( "Profusion" );
    
  talent.stonebark                      = find_talent_spell( "Stonebark" );
  talent.flourish                       = find_talent_spell( "Flourish" );

  // Artifact ===============================================================

  // Balance -- Scythe of Elune
  artifact.new_moon                     = find_artifact_spell( "New Moon" );
  artifact.moon_and_stars               = find_artifact_spell( "Moon and Stars" );
  artifact.power_of_goldrinn            = find_artifact_spell( "Power of Goldrinn" );
  artifact.scion_of_the_night_sky       = find_artifact_spell( "Scion of the Night Sky" );
  artifact.falling_star                 = find_artifact_spell( "Falling Star" );
  artifact.touch_of_the_moon            = find_artifact_spell( "Touch of the Moon" );
  artifact.bladed_feathers              = find_artifact_spell( "Bladed Feathers" );
  artifact.sunfire_burns                = find_artifact_spell( "Sunfire Burns" );
  artifact.solar_stabbing               = find_artifact_spell( "Solar Stabbing" );
  artifact.dark_side_of_the_moon        = find_artifact_spell( "Dark Side of the Moon" );
  artifact.rejuvenating_innervation     = find_artifact_spell( "Rejuvenating Innervation" );
  artifact.twilight_glow                = find_artifact_spell( "Twilight Glow" );
  artifact.scythe_of_the_stars          = find_artifact_spell( "Scythe of the Stars" );
  artifact.mooncraze                    = find_artifact_spell( "Mooncraze" );
  artifact.light_of_the_sun             = find_artifact_spell( "Light of the Sun" );
  artifact.empowerment                  = find_artifact_spell( "Empowerment" );

  // Masteries ==============================================================

  mastery.razor_claws         = find_mastery_spell( DRUID_FERAL );
  mastery.harmony             = find_mastery_spell( DRUID_RESTORATION );
  mastery.natures_guardian    = find_mastery_spell( DRUID_GUARDIAN );
  mastery.natures_guardian_AP = find_spell( 159195 );
  mastery.starlight           = find_mastery_spell( DRUID_BALANCE );

  // Spells =================================================================

  spell.ferocious_bite                  = find_class_spell( "Ferocious Bite"              ) -> ok() ? find_spell( 22568  ) : spell_data_t::not_found(); // Get spell data for max_fb_energy calculation.
  spell.bear_form_passive               = find_class_spell( "Bear Form"                   ) -> ok() ? find_spell( 1178   ) : spell_data_t::not_found(); // This is the passive applied on shapeshift!
  spell.berserk_cat                     = find_class_spell( "Berserk"                     ) -> ok() ? find_spell( 106951 ) : spell_data_t::not_found(); // Berserk cat resource cost reducer
  spell.cat_form                        = find_class_spell( "Cat Form"                    ) -> ok() ? find_spell( 3025   ) : spell_data_t::not_found();
  spell.cat_form_speed                  = find_class_spell( "Cat Form"                    ) -> ok() ? find_spell( 113636 ) : spell_data_t::not_found();

  spell.blessing_of_anshe               = find_spell( 202739 );
  spell.blessing_of_elune               = find_spell( 202737 );

  if ( specialization() == DRUID_FERAL )
  {
    spec.dream_of_cenarius = find_spell ( 158497 );
    spell.primal_fury      = find_spell( 16953 );
    spell.gushing_wound    = find_spell( 165432 );
  }
  else if ( specialization() == DRUID_GUARDIAN )
  {
    spec.dream_of_cenarius = find_spell( 158501 );
    spell.primal_fury      = find_spell( 16959 );
  }
  else if ( specialization() == DRUID_RESTORATION )
  {
    spec.dream_of_cenarius = find_spell( 158504 );
  }
  else if ( specialization() == DRUID_BALANCE )
  {
    spec.dream_of_cenarius  = find_spell( 108373 );
  }

  // Affinities =============================================================

  if ( specialization() == DRUID_FERAL || talent.feral_affinity -> ok() )
  {
    spell.feline_swiftness = find_specialization_spell( "Feline Swiftness" );
  }
  if ( specialization() == DRUID_BALANCE || talent.balance_affinity -> ok() )
  {
    spell.astral_influence = find_specialization_spell( "Astral Influence" );
  }
  if ( specialization() == DRUID_GUARDIAN || talent.guardian_affinity -> ok() )
  {
    spell.thick_hide = find_specialization_spell( "Thick Hide" );
  }
  if ( specialization() == DRUID_RESTORATION || talent.restoration_affinity -> ok() )
  {
    spell.yseras_gift = find_spell( 145108 );
  }

  // Glyphs =================================================================
  
  glyph.regrowth              = find_glyph_spell( "Glyph of Regrowth" );
  glyph.stampeding_roar       = find_glyph_spell( "Glyph of Stampeding Roar" );

  if ( sets.has_set_bonus( SET_CASTER, T16, B2 ) )
  {
    t16_2pc_starfall_bolt = new spells::t16_2pc_starfall_bolt_t( this );
    t16_2pc_sun_bolt = new spells::t16_2pc_sun_bolt_t( this );
  }

  caster_melee_attack = new caster_attacks::druid_melee_t( this );

  // Active Actions =========================================================

  if ( talent.cenarion_ward -> ok() )
    active.cenarion_ward_hot  = new heals::cenarion_ward_hot_t( this );
  if ( spell.yseras_gift )
    active.yseras_gift        = new heals::yseras_tick_t( this );
  if ( sets.has_set_bonus( DRUID_FERAL, T17, B4 ) )
    active.gushing_wound      = new cat_attacks::gushing_wound_t( this );
  if ( specialization() == DRUID_BALANCE )
    active.starshards         = new spells::starshards_t( this );
  if ( specialization() == DRUID_GUARDIAN )
    active.stalwart_guardian  = new stalwart_guardian_t( this );
  if ( talent.brambles -> ok() )
  {
    active.brambles           = new brambles_t( this );
    instant_absorb_list[ talent.brambles -> id() ] =
      new instant_absorb_t( this, talent.brambles, "brambles", &brambles_handler );
  }
  if ( talent.galactic_guardian -> ok() )
    active.galactic_guardian  = new spells::moonfire_t( this, "" );
}

// druid_t::init_base =======================================================

void druid_t::init_base_stats()
{
  player_t::init_base_stats();

  // Set base distance based on spec
  base.distance = ( specialization() == DRUID_FERAL || specialization() == DRUID_GUARDIAN ) ? 3 : 30;

  // All specs get benefit from both agi and intellect.
  base.attack_power_per_agility  = 1.0;
  base.spell_power_per_intellect = 1.0;

  // Resources
  resources.base[ RESOURCE_ENERGY       ] = 100 + sets.set( DRUID_FERAL, T18, B2 ) -> effectN( 2 ).resource( RESOURCE_ENERGY );
  resources.base[ RESOURCE_RAGE         ] = 100;
  resources.base[ RESOURCE_COMBO_POINT  ] = 5;
  resources.base[ RESOURCE_ASTRAL_POWER ] = 100;
  
  resources.active_resource[ RESOURCE_ASTRAL_POWER ] = specialization() == DRUID_BALANCE;
  resources.active_resource[ RESOURCE_HEALTH       ] = primary_role() == ROLE_TANK || talent.guardian_affinity -> ok();
  resources.active_resource[ RESOURCE_MANA         ] = primary_role() == ROLE_HEAL || talent.restoration_affinity -> ok() || talent.balance_affinity -> ok();
  resources.active_resource[ RESOURCE_ENERGY       ] = primary_role() == ROLE_ATTACK || talent.feral_affinity -> ok();
  resources.active_resource[ RESOURCE_COMBO_POINT  ] = primary_role() == ROLE_ATTACK || talent.feral_affinity -> ok();
  resources.active_resource[ RESOURCE_RAGE         ] = primary_role() == ROLE_TANK || talent.guardian_affinity -> ok();

  base_energy_regen_per_second = 10;

  // Max Mana & Mana Regen modifiers
  resources.base_multiplier[ RESOURCE_MANA ] *= 1.0 + spec.natural_insight -> effectN( 1 ).percent();
  base.mana_regen_per_second *= 1.0 + spec.natural_insight -> effectN( 1 ).percent();
  base.mana_regen_per_second *= 1.0 + spec.mana_attunement -> effectN( 1 ).percent();

  base_gcd = timespan_t::from_seconds( 1.5 );

  // initialize resolve for Guardians
  if ( specialization() == DRUID_GUARDIAN )
    resolve_manager.init();
}

// druid_t::init_buffs ======================================================

void druid_t::create_buffs()
{
  player_t::create_buffs();

  using namespace buffs;

  // Generic / Multi-spec druid buffs
  buff.bear_form             = new bear_form_t( *this );
  buff.berserk               = new berserk_buff_t( *this );
  buff.cat_form              = new cat_form_t( *this );
  buff.dash                  = buff_creator_t( this, "dash", find_class_spell( "Dash" ) )
                               .cd( timespan_t::zero() )
                               .default_value( find_class_spell( "Dash" ) -> effectN( 1 ).percent() );
  buff.moonkin_form          = new moonkin_form_t( *this );
  buff.clearcasting          = new clearcasting_buff_t( *this );
  buff.soul_of_the_forest    = buff_creator_t( this, "soul_of_the_forest", talent.soul_of_the_forest -> ok() ? find_spell( 114108 ) : spell_data_t::not_found() )
                               .default_value( find_spell( 114108 ) -> effectN( 1 ).percent() );
  buff.prowl                 = buff_creator_t( this, "prowl", find_class_spell( "Prowl" ) );

  // Talent buffs

  buff.displacer_beast    = buff_creator_t( this, "displacer_beast", talent.displacer_beast -> effectN( 2 ).trigger() )
                            .default_value( talent.displacer_beast -> effectN( 2 ).trigger() -> effectN( 1 ).percent() );

  buff.wild_charge_movement = buff_creator_t( this, "wild_charge_movement" );

  buff.cenarion_ward = buff_creator_t( this, "cenarion_ward", find_talent_spell( "Cenarion Ward" ) );

  switch ( specialization() ) {
    case DRUID_BALANCE:     buff.incarnation = buff_creator_t( this, "incarnation", talent.incarnation_moonkin )
                             .default_value( talent.incarnation_moonkin -> effectN( 1 ).percent() )
                             .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
                             .cd( timespan_t::zero() );
                            break;
    case DRUID_FERAL:       buff.incarnation = buff_creator_t( this, "incarnation", talent.incarnation_cat )
                             .cd( timespan_t::zero() );
                            break;
    case DRUID_GUARDIAN:    buff.incarnation = buff_creator_t( this, "incarnation", talent.incarnation_bear )
                             .cd( timespan_t::zero() );
                            break;
    case DRUID_RESTORATION: buff.incarnation = buff_creator_t( this, "incarnation", talent.incarnation_tree )
                             .duration( timespan_t::from_seconds( 30 ) )
                             .cd( timespan_t::zero() );
                            break;
    default:
      break;
  }

  if ( specialization() == DRUID_GUARDIAN )
    buff.dream_of_cenarius = buff_creator_t( this, "dream_of_cenarius", spec.dream_of_cenarius )
                            .chance( spec.dream_of_cenarius -> effectN( 1 ).percent() );
  else
    buff.dream_of_cenarius = buff_creator_t( this, "dream_of_cenarius", spec.dream_of_cenarius );

  buff.bloodtalons        = buff_creator_t( this, "bloodtalons", talent.bloodtalons -> ok() ? find_spell( 145152 ) : spell_data_t::not_found() )
                            .max_stack( 2 );

  buff.elunes_guidance    = buff_creator_t( this, "elunes_guidance", talent.elunes_guidance )
                            .tick_callback( [ this ]( buff_t*, int, const timespan_t& ) {
                              resource_gain( RESOURCE_COMBO_POINT,
                                talent.elunes_guidance -> effectN( 2 ).trigger() -> effectN( 1 ).resource( RESOURCE_COMBO_POINT ),
                                gain.elunes_guidance ); } )
                            .cd( timespan_t::zero() )
                            .period( talent.elunes_guidance -> effectN( 2 ).period() );

  // Balance

  buff.blessing_of_anshe         = buff_creator_t( this, "blessing_of_anshe", spell.blessing_of_anshe )
                                   .tick_time_behavior( BUFF_TICK_TIME_HASTED )
                                   .tick_callback( [ this ]( buff_t* b, int, const timespan_t& ) {
                                       resource_gain( RESOURCE_ASTRAL_POWER, b -> data().effectN( 1 ).resource( RESOURCE_ASTRAL_POWER ), gain.blessing_of_anshe );
                                    });

  buff.blessing_of_elune         = buff_creator_t( this, "blessing_of_elune", spell.blessing_of_elune );

  buff.celestial_alignment       = new celestial_alignment_buff_t( *this );

  buff.collapsing_stars_up       = buff_creator_t( this, "collapsing_stars_up", spell_data_t::nil() )
                                   .max_stack( 10 ); // Tracking buff for APL use

  buff.owlkin_frenzy             = buff_creator_t( this, "owlkin_frenzy", find_spell( 157228 ) )
                                   .chance( spec.moonkin_form -> proc_chance() );

  buff.lunar_empowerment         = buff_creator_t( this, "lunar_empowerment", find_spell( 164547 ) )
                                   .default_value( find_spell( 164547 ) -> effectN( 1 ).percent()
                                                 + talent.soul_of_the_forest -> effectN( 1 ).percent()
                                                 + artifact.empowerment.percent() );

  buff.solar_empowerment         = buff_creator_t( this, "solar_empowerment", find_spell( 164545 ) )
                                   .default_value( find_spell( 164545 ) -> effectN( 1 ).percent()
                                                 + talent.soul_of_the_forest -> effectN( 1 ).percent()
                                                 + artifact.empowerment.percent() );

  buff.star_power                = buff_creator_t( this, "star_power", find_spell( 202942 ) )
                                   .default_value( find_spell( 202942 ) -> effectN( 1 ).percent() )
                                   .add_invalidate( CACHE_SPELL_HASTE );

  buff.warrior_of_elune          = new warrior_of_elune_buff_t( *this );

  buff.balance_tier18_4pc        = buff_creator_t( this, "faerie_blessing", find_spell( 188086 ) )
                                   .chance( sets.has_set_bonus( DRUID_BALANCE, T18, B4 ) )
                                   .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );

  // Feral
  buff.tigers_fury           = buff_creator_t( this, "tigers_fury", find_specialization_spell( "Tiger's Fury" ) )
                               .default_value( find_specialization_spell( "Tiger's Fury" ) -> effectN( 1 ).percent() )
                               .cd( timespan_t::zero() )
                               .refresh_behavior( BUFF_REFRESH_PANDEMIC ); // Legion TOCHECK
  buff.savage_roar           = buff_creator_t( this, "savage_roar", talent.savage_roar )
                               .default_value( talent.savage_roar -> effectN( 2 ).percent() )
                               .refresh_behavior( BUFF_REFRESH_DURATION ) // Pandemic refresh is done by the action
                               .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  buff.predatory_swiftness   = buff_creator_t( this, "predatory_swiftness", spec.predatory_swiftness -> ok() ? find_spell( 69369 ) : spell_data_t::not_found() );
  buff.feral_tier15_4pc      = buff_creator_t( this, "feral_tier15_4pc", find_spell( 138358 ) );
  buff.feral_tier16_2pc      = buff_creator_t( this, "feral_tier16_2pc", find_spell( 144865 ) ); // tier16_2pc_melee
  buff.feral_tier16_4pc      = buff_creator_t( this, "feral_tier16_4pc", find_spell( 146874 ) ); // tier16_4pc_melee
  buff.feral_tier17_4pc      = buff_creator_t( this, "feral_tier17_4pc", find_spell( 166639 ) )
                               .quiet( true );

  // Guardian
  buff.barkskin              = buff_creator_t( this, "barkskin", find_specialization_spell( "Barkskin" ) )
                               .cd( timespan_t::zero() )
                               .default_value( find_specialization_spell( "Barkskin" ) -> effectN( 2 ).percent() );
  buff.bladed_armor          = buff_creator_t( this, "bladed_armor", spec.bladed_armor )
                               .add_invalidate( CACHE_ATTACK_POWER );
  buff.bristling_fur         = buff_creator_t( this, "bristling_fur", talent.bristling_fur )
                               .cd( timespan_t::zero() );
  buff.earthwarden           = buff_creator_t( this, "earthwarden", find_spell( 203975 ) )
                               .default_value( talent.earthwarden -> effectN( 1 ).percent() );
  buff.earthwarden_driver    = buff_creator_t( this, "earthwarden_driver", talent.earthwarden )
                               .quiet( true )
                               .tick_callback( [ this ] ( buff_t*, int, const timespan_t& ) { buff.earthwarden -> trigger(); } )
                               .tick_zero( true );
  buff.mark_of_ursol         = buff_creator_t( this, "mark_of_ursol", find_specialization_spell( "Mark of Ursol" ) )
                               .default_value( find_specialization_spell( "Mark of Ursol" ) -> effectN( 1 ).percent() )
                               .cd( timespan_t::zero() ) // cooldown handled by spell
                               .refresh_behavior( BUFF_REFRESH_EXTEND ) // Legion TOCHECK
                               .duration( find_specialization_spell( "Mark of Ursol" ) -> duration()
                                        + talent.guardian_of_elune -> effectN( 1 ).time_value() ); 
  buff.pulverize             = buff_creator_t( this, "pulverize", find_spell( 158792 ) )
                               .default_value( find_spell( 158792 ) -> effectN( 1 ).percent() )
                               .refresh_behavior( BUFF_REFRESH_PANDEMIC );
  buff.survival_instincts    = buff_creator_t( this, "survival_instincts", find_specialization_spell( "Survival Instincts" ) )
                               .cd( timespan_t::zero() )
                               .default_value( 0.0 - find_specialization_spell( "Survival Instincts" ) -> effectN( 1 ).percent() );
  buff.guardian_tier15_2pc   = buff_creator_t( this, "guardian_tier15_2pc", find_spell( 138217 ) );
  buff.guardian_tier17_4pc   = buff_creator_t( this, "guardian_tier17_4pc", find_spell( 177969 ) )
                               .chance( find_spell( 177969 ) -> proc_chance() )
                               .duration( find_spell( 177969 ) -> duration() )
                               .max_stack( find_spell( 177969 ) -> max_stacks() )
                               .default_value( find_spell( 177969 ) -> effectN( 1 ).percent() );
  buff.ironfur               = buff_creator_t( this, "ironfur", spec.ironfur )
                               .duration( timespan_t::zero() )
                               .default_value( spec.ironfur -> effectN( 1 ).percent() )
                               .add_invalidate( CACHE_ARMOR )
                               .max_stack( spec.ironfur -> max_stacks() + 2 ); // Just add two regardless of talents because why not.

  for ( size_t i = 0; i < 9; i++ )
  {
    char s[50];
    sprintf( s, "ironfur_%x", static_cast<unsigned>( 10 + i ) );
    buff.ironfur_stack[i]    = new ironfur_buff_t( *this, s );
  }

  // Restoration
  buff.harmony               = buff_creator_t( this, "harmony", mastery.harmony -> ok() ? find_spell( 100977 ) : spell_data_t::not_found() );

  if ( specialization() == DRUID_RESTORATION || talent.restoration_affinity -> ok() )
  {
    buff.yseras_gift         = buff_creator_t( this, "yseras_gift_driver", spell.yseras_gift )
                               .quiet( true )
                               .tick_callback( [ this ]( buff_t*, int, const timespan_t& )
                                               { active.yseras_gift -> base_dd_min = spell.yseras_gift -> effectN( 1 ).percent() * resources.max[ RESOURCE_HEALTH ];
                                                 active.yseras_gift -> execute(); } )
                               .tick_zero( true );
  }
}

// ALL Spec Pre-Combat Action Priority List =================================

void druid_t::apl_precombat()
{
  action_priority_list_t* precombat = get_action_priority_list( "precombat" );

  // Flask or Elixir
  if ( sim -> allow_flasks && true_level >= 80 )
  {
    std::string flask = "flask,type=";
    std::string elixir1, elixir2;
    elixir1 = elixir2 = "elixir,type=";

    if ( primary_role() == ROLE_TANK ) // Guardian
    {
      if ( true_level > 90 )
        flask += "greater_draenic_agility_flask";
      else if ( true_level > 85 )
        flask += "winds";
      else
        flask += "steelskin";
    }
    else if ( primary_role() == ROLE_ATTACK ) // Feral
    {
      if ( true_level > 90 )
        flask += "greater_draenic_agility_flask";
      else
        flask += "winds";
    }
    else // Balance & Restoration
    {
      if ( true_level > 90 )
        flask += "greater_draenic_intellect_flask";
      else if ( true_level > 85 )
        flask += "warm_sun";
      else
        flask += "draconic_mind";
    }

    if ( ! util::str_compare_ci( flask, "flask,type=" ) )
      precombat -> add_action( flask );
    else if ( ! util::str_compare_ci( elixir1, "elixir,type=" ) )
    {
      precombat -> add_action( elixir1 );
      precombat -> add_action( elixir2 );
    }
  }

  // Food
  if ( sim -> allow_food && level() > 80 )
  {
    std::string food = "food,type=";

    if ( level() > 90 )
    {
      if ( specialization() == DRUID_FERAL )
        food += "pickled_eel";
      else if ( specialization() == DRUID_BALANCE )
        food += "sleeper_sushi";
      else if ( specialization() == DRUID_GUARDIAN )
        food += "sleeper_sushi";
      else
        food += "buttered_sturgeon";
    }
    else if ( level() > 85 )
      food += "seafood_magnifique_feast";
    else
      food += "seafood_magnifique_feast";

    precombat -> add_action( food );
  }

  // Mark of the Wild
  precombat -> add_action( this, "Mark of the Wild", "if=!aura.str_agi_int.up" );

  // Feral: Bloodtalons
  if ( specialization() == DRUID_FERAL && true_level >= 100 )
    precombat -> add_action( this, "Healing Touch", "if=talent.bloodtalons.enabled" );

  // Forms
  if ( ( specialization() == DRUID_FERAL && primary_role() == ROLE_ATTACK ) || primary_role() == ROLE_ATTACK )
  {
    precombat -> add_action( this, "Cat Form" );
    precombat -> add_action( this, "Prowl" );
  }
  else if ( primary_role() == ROLE_TANK )
  {
    precombat -> add_action( this, "Bear Form" );
  }
  else if ( specialization() == DRUID_BALANCE && ( primary_role() == ROLE_DPS || primary_role() == ROLE_SPELL ) )
  {
    precombat -> add_action( this, "Moonkin Form" );
    precombat -> add_action( "blessing_of_elune" );
  }

  // Snapshot stats
  precombat -> add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );

  // Pre-Potion
  if ( sim -> allow_potions && true_level >= 80 )
  {
    std::string potion_action = "potion,name=";
    if ( specialization() == DRUID_FERAL && primary_role() == ROLE_ATTACK )
    {
      if ( true_level > 90 )
        potion_action += "draenic_agility";
      else if ( true_level > 85 )
        potion_action += "tolvir";
      else
        potion_action += "tolvir";
      precombat -> add_action( potion_action );
    }
    else if ( ( specialization() == DRUID_BALANCE || specialization() == DRUID_RESTORATION ) && ( primary_role() == ROLE_SPELL || primary_role() == ROLE_HEAL ) )
    {
      if ( true_level > 90 )
        potion_action += "draenic_intellect";
      else if ( true_level > 85 )
        potion_action += "jade_serpent";
      else
        potion_action += "volcanic";
      precombat -> add_action( potion_action );
    }
  }

  // Spec Specific Optimizations
  if ( specialization() == DRUID_BALANCE )
  {
    precombat -> add_action( "incarnation" );
    precombat -> add_action( this, "Lunar Strike" );
  }
  else if ( specialization() == DRUID_GUARDIAN )
    precombat -> add_talent( this, "Cenarion Ward" );
  else if ( specialization() == DRUID_FERAL && ( find_item( "soul_capacitor") || find_item( "maalus_the_blood_drinker" ) ) )
    precombat -> add_action( "incarnation" );
}

// NO Spec Combat Action Priority List ======================================

void druid_t::apl_default()
{
  action_priority_list_t* def = get_action_priority_list( "default" );

  // Assemble Racials / On-Use Items / Professions
  std::string extra_actions = "";

  std::vector<std::string> racial_actions = get_racial_actions();
  for ( size_t i = 0; i < racial_actions.size(); i++ )
    extra_actions += add_action( racial_actions[ i ] );

  std::vector<std::string> item_actions = get_item_actions();
  for ( size_t i = 0; i < item_actions.size(); i++ )
    extra_actions += add_action( item_actions[ i ] );

  std::vector<std::string> profession_actions = get_profession_actions();
  for ( size_t i = 0; i < profession_actions.size(); i++ )
    extra_actions += add_action( profession_actions[ i ] );

  if ( primary_role() == ROLE_ATTACK )
  {
    def -> add_action( this, "Faerie Fire", "if=debuff.weakened_armor.stack<3" );
    def -> add_action( extra_actions );
    def -> add_action( this, "Rake", "if=remains<=duration*0.3" );
    def -> add_action( this, "Shred" );
    def -> add_action( this, "Ferocious Bite", "if=combo_points>=5" );
  }
  // Specless (or speced non-main role) druid who has a primary role of a healer
  else if ( primary_role() == ROLE_HEAL )
  {
    def -> add_action( extra_actions );
    def -> add_action( this, "Rejuvenation", "if=remains<=duration*0.3" );
    def -> add_action( this, "Healing Touch", "if=mana.pct>=30" );
  }
}

// Feral Combat Action Priority List =======================================

void druid_t::apl_feral()
{
  action_priority_list_t* def      = get_action_priority_list( "default"   );
  action_priority_list_t* finish   = get_action_priority_list( "finisher"  );
  action_priority_list_t* maintain = get_action_priority_list( "maintain"  );
  action_priority_list_t* generate = get_action_priority_list( "generator" );

  std::vector<std::string> racial_actions = get_racial_actions();
  std::string              potion_action  = "potion,name=";
  if ( true_level > 90 )
    potion_action += "draenic_agility";
  else if ( true_level > 85 )
    potion_action += "tolvir";
  else
    potion_action += "tolvir";

  // Main List =============================================================

  def -> add_action( this, "Cat Form" );
  def -> add_talent( this, "Wild Charge" );
  def -> add_talent( this, "Displacer Beast", "if=movement.distance>10" );
  def -> add_action( this, "Dash", "if=movement.distance&buff.displacer_beast.down&buff.wild_charge_movement.down" );
  if ( race == RACE_NIGHT_ELF )
    def -> add_action( this, "Rake", "if=buff.prowl.up|buff.shadowmeld.up" );
  else
    def -> add_action( this, "Rake", "if=buff.prowl.up" );
  def -> add_action( "auto_attack" );
  def -> add_action( this, "Skull Bash" );
  def -> add_talent( this, "Force of Nature", "if=charges=3|trinket.proc.all.react|target.time_to_die<20");
  def -> add_action( this, "Berserk", "if=buff.tigers_fury.up&(buff.incarnation.up|!talent.incarnation_king_of_the_jungle.enabled)" );

  // On-Use Items
  for ( size_t i = 0; i < items.size(); i++ )
  {
    if ( items[ i ].has_use_special_effect() )
    {
      std::string line = std::string( "use_item,slot=" ) + items[ i ].slot_name();
      if ( items[ i ].name_str == "mirror_of_the_blademaster" )
        line += ",if=raid_event.adds.in>60|!raid_event.adds.exists|spell_targets.swipe>desired_targets";
      else if ( items[ i ].name_str != "maalus_the_blood_drinker" )
        line += ",if=(prev.tigers_fury&(target.time_to_die>trinket.stat.any.cooldown|target.time_to_die<45))|prev.berserk|(buff.incarnation.up&time<10)";

      def -> add_action( line );
    }
  }

  if ( sim -> allow_potions && true_level >= 80 )
    def -> add_action( potion_action + ",if=(buff.berserk.remains>10&(target.time_to_die<180|(trinket.proc.all.react&target.health.pct<25)))|target.time_to_die<=40" );

  // Racials
  for ( size_t i = 0; i < racial_actions.size(); i++ )
  {
    def -> add_action( racial_actions[ i ] + ",sync=tigers_fury" );
  }

  def -> add_action( this, "Tiger's Fury", "if=(!buff.clearcasting.react&energy.deficit>=60)|energy.deficit>=80|(t18_class_trinket&buff.berserk.up&buff.tigers_fury.down)" );
  def -> add_action( "incarnation,if=cooldown.berserk.remains<10&energy.time_to_max>1" );
  def -> add_action( this, "Ferocious Bite", "cycle_targets=1,if=dot.rip.ticking&dot.rip.remains<3&target.health.pct<25",
                     "Keep Rip from falling off during execute range." );
  def -> add_action( this, "Healing Touch", "if=talent.bloodtalons.enabled&buff.predatory_swiftness.up&((combo_points>=4&!set_bonus.tier18_4pc)|combo_points=5|buff.predatory_swiftness.remains<1.5)" );
  def -> add_action( this, "Savage Roar", "if=buff.savage_roar.down" );
  def -> add_action( "thrash_cat,if=set_bonus.tier18_4pc&buff.clearcasting.react&remains<4.5&combo_points+buff.bloodtalons.stack!=6" );
  def -> add_action( "pool_resource,for_next=1" );
  def -> add_action( "thrash_cat,cycle_targets=1,if=remains<4.5&(spell_targets.thrash_cat>=2&set_bonus.tier17_2pc|spell_targets.thrash_cat>=4)" );
  def -> add_action( "call_action_list,name=finisher,if=combo_points=5" );
  def -> add_action( this, "Savage Roar", "if=buff.savage_roar.remains<gcd" );
  def -> add_action( "call_action_list,name=maintain,if=combo_points<5" );
  def -> add_action( "pool_resource,for_next=1" );
  def -> add_action( "thrash_cat,cycle_targets=1,if=remains<4.5&spell_targets.thrash_cat>=2" );
  def -> add_action( "call_action_list,name=generator,if=combo_points<5" );

  // Finishers  
  finish -> add_action( this, "Rip", "cycle_targets=1,if=remains<2&target.time_to_die-remains>18&(target.health.pct>25|!dot.rip.ticking)" );
  finish -> add_action( this, "Ferocious Bite", "cycle_targets=1,max_energy=1,if=target.health.pct<25&dot.rip.ticking" );
  finish -> add_action( this, "Rip", "cycle_targets=1,if=remains<7.2&persistent_multiplier>dot.rip.pmultiplier&target.time_to_die-remains>18" );
  finish -> add_action( this, "Rip", "cycle_targets=1,if=remains<7.2&persistent_multiplier=dot.rip.pmultiplier&(energy.time_to_max<=1|(set_bonus.tier18_4pc&energy>50)|(set_bonus.tier18_2pc&buff.clearcasting.react)|!talent.bloodtalons.enabled)&target.time_to_die-remains>18" );
  finish -> add_action( this, "Savage Roar", "if=((set_bonus.tier18_4pc&energy>50)|(set_bonus.tier18_2pc&buff.clearcasting.react)|energy.time_to_max<=1|buff.berserk.up|cooldown.tigers_fury.remains<3)&buff.savage_roar.remains<12.6" );
  finish -> add_action( this, "Ferocious Bite", "max_energy=1,if=(set_bonus.tier18_4pc&energy>50)|(set_bonus.tier18_2pc&buff.clearcasting.react)|energy.time_to_max<=1|buff.berserk.up|cooldown.tigers_fury.remains<3" );

  // DoT Maintenance
  if ( race == RACE_NIGHT_ELF )
  {
    maintain -> add_action( "shadowmeld,if=energy>=35&dot.rake.pmultiplier<2.1&buff.tigers_fury.up&(buff.bloodtalons.up|!talent.bloodtalons.enabled)&(!talent.incarnation.enabled|cooldown.incarnation.remains>18)&!buff.incarnation.up" );
  }
  maintain -> add_action( this, "Rake", "cycle_targets=1,if=remains<3&((target.time_to_die-remains>3&spell_targets.swipe<3)|target.time_to_die-remains>6)" );
  maintain -> add_action( this, "Rake", "cycle_targets=1,if=remains<4.5&(persistent_multiplier>=dot.rake.pmultiplier|(talent.bloodtalons.enabled&(buff.bloodtalons.up|!buff.predatory_swiftness.up)))&((target.time_to_die-remains>3&spell_targets.swipe<3)|target.time_to_die-remains>6)" );
  maintain -> add_action( "moonfire_cat,cycle_targets=1,if=remains<4.2&spell_targets.swipe<=5&target.time_to_die-remains>tick_time*5" );
  maintain -> add_action( this, "Rake", "cycle_targets=1,if=persistent_multiplier>dot.rake.pmultiplier&spell_targets.swipe=1&((target.time_to_die-remains>3&spell_targets.swipe<3)|target.time_to_die-remains>6)" );

  // Generators
  generate -> add_action( this, "Swipe", "if=spell_targets.swipe>=4|(spell_targets.swipe>=3&buff.incarnation.down)" );
  generate -> add_action( this, "Shred", "if=spell_targets.swipe<3|(spell_targets.swipe=3&buff.incarnation.up)" );
}

// Balance Combat Action Priority List ==============================

void druid_t::apl_balance()
{
  std::vector<std::string> racial_actions = get_racial_actions();
  std::vector<std::string> item_actions   = get_item_actions();
  std::string              potion_action  = "potion,name=";
  if ( true_level > 90 )
    potion_action += "draenic_intellect";
  else if ( true_level > 85 )
    potion_action += "jade_serpent";
  else
    potion_action += "volcanic";

  action_priority_list_t* default_list        = get_action_priority_list( "default" );
  //action_priority_list_t* single_target       = get_action_priority_list( "single_target" );
  //action_priority_list_t* aoe                 = get_action_priority_list( "aoe" );

  if ( sim -> allow_potions && true_level >= 80 )
    default_list -> add_action( potion_action + ",if=buff.celestial_alignment.up" );

  for ( size_t i = 0; i < racial_actions.size(); i++ )
    default_list -> add_action( racial_actions[i] + ",if=buff.celestial_alignment.up" );
  for ( size_t i = 0; i < item_actions.size(); i++ )
    default_list -> add_action( item_actions[i] );

  default_list -> add_action( "blessing_of_elune,moving=0" );
  default_list -> add_action( "blessing_of_anshe,moving=1" );
  default_list -> add_action( "warrior_of_elune,if=buff.lunar_empowerment.stack>=2" );
  default_list -> add_action( "stellar_flare,if=remains<2" );
  default_list -> add_action( this, "Moonfire", "if=remains<2" );
  default_list -> add_action( this, "Sunfire", "if=remains<2" );
  default_list -> add_action( "astral_communion,if=astral_power.deficit>=75" );
  default_list -> add_action( "incarnation,if=astral_power>=40" );
  default_list -> add_action( this, "Celestial Alignment", "if=astral_power>=40" );
  default_list -> add_action( "collapsing_stars,if=astral_power.deficit<=10" );
  default_list -> add_action( this, "Lunar Strike", "if=talent.natures_balance.enabled&dot.moonfire.remains<5" );
  default_list -> add_action( this, "Solar Wrath", "if=talent.natures_balance.enabled&dot.sunfire.remains<5" );
  default_list -> add_action( this, "Lunar Strike", "if=buff.lunar_empowerment.stack=3" );
  default_list -> add_action( this, "Solar Wrath", "if=buff.solar_empowerment.stack=3" );
  default_list -> add_action( this, "Starsurge", "if=!talent.collapsing_stars.enabled|(buff.collapsing_stars_up.down&(cooldown.collapsing_stars.remains>10|astral_power.deficit<=10))" );
  default_list -> add_action( this, "Lunar Strike", "if=buff.lunar_empowerment.up&(!talent.warrior_of_elune.enabled|buff.warrior_of_elune.up)" );
  default_list -> add_action( this, "Solar Wrath", "if=buff.solar_empowerment.up" );
  default_list -> add_action( this, "Lunar Strike", "if=talent.full_moon.enabled|action.solar_wrath.cast_time<1" );
  default_list -> add_action( this, "Solar Wrath" );
}

// Guardian Combat Action Priority List ==============================

void druid_t::apl_guardian()
{
  action_priority_list_t* default_list    = get_action_priority_list( "default" );

  std::vector<std::string> item_actions       = get_item_actions();
  std::vector<std::string> racial_actions     = get_racial_actions();

  default_list -> add_action( "auto_attack" );
  default_list -> add_action( this, "Skull Bash" );

  for ( size_t i = 0; i < racial_actions.size(); i++ )
    default_list -> add_action( racial_actions[i] );
  for ( size_t i = 0; i < item_actions.size(); i++ )
    default_list -> add_action( item_actions[i] );
  
  default_list -> add_action( this, "Barkskin" );
  default_list -> add_action( "bristling_fur,if=buff.ironfur.remains<2&rage<40" );
  default_list -> add_action( this, "Ironfur", "if=buff.ironfur.down|rage.deficit<25" );
  default_list -> add_action( this, "Frenzied Regeneration", "if=!ticking&incoming_damage_6s%health.max>0.25+(2-charges_fractional)*0.15" );
  default_list -> add_action( "pulverize,cycle_targets=1,if=buff.pulverize.down" );
  default_list -> add_action( this, "Mangle" );
  default_list -> add_action( "pulverize,cycle_targets=1,if=buff.pulverize.remains<gcd" );
  default_list -> add_action( "lunar_beam" );
  default_list -> add_action( "incarnation" );
  default_list -> add_action( "thrash_bear,if=active_enemies>=2" );
  default_list -> add_action( "pulverize,cycle_targets=1,if=buff.pulverize.remains<3.6" );
  default_list -> add_action( this, "Lacerate", "cycle_targets=1,if=dot.lacerate.stack<3" );
  default_list -> add_action( this, "Lacerate" );
  default_list -> add_action( "thrash_bear,if=talent.pulverize.enabled&buff.pulverize.remains<3.6" );
  default_list -> add_action( this, "Moonfire", "cycle_targets=1,if=!ticking" );
  default_list -> add_action( this, "Moonfire", "cycle_targets=1,if=remains<3.6" );
  default_list -> add_action( this, "Moonfire", "cycle_targets=1,if=remains<7.2" );
  default_list -> add_action( this, "Moonfire" );
}

// Restoration Combat Action Priority List ==============================

void druid_t::apl_restoration()
{
  action_priority_list_t* default_list    = get_action_priority_list( "default" );

  std::vector<std::string> item_actions       = get_item_actions();
  std::vector<std::string> racial_actions     = get_racial_actions();

  for ( size_t i = 0; i < racial_actions.size(); i++ )
    default_list -> add_action( racial_actions[i] );
  for ( size_t i = 0; i < item_actions.size(); i++ )
    default_list -> add_action( item_actions[i] );

  default_list -> add_action( this, "Natures Swiftness" );
  default_list -> add_talent( this, "Incarnation" );
  default_list -> add_action( this, "Healing Touch", "if=buff.clearcasting.up" );
  default_list -> add_action( this, "Rejuvenation", "if=remains<=duration*0.3" );
  default_list -> add_action( this, "Lifebloom", "if=debuff.lifebloom.down" );
  default_list -> add_action( this, "Swiftmend" );
  default_list -> add_action( this, "Healing Touch" );
}

// druid_t::init_scaling ====================================================

void druid_t::init_scaling()
{
  player_t::init_scaling();

  equipped_weapon_dps = main_hand_weapon.damage / main_hand_weapon.swing_time.total_seconds();

  if ( specialization() == DRUID_GUARDIAN )
  {
    scales_with[ STAT_WEAPON_DPS ] = false;
    scales_with[ STAT_PARRY_RATING ] = false;
    scales_with[ STAT_BONUS_ARMOR ] = true;
  }

  scales_with[ STAT_STRENGTH ] = false;

  // Save a copy of the weapon
  caster_form_weapon = main_hand_weapon;
}

// druid_t::init ============================================================

void druid_t::init()
{
  player_t::init();

  if ( specialization() == DRUID_RESTORATION )
    sim -> errorf( "%s is using an unsupported spec.", name() );
}
// druid_t::init_gains ======================================================

void druid_t::init_gains()
{
  player_t::init_gains();

  // Balance
  gain.astral_communion      = get_gain( "astral_communion"      );
  gain.blessing_of_anshe     = get_gain( "blessing_of_anshe"     );
  gain.blessing_of_elune     = get_gain( "blessing_of_elune"     );
  gain.celestial_alignment   = get_gain( "celestial_alignment"   );
  gain.lunar_strike          = get_gain( "lunar_strike"          );
  gain.shooting_stars        = get_gain( "shooting_stars"        );
  gain.solar_wrath           = get_gain( "solar_wrath"           );

  // Feral
  gain.bloody_slash          = get_gain( "bloody_slash"          );
  gain.energy_refund         = get_gain( "energy_refund"         );
  gain.elunes_guidance       = get_gain( "elunes_guidance"       );
  gain.moonfire              = get_gain( "moonfire"              );
  gain.clearcasting          = get_gain( "clearcasting"          );
  gain.primal_fury           = get_gain( "primal_fury"           );
  gain.rake                  = get_gain( "rake"                  );
  gain.shred                 = get_gain( "shred"                 );
  gain.soul_of_the_forest    = get_gain( "soul_of_the_forest"    );
  gain.swipe                 = get_gain( "swipe"                 );
  gain.tigers_fury           = get_gain( "tigers_fury"           );

  // Guardian
  gain.bear_form             = get_gain( "bear_form"             );
  gain.brambles              = get_gain( "brambles"              );
  gain.bristling_fur         = get_gain( "bristling_fur"         );
  gain.rage_refund           = get_gain( "rage_refund"           );
  gain.stalwart_guardian     = get_gain( "stalwart_guardian"     );
  
  // Set Bonuses
  gain.feral_tier15_2pc      = get_gain( "feral_tier15_2pc"      );
  gain.feral_tier16_4pc      = get_gain( "feral_tier16_4pc"      );
  gain.feral_tier17_2pc      = get_gain( "feral_tier17_2pc"      );
  gain.feral_tier18_4pc      = get_gain( "feral_tier18_4pc"      );
  gain.guardian_tier17_2pc   = get_gain( "guardian_tier17_2pc"   );
  gain.guardian_tier18_2pc   = get_gain( "guardian_tier18_2pc"   );
}

// druid_t::init_procs ======================================================

void druid_t::init_procs()
{
  player_t::init_procs();

  proc.clearcasting             = get_proc( "clearcasting"           );
  proc.clearcasting_wasted      = get_proc( "clearcasting_wasted"    );
  proc.predator                 = get_proc( "predator"               );
  proc.predator_wasted          = get_proc( "predator_wasted"        );
  proc.primal_fury              = get_proc( "primal_fury"            );
  proc.starshards               = get_proc( "Starshards"             );
  proc.tier15_2pc_melee         = get_proc( "tier15_2pc_melee"       );
  proc.tier17_2pc_melee         = get_proc( "tier17_2pc_melee"       );
}

// druid_t::init_resources ===========================================

void druid_t::init_resources( bool force )
{
  player_t::init_resources( force );

  resources.current[ RESOURCE_RAGE ] = 0;
  resources.current[ RESOURCE_COMBO_POINT ] = 0;
  resources.current[ RESOURCE_ASTRAL_POWER ] = initial_astral_power;
}

// druid_t::init_rng =======================================================

void druid_t::init_rng()
{
  // RPPM objects
  balance_tier18_2pc.set_frequency( sets.set( DRUID_BALANCE, T18, B2 ) -> real_ppm() );

  // Predator: optional RPPM approximation.
  predator.set_frequency( predator_rppm_rate );
  // Set all rampup to 0 so it doesn't proc on pull.
  predator.set_initial_precombat_time( timespan_t::zero() );
  predator.set_last_trigger_attempt( timespan_t::from_seconds( -1.0 ) );
  predator.set_last_trigger_success( timespan_t::zero() );

  player_t::init_rng();
}

// druid_t::init_actions ====================================================

void druid_t::init_action_list()
{
  if ( ! action_list_str.empty() )
  {
    player_t::init_action_list();
    return;
  }
  clear_action_priority_lists();

  apl_precombat(); // PRE-COMBAT

  switch ( specialization() )
  {
    case DRUID_FERAL:
      apl_feral(); // FERAL
      break;
    case DRUID_BALANCE:
      apl_balance();  // BALANCE
      break;
    case DRUID_GUARDIAN:
      apl_guardian(); // GUARDIAN
      break;
    case DRUID_RESTORATION:
      apl_restoration(); // RESTORATION
      break;
    default:
      apl_default(); // DEFAULT
      break;
  }

  use_default_action_list = true;

  player_t::init_action_list();
}

// druid_t::has_t18_class_trinket ===========================================

bool druid_t::has_t18_class_trinket() const
{
  switch( specialization() )
  {
    case DRUID_BALANCE:     return starshards != nullptr;
    case DRUID_FERAL:       return wildcat_celerity != nullptr;
    case DRUID_GUARDIAN:    return stalwart_guardian != nullptr;
    case DRUID_RESTORATION: return flourish != nullptr;
    default:                return false;
  }
}

// druid_t::reset ===========================================================

void druid_t::reset()
{
  player_t::reset();

  // Reset druid_t variables to their original state.
  form = NO_FORM;
  max_fb_energy = spell.ferocious_bite -> powerN( 1 ).cost() - spell.ferocious_bite -> effectN( 2 ).base_value();
  active_starfalls = 0;
  moon_stage = ( moon_stage_e ) initial_moon_stage;

  base_gcd = timespan_t::from_seconds( 1.5 );

  // Restore main hand attack / weapon to normal state
  main_hand_attack = caster_melee_attack;
  main_hand_weapon = caster_form_weapon;

  // Reset any custom events to be safe.
  persistent_buff_delay.clear();

  if ( mastery.natures_guardian -> ok() )
    recalculate_resource_max( RESOURCE_HEALTH );

  for ( size_t i = 0; i < sim -> actor_list.size(); i++ )
  {
    druid_td_t* td = target_data[ sim -> actor_list[ i ] ];
    if ( td ) td -> reset();
  }
}

// druid_t::merge ===========================================================

void druid_t::merge( player_t& other )
{
  player_t::merge( other );

  druid_t& od = static_cast<druid_t&>( other );

  for ( size_t i = 0, end = counters.size(); i < end; i++ )
    counters[ i ] -> merge( *od.counters[ i ] );
}

// druid_t::mana_regen_per_second ============================================================

double druid_t::mana_regen_per_second() const
{
  double mp5 = player_t::mana_regen_per_second();

  if ( buff.moonkin_form -> check() ) // Boomkins get 150% increased mana regeneration, scaling with haste.
    mp5 *= 1.0 + buff.moonkin_form -> data().effectN( 5 ).percent() + ( 1 / cache.spell_haste() );

  mp5 *= 1.0 + spec.mana_attunement -> effectN( 1 ).percent();

  return mp5;
}

// druid_t::available =======================================================

timespan_t druid_t::available() const
{
  if ( primary_resource() != RESOURCE_ENERGY )
    return timespan_t::from_seconds( 0.1 );

  double energy = resources.current[ RESOURCE_ENERGY ];

  if ( energy > 25 ) return timespan_t::from_seconds( 0.1 );

  return std::max(
           timespan_t::from_seconds( ( 25 - energy ) / energy_regen_per_second() ),
           timespan_t::from_seconds( 0.1 )
         );
}

// druid_t::arise ===========================================================

void druid_t::arise()
{
  player_t::arise();

  if ( talent.earthwarden -> ok() )
    buff.earthwarden -> trigger( buff.earthwarden -> max_stack() );

  if ( scythe_of_elune )
  {
    // Calculate chance for a Reaping proc to have occurred prior to combat.
    int ticks = buff.owlkin_frenzy -> buff_duration / buff.the_reaping -> buff_period;
    double chance = 1.0 - std::pow( 1.0 - buff.the_reaping -> default_value, ticks );

    buff.owlkin_frenzy -> trigger( 1, buff_t::DEFAULT_VALUE(), chance );
  }
}

// druid_t::combat_begin ====================================================

void druid_t::combat_begin()
{
  player_t::combat_begin();
  
  // Trigger persistent buffs
  if ( buff.yseras_gift )
    persistent_buff_delay.push_back( new (*sim) persistent_buff_delay_event_t( this, buff.yseras_gift ) );

  if ( talent.earthwarden -> ok() )
    persistent_buff_delay.push_back( new (*sim) persistent_buff_delay_event_t( this, buff.earthwarden_driver ) );

  if ( scythe_of_elune )
    persistent_buff_delay.push_back( new (*sim) persistent_buff_delay_event_t( this, buff.the_reaping ) );

  if ( spec.bladed_armor -> ok() )
    buff.bladed_armor -> trigger();
}

// druid_t::recalculate_resource_max ====================================

void druid_t::recalculate_resource_max( resource_e rt )
{
  double pct_health, current_health;
  if ( mastery.natures_guardian -> ok() && rt == RESOURCE_HEALTH )
  {
    current_health = resources.current[ rt ];
    pct_health = resources.pct( rt );
  }

  player_t::recalculate_resource_max( rt );

  if ( mastery.natures_guardian -> ok() && rt == RESOURCE_HEALTH )
  {
    resources.max[ rt ] *= 1.0 + cache.mastery_value();
    // Maintain current health pct. Legion TOCHECK
    resources.current[ rt ] = resources.max[ rt ] * pct_health;

    if ( sim -> log )
      sim -> out_log.printf( "%s recalculates maximum health. old_current=%.0f new_current=%.0f net_health=%.0f",
        name(), current_health, resources.current[ rt ], resources.current[ rt ] - current_health );
  }
}

// druid_t::invalidate_cache ================================================

void druid_t::invalidate_cache( cache_e c )
{
  player_t::invalidate_cache( c );

  switch ( c )
  {
    case CACHE_AGILITY:
      if ( spec.nurturing_instinct -> ok() )
        player_t::invalidate_cache( CACHE_SPELL_POWER );
      break;
    case CACHE_INTELLECT:
      if ( spec.killer_instinct -> ok() )
        player_t::invalidate_cache( CACHE_AGILITY );
      break;
    case CACHE_MASTERY:
      if ( mastery.natures_guardian -> ok() )
      {
        player_t::invalidate_cache( CACHE_ATTACK_POWER );
        recalculate_resource_max( RESOURCE_HEALTH );
      }
      break;
    case CACHE_BONUS_ARMOR:
      if ( spec.bladed_armor -> ok() )
        player_t::invalidate_cache( CACHE_ATTACK_POWER );
      break;
    default: break;
  }
}

// druid_t::composite_attack_power_multiplier ===============================

double druid_t::composite_attack_power_multiplier() const
{
  double ap = player_t::composite_attack_power_multiplier();

  if ( mastery.natures_guardian -> ok() )
    ap *= 1.0 + cache.mastery() * mastery.natures_guardian_AP -> effectN( 1 ).mastery_value();

  return ap;
}

// druid_t::composite_armor_multiplier ======================================

double druid_t::composite_armor_multiplier() const
{
  double a = player_t::composite_armor_multiplier();

  if ( buff.bear_form -> check() )
    a *= 1.0 + buff.bear_form -> data().effectN( 3 ).percent();

  if ( buff.moonkin_form -> check() )
    a *= 1.0 + buff.moonkin_form -> data().effectN( 3 ).percent() + artifact.bladed_feathers.percent();

  if ( buff.ironfur -> check() )
    a *= 1.0 + buff.ironfur -> current_value * buff.ironfur -> current_stack;

  return a;
}

// druid_t::composite_persistent_multiplier =================================

double druid_t::composite_persistent_multiplier( school_e school ) const
{
  double pm = player_t::composite_persistent_multiplier( school );

  // Don't call up() for accurate benefit tracking.
  pm *= 1.0 + buff.tigers_fury -> check() * buff.tigers_fury -> default_value;

  return pm;
}

// druid_t::composite_player_multiplier =====================================

double druid_t::composite_player_multiplier( school_e school ) const
{
  double m = player_t::composite_player_multiplier( school );

  if ( specialization() == DRUID_BALANCE )
  {
    if ( dbc::is_school( school, SCHOOL_ARCANE ) || dbc::is_school( school, SCHOOL_NATURE ) )
    {
      if ( buff.incarnation -> check() )
        m *= 1.0 + buff.incarnation -> current_value;
      if ( buff.balance_tier18_4pc -> check() )
        m *= 1.0 + buff.balance_tier18_4pc -> data().effectN( 1 ).percent();
      if ( buff.moonkin_form -> check() )
        m *= 1.0 + buff.moonkin_form -> data().effectN( 9 ).percent();
    }
  }
  return m;
}

// druid_t::composite_melee_expertise( weapon_t* ) ==========================

double druid_t::composite_melee_expertise( const weapon_t* ) const
{
  double exp = player_t::composite_melee_expertise();

  if ( buff.bear_form -> check() )
    exp += buff.bear_form -> data().effectN( 8 ).base_value();

  return exp;
}

// druid_t::composite_melee_attack_power ==================================

double druid_t::composite_melee_attack_power() const
{
  double ap = player_t::composite_melee_attack_power();

  ap += buff.bladed_armor -> data().effectN( 1 ).percent() * current.stats.get_stat( STAT_BONUS_ARMOR );

  return ap;
}

// druid_t::composite_melee_crit ============================================

double druid_t::composite_melee_crit() const
{
  double crit = player_t::composite_melee_crit();

  crit += spec.critical_strikes -> effectN( 1 ).percent();

  return crit;
}

// druid_t::temporary_movement_modifier =========================================

double druid_t::temporary_movement_modifier() const
{
  double active = player_t::temporary_movement_modifier();

  if ( buff.dash -> up() )
    active = std::max( active, buff.dash -> value() );

  if ( buff.wild_charge_movement -> up() )
    active = std::max( active, buff.wild_charge_movement -> value() );

  if ( buff.displacer_beast -> up() )
    active = std::max( active, buff.displacer_beast -> value() );

  return active;
}

// druid_t::passive_movement_modifier ========================================

double druid_t::passive_movement_modifier() const
{
  double ms = player_t::passive_movement_modifier();

  if ( buff.cat_form -> up() )
    ms += spell.cat_form_speed -> effectN( 1 ).percent();

  ms += spell.feline_swiftness -> effectN( 1 ).percent();

  return ms;
}

// druid_t::composite_spell_crit ============================================

double druid_t::composite_spell_crit() const
{
  double crit = player_t::composite_spell_crit();

  crit += spec.critical_strikes -> effectN( 1 ).percent();

  return crit;
}

// druid_t::composite_spell_haste ============================================

double druid_t::composite_spell_haste() const
{
  double sh = player_t::composite_spell_haste();

  sh /= 1.0 + buff.star_power -> current_value * buff.star_power -> current_stack;

  return sh;
}

// druid_t::composite_spell_power ===========================================

double druid_t::composite_spell_power( school_e school ) const
{
  double p = player_t::composite_spell_power( school );

  if ( spec.nurturing_instinct -> ok() )
    p += spec.nurturing_instinct -> effectN( 1 ).percent() * cache.agility();

  return p;
}

// druid_t::composite_attribute =============================================

double druid_t::composite_attribute( attribute_e attr ) const
{
  double a = player_t::composite_attribute( attr );

  switch ( attr )
  {
    case ATTR_AGILITY:
      if ( spec.killer_instinct -> ok() && ( buff.bear_form -> up() || buff.cat_form -> up() ) )
        a += spec.killer_instinct -> effectN( 1 ).percent() * cache.intellect();
      break;
    default:
      break;
  }

  return a;
}

// druid_t::composite_attribute_multiplier ==================================

double druid_t::composite_attribute_multiplier( attribute_e attr ) const
{
  double m = player_t::composite_attribute_multiplier( attr );

  switch ( attr )
  {
    case ATTR_STAMINA:
      if( buff.bear_form -> check() )
        m *= 1.0 + spell.bear_form_passive -> effectN( 2 ).percent();
      break;
    default:
      break;
  }

  return m;
}

// druid_t::matching_gear_multiplier ========================================

double druid_t::matching_gear_multiplier( attribute_e attr ) const
{
  unsigned idx;

  switch ( attr )
  {
    case ATTR_AGILITY:
      idx = 1;
      break;
    case ATTR_INTELLECT:
      idx = 2;
      break;
    case ATTR_STAMINA:
      idx = 3;
      break;
    default:
      return 0;
  }

  return spec.leather_specialization -> effectN( idx ).percent();
}

// druid_t::composite_crit_avoidance =============================================

double druid_t::composite_crit_avoidance() const
{
  double c = player_t::composite_crit_avoidance();

  if ( buff.bear_form -> check() )
    c += buff.bear_form -> data().effectN( 7 ).percent();

  return c;
}

// druid_t::create_expression ===============================================

expr_t* druid_t::create_expression( action_t* a, const std::string& name_str )
{
  struct druid_expr_t : public expr_t
  {
    druid_t& druid;
    druid_expr_t( const std::string& n, druid_t& p ) :
      expr_t( n ), druid( p )
    {}
  };

  std::vector<std::string> splits = util::string_split( name_str, "." );

  if ( util::str_compare_ci( name_str, "astral_power" ) )
  {
    return make_ref_expr( name_str, resources.current[ RESOURCE_ASTRAL_POWER ] );
  }
  else if ( util::str_compare_ci( name_str, "combo_points" ) )
  {
    return make_ref_expr( "combo_points", resources.current[ RESOURCE_COMBO_POINT ] );
  }
  else if ( util::str_compare_ci( name_str, "max_fb_energy" ) )
  {
    return make_ref_expr( "max_fb_energy", max_fb_energy );
  }
  else if ( util::str_compare_ci( name_str, "new_moon" )
         || util::str_compare_ci( name_str, "half_moon" )
         || util::str_compare_ci( name_str, "full_moon" )  )
  {
    struct moon_stage_expr_t : public druid_expr_t
    {
      int stage;

      moon_stage_expr_t( druid_t& p, const std::string& name_str ) :
        druid_expr_t( name_str, p )
      {
        if ( util::str_compare_ci( name_str, "new_moon" ) )
          stage = 0;
        else if ( util::str_compare_ci( name_str, "half_moon" ) )
          stage = 1;
        else if ( util::str_compare_ci( name_str, "full_moon" ) )
          stage = 2;
        else
          assert( "Bad name_str passed to moon_stage_expr_t" );
      }

      virtual double evaluate() override
      { return druid.moon_stage == stage; }
    };

    return new moon_stage_expr_t( *this, name_str );
  }

  return player_t::create_expression( a, name_str );
}

// druid_t::create_options ==================================================

void druid_t::create_options()
{
  player_t::create_options();

  add_option( opt_float( "predator_rppm", predator_rppm_rate ) );
  add_option( opt_float( "initial_astral_power", initial_astral_power ) );
  add_option( opt_int( "initial_moon_stage", initial_moon_stage ) );
}

// druid_t::create_proc_action =============================================

action_t* druid_t::create_proc_action( const std::string& name, const special_effect_t& )
{
  if ( util::str_compare_ci( name, "shattered_bleed" ) && specialization() == DRUID_FERAL )
    return new cat_attacks::shattered_bleed_t( this );
  if ( util::str_compare_ci( name, "flurry_of_xuen" ) && specialization() == DRUID_FERAL )
    return new cat_attacks::flurry_of_xuen_t( this );

  return nullptr;
}

// druid_t::create_profile ==================================================

std::string druid_t::create_profile( save_e type )
{
  return player_t::create_profile( type );
}

// druid_t::primary_role ====================================================

role_e druid_t::primary_role() const
{
  if ( specialization() == DRUID_BALANCE )
  {
    if ( player_t::primary_role() == ROLE_HEAL )
      return ROLE_HEAL;

    return ROLE_SPELL;
  }

  else if ( specialization() == DRUID_FERAL )
  {
    if ( player_t::primary_role() == ROLE_TANK )
      return ROLE_TANK;

    return ROLE_ATTACK;
  }

  else if ( specialization() == DRUID_GUARDIAN )
  {
    if ( player_t::primary_role() == ROLE_ATTACK )
      return ROLE_ATTACK;

    return ROLE_TANK;
  }

  else if ( specialization() == DRUID_RESTORATION )
  {
    if ( player_t::primary_role() == ROLE_DPS || player_t::primary_role() == ROLE_SPELL )
      return ROLE_SPELL;

    return ROLE_HEAL;
  }

  return player_t::primary_role();
}

// druid_t::convert_hybrid_stat ==============================================

stat_e druid_t::convert_hybrid_stat( stat_e s ) const
{
  // this converts hybrid stats that either morph based on spec or only work
  // for certain specs into the appropriate "basic" stats
  switch ( s )
  {
  case STAT_STR_AGI_INT:
    switch ( specialization() )
    {
      case DRUID_BALANCE:
      case DRUID_RESTORATION:
        return STAT_INTELLECT;
      case DRUID_FERAL:
      case DRUID_GUARDIAN:
        return STAT_AGILITY;
      default:
        return STAT_NONE;
    }
  case STAT_AGI_INT: 
    if ( specialization() == DRUID_BALANCE || specialization() == DRUID_RESTORATION )
      return STAT_INTELLECT;
    else
      return STAT_AGILITY; 
  // This is a guess at how AGI/STR gear will work for Balance/Resto, TODO: confirm  
  case STAT_STR_AGI:
    return STAT_AGILITY;
  // This is a guess at how STR/INT gear will work for Feral/Guardian, TODO: confirm  
  // This should probably never come up since druids can't equip plate, but....
  case STAT_STR_INT:
    return STAT_INTELLECT;
  case STAT_SPIRIT:
    if ( specialization() == DRUID_RESTORATION )
      return s;
    else
      return STAT_NONE;
  case STAT_BONUS_ARMOR:
    if ( specialization() == DRUID_GUARDIAN )
      return s;
    else
      return STAT_NONE;     
  default: return s; 
  }
}

// druid_t::primary_resource ================================================

resource_e druid_t::primary_resource() const
{
  if ( specialization() == DRUID_BALANCE && primary_role() == ROLE_SPELL )
    return RESOURCE_ASTRAL_POWER;

  if ( primary_role() == ROLE_HEAL || primary_role() == ROLE_SPELL )
    return RESOURCE_MANA;

  if ( primary_role() == ROLE_TANK )
    return RESOURCE_RAGE;

  return RESOURCE_ENERGY;
}

// druid_t::init_absorb_priority ============================================

void druid_t::init_absorb_priority()
{
  player_t::init_absorb_priority();

  absorb_priority.push_back( 184878 ); // Stalwart Guardian
  absorb_priority.push_back( talent.brambles -> id() ); // Brambles
}

// druid_t::target_mitigation ===============================================

void druid_t::target_mitigation( school_e school, dmg_e type, action_state_t* s )
{
  s -> result_amount *= 1.0 + buff.barkskin -> value();

  s -> result_amount *= 1.0 + buff.survival_instincts -> value();

  s -> result_amount *= 1.0 + buff.pulverize -> value();

  // TOCHECK: This talent only has one effect for some reason, may change in the future.
  if ( talent.galactic_guardian -> ok() && get_target_data( s -> action -> player ) -> dots.moonfire -> is_ticking() )
    s -> result_amount *= 1.0 - talent.galactic_guardian -> effectN( 1 ).percent();

  if ( spell.thick_hide )
    s -> result_amount *= 1.0 + spell.thick_hide -> effectN( 1 ).percent();

  if ( talent.earthwarden -> ok() && ! s -> action -> special && buff.earthwarden -> up() )
  {
    s -> result_amount *= 1.0 - buff.earthwarden -> current_value;
    buff.earthwarden -> decrement();
  }

  if ( talent.rend_and_tear -> ok() )
    s -> result_amount *= 1.0 - talent.rend_and_tear -> effectN( 2 ).percent()
                              * get_target_data( s -> action -> player ) -> lacerate_stack;

  if ( dbc::get_school_mask( school ) & SCHOOL_MAGIC_MASK )
    s -> result_amount *= 1.0 + buff.mark_of_ursol -> value();

  player_t::target_mitigation( school, type, s );
}

// druid_t::assess_damage ===================================================

void druid_t::assess_damage( school_e school,
                             dmg_e    dtype,
                             action_state_t* s )
{
  if ( sets.has_set_bonus( SET_TANK, T15, B2 ) && s -> result == RESULT_DODGE && buff.ironfur -> check() )
    buff.guardian_tier15_2pc -> trigger();

  if ( dbc::is_school( school, SCHOOL_PHYSICAL ) && s -> result == RESULT_HIT )
    buff.ironfur -> up();

  player_t::assess_damage( school, dtype, s );
}

// druid_t::assess_damage_imminent_preabsorb ================================

// Trigger effects based on being hit or taking damage.

void druid_t::assess_damage_imminent_pre_absorb( school_e, dmg_e, action_state_t* s )
{
  if ( buff.cenarion_ward -> up() && s -> result_amount > 0 )
    active.cenarion_ward_hot -> execute();

  if ( buff.moonkin_form -> up() && s -> result_amount > 0 && ! s -> action -> aoe )
    buff.owlkin_frenzy -> trigger();

  if ( s -> result_amount > 0 && buff.bristling_fur -> up() )
  {
    // TOCHECK: This is a total guess based on ancient wiki pages on how Berserk Stance worked.
    // 1 rage per 1% health taken
    double rage = s -> result_amount / resources.max[ RESOURCE_HEALTH ] * 100;

    resource_gain( RESOURCE_RAGE, rage, gain.bristling_fur );
  }
}

// druid_t::assess_heal =====================================================

void druid_t::assess_heal( school_e school,
                           dmg_e    dmg_type,
                           action_state_t* s )
{
  if ( sets.has_set_bonus( DRUID_GUARDIAN, T18, B2 ) && buff.ironfur -> check() )
  {
    double pct = sets.set( DRUID_GUARDIAN, T18, B2 ) -> effectN( 1 ).percent();

    // Trigger a gain so we can track how much the set bonus helped.
    // The gain is 100% overflow so it doesn't distort charts.
    gain.guardian_tier18_2pc -> add( RESOURCE_HEALTH, 0, s -> result_total * pct );

    s -> result_total *= 1.0 + pct;
  }

  if ( mastery.natures_guardian -> ok() )
    s -> result_total *= 1.0 + cache.mastery_value();

  player_t::assess_heal( school, dmg_type, s );
}

druid_td_t::druid_td_t( player_t& target, druid_t& source )
  : actor_target_data_t( &target, &source ),
    dots( dots_t() ),
    buffs( buffs_t() ),
    lacerate_stack( 0 )
{
  dots.collapsing_stars = target.get_dot( "collapsing_stars", &source );
  dots.gushing_wound    = target.get_dot( "gushing_wound",    &source );
  dots.lacerate         = target.get_dot( "lacerate",         &source );
  dots.lifebloom        = target.get_dot( "lifebloom",        &source );
  dots.moonfire         = target.get_dot( "moonfire",         &source );
  dots.stellar_flare    = target.get_dot( "stellar_flare",    &source );
  dots.rake             = target.get_dot( "rake",             &source );
  dots.regrowth         = target.get_dot( "regrowth",         &source );
  dots.rejuvenation     = target.get_dot( "rejuvenation",     &source );
  dots.rip              = target.get_dot( "rip",              &source );
  dots.sunfire          = target.get_dot( "sunfire",          &source );
  dots.starfall         = target.get_dot( "starfall",         &source );
  dots.thrash_cat       = target.get_dot( "thrash_cat",       &source );
  dots.wild_growth      = target.get_dot( "wild_growth",      &source );

  buffs.lifebloom       = buff_creator_t( *this, "lifebloom", source.find_class_spell( "Lifebloom" ) );
  buffs.bloodletting    = buff_creator_t( *this, "bloodletting", source.find_spell( 165699 ) )
                          .default_value( source.find_spell( 165699 ) -> ok() ? source.find_spell( 165699 ) -> effectN( 1 ).percent() : 0.10 )
                          .duration( source.find_spell( 165699 ) -> ok() ? source.find_spell( 165699 ) -> duration() : timespan_t::from_seconds( 6.0 ) )
                          .chance( 1.0 );
  buffs.starfall        = buff_creator_t( *this, "starfall", source.find_spell( 197637 ) )
                          .default_value( source.find_spell( 197637 ) -> effectN( 1 ).percent() + source.artifact.falling_star.percent() );
}

// Copypasta for reporting
bool has_amount_results( const std::vector<stats_t::stats_results_t>& res )
{
  return (
      res[ RESULT_HIT ].actual_amount.mean() > 0 ||
      res[ RESULT_CRIT ].actual_amount.mean() > 0 ||
      res[ RESULT_MULTISTRIKE ].actual_amount.mean() > 0 ||
      res[ RESULT_MULTISTRIKE_CRIT ].actual_amount.mean() > 0
  );
}

/* Report Extension Class
 * Here you can define class specific report extensions/overrides
 */
class druid_report_t : public player_report_extension_t
{
public:
  druid_report_t( druid_t& player ) :
      p( player )
  { }

  void feral_snapshot_table( report::sc_html_stream& os )
  {
    // Write header
    os << "<table class=\"sc\">\n"
         << "<tr>\n"
           << "<th >Ability</th>\n"
           << "<th colspan=2>Tiger's Fury</th>\n";
    if ( p.talent.bloodtalons -> ok() )
    {
      os << "<th colspan=2>Bloodtalons</th>\n";
    }
    os << "</tr>\n";

    os << "<tr>\n"
         << "<th>Name</th>\n"
         << "<th>Execute %</th>\n"
         << "<th>Benefit %</th>\n";
    if ( p.talent.bloodtalons -> ok() )
    {
      os << "<th>Execute %</th>\n"
         << "<th>Benefit %</th>\n";
    }
    os << "</tr>\n";

    // Compile and Write Contents 
    for ( size_t i = 0, end = p.stats_list.size(); i < end; i++ )
    {
      stats_t* stats = p.stats_list[ i ];
      double tf_exe_up = 0, tf_exe_total = 0;
      double tf_benefit_up = 0, tf_benefit_total = 0;
      double bt_exe_up = 0, bt_exe_total = 0;
      double bt_benefit_up = 0, bt_benefit_total = 0;
      int n = 0;

      for ( size_t j = 0, end2 = stats -> action_list.size(); j < end2; j++ )
      {
        cat_attacks::cat_attack_t* a = dynamic_cast<cat_attacks::cat_attack_t*>( stats -> action_list[ j ] );
        if ( ! a )
          continue;

        if ( ! a -> consume_bloodtalons )
          continue;

        tf_exe_up += a -> tf_counter -> mean_exe_up();
        tf_exe_total += a -> tf_counter -> mean_exe_total();
        tf_benefit_up += a -> tf_counter -> mean_tick_up();
        tf_benefit_total += a -> tf_counter -> mean_tick_total();
        if ( has_amount_results( stats -> direct_results ) )
        {
          tf_benefit_up += a -> tf_counter -> mean_exe_up();
          tf_benefit_total += a -> tf_counter -> mean_exe_total();
        }
        if ( p.talent.bloodtalons -> ok() )
        {
          bt_exe_up += a -> bt_counter -> mean_exe_up();
          bt_exe_total += a -> bt_counter -> mean_exe_total();
          bt_benefit_up += a -> bt_counter -> mean_tick_up();
          bt_benefit_total += a -> bt_counter -> mean_tick_total();
          if ( has_amount_results( stats -> direct_results ) )
          {
            bt_benefit_up += a -> bt_counter -> mean_exe_up();
            bt_benefit_total += a -> bt_counter -> mean_exe_total();
          }
        }
      }

      if ( tf_exe_total > 0 || bt_exe_total > 0 )
      {
        std::string name_str = report::decorated_action_name( stats -> action_list[ 0 ] );
        std::string row_class_str = "";
        if ( ++n & 1 )
          row_class_str = " class=\"odd\"";

        // Table Row : Name, TF up, TF total, TF up/total, TF up/sum(TF up)
        os.format("<tr%s><td class=\"left\">%s</td><td class=\"right\">%.2f %%</td><td class=\"right\">%.2f %%</td>\n",
            row_class_str.c_str(),
            name_str.c_str(),
            util::round( tf_exe_up / tf_exe_total * 100, 2 ),
            util::round( tf_benefit_up / tf_benefit_total * 100, 2 ) );

        if ( p.talent.bloodtalons -> ok() )
        {
          // Table Row : Name, TF up, TF total, TF up/total, TF up/sum(TF up)
          os.format("<td class=\"right\">%.2f %%</td><td class=\"right\">%.2f %%</td>\n",
              util::round( bt_exe_up / bt_exe_total * 100, 2 ),
              util::round( bt_benefit_up / bt_benefit_total * 100, 2 ) );
        }

        os << "</tr>";
      }
      
    }

    os << "</tr>";

    // Write footer
    os << "</table>\n";
  }

private:
  druid_t& p;
};

// Druid Special Effects ====================================================

static void do_trinket_init( druid_t*                player,
                             specialization_e         spec,
                             const special_effect_t*& ptr,
                             const special_effect_t&  effect )
{
  // Ensure we have the spell data. This will prevent the trinket effect from working on live
  // Simulationcraft. Also ensure correct specialization.
  if ( ! player -> find_spell( effect.spell_id ) -> ok() ||
       player -> specialization() != spec )
  {
    return;
  }

  // Set pointer, module considers non-null pointer to mean the effect is "enabled"
  ptr = &( effect );
}

// Balance T18 (WoD 6.2) trinket effect
static void starshards( special_effect_t& effect )
{
  druid_t* s = debug_cast<druid_t*>( effect.player );
  do_trinket_init( s, DRUID_BALANCE, s -> starshards, effect );
}

// Feral T18 (WoD 6.2) trinket effect
static void wildcat_celerity( special_effect_t& effect )
{
  druid_t* s = debug_cast<druid_t*>( effect.player );
  do_trinket_init( s, DRUID_FERAL, s -> wildcat_celerity, effect );
}

// Guardian T18 (WoD 6.2) trinket effect
static double stalwart_guardian_handler( const action_state_t* s )
{
  druid_t* p = static_cast<druid_t*>( s -> target );
  assert( p -> active.stalwart_guardian );
  assert( s );

  // Pass incoming damage value so the absorb can be calculated.
  // TOCHECK: Does this use result_amount or result_mitigated?
  p -> active.stalwart_guardian -> incoming_damage = s -> result_mitigated;
  // Pass the triggering enemy so that the damage reflect has a target;
  p -> active.stalwart_guardian -> triggering_enemy = s -> action -> player;
  p -> active.stalwart_guardian -> execute();

  return p -> active.stalwart_guardian -> absorb_size;
}

static void stalwart_guardian( special_effect_t& effect )
{
  druid_t* s = debug_cast<druid_t*>( effect.player );
  do_trinket_init( s, DRUID_GUARDIAN, s -> stalwart_guardian, effect );

  if ( !s -> stalwart_guardian )
  {
    return;
  }

  effect.player -> instant_absorb_list[ 184878 ] =
    new instant_absorb_t( s, s -> find_spell( 184878 ), "stalwart_guardian", &stalwart_guardian_handler );
}

// Restoration T18 (WoD 6.2) trinket effect
static void flourish( special_effect_t& effect )
{
  druid_t* s = debug_cast<druid_t*>( effect.player );
  do_trinket_init( s, DRUID_RESTORATION, s -> flourish, effect );
}

// Scythe of Elune
static void scythe_of_elune( special_effect_t& effect )
{
  druid_t* s = debug_cast<druid_t*>( effect.player );
  do_trinket_init( s, DRUID_BALANCE, s -> scythe_of_elune, effect );

  s -> buff.the_reaping = buff_creator_t( s, "the_reaping_driver", s -> scythe_of_elune -> driver() )
                          .quiet( true )
                          .tick_callback( [ s ]( buff_t*, int, const timespan_t& )
                                          { s -> buff.owlkin_frenzy ->  trigger( 1, buff_t::DEFAULT_VALUE(), s -> buff.the_reaping -> current_value ); } )
                          .default_value( s -> buff.owlkin_frenzy -> default_chance )
                          .tick_zero( true );
}

// DRUID MODULE INTERFACE ===================================================

struct druid_module_t : public module_t
{
  druid_module_t() : module_t( DRUID ) {}

  virtual player_t* create_player( sim_t* sim, const std::string& name, race_e r = RACE_NONE ) const override
  {
    auto  p = new druid_t( sim, name, r );
    p -> report_extension = std::unique_ptr<player_report_extension_t>( new druid_report_t( *p ) );
    return p;
  }
  virtual bool valid() const override { return true; }
  virtual void init( player_t* p ) const override
  {
    p -> buffs.stampeding_roar = buff_creator_t( p, "stampeding_roar", p -> find_spell( 77764 ) )
                                 .max_stack( 1 )
                                 .duration( timespan_t::from_seconds( 8.0 ) );
  }

  virtual void static_init() const override
  {
    unique_gear::register_special_effect( 184876, starshards );
    unique_gear::register_special_effect( 184877, wildcat_celerity );
    unique_gear::register_special_effect( 184878, stalwart_guardian );
    unique_gear::register_special_effect( 184879, flourish );
    unique_gear::register_special_effect( 202509, scythe_of_elune );
  } 

  virtual void register_hotfixes() const override
  {
  }

  virtual void combat_begin( sim_t* ) const override {}
  virtual void combat_end( sim_t* ) const override {}
};

} // UNNAMED NAMESPACE

const module_t* module_t::druid()
{
  static druid_module_t m;
  return &m;
}
