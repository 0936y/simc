// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
//
// ==========================================================================
// WOD TODO
// ==========================================================================
// - Unleash Flame expiration delay
// - Precision power inherit coefficients for pets
// - Echo of the Elements buff spell data (159101, 159105, 159103)
//
// ==========================================================================
// BUGS
// ==========================================================================

#include "simulationcraft.hpp"

// ==========================================================================
// Shaman
// ==========================================================================

namespace { // UNNAMED NAMESPACE

typedef std::pair<std::string, simple_sample_data_with_min_max_t> data_t;
typedef std::pair<std::string, simple_sample_data_t> simple_data_t;

struct shaman_t;

enum totem_e { TOTEM_NONE = 0, TOTEM_AIR, TOTEM_EARTH, TOTEM_FIRE, TOTEM_WATER, TOTEM_MAX };
enum imbue_e { IMBUE_NONE = 0, FLAMETONGUE_IMBUE, WINDFURY_IMBUE, FROSTBRAND_IMBUE, EARTHLIVING_IMBUE };

struct shaman_attack_t;
struct shaman_spell_t;
struct shaman_heal_t;
struct shaman_totem_pet_t;
struct totem_pulse_event_t;
struct totem_pulse_action_t;

struct shaman_td_t : public actor_target_data_t
{
  struct dots
  {
    dot_t* flame_shock;
  } dot;

  struct debuffs
  {
    buff_t* t16_2pc_caster;
    buff_t* earthen_spike;
  } debuff;

  struct heals
  {
    dot_t* riptide;
    dot_t* earthliving;
  } heal;

  shaman_td_t( player_t* target, shaman_t* p );
};

struct counter_t
{
  const sim_t* sim;

  double value, interval;
  timespan_t last;

  counter_t( shaman_t* p );

  void add( double val )
  {
    // Skip iteration 0 for non-debug, non-log sims
    if ( sim -> current_iteration == 0 && sim -> iterations > sim -> threads && ! sim -> debug && ! sim -> log )
      return;

    value += val;
    if ( last > timespan_t::min() )
      interval += ( sim -> current_time() - last ).total_seconds();
    last = sim -> current_time();
  }

  void reset()
  { last = timespan_t::min(); }

  double divisor() const
  {
    if ( ! sim -> debug && ! sim -> log && sim -> iterations > sim -> threads )
      return sim -> iterations - sim -> threads;
    else
      return std::min( sim -> iterations, sim -> threads );
  }

  double mean() const
  { return value / divisor(); }

  double interval_mean() const
  { return interval / divisor(); }

  void merge( const counter_t& other )
  {
    value += other.value;
    interval += other.interval;
  }
};

struct shaman_t : public player_t
{
public:
  // Misc
  timespan_t ls_reset;
  bool       lava_surge_during_lvb;
  std::vector<counter_t*> counters;

  // Data collection for cooldown waste
  auto_dispose< std::vector<data_t*> > cd_waste_exec, cd_waste_cumulative;
  auto_dispose< std::vector<simple_data_t*> > cd_waste_iter;

  // Options
  timespan_t uf_expiration_delay;
  timespan_t uf_expiration_delay_stddev;
  double     mental_quickness;

  // Active
  action_t* active_lightning_charge;
  std::array<action_t*, 2> unleash_doom;
  action_t* doom_vortex;

  // Cached actions
  action_t* action_ancestral_awakening;
  action_t* action_improved_lava_lash;
  action_t* action_lightning_strike;
  spell_t*  electrocute;

  // Pets
  std::vector<pet_t*> pet_feral_spirit;
  pet_t* pet_fire_elemental;
  pet_t* guardian_fire_elemental;
  pet_t* pet_storm_elemental;
  pet_t* guardian_storm_elemental;
  pet_t* pet_earth_elemental;
  pet_t* guardian_earth_elemental;
  pet_t* guardian_lightning_elemental;

  // Totems
  shaman_totem_pet_t* totems[ TOTEM_MAX ];

  // Tier 18 (WoD 6.2) class specific trinket effects
  const special_effect_t* elemental_bellows;
  const special_effect_t* furious_winds;

  // Constants
  struct
  {
    double matching_gear_multiplier;
    double speed_attack_ancestral_swiftness;
    double haste_ancestral_swiftness;
  } constant;

  // Buffs
  struct
  {
    buff_t* ascendance;
    buff_t* echo_of_the_elements;
    buff_t* enhanced_chain_lightning;
    buff_t* lava_surge;
    buff_t* liquid_magma;
    buff_t* lightning_shield;
    buff_t* shamanistic_rage;
    buff_t* spirit_walk;
    buff_t* spiritwalkers_grace;
    buff_t* tier16_2pc_caster;
    buff_t* tidal_waves;
    buff_t* focus_of_the_elements;
    buff_t* feral_spirit;
    haste_buff_t* feral_spirit2;
    buff_t* t18_4pc_elemental;
    buff_t* gathering_vortex;

    haste_buff_t* tier13_4pc_healer;

    stat_buff_t* elemental_blast_agility;
    stat_buff_t* elemental_blast_crit;
    stat_buff_t* elemental_blast_haste;
    stat_buff_t* elemental_blast_mastery;
    stat_buff_t* elemental_blast_multistrike;
    stat_buff_t* tier13_2pc_caster;
    stat_buff_t* tier13_4pc_caster;

    buff_t* flametongue;
    buff_t* stormfury;
    buff_t* crash_lightning;
    haste_buff_t* windsong;
    haste_buff_t* fists_of_stone;
    buff_t* rockbiter;
    buff_t* doom_winds;
    buff_t* unleash_doom;
    haste_buff_t* wind_strikes;
    buff_t* gathering_storms;
    buff_t* fire_empowered;
    buff_t* lava_dredger;
  } buff;

  // Cooldowns
  struct
  {
    cooldown_t* ascendance;
    cooldown_t* earth_elemental_totem;
    cooldown_t* storm_elemental_totem;
    cooldown_t* feral_spirits;
    cooldown_t* lava_burst;
    cooldown_t* lava_lash;
    cooldown_t* strike;
    cooldown_t* t16_2pc_melee;
    cooldown_t* t16_4pc_caster;
    cooldown_t* t16_4pc_melee;
    cooldown_t* windfury_weapon;
  } cooldown;

  // Gains
  struct
  {
    gain_t* resurgence;
    gain_t* feral_spirit;
    gain_t* fulmination;
  } gain;

  // Tracked Procs
  struct
  {
    proc_t* lava_surge;
    proc_t* ls_fast;
    proc_t* swings_clipped_mh;
    proc_t* swings_clipped_oh;
    proc_t* swings_reset_mh;
    proc_t* swings_reset_oh;
    proc_t* t15_2pc_melee;
    proc_t* t16_2pc_melee;
    proc_t* t16_4pc_caster;
    proc_t* t16_4pc_melee;
    proc_t* wasted_t15_2pc_melee;
    proc_t* wasted_lava_surge;
    proc_t* wasted_ls;
    proc_t* wasted_ls_shock_cd;
    proc_t* wasted_mw;
    proc_t* windfury;

    proc_t* uf_flame_shock;
    proc_t* uf_fire_nova;
    proc_t* uf_lava_burst;
    proc_t* uf_elemental_blast;
    proc_t* uf_wasted;

    proc_t* surge_during_lvb;
  } proc;

  struct
  {
    real_ppm_t unleash_doom;
  } real_ppm;

  // Class Specializations
  struct
  {
    // Generic
    const spell_data_t* mail_specialization;

    // Elemental
    const spell_data_t* elemental_fury;
    const spell_data_t* fulmination;
    const spell_data_t* lava_surge;
    const spell_data_t* shamanism;
    const spell_data_t* spiritual_insight;

    // Enhancement
    const spell_data_t* critical_strikes;
    const spell_data_t* dual_wield;
    const spell_data_t* flurry;
    const spell_data_t* maelstrom_weapon;
    const spell_data_t* stormfury;
    const spell_data_t* stormlash;
    const spell_data_t* windfury;

    // Restoration
    const spell_data_t* ancestral_awakening;
    const spell_data_t* ancestral_focus;
    const spell_data_t* earth_shield;
    const spell_data_t* meditation;
    const spell_data_t* purification;
    const spell_data_t* resurgence;
    const spell_data_t* riptide;
    const spell_data_t* tidal_waves;
  } spec;

  // Masteries
  struct
  {
    const spell_data_t* elemental_overload;
    const spell_data_t* enhanced_elements;
    const spell_data_t* deep_healing;
  } mastery;

  // Talents
  struct
  {
    // Generic / Shared
    const spell_data_t* ancestral_swiftness;
    const spell_data_t* ascendance;
    const spell_data_t* gust_of_wind;

    // Elemental
    const spell_data_t* path_of_flame;
    const spell_data_t* path_of_elements;
    const spell_data_t* maelstrom_totem;

    const spell_data_t* fleet_of_foot;

    const spell_data_t* elemental_blast;
    const spell_data_t* echo_of_the_elements;

    const spell_data_t* elemental_fusion;
    const spell_data_t* sons_of_flame;
    const spell_data_t* magnitude;

    const spell_data_t* lightning_rod;
    const spell_data_t* storm_elemental;
    const spell_data_t* liquid_magma_totem;

    const spell_data_t* primal_elementalist;
    const spell_data_t* totemic_fury;

    // Enhancement
    const spell_data_t* windsong;
    const spell_data_t* spiritual_resonance;
    const spell_data_t* fists_of_stone;

    const spell_data_t* feral_lunge;

    const spell_data_t* lightning_shield;
    const spell_data_t* landslide;

    const spell_data_t* tempest;
    const spell_data_t* spiritual_affinity;
    const spell_data_t* sundering;

    const spell_data_t* fury_of_air;
    const spell_data_t* crashing_storm;
    const spell_data_t* stonefist_strike;

    const spell_data_t* feral_kin;
    const spell_data_t* earthen_spike;
  } talent;

  // Artifact
  struct artifact_spell_data_t
  {
    artifact_power_t doom_winds;
    artifact_power_t unleash_doom;
    artifact_power_t hurricane;
    artifact_power_t stormflurry;
    artifact_power_t hammer_of_storms;
    artifact_power_t forged_in_lava;
    artifact_power_t surge_of_elements;
    artifact_power_t wind_strikes;
    artifact_power_t gathering_storms;
    artifact_power_t gathering_of_the_maelstrom;
    artifact_power_t doom_vortex;
  } artifact;

  // Misc Spells
  struct
  {
    const spell_data_t* resurgence;
    const spell_data_t* echo_of_the_elements;
    const spell_data_t* flame_shock;
    const spell_data_t* lightning_strike;
    const spell_data_t* eruption;
    const spell_data_t* maelstrom_melee_gain;
  } spell;

  // Cached pointer for ascendance / normal white melee
  shaman_attack_t* melee_mh;
  shaman_attack_t* melee_oh;
  shaman_attack_t* ascendance_mh;
  shaman_attack_t* ascendance_oh;

  // Weapon Enchants
  shaman_attack_t* windfury;
  shaman_spell_t*  flametongue;

  shaman_t( sim_t* sim, const std::string& name, race_e r = RACE_TAUREN ) :
    player_t( sim, SHAMAN, name, r ),
    ls_reset( timespan_t::zero() ), lava_surge_during_lvb( false ),
    uf_expiration_delay( timespan_t::from_seconds( 0.3 ) ), uf_expiration_delay_stddev( timespan_t::from_seconds( 0.05 ) ),
    mental_quickness( 1.3 ), // TODO: Legion placeholder
    active_lightning_charge( nullptr ),
    action_ancestral_awakening( nullptr ),
    action_improved_lava_lash( nullptr ),
    action_lightning_strike( nullptr ),
    pet_fire_elemental( nullptr ),
    guardian_fire_elemental( nullptr ),
    pet_earth_elemental( nullptr ),
    guardian_earth_elemental( nullptr ),
    elemental_bellows( nullptr ),
    furious_winds( nullptr ),
    constant(),
    buff(),
    cooldown(),
    gain(),
    proc(),
    spec(),
    mastery(),
    talent(),
    spell()
  {
    range::fill( pet_feral_spirit, nullptr );
    range::fill( totems, nullptr );

    // Cooldowns
    cooldown.ascendance           = get_cooldown( "ascendance"            );
    cooldown.earth_elemental_totem= get_cooldown( "earth_elemental_totem" );
    cooldown.feral_spirits        = get_cooldown( "feral_spirit"          );
    cooldown.lava_burst           = get_cooldown( "lava_burst"            );
    cooldown.lava_lash            = get_cooldown( "lava_lash"             );
    cooldown.strike               = get_cooldown( "strike"                );
    cooldown.storm_elemental_totem= get_cooldown( "storm_elemental_totem" );
    cooldown.t16_2pc_melee        = get_cooldown( "t16_2pc_melee"         );
    cooldown.t16_4pc_caster       = get_cooldown( "t16_4pc_caster"        );
    cooldown.t16_4pc_melee        = get_cooldown( "t16_4pc_melee"         );
    cooldown.windfury_weapon      = get_cooldown( "windfury_weapon"       );

    melee_mh = nullptr;
    melee_oh = nullptr;
    ascendance_mh = nullptr;
    ascendance_oh = nullptr;

    // Weapon Enchants
    windfury    = nullptr;
    flametongue = nullptr;

    regen_type = REGEN_DISABLED;
  }

  virtual           ~shaman_t();

  // triggers
  void trigger_windfury_weapon( const action_state_t* );
  void trigger_flametongue_weapon( const action_state_t* );
  void trigger_improved_lava_lash( const action_state_t* );
  void trigger_tier15_2pc_caster( const action_state_t* );
  void trigger_tier16_2pc_melee( const action_state_t* );
  void trigger_tier16_4pc_melee( const action_state_t* );
  void trigger_tier16_4pc_caster( const action_state_t* );
  void trigger_tier17_2pc_elemental( int );
  void trigger_tier17_4pc_elemental( int );
  void trigger_tier18_4pc_elemental( int );
  void trigger_stormfury( const action_state_t* state );
  void trigger_unleash_doom( const action_state_t* state );

  // Character Definition
  virtual void      init_spells() override;
  virtual void      init_resources( bool force = false ) override;
  virtual void      init_base_stats() override;
  virtual void      init_scaling() override;
  virtual void      create_buffs() override;
  virtual void      init_gains() override;
  virtual void      init_procs() override;
  virtual void      init_action_list() override;
  virtual void      init_rng() override;
  virtual void      moving() override;
  virtual void      invalidate_cache( cache_e c ) override;
  virtual double    temporary_movement_modifier() const override;
  virtual double    composite_melee_crit() const override;
  virtual double    composite_melee_haste() const override;
  virtual double    composite_melee_speed() const override;
  virtual double    composite_attack_power_multiplier() const override;
  virtual double    composite_spell_crit() const override;
  virtual double    composite_spell_haste() const override;
  virtual double    composite_spell_power( school_e school ) const override;
  virtual double    composite_spell_power_multiplier() const override;
  virtual double    composite_player_multiplier( school_e school ) const override;
  virtual double    matching_gear_multiplier( attribute_e attr ) const override;
  virtual void      create_options() override;
  virtual action_t* create_action( const std::string& name, const std::string& options ) override;
  virtual action_t* create_proc_action( const std::string& /* name */, const special_effect_t& ) override;
  virtual pet_t*    create_pet   ( const std::string& name, const std::string& type = std::string() ) override;
  virtual void      create_pets() override;
  virtual expr_t* create_expression( action_t*, const std::string& name ) override;
  virtual resource_e primary_resource() const override { return RESOURCE_MANA; }
  virtual role_e primary_role() const override;
  virtual stat_e convert_hybrid_stat( stat_e s ) const override;
  virtual void      arise() override;
  virtual void      reset() override;
  virtual void      merge( player_t& other ) override;

  virtual void     datacollection_begin() override;
  virtual void     datacollection_end() override;
  virtual bool     has_t18_class_trinket() const override;

  target_specific_t<shaman_td_t> target_data;

  virtual shaman_td_t* get_target_data( player_t* target ) const override
  {
    shaman_td_t*& td = target_data[ target ];
    if ( ! td )
    {
      td = new shaman_td_t( target, const_cast<shaman_t*>(this) );
    }
    return td;
  }

  template <typename T_CONTAINER, typename T_DATA>
  T_CONTAINER* get_data_entry( const std::string& name, std::vector<T_DATA*>& entries )
  {
    for ( size_t i = 0; i < entries.size();i ++ )
    {
      if ( entries[ i ] -> first == name )
      {
        return &( entries[ i ] -> second );
      }
    }

    entries.push_back( new T_DATA( name, T_CONTAINER() ) );
    return &( entries.back() -> second );
  }
};

shaman_t::~shaman_t()
{
  range::dispose( counters );
}

counter_t::counter_t( shaman_t* p ) :
  sim( p -> sim ), value( 0 ), interval( 0 ), last( timespan_t::min() )
{
  p -> counters.push_back( this );
}

shaman_td_t::shaman_td_t( player_t* target, shaman_t* p ) :
  actor_target_data_t( target, p )
{
  dot.flame_shock       = target -> get_dot( "flame_shock", p );

  debuff.t16_2pc_caster = buff_creator_t( *this, "tier16_2pc_caster", p -> sets.set( SET_CASTER, T16, B2 ) -> effectN( 1 ).trigger() )
                          .chance( static_cast< double >( p -> sets.has_set_bonus( SET_CASTER, T16, B2 ) ) );
  debuff.earthen_spike  = buff_creator_t( *this, "earthen_spike", p -> talent.earthen_spike )
                          // -10% resistance in spell data, treat it as a multiplier instead
                          .default_value( -p -> talent.earthen_spike -> effectN( 2 ).percent() );
}

// ==========================================================================
// Shaman Custom Buff Declaration
// ==========================================================================

struct ascendance_buff_t : public buff_t
{
  action_t* lava_burst;

  ascendance_buff_t( shaman_t* p ) :
    buff_t( buff_creator_t( p, "ascendance", p -> talent.ascendance )
            .cd( timespan_t::zero() ) ), // Cooldown is handled by the action
    lava_burst( nullptr )
  { }

  void ascendance( attack_t* mh, attack_t* oh, timespan_t lvb_cooldown );
  bool trigger( int stacks, double value, double chance, timespan_t duration ) override;
  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override;
};

// ==========================================================================
// Shaman Action Base Template
// ==========================================================================

template <class Base>
struct shaman_action_t : public Base
{
private:
  typedef Base ab; // action base, eg. spell_t
public:
  typedef shaman_action_t base_t;

  // Misc stuff
  bool        totem;

  // Echo of Elements functionality
  bool        uses_eoe;

  // Flurry
  bool        hasted_cd;
  bool        hasted_gcd;

  // Elemental Fusion tracking
  proc_t*     ef_proc;

  // Cooldown tracking
  bool        track_cd_waste;
  simple_sample_data_with_min_max_t* cd_wasted_exec, *cd_wasted_cumulative;
  simple_sample_data_t* cd_wasted_iter;

  gain_t*     gain;

  shaman_action_t( const std::string& n, shaman_t* player,
                   const spell_data_t* s = spell_data_t::nil() ) :
    ab( n, player, s ),
    totem( false ),
    uses_eoe( false ),
    hasted_cd( ab::data().affected_by( player -> spec.flurry -> effectN( 1 ) ) ),
    hasted_gcd( ab::data().affected_by( player -> spec.flurry -> effectN( 2 ) ) ),
    ef_proc( nullptr ),
    track_cd_waste( s -> cooldown() > timespan_t::zero() || s -> charge_cooldown() > timespan_t::zero() ),
    cd_wasted_exec( nullptr ), cd_wasted_cumulative( nullptr ), cd_wasted_iter( nullptr ),
    gain( player -> get_gain( s -> id() > 0 ? s -> name_cstr() : n ) )
  {
    ab::may_crit = true;
  }

  void init()
  {
    ab::init();

    if ( track_cd_waste )
    {
      cd_wasted_exec = p() -> template get_data_entry<simple_sample_data_with_min_max_t, data_t>( ab::name_str, p() -> cd_waste_exec );
      cd_wasted_cumulative = p() -> template get_data_entry<simple_sample_data_with_min_max_t, data_t>( ab::name_str, p() -> cd_waste_cumulative );
      cd_wasted_iter = p() -> template get_data_entry<simple_sample_data_t, simple_data_t>( ab::name_str, p() -> cd_waste_iter );
    }

    if ( ab::data().charges() > 0 )
    {
      ab::cooldown -> duration = ab::data().charge_cooldown();
      ab::cooldown -> charges = ab::data().charges() + p() -> talent.echo_of_the_elements -> effectN( 1 ).base_value();
    }
  }

  shaman_t* p()
  { return debug_cast< shaman_t* >( ab::player ); }
  const shaman_t* p() const
  { return debug_cast< shaman_t* >( ab::player ); }

  shaman_td_t* td( player_t* t ) const
  { return p() -> get_target_data( t ); }

  double cost() const
  {
    if ( ab::player -> resources.infinite_resource[ RESOURCE_MANA ] == 1 )
      return 0;

    double c = ab::cost();
    if ( c <= 0 )
      return 0;
    return c;
  }

  double cooldown_reduction() const
  {
    double cdr = ab::cooldown_reduction();

    if ( hasted_cd )
    {
      cdr *= ab::player -> cache.attack_haste();
    }

    return cdr;
  }

  void update_ready( timespan_t cd )
  {
    if ( cd_wasted_exec &&
         ( cd > timespan_t::zero() || ( cd <= timespan_t::zero() && ab::cooldown -> duration > timespan_t::zero() ) ) &&
         ab::cooldown -> current_charge == ab::cooldown -> charges &&
         ab::cooldown -> last_charged > timespan_t::zero() &&
         ab::cooldown -> last_charged < ab::sim -> current_time() )
    {
      double time_ = ( ab::sim -> current_time() - ab::cooldown -> last_charged ).total_seconds();
      if ( p() -> sim -> debug )
      {
        p() -> sim -> out_debug.printf( "%s %s cooldown waste tracking waste=%.3f exec_time=%.3f",
            p() -> name(), ab::name(), time_, ab::time_to_execute.total_seconds() );
      }
      time_ -= ab::time_to_execute.total_seconds();

      if ( time_ > 0 )
      {
        cd_wasted_exec -> add( time_ );
        cd_wasted_iter -> add( time_ );
      }
    }

    ab::update_ready( cd );
  }

  virtual expr_t* create_expression( const std::string& name )
  {
    if ( ! util::str_compare_ci( name, "cooldown.higher_priority.min_remains" ) )
      return ab::create_expression( name );

    struct hprio_cd_min_remains_expr_t : public expr_t
    {
      action_t* action_;
      std::vector<cooldown_t*> cd_;

      // TODO: Line_cd support
      hprio_cd_min_remains_expr_t( action_t* a ) :
        expr_t( "min_remains" ), action_( a )
      {
        action_priority_list_t* list = a -> player -> get_action_priority_list( a -> action_list -> name_str );
        for (auto list_action : list -> foreground_action_list)
        {
          // Jump out when we reach this action
          if ( list_action == action_ )
            break;

          // Skip if this action's cooldown is the same as the list action's cooldown
          if ( list_action -> cooldown == action_ -> cooldown )
            continue;

          // Skip actions with no cooldown
          if ( list_action -> cooldown && list_action -> cooldown -> duration == timespan_t::zero() )
            continue;

          // Skip cooldowns that are already accounted for
          if ( std::find( cd_.begin(), cd_.end(), list_action -> cooldown ) != cd_.end() )
            continue;

          //std::cout << "Appending " << list_action -> name() << " to check list" << std::endl;
          cd_.push_back( list_action -> cooldown );
        }
      }

      double evaluate() override
      {
        if ( cd_.size() == 0 )
          return 0;

        timespan_t min_cd = cd_[ 0 ] -> remains();
        for ( size_t i = 1, end = cd_.size(); i < end; i++ )
        {
          timespan_t remains = cd_[ i ] -> remains();
          //std::cout << "cooldown.higher_priority.min_remains " << cd_[ i ] -> name_str << " remains=" << remains.total_seconds() << std::endl;
          if ( remains < min_cd )
            min_cd = remains;
        }

        //std::cout << "cooldown.higher_priority.min_remains=" << min_cd.total_seconds() << std::endl;
        return min_cd.total_seconds();
      }
    };

    return new hprio_cd_min_remains_expr_t( this );
  }

  void trigger_maelstrom_weapon( const action_state_t* source_state, double amount = 0 )
  {
    if ( ! p() -> spec.maelstrom_weapon -> ok() )
    {
      return;
    }

    if ( ! this -> weapon )
    {
      return;
    }

    if ( source_state -> result_raw <= 0 )
    {
      return;
    }

    if ( amount == 0 )
    {
      amount = p() -> spell.maelstrom_melee_gain -> effectN( 1 ).resource( RESOURCE_MAELSTROM );
    }

    p() -> resource_gain( RESOURCE_MAELSTROM, amount, gain );
  }

};

// ==========================================================================
// Shaman Attack
// ==========================================================================

struct shaman_attack_t : public shaman_action_t<melee_attack_t>
{
private:
  typedef shaman_action_t<melee_attack_t> ab;
public:
  bool may_proc_windfury;
  bool may_proc_flametongue;

  // Maelstrom Weapon functionality
  bool        may_proc_maelstrom;
  counter_t*  maelstrom_procs;
  counter_t*  maelstrom_procs_wasted;

  shaman_attack_t( const std::string& token, shaman_t* p, const spell_data_t* s ) :
    base_t( token, p, s ),
    may_proc_windfury( p -> spec.windfury -> ok() ),
    may_proc_flametongue( true ),
    may_proc_maelstrom( p -> spec.maelstrom_weapon -> ok() ),
    maelstrom_procs( nullptr ), maelstrom_procs_wasted( nullptr )
  {
    special = may_crit = true;
    may_glance = false;
  }

  void init() override
  {
    ab::init();

    if ( may_proc_maelstrom )
    {
      maelstrom_procs = new counter_t( p() );
      maelstrom_procs_wasted = new counter_t( p() );
    }
  }

  timespan_t gcd() const override
  {
    timespan_t g = ab::gcd();

    if ( g == timespan_t::zero() )
      return timespan_t::zero();

    if ( hasted_gcd )
    {
      g *= player -> cache.attack_haste();
      if ( g < min_gcd )
        g = min_gcd;
    }

    return g;
  }

  void impact( action_state_t* state ) override
  {
    base_t::impact( state );

    // Bail out early if the result is a miss/dodge/parry/ms
    if ( ! result_is_hit( state -> result ) )
      return;

    trigger_maelstrom_weapon( execute_state );
    p() -> trigger_windfury_weapon( state );
    p() -> trigger_stormfury( state );
    p() -> trigger_flametongue_weapon( state );
    p() -> trigger_unleash_doom( state );
    //p() -> trigger_tier16_2pc_melee( state ); TODO: Legion will change this
  }
};

// ==========================================================================
// Shaman Base Spell
// ==========================================================================

template <class Base>
struct shaman_spell_base_t : public shaman_action_t<Base>
{
private:
  typedef shaman_action_t<Base> ab;
public:
  typedef shaman_spell_base_t<Base> base_t;

  std::vector<counter_t*> maelstrom_weapon_cast, maelstrom_weapon_executed;

  shaman_spell_base_t( const std::string& n, shaman_t* player,
                       const spell_data_t* s = spell_data_t::nil() ) :
    ab( n, player, s )
  { }

  void execute();
};

// ==========================================================================
// Shaman Offensive Spell
// ==========================================================================

struct shaman_spell_t : public shaman_spell_base_t<spell_t>
{
  shaman_spell_t( const std::string& token, shaman_t* p,
                  const spell_data_t* s = spell_data_t::nil(), const std::string& options = std::string() ) :
    base_t( token, p, s )
  {
    parse_options( options );

    if ( data().affected_by( p -> spec.elemental_fury -> effectN( 1 ) ) )
    {
      crit_bonus_multiplier *= 1.0 + p -> spec.elemental_fury -> effectN( 1 ).percent();
    }

    if ( data().affected_by( p -> spec.shamanism -> effectN( 1 ) ) )
    {
      base_multiplier *= 1.0 + p -> spec.shamanism -> effectN( 1 ).percent();
    }

    if ( data().affected_by( p -> spec.shamanism -> effectN( 2 ) ) )
    {
      base_multiplier *= 1.0 + p -> spec.shamanism -> effectN( 2 ).percent();
    }

    if ( data().affected_by( p -> spec.shamanism -> effectN( 3 ) ) )
    {
      base_execute_time += p -> spec.shamanism -> effectN( 3 ).time_value();
    }

    if ( data().affected_by( p -> spec.shamanism -> effectN( 4 ) ) )
    {
      cooldown -> duration += p -> spec.shamanism -> effectN( 4 ).time_value();
    }
  }

  void impact( action_state_t* state ) override
  {
    base_t::impact( state );

    if ( ! result_is_hit( state -> result ) )
    {
      return;
    }

    p() -> trigger_unleash_doom( state );
  }

  virtual bool usable_moving() const override
  {
    if ( p() -> buff.spiritwalkers_grace -> check() || execute_time() == timespan_t::zero() )
      return true;

    return base_t::usable_moving();
  }

  virtual double composite_target_multiplier( player_t* target ) const override
  {
    double m = base_t::composite_target_multiplier( target );

    if ( td( target ) -> debuff.t16_2pc_caster -> check() &&
         ( dbc::is_school( school, SCHOOL_FIRE ) || dbc::is_school( school, SCHOOL_NATURE ) ) )
    {
      m *= 1.0 + td( target ) -> debuff.t16_2pc_caster -> data().effectN( 1 ).percent();
    }

    if ( td( target ) -> debuff.earthen_spike -> up() &&
         ( dbc::is_school( school, SCHOOL_PHYSICAL ) || dbc::is_school( school, SCHOOL_NATURE ) ) )
    {
      m *= 1.0 + td( target ) -> debuff.earthen_spike -> check_value();
    }

    return m;
  }
};

// ==========================================================================
// Shaman Heal
// ==========================================================================

struct shaman_heal_t : public shaman_spell_base_t<heal_t>
{
  double elw_proc_high,
         elw_proc_low,
         resurgence_gain;

  bool proc_tidal_waves,
       consume_tidal_waves;

  shaman_heal_t( const std::string& token, shaman_t* p,
                 const spell_data_t* s = spell_data_t::nil(),
                 const std::string& options = std::string() ) :
    base_t( token, p, s ),
    elw_proc_high( .2 ), elw_proc_low( 1.0 ),
    resurgence_gain( 0 ),
    proc_tidal_waves( false ), consume_tidal_waves( false )
  {
    parse_options( options );
  }

  shaman_heal_t( shaman_t* p, const spell_data_t* s = spell_data_t::nil(),
                 const std::string& options = std::string() ) :
    base_t( "", p, s ),
    elw_proc_high( .2 ), elw_proc_low( 1.0 ),
    resurgence_gain( 0 ),
    proc_tidal_waves( false ), consume_tidal_waves( false )
  {
    parse_options( options );
  }

  double composite_spell_power() const override
  {
    double sp = base_t::composite_spell_power();

    if ( p() -> main_hand_weapon.buff_type == EARTHLIVING_IMBUE )
      sp += p() -> main_hand_weapon.buff_value;

    return sp;
  }

  double composite_da_multiplier( const action_state_t* state ) const override
  {
    double m = base_t::composite_da_multiplier( state );
    m *= 1.0 + p() -> spec.purification -> effectN( 1 ).percent();
    return m;
  }

  double composite_ta_multiplier( const action_state_t* state ) const override
  {
    double m = base_t::composite_ta_multiplier( state );
    m *= 1.0 + p() -> spec.purification -> effectN( 1 ).percent();
    return m;
  }

  double composite_target_multiplier( player_t* target ) const override
  {
    double m = base_t::composite_target_multiplier( target );
    if ( target -> buffs.earth_shield -> up() )
      m *= 1.0 + p() -> spec.earth_shield -> effectN( 2 ).percent();
    return m;
  }

  void impact( action_state_t* s ) override;

  void execute() override
  {
    base_t::execute();

    if ( consume_tidal_waves )
      p() -> buff.tidal_waves -> decrement();
  }

  virtual double deep_healing( const action_state_t* s )
  {
    if ( ! p() -> mastery.deep_healing -> ok() )
      return 0.0;

    double hpp = ( 1.0 - s -> target -> health_percentage() / 100.0 );

    return 1.0 + hpp * p() -> cache.mastery_value();
  }
};

// ==========================================================================
// Pet Spirit Wolf
// ==========================================================================
namespace pet
{
struct feral_spirit_pet_t : public pet_t
{
  struct windfury_t : public melee_attack_t
  {
    windfury_t( feral_spirit_pet_t* player ) :
      melee_attack_t( "windfury_attack", player, player -> find_spell( 170512 ) )
    {
      background = true;
      //weapon = &( player -> main_hand_weapon );
      may_crit = true;
    }

    feral_spirit_pet_t* p() { return static_cast<feral_spirit_pet_t*>( player ); }

    void init() override
    {
      melee_attack_t::init();
      if ( ! player -> sim -> report_pets_separately && player != p() -> o() -> pet_feral_spirit[ 0 ] )
        stats = p() -> o() -> pet_feral_spirit[ 0 ] -> get_stats( name(), this );
    }
  };

  struct melee_t : public melee_attack_t
  {
    windfury_t* wf;

    melee_t( feral_spirit_pet_t* player ) :
      melee_attack_t( "melee", player, spell_data_t::nil() ),
      wf( new windfury_t( player ) )
    {
      auto_attack = true;
      weapon = &( player -> main_hand_weapon );
      base_execute_time = weapon -> swing_time;
      background = true;
      repeating = true;
      may_crit = true;
      school      = SCHOOL_PHYSICAL;
    }

    feral_spirit_pet_t* p() { return static_cast<feral_spirit_pet_t*>( player ); }

    void init() override
    {
      melee_attack_t::init();
      if ( ! player -> sim -> report_pets_separately && player != p() -> o() -> pet_feral_spirit[ 0 ] )
        stats = p() -> o() -> pet_feral_spirit[ 0 ] -> get_stats( name(), this );
    }

    virtual void impact( action_state_t* state ) override
    {
      melee_attack_t::impact( state );

      if ( result_is_hit( state -> result ) )
      {
        shaman_t* o = p() -> o();

        if ( o -> buff.feral_spirit -> up() && rng().roll( p() -> wf_driver -> proc_chance() ) )
        {
          wf -> target = state -> target;
          wf -> schedule_execute();
          wf -> schedule_execute();
          wf -> schedule_execute();
        }
      }
    }
  };

  melee_t* melee;
  const spell_data_t* command;
  const spell_data_t* wf_driver;

  feral_spirit_pet_t( shaman_t* owner ) :
    pet_t( owner -> sim, owner, "spirit_wolf", true, true ), melee( nullptr )
  {
    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.min_dmg    = dbc.spell_scaling( o() -> type, level() ) * 0.5;
    main_hand_weapon.max_dmg    = dbc.spell_scaling( o() -> type, level() ) * 0.5;
    main_hand_weapon.damage     = ( main_hand_weapon.min_dmg + main_hand_weapon.max_dmg ) / 2;
    main_hand_weapon.swing_time = timespan_t::from_seconds( 1.5 );

    owner_coeff.ap_from_ap = 0.66;

    command = owner -> find_spell( 65222 );
    wf_driver = owner -> find_spell( 170523 );
    regen_type = REGEN_DISABLED;
  }

  shaman_t* o() const { return static_cast<shaman_t*>( owner ); }

  void arise() override
  {
    pet_t::arise();
    schedule_ready();
  }

  void init_action_list() override
  {
    pet_t::init_action_list();

    melee = new melee_t( this );
  }

  void schedule_ready( timespan_t delta_time = timespan_t::zero(), bool waiting = false ) override
  {
    if ( ! melee -> execute_event )
      melee -> schedule_execute();

    pet_t::schedule_ready( delta_time, waiting );
  }

  double composite_player_multiplier( school_e school ) const override
  {
    double m = pet_t::composite_player_multiplier( school );

    if ( owner -> race == RACE_ORC )
      m *= 1.0 + command -> effectN( 1 ).percent();

    return m;
  }
};

// ==========================================================================
// Primal Elementals
// ==========================================================================

struct primal_elemental_t : public pet_t
{
  struct travel_t : public action_t
  {
    travel_t( player_t* player ) : action_t( ACTION_OTHER, "travel", player ) {}
    void execute() override { player -> current.distance = 1; }
    timespan_t execute_time() const override { return timespan_t::from_seconds( player -> current.distance / 10.0 ); }
    bool ready() override { return ( player -> current.distance > 1 ); }
    bool usable_moving() const override { return true; }
  };

  struct melee_t : public melee_attack_t
  {
    melee_t( primal_elemental_t* player, school_e school_, double multiplier ) :
      melee_attack_t( "melee", player, spell_data_t::nil() )
    {
      auto_attack = may_crit = background = repeating = true;

      school                 = school_;
      weapon                 = &( player -> main_hand_weapon );
      weapon_multiplier      = multiplier;
      base_execute_time      = weapon -> swing_time;
      crit_bonus_multiplier *= 1.0 + o() -> spec.elemental_fury -> effectN( 1 ).percent();

      // Non-physical damage melee is spell powered
      if ( school != SCHOOL_PHYSICAL )
        spell_power_mod.direct = 1.0;
    }

    shaman_t* o() const
    { return debug_cast<shaman_t*>( debug_cast< pet_t* > ( player ) -> owner ); }

    void execute() override
    {
      // If we're casting, we should clip a swing
      if ( time_to_execute > timespan_t::zero() && player -> executing )
        schedule_execute();
      else
        melee_attack_t::execute();
    }
  };

  struct auto_attack_t : public melee_attack_t
  {
    auto_attack_t( primal_elemental_t* player, school_e school, double multiplier = 1.0 ) :
      melee_attack_t( "auto_attack", player )
    {
      assert( player -> main_hand_weapon.type != WEAPON_NONE );
      player -> main_hand_attack = new melee_t( player, school, multiplier );
    }

    void execute() override
    { player -> main_hand_attack -> schedule_execute(); }

    virtual bool ready() override
    {
      if ( player -> is_moving() ) return false;
      return ( player -> main_hand_attack -> execute_event == nullptr );
    }
  };

  struct primal_elemental_spell_t : public spell_t
  {
    primal_elemental_t* p;

    primal_elemental_spell_t( const std::string& t,
                              primal_elemental_t* p,
                              const spell_data_t* s = spell_data_t::nil(),
                              const std::string& options = std::string() ) :
      spell_t( t, p, s ), p( p )
    {
      parse_options( options );

      may_crit                    = true;
      base_costs[ RESOURCE_MANA ] = 0;
      crit_bonus_multiplier *= 1.0 + o() -> spec.elemental_fury -> effectN( 1 ).percent();
    }

    shaman_t* o() const
    { return debug_cast<shaman_t*>( debug_cast< pet_t* > ( player ) -> owner ); }
  };

  const spell_data_t* command;
  bool gains_damage;

  primal_elemental_t( shaman_t* owner, const std::string& name, bool guardian = false ) :
    pet_t( owner -> sim, owner, name, guardian ), gains_damage( true )
  {
    stamina_per_owner = 1.0;
    regen_type = REGEN_DISABLED;
  }

  shaman_t* o() const
  { return static_cast< shaman_t* >( owner ); }

  resource_e primary_resource() const override
  { return RESOURCE_MANA; }

  void init_spells() override
  {
    pet_t::init_spells();

    command = find_spell( 21563 );
  }

  action_t* create_action( const std::string& name,
                           const std::string& options_str ) override
  {
    if ( name == "travel"      ) return new travel_t( this );

    return pet_t::create_action( name, options_str );
  }

  double composite_attack_power_multiplier() const override
  {
    double m = pet_t::composite_attack_power_multiplier();

    if ( gains_damage )
      m *= 1.0 + o() -> talent.primal_elementalist -> effectN( 1 ).percent();

    return m;
  }

  double composite_spell_power_multiplier() const override
  {
    double m = pet_t::composite_spell_power_multiplier();

    if ( gains_damage )
      m *= 1.0 + o() -> talent.primal_elementalist -> effectN( 1 ).percent();

    return m;
  }

  double composite_player_multiplier( school_e school ) const override
  {
    double m = pet_t::composite_player_multiplier( school );

    if ( o() -> race == RACE_ORC )
      m *= 1.0 + command -> effectN( 1 ).percent();

    if ( ( dbc::is_school( school, SCHOOL_FIRE ) ||
           dbc::is_school( school, SCHOOL_FROST ) ||
           dbc::is_school( school, SCHOOL_NATURE ) ) &&
         o() -> mastery.enhanced_elements -> ok() )
      m *= 1.0 + o() -> cache.mastery_value();

    return m;
  }
};

// ==========================================================================
// Earth Elemental
// ==========================================================================

struct earth_elemental_t : public primal_elemental_t
{
  earth_elemental_t( shaman_t* owner, bool guardian ) :
    primal_elemental_t( owner, ( ! guardian ) ? "primal_earth_elemental" : "greater_earth_elemental", guardian )
  { }

  virtual void init_base_stats() override
  {
    primal_elemental_t::init_base_stats();

    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.swing_time = timespan_t::from_seconds( 2.0 );

    resources.base[ RESOURCE_HEALTH ] = 8000; // Approximated from lvl85 earth elemental in game
    resources.base[ RESOURCE_MANA   ] = 0; //

    owner_coeff.ap_from_sp = 0.05;
  }

  void init_action_list() override
  {
    // Simple as it gets, travel to target, kick off melee
    action_list_str = "travel/auto_attack,moving=0";

    primal_elemental_t::init_action_list();
  }

  virtual action_t* create_action( const std::string& name,
                                   const std::string& options_str ) override
  {
    // EE seems to use 130% weapon multiplier on attacks, while inheriting 5% SP as AP
    if ( name == "auto_attack" ) return new auto_attack_t ( this, SCHOOL_PHYSICAL );

    return primal_elemental_t::create_action( name, options_str );
  }
};

// ==========================================================================
// Fire Elemental
// ==========================================================================

struct fire_elemental_t : public primal_elemental_t
{
  struct fire_nova_t  : public primal_elemental_spell_t
  {
    fire_nova_t( fire_elemental_t* player, const std::string& options ) :
      primal_elemental_spell_t( "fire_nova", player, player -> find_spell( 117588 ), options )
    {
      aoe = -1;
    }
  };

  struct fire_blast_t : public primal_elemental_spell_t
  {
    fire_blast_t( fire_elemental_t* player, const std::string& options ) :
      primal_elemental_spell_t( "fire_blast", player, player -> find_spell( 57984 ), options )
    {
    }

    bool usable_moving() const override
    { return true; }
  };

  struct immolate_t : public primal_elemental_spell_t
  {
    immolate_t( fire_elemental_t* player, const std::string& options ) :
      primal_elemental_spell_t( "immolate", player, player -> find_spell( 118297 ), options )
    {
      hasted_ticks = tick_may_crit = true;
    }
  };

  fire_elemental_t( shaman_t* owner, bool guardian ) :
    primal_elemental_t( owner, ( ! guardian ) ? "primal_fire_elemental" : "greater_fire_elemental", guardian )
  { }

  void init_base_stats() override
  {
    primal_elemental_t::init_base_stats();

    resources.base[ RESOURCE_HEALTH ] = 32268; // Level 85 value
    resources.base[ RESOURCE_MANA   ] = 8908; // Level 85 value

    // FE Swings with a weapon, however the damage is "magical"
    main_hand_weapon.type            = WEAPON_BEAST;
    main_hand_weapon.swing_time      = timespan_t::from_seconds( 1.4 );

    owner_coeff.sp_from_sp = 0.25;
  }

  void init_action_list() override
  {
    action_list_str = "travel/auto_attack/fire_blast/fire_nova,if=spell_targets.fire_nova>=3";
    if ( type == PLAYER_PET )
      action_list_str += "/immolate,if=!ticking";

    pet_t::init_action_list();
  }

  action_t* create_action( const std::string& name,
                           const std::string& options_str ) override
  {
    if ( name == "auto_attack" ) return new auto_attack_t( this, SCHOOL_FIRE );
    if ( name == "fire_blast"  ) return new fire_blast_t( this, options_str );
    if ( name == "fire_nova"   ) return new fire_nova_t( this, options_str );
    if ( name == "immolate"    ) return new immolate_t( this, options_str );

    return primal_elemental_t::create_action( name, options_str );
  }
};

// ==========================================================================
// Storm Elemental
// ==========================================================================

struct storm_elemental_t : public primal_elemental_t
{
  // TODO: Healing
  struct wind_gust_t : public primal_elemental_spell_t
  {
    wind_gust_t( storm_elemental_t* player, const std::string& options ) :
      primal_elemental_spell_t( "wind_gust", player, player -> find_spell( 157331 ), options )
    { }
  };

  struct call_lightning_t : public primal_elemental_spell_t
  {
    call_lightning_t( storm_elemental_t* player, const std::string& options ) :
      primal_elemental_spell_t( "call_lightning", player, player -> find_spell( 157348 ), options )
    { }

    void execute() override
    {
      primal_elemental_spell_t::execute();

      static_cast<storm_elemental_t*>( player ) -> call_lightning -> trigger();
    }
  };

  buff_t* call_lightning;

  storm_elemental_t( shaman_t* owner, bool guardian ) :
    primal_elemental_t( owner, ( ! guardian ) ? "primal_storm_elemental" : "greater_storm_elemental", guardian )
  {
    gains_damage = false;
  }

  void init_base_stats() override
  {
    primal_elemental_t::init_base_stats();

    resources.base[ RESOURCE_HEALTH ] = 32268; // TODO-WOD: FE values, placeholder
    resources.base[ RESOURCE_MANA   ] = 8908;

    owner_coeff.sp_from_sp = 1.0000;
  }

  void init_action_list() override
  {
    action_list_str = "call_lightning/wind_gust";

    primal_elemental_t::init_action_list();
  }

  void create_buffs() override
  {
    primal_elemental_t::create_buffs();

    call_lightning = buff_creator_t( this, "call_lightning", find_spell( 157348 ) )
                     .cd( timespan_t::zero() );
  }

  double composite_player_multiplier( school_e school ) const override
  {
    double m = primal_elemental_t::composite_player_multiplier( school );

    // TODO-WOD: Enhance/Elemental has damage, Restoration has healing
    if ( call_lightning -> up() )
      m *= 1.0 + call_lightning -> data().effectN( 2 ).percent();

    return m;
  }

  action_t* create_action( const std::string& name,
                           const std::string& options_str ) override
  {
    if ( name == "call_lightning" ) return new call_lightning_t( this, options_str );
    if ( name == "wind_gust"      ) return new wind_gust_t( this, options_str );

    return primal_elemental_t::create_action( name, options_str );
  }
};

struct lightning_elemental_t : public pet_t
{
  struct lightning_blast_t : public spell_t
  {
    lightning_blast_t( lightning_elemental_t* p ) :
      spell_t( "lightning_blast", p, p -> find_spell( 145002 ) )
    {
      base_costs[ RESOURCE_MANA ] = 0;
      may_crit = true;
    }

    double composite_haste() const override
    { return 1.0; }
  };

  lightning_elemental_t( shaman_t* owner ) :
    pet_t( owner -> sim, owner, "lightning_elemental", true, true )
  {
    stamina_per_owner = 1.0;
    regen_type = REGEN_DISABLED;
  }

  void init_base_stats() override
  {
    pet_t::init_base_stats();
    owner_coeff.sp_from_sp = 0.75;
  }

  action_t* create_action( const std::string& name, const std::string& options_str ) override
  {
    if ( name == "lightning_blast" ) return new lightning_blast_t( this );
    return pet_t::create_action( name, options_str );
  }

  void init_action_list() override
  {
    action_list_str = "lightning_blast";

    pet_t::init_action_list();
  }

  resource_e primary_resource() const override
  { return specialization() == SHAMAN_RESTORATION ? RESOURCE_MANA : RESOURCE_MAELSTROM; }
};
} // Namespace pet ends

// ==========================================================================
// Shaman Secondary Spells / Attacks
// ==========================================================================

struct t15_2pc_caster_t : public shaman_spell_t
{
  t15_2pc_caster_t( shaman_t* player ) :
    shaman_spell_t( "t15_lightning_strike", player,
                    player -> sets.set( SET_CASTER, T15, B2 ) -> effectN( 1 ).trigger() )
  {
    proc = background = split_aoe_damage = true;
    callbacks = false;
    aoe = -1;
  }
};

struct flametongue_weapon_spell_t : public shaman_spell_t
{
  flametongue_weapon_spell_t( const std::string& n, shaman_t* player, weapon_t* w ) :
    shaman_spell_t( n, player, player -> find_spell( 10444 ) )
  {
    may_crit = background = true;

    if ( player -> specialization() == SHAMAN_ENHANCEMENT )
    {
      snapshot_flags          = STATE_AP;
      attack_power_mod.direct = w -> swing_time.total_seconds() / 2.6 * 0.075;
    }
  }
};

struct ancestral_awakening_t : public shaman_heal_t
{
  ancestral_awakening_t( shaman_t* player ) :
    shaman_heal_t( "ancestral_awakening", player, player -> find_spell( 52752 ) )
  {
    background = proc = true;
  }

  double composite_da_multiplier( const action_state_t* state ) const override
  {
    double m = shaman_heal_t::composite_da_multiplier( state );
    m *= p() -> spec.ancestral_awakening -> effectN( 1 ).percent();
    return m;
  }

  void execute() override
  {
    target = find_lowest_player();
    shaman_heal_t::execute();
  }
};

struct windfury_weapon_melee_attack_t : public shaman_attack_t
{
  double furious_winds_chance;

  windfury_weapon_melee_attack_t( const std::string& n, shaman_t* player, weapon_t* w ) :
    shaman_attack_t( n, player, player -> find_spell( 25504 ) ), furious_winds_chance( 0 )
  {
    weapon           = w;
    school           = SCHOOL_PHYSICAL;
    background       = true;
    callbacks        = false;

    // Windfury can not proc itself
    may_proc_windfury = false;

    // Enhancement Tier 18 (WoD 6.2) trinket effect
    if ( player -> furious_winds )
    {
      const spell_data_t* data = player -> furious_winds -> driver();
      double damage_value = data -> effectN( 1 ).average( player -> furious_winds -> item ) / 100.0;

      furious_winds_chance = data -> effectN( 2 ).average( player -> furious_winds -> item ) / 100.0;

      base_multiplier *= 1.0 + damage_value;
    }
  }

  double action_multiplier() const
  {
    double m = shaman_attack_t::action_multiplier();

    if ( p() -> buff.doom_winds -> up() )
    {
      m *= 1.0 + p() -> buff.doom_winds -> data().effectN( 2 ).percent();
    }

    return m;
  }
};

struct crash_lightning_attack_t : public shaman_attack_t
{
  crash_lightning_attack_t( shaman_t* p, const std::string& n, weapon_t* w ) :
    shaman_attack_t( n, p, p -> find_spell( 195592 ) )
  {
    weapon = w;
    background = true;
    callbacks = false;
    aoe = -1;
    cooldown -> duration = timespan_t::zero();
  }
};

struct stormstrike_attack_t : public shaman_attack_t
{
  crash_lightning_attack_t* cl;

  stormstrike_attack_t( const std::string& n, shaman_t* player, const spell_data_t* s, weapon_t* w ) :
    shaman_attack_t( n, player, s ),
    cl( new crash_lightning_attack_t( player, n + "_cl", w ) )
  {
    background = true;
    may_miss = may_dodge = may_parry = false;
    weapon = w;
    base_multiplier *= 1.0 + player -> artifact.hammer_of_storms.percent();
  }

  double action_multiplier() const
  {
    double m = shaman_attack_t::action_multiplier();

    if ( p() -> artifact.hurricane.rank() && p() -> buff.stormfury -> up() )
    {
      m *= 1.0 + p() -> artifact.hurricane.percent();
    }

    if ( p() -> buff.gathering_storms -> up() )
    {
      m *= p() -> buff.gathering_storms -> check_value();
    }

    return m;
  }

  void impact( action_state_t* s )
  {
    shaman_attack_t::impact( s );

    if ( result_is_hit( s -> result ) && p() -> buff.crash_lightning -> up() )
    {
      cl -> target = s -> target;
      cl -> schedule_execute();
    }
  }
};

struct windstrike_attack_t : public stormstrike_attack_t
{
  windstrike_attack_t( const std::string& n, shaman_t* player, const spell_data_t* s, weapon_t* w ) :
    stormstrike_attack_t( n, player, s, w )
  { }

  double target_armor( player_t* ) const override
  { return 0.0; }
};

struct windlash_t : public shaman_attack_t
{
  double swing_timer_variance;

  windlash_t( const std::string& n, const spell_data_t* s, shaman_t* player, weapon_t* w, double stv ) :
    shaman_attack_t( n, player, s ), swing_timer_variance( stv )
  {
    background = repeating = may_miss = may_dodge = may_parry = true;
    may_glance = special = false;
    weapon            = w;
    base_execute_time = w -> swing_time;
    trigger_gcd       = timespan_t::zero();
  }

  double target_armor( player_t* ) const override
  { return 0.0; }

  timespan_t execute_time() const override
  {
    timespan_t t = shaman_attack_t::execute_time();

    if ( swing_timer_variance > 0 )
    {
      timespan_t st = timespan_t::from_seconds( const_cast<windlash_t*>(this) -> rng().gauss( t.total_seconds(), t.total_seconds() * swing_timer_variance ) );
      if ( sim -> debug )
        sim -> out_debug.printf( "Swing timer variance for %s, real_time=%.3f swing_timer=%.3f", name(), t.total_seconds(), st.total_seconds() );

      return st;
    }
    else
      return t;
  }

  void execute() override
  {
    if ( time_to_execute > timespan_t::zero() && p() -> executing )
    {
      if ( sim -> debug )
        sim -> out_debug.printf( "Executing '%s' during melee (%s).", p() -> executing -> name(), util::slot_type_string( weapon -> slot ) );

      if ( weapon -> slot == SLOT_OFF_HAND )
        p() -> proc.swings_clipped_oh -> occur();
      else
        p() -> proc.swings_clipped_mh -> occur();

      schedule_execute();
    }
    else
    {
      shaman_attack_t::execute();
    }
  }
};

struct shaman_flurry_of_xuen_t : public shaman_attack_t
{
  shaman_flurry_of_xuen_t( shaman_t* p ) :
    shaman_attack_t( "flurry_of_xuen", p, p -> find_spell( 147891 ) )
  {
    special = may_miss = may_parry = may_block = may_dodge = may_crit = background = true;

    may_proc_windfury = false;
    may_proc_maelstrom = false;
    may_proc_flametongue = false;
    aoe = 5;
  }

  // We need to override shaman_action_state_t returning here, as tick_action
  // and custom state objects do not mesh at all really. They technically
  // work, but in reality we are doing naughty things in the code that are
  // not safe.
  action_state_t* new_state() override
  { return new action_state_t( this, target ); }
};

struct electrocute_t : public shaman_spell_t
{
  electrocute_t( shaman_t* p ) :
    shaman_spell_t( "electrocute", p, p -> find_spell( 189509 ) )
  {
    background = true;
    aoe = -1;
  }
};

struct unleash_doom_spell_t : public shaman_spell_t
{
  unleash_doom_spell_t( const std::string& n, shaman_t* p, const spell_data_t* s ) :
    shaman_spell_t( n, p, s )
  {
    callbacks = false;
    background = may_crit = true;
  }
};

struct doom_vortex_t : public shaman_spell_t
{
  doom_vortex_t( shaman_t* p ) :
    shaman_spell_t( "doom_vortex", p, p -> find_spell( 199116 ) )
  {
    aoe = -1;
    callbacks = false;
    background = may_crit = true;
  }
};

struct elemental_overload_spell_t : public shaman_spell_t
{
  double maelstrom_gain;

  elemental_overload_spell_t( shaman_t* p, const std::string& name, const spell_data_t* s ) :
    shaman_spell_t( name, p, s ),
    maelstrom_gain( s -> effectN( 2 ).resource( RESOURCE_MAELSTROM ) )
  {
    background = true;
    callbacks = false;

  }

  void execute() override
  {
    shaman_spell_t::execute();

    if ( maelstrom_gain > 0 )
    {
      double g = maelstrom_gain;
      if ( p() -> buff.fire_empowered -> up() )
      {
        g *= 1.0 + p() -> buff.fire_empowered -> data().effectN( 1 ).percent();
      }

      player -> resource_gain( RESOURCE_MAELSTROM, g, gain, this );
    }
  }
};

struct earthen_might_damage_t : public shaman_spell_t
{
  earthen_might_damage_t( shaman_t* p ) :
    shaman_spell_t( "earthen_might_damage", p, p -> find_spell( 199019 ) -> effectN( 1 ).trigger() )
  {
    may_crit = background = true;
    callbacks = false;
  }
};

struct earthen_might_t : public shaman_spell_t
{
  earthen_might_t( shaman_t* p ) :
    shaman_spell_t( "earthen_might", p, p -> find_spell( 199019 ) )
  {
    callbacks = may_crit = may_miss = hasted_ticks = false;
    background = true;
    tick_action = new earthen_might_damage_t( p );
  }
};

// ==========================================================================
// Shaman Action / Spell Base
// ==========================================================================

// shaman_spell_base_t::execute =============================================

template <class Base>
void shaman_spell_base_t<Base>::execute()
{
  ab::execute();

  shaman_t* p = ab::p();

  // Shamans have specialized swing timer reset system, where every cast time spell
  // resets the swing timers, _IF_ the spell is not maelstromable, or the maelstrom
  // weapon stack is zero, or ancient swiftness was not used to cast the spell
  if ( p -> specialization() != SHAMAN_ENHANCEMENT && ab::execute_time() > timespan_t::zero() )
  {
    if ( ab::sim -> debug )
    {
      ab::sim -> out_debug.printf( "%s resetting swing timers for '%s'", p -> name(),
          ab::name_str.c_str() );
    }

    timespan_t time_to_next_hit;

    if ( ab::player -> main_hand_attack && ab::player -> main_hand_attack -> execute_event )
    {
      time_to_next_hit = ab::player -> main_hand_attack -> execute_time();
      ab::player -> main_hand_attack -> execute_event -> reschedule( time_to_next_hit );
      p -> proc.swings_reset_mh -> occur();
    }

    if ( ab::player -> off_hand_attack && ab::player -> off_hand_attack -> execute_event )
    {
      time_to_next_hit = ab::player -> off_hand_attack -> execute_time();
      ab::player -> off_hand_attack -> execute_event -> reschedule( time_to_next_hit );
      p -> proc.swings_reset_oh -> occur();
    }
  }

  p -> buff.spiritwalkers_grace -> up();
}

// shaman_heal_t::impact ====================================================

void shaman_heal_t::impact( action_state_t* s )
{
  // Todo deep healing to adjust s -> result_amount by x% before impacting
  if ( sim -> debug && p() -> mastery.deep_healing -> ok() )
  {
    sim -> out_debug.printf( "%s Deep Heals %s@%.2f%% mul=%.3f %.0f -> %.0f",
                   player -> name(), s -> target -> name(),
                   s -> target -> health_percentage(), deep_healing( s ),
                   s -> result_amount, s -> result_amount * deep_healing( s ) );
  }

  s -> result_amount *= deep_healing( s );

  base_t::impact( s );

  if ( proc_tidal_waves )
    p() -> buff.tidal_waves -> trigger( p() -> buff.tidal_waves -> data().initial_stacks() );

  if ( s -> result == RESULT_CRIT )
  {
    if ( resurgence_gain > 0 )
      p() -> resource_gain( RESOURCE_MANA, resurgence_gain, p() -> gain.resurgence );

    if ( p() -> spec.ancestral_awakening -> ok() )
    {
      if ( ! p() -> action_ancestral_awakening )
      {
        p() -> action_ancestral_awakening = new ancestral_awakening_t( p() );
        p() -> action_ancestral_awakening -> init();
      }

      p() -> action_ancestral_awakening -> base_dd_min = s -> result_total;
      p() -> action_ancestral_awakening -> base_dd_max = s -> result_total;
    }
  }

  if ( p() -> main_hand_weapon.buff_type == EARTHLIVING_IMBUE )
  {
    double chance = ( s -> target -> resources.pct( RESOURCE_HEALTH ) > .35 ) ? elw_proc_high : elw_proc_low;

    if ( rng().roll( chance ) )
    {
      // Todo proc earthliving on target
    }
  }
}

// ==========================================================================
// Shaman Attack
// ==========================================================================

// shaman_attack_t::impact ============================================

// Melee Attack =============================================================

struct melee_t : public shaman_attack_t
{
  int sync_weapons;
  bool first;
  double swing_timer_variance;

  melee_t( const std::string& name, const spell_data_t* s, shaman_t* player, weapon_t* w, int sw, double stv ) :
    shaman_attack_t( name, player, s ), sync_weapons( sw ),
    first( true ), swing_timer_variance( stv )
  {
    auto_attack = true;
    background = repeating = may_glance = true;
    special           = false;
    trigger_gcd       = timespan_t::zero();
    weapon            = w;
    base_execute_time = w -> swing_time;

    if ( p() -> specialization() == SHAMAN_ENHANCEMENT && p() -> dual_wield() )
      base_hit -= 0.19;
  }

  void reset() override
  {
    shaman_attack_t::reset();

    first = true;
  }

  virtual timespan_t execute_time() const override
  {
    timespan_t t = shaman_attack_t::execute_time();
    if ( first )
    {
      return ( weapon -> slot == SLOT_OFF_HAND ) ? ( sync_weapons ? std::min( t / 2, timespan_t::zero() ) : t / 2 ) : timespan_t::zero();
    }

    if ( swing_timer_variance > 0 )
    {
      timespan_t st = timespan_t::from_seconds(const_cast<melee_t*>(this) ->  rng().gauss( t.total_seconds(), t.total_seconds() * swing_timer_variance ) );
      if ( sim -> debug )
        sim -> out_debug.printf( "Swing timer variance for %s, real_time=%.3f swing_timer=%.3f", name(), t.total_seconds(), st.total_seconds() );
      return st;
    }
    else
      return t;
  }

  void execute() override
  {
    if ( first )
    {
      first = false;
    }

    if ( time_to_execute > timespan_t::zero() && p() -> executing && p() -> executing -> interrupt_auto_attack )
    {
      if ( sim -> debug )
        sim -> out_debug.printf( "Executing '%s' during melee (%s).", p() -> executing -> name(), util::slot_type_string( weapon -> slot ) );

      if ( weapon -> slot == SLOT_OFF_HAND )
        p() -> proc.swings_clipped_oh -> occur();
      else
        p() -> proc.swings_clipped_mh -> occur();

      schedule_execute();
    }
    else
    {
      shaman_attack_t::execute();
    }
  }
};

// Auto Attack ==============================================================

struct auto_attack_t : public shaman_attack_t
{
  int sync_weapons;
  double swing_timer_variance;

  auto_attack_t( shaman_t* player, const std::string& options_str ) :
    shaman_attack_t( "auto_attack", player, spell_data_t::nil() ),
    sync_weapons( 0 ), swing_timer_variance( 0.00 )
  {
    add_option( opt_bool( "sync_weapons", sync_weapons ) );
    add_option( opt_float( "swing_timer_variance", swing_timer_variance ) );
    parse_options( options_str );
    ignore_false_positive = true;

    assert( p() -> main_hand_weapon.type != WEAPON_NONE );

    p() -> melee_mh      = new melee_t( "Main Hand", spell_data_t::nil(), player, &( p() -> main_hand_weapon ), sync_weapons, swing_timer_variance );
    p() -> melee_mh      -> school = SCHOOL_PHYSICAL;
    p() -> ascendance_mh = new windlash_t( "Wind Lash", player -> find_spell( 114089 ), player, &( p() -> main_hand_weapon ), swing_timer_variance );

    p() -> main_hand_attack = p() -> melee_mh;

    if ( p() -> off_hand_weapon.type != WEAPON_NONE && p() -> specialization() == SHAMAN_ENHANCEMENT )
    {
      if ( ! p() -> dual_wield() ) return;

      p() -> melee_oh = new melee_t( "Off-Hand", spell_data_t::nil(), player, &( p() -> off_hand_weapon ), sync_weapons, swing_timer_variance );
      p() -> melee_oh -> school = SCHOOL_PHYSICAL;
      p() -> ascendance_oh = new windlash_t( "Wind Lash Off-Hand", player -> find_spell( 114093 ), player, &( p() -> off_hand_weapon ), swing_timer_variance );

      p() -> off_hand_attack = p() -> melee_oh;

      p() -> off_hand_attack -> id = 1;
    }

    trigger_gcd = timespan_t::zero();
  }

  virtual void execute() override
  {
    p() -> main_hand_attack -> schedule_execute();
    if ( p() -> off_hand_attack )
      p() -> off_hand_attack -> schedule_execute();
  }

  virtual bool ready() override
  {
    if ( p() -> is_moving() ) return false;
    return ( p() -> main_hand_attack -> execute_event == nullptr ); // not swinging
  }
};

// Lava Lash Attack =========================================================

struct lava_lash_t : public shaman_attack_t
{
  double ft_bonus;
  crash_lightning_attack_t* cl;

  lava_lash_t( shaman_t* player, const std::string& options_str ) :
    shaman_attack_t( "lava_lash", player, player -> find_specialization_spell( "Lava Lash" ) ),
    ft_bonus( data().effectN( 2 ).percent() ),
    cl( new crash_lightning_attack_t( player, "lava_lash_cl", &( player -> off_hand_weapon ) ) )
  {
    check_spec( SHAMAN_ENHANCEMENT );
    school = SCHOOL_FIRE;

    base_multiplier *= 1.0 + player -> sets.set( SET_MELEE, T14, B2 ) -> effectN( 1 ).percent();
    base_multiplier *= 1.0 + player -> artifact.forged_in_lava.percent();

    parse_options( options_str );
    weapon              = &( player -> off_hand_weapon );

    if ( weapon -> type == WEAPON_NONE )
      background = true; // Do not allow execution.

    uses_eoe = player -> talent.echo_of_the_elements -> ok();
    add_child( cl );
    if ( player -> artifact.doom_vortex.rank() )
    {
      add_child( player -> doom_vortex );
    }
  }

  double cost() const override
  {
    if ( background )
    {
      return 0;
    }

    return shaman_attack_t::cost();
  }

  void update_ready( timespan_t cd = timespan_t::min() ) override
  {
    if ( background )
    {
      cd = timespan_t::zero();
    }

    shaman_attack_t::update_ready( cd );
  }

  void execute() override
  {
    shaman_attack_t::execute();

    if ( p() -> buff.lava_dredger -> up() )
    {
      background = true;
      schedule_execute();
      p() -> buff.lava_dredger -> decrement();
    }
    else
    {
      background = false;
    }
  }

  void impact( action_state_t* state ) override
  {
    shaman_attack_t::impact( state );

    p() -> trigger_improved_lava_lash( state );
    if ( result_is_hit( state -> result ) && p() -> buff.crash_lightning -> up() )
    {
      cl -> target = state -> target;
      cl -> schedule_execute();
    }

    if ( p() -> artifact.doom_vortex.rank() )
    {
      p() -> doom_vortex -> target = state -> target;
      p() -> doom_vortex -> schedule_execute();
    }
  }
};

struct improved_lava_lash_t : public shaman_spell_t
{
  improved_lava_lash_t( shaman_t* p ) :
    shaman_spell_t( "improved_lava_lash", p )
  {
    const spell_data_t* lava_lash = p -> find_specialization_spell( "Lava Lash" );

    may_miss = may_crit = false;
    proc = true;
    callbacks = false;
    background = true;
    dual = true;

    aoe = lava_lash -> effectN( 4 ).base_value();
  }

  // Exclude targets with your flame shock on
  size_t available_targets( std::vector< player_t* >& tl ) const override
  {
    tl.clear();

    shaman_td_t* main_target_td = td( target );
    dot_t* target_dot = main_target_td -> dot.flame_shock;
    for ( size_t i = 0, end = sim -> target_non_sleeping_list.size(); i < end; i++ )
    {
      player_t* actor = sim -> target_non_sleeping_list[ i ];
      if ( actor == target )
        continue;

      if ( td( actor ) -> dot.flame_shock -> remains() >= target_dot -> remains() )
        continue;

      tl.push_back( actor );
    }

    return tl.size();
  }

  std::vector< player_t* >& target_list() const override
  {
    size_t total_targets = available_targets( target_cache.list );

    // Reduce targets to aoe amount by removing random entries from the
    // target list until it's at aoe amount
    while ( total_targets > static_cast< size_t >( aoe ) )
    {
      bool removed = false;

      // Remove targets that have a flame shock first
      // TODO: The flame shocked targets should be randomly removed too, but
      // this will have to do for now
      for ( size_t i = 0; i < target_cache.list.size(); i++ )
      {
        shaman_td_t* td = this -> td( target_cache.list[ i ] );
        if ( td -> dot.flame_shock -> is_ticking() )
        {
          target_cache.list.erase( target_cache.list.begin() + i );
          removed = true;
          break;
        }
      }

      // There's no flame shocked targets to remove, eliminate a random target
      if ( ! removed )
        target_cache.list.erase( target_cache.list.begin() + static_cast< size_t >( const_cast<improved_lava_lash_t*>(this) -> rng().range( 0, as<double>( target_cache.list.size() ) ) ) );

      total_targets--;
    }

    return target_cache.list;
  }

  // A simple impact method that triggers the proxy flame shock application
  // on the selected target of the lava lash spread driver
  void impact( action_state_t* state ) override
  {
    if ( sim -> debug )
      sim -> out_debug.printf( "%s spreads Flame Shock (off of %s) on %s",
                     player -> name(),
                     target -> name(),
                     state -> target -> name() );

    dot_t* dot = td( target ) -> dot.flame_shock;
    if ( dot -> is_ticking() )
    {
      dot -> copy( state -> target, DOT_COPY_CLONE );
    }
  }
};

// Stormstrike Attack =======================================================

struct stormstrike_t : public shaman_attack_t
{
  stormstrike_attack_t * stormstrike_mh;
  stormstrike_attack_t * stormstrike_oh;

  stormstrike_t( shaman_t* player, const std::string& options_str ) :
    shaman_attack_t( "stormstrike", player, player -> find_specialization_spell( "Stormstrike" ) ),
    stormstrike_mh( nullptr ), stormstrike_oh( nullptr )
  {
    check_spec( SHAMAN_ENHANCEMENT );

    parse_options( options_str );

    weapon               = &( p() -> main_hand_weapon );
    weapon_multiplier    = 0.0;
    may_crit             = false;
    cooldown             = p() -> cooldown.strike;
    cooldown -> duration = p() -> find_spell( id ) -> cooldown();
    may_proc_flametongue = false;

    // Actual damaging attacks are done by stormstrike_attack_t
    // stormstrike_attack_t( std::string& n, shaman_t* player, const spell_data_t* s, weapon_t* w ) :
    stormstrike_mh = new stormstrike_attack_t( "stormstrike_mh", player, data().effectN( 1 ).trigger(), &( player -> main_hand_weapon ) );
    add_child( stormstrike_mh );
    add_child( stormstrike_mh -> cl );

    if ( p() -> off_hand_weapon.type != WEAPON_NONE )
    {
      stormstrike_oh = new stormstrike_attack_t( "stormstrike_offhand", player, data().effectN( 2 ).trigger(), &( player -> off_hand_weapon ) );
      add_child( stormstrike_oh );
      add_child( stormstrike_oh -> cl );
    }

    uses_eoe = player -> talent.echo_of_the_elements -> ok();
  }

  double cost() const override
  {
    double c = shaman_attack_t::cost();

    // Stormflurried
    if ( background == true )
    {
      return 0;
    }

    if ( p() -> buff.stormfury -> check() )
    {
      c *= 1.0 + p() -> buff.stormfury -> data().effectN( 3 ).percent();
    }

    return c;
  }

  void update_ready( timespan_t cd_duration = timespan_t::min() ) override
  {
    if ( p() -> buff.stormfury -> up() || background == true )
    {
      cd_duration = timespan_t::zero();
    }

    shaman_attack_t::update_ready( cd_duration );
  }

  void execute() override
  {
    shaman_attack_t::execute();

    if ( result_is_hit( execute_state -> result ) )
    {
      if ( p() -> sets.has_set_bonus( SHAMAN_ENHANCEMENT, T17, B2 ) )
      {
        p() -> cooldown.feral_spirits -> adjust( - p() -> sets.set( SHAMAN_ENHANCEMENT, T17, B2 ) -> effectN( 1 ).time_value() );
      }

      stormstrike_mh -> execute();
      if ( stormstrike_oh )
      {
        stormstrike_oh -> execute();
      }

      if ( p() -> artifact.unleash_doom.rank() == 1 && p() -> real_ppm.unleash_doom.trigger() )
      {
        p() -> buff.unleash_doom -> trigger();
      }
    }

    p() -> buff.stormfury -> decrement();

    // Don't try this at home, or anywhere else ..
    if ( p() -> artifact.stormflurry.rank() && rng().roll( p() -> artifact.stormflurry.percent() ) )
    {
      background = true;
      schedule_execute();
    }
    else
    {
      background = false;
    }
  }

  void reset() override
  {
    shaman_attack_t::reset();
    background = false;
  }

  bool ready() override
  {
    if ( p() -> buff.ascendance -> check() )
      return false;

    return shaman_attack_t::ready();
  }
};

// Windstrike Attack ========================================================

struct windstrike_t : public shaman_attack_t
{
  windstrike_attack_t * windstrike_mh;
  windstrike_attack_t * windstrike_oh;

  windstrike_t( shaman_t* player, const std::string& options_str ) :
    shaman_attack_t( "windstrike", player, player -> find_spell( 115356 ) ),
    windstrike_mh( nullptr ), windstrike_oh( nullptr )
  {
    check_spec( SHAMAN_ENHANCEMENT );

    parse_options( options_str );

    school               = SCHOOL_PHYSICAL;
    weapon               = &( p() -> main_hand_weapon );
    weapon_multiplier    = 0.0;
    may_crit             = false;
    cooldown             = p() -> cooldown.strike;
    may_proc_flametongue = false;
    // TODO: Legion windstrike is broken, and has bogus cooldown info for now
    //cooldown -> duration = p() -> find_spell( id ) -> cooldown();

    // Actual damaging attacks are done by stormstrike_attack_t
    windstrike_mh = new windstrike_attack_t( "windstrike_mh", player, data().effectN( 2 ).trigger(), &( player -> main_hand_weapon ) );
    windstrike_mh -> normalize_weapon_speed = true;
    windstrike_mh -> school = SCHOOL_PHYSICAL;
    add_child( windstrike_mh );
    add_child( windstrike_mh -> cl );

    if ( p() -> off_hand_weapon.type != WEAPON_NONE )
    {
      windstrike_oh = new windstrike_attack_t( "windstrike_offhand", player, data().effectN( 3 ).trigger(), &( player -> off_hand_weapon ) );
      windstrike_oh -> normalize_weapon_speed = true;
      windstrike_oh -> school = SCHOOL_PHYSICAL;
      add_child( windstrike_oh );
      add_child( windstrike_oh -> cl );
    }

    uses_eoe = player -> talent.echo_of_the_elements -> ok();
  }

  double cost() const override
  {
    double c = shaman_attack_t::cost();

    // Stormflurried
    if ( background == true )
    {
      return 0;
    }

    if ( p() -> buff.stormfury -> check() )
    {
      c *= 1.0 + p() -> buff.stormfury -> data().effectN( 3 ).percent();
    }

    return c;
  }

  void update_ready( timespan_t cd_duration = timespan_t::min() ) override
  {
    if ( p() -> buff.stormfury -> up() )
    {
      cd_duration = timespan_t::zero();
    }

    shaman_attack_t::update_ready( cd_duration );
  }

  void execute() override
  {
    shaman_attack_t::execute();

    if ( result_is_hit( execute_state -> result ) )
    {
      if ( p() -> sets.has_set_bonus( SHAMAN_ENHANCEMENT, T17, B2 ) )
      {
        p() -> cooldown.feral_spirits -> adjust( - p() -> sets.set( SHAMAN_ENHANCEMENT, T17, B2 ) -> effectN( 1 ).time_value() );
      }

      windstrike_mh -> execute();
      if ( windstrike_oh )
      {
        windstrike_oh -> execute();
      }

      if ( p() -> artifact.unleash_doom.rank() == 1 && p() -> real_ppm.unleash_doom.trigger() )
      {
        p() -> buff.unleash_doom -> trigger();
      }
    }

    p() -> buff.stormfury -> decrement();
    // Don't try this at home, or anywhere else ..
    if ( p() -> artifact.stormflurry.rank() && rng().roll( p() -> artifact.stormflurry.percent() ) )
    {
      background = true;
      schedule_execute();
    }
    else
    {
      background = false;
    }
  }

  void reset() override
  {
    shaman_attack_t::reset();
    background = false;
  }

  bool ready() override
  {
    if ( ! p() -> buff.ascendance -> check() )
      return false;

    return shaman_attack_t::ready();
  }
};

// Rockbiter Spell =========================================================

struct rockbiter_t : public shaman_spell_t
{
  double maelstrom_gain;

  rockbiter_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( "rockbiter", player, player -> find_specialization_spell( "Rockbiter" ), options_str ),
    maelstrom_gain( data().effectN( 2 ).resource( RESOURCE_MAELSTROM ) )
  {
    maelstrom_gain += player -> talent.spiritual_resonance -> effectN( 1 ).base_value();
    base_multiplier *= 1.0 + player -> artifact.surge_of_elements.percent();
  }

  void execute() override
  {
    shaman_spell_t::execute();

    double g = maelstrom_gain;
    if ( rng().roll( p() -> artifact.gathering_of_the_maelstrom.percent() ) )
    {
      g *= 2.0;
    }

    p() -> resource_gain( RESOURCE_MAELSTROM, g, gain );
    p() -> buff.rockbiter -> trigger();
  }
};

// Flametongue Spell =========================================================

struct flametongue_t : public shaman_spell_t
{
  flametongue_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( "flametongue", player, player -> find_specialization_spell( "Flametongue" ), options_str )
  {
    base_multiplier *= 1.0 + player -> artifact.surge_of_elements.percent();

    add_child( player -> flametongue );
  }

  void execute() override
  {
    shaman_spell_t::execute();

    p() -> buff.flametongue -> trigger();
  }
};

// Crash Lightning Attack ===================================================

struct crash_lightning_t : public shaman_attack_t
{
  crash_lightning_t( shaman_t* player, const std::string& options_str ) :
    shaman_attack_t( "crash_lightning", player, player -> find_specialization_spell( "Crash Lightning" ) )
  {
    parse_options( options_str );

    aoe = -1;
    radius += player -> talent.crashing_storm -> effectN( 1 ).base_value();
    weapon = &( p() -> main_hand_weapon );
  }

  void execute() override
  {
    shaman_attack_t::execute();

    if ( result_is_hit( execute_state -> result ) )
    {
      if ( execute_state -> n_targets > 1 )
      {
        p() -> buff.crash_lightning -> trigger();
      }

      if ( p() -> artifact.gathering_storms.rank() )
      {
        double v = 1.0 + p() -> artifact.gathering_storms.percent() * execute_state -> n_targets;
        p() -> buff.gathering_storms -> trigger( 1, v );
      }
    }
  }
};

struct fire_elemental_t : public shaman_spell_t
{
  fire_elemental_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( "fire_elemental", player, player -> find_specialization_spell( "Fire Elemental" ), options_str )
  {
    harmful = false;
  }

  void execute() override
  {
    shaman_spell_t::execute();

    // TODO: Incomplete spell data
    p() -> guardian_fire_elemental -> summon( timespan_t::from_seconds( 15 ) );
    p() -> buff.fire_empowered -> trigger();
  }
};

struct windsong_t : public shaman_spell_t
{
  windsong_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( "windsong", player, player -> talent.windsong, options_str )
  { }

  void execute() override
  {
    shaman_spell_t::execute();

    p() -> buff.windsong -> trigger();
  }
};

struct fists_of_stone_t : public shaman_spell_t
{
  fists_of_stone_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( "fists_of_stone", player, player -> talent.fists_of_stone, options_str )
  {
    harmful = may_crit = may_miss = false;
  }

  void execute() override
  {
    shaman_spell_t::execute();

    p() -> buff.fists_of_stone -> trigger();
  }
};

struct sundering_t : public shaman_spell_t
{
  sundering_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( "sundering", player, player -> talent.sundering, options_str )
  {
    background = true;
    aoe = -1; // TODO: This is likely not going to affect all enemies but it will do for now
  }
};

struct fury_of_air_aoe_t : public shaman_attack_t
{
  fury_of_air_aoe_t( shaman_t* player ) :
    shaman_attack_t( "fury_of_air_damage", player, player -> find_spell( 197385 ) )
  {
    background = true;
    aoe = -1;
    school = SCHOOL_PHYSICAL;

    weapon = &( player -> main_hand_weapon );
  }
};

struct fury_of_air_t : public shaman_spell_t
{
  fury_of_air_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( "fury_of_air", player, player -> talent.fury_of_air, options_str )
  {
    hasted_ticks = callbacks = false;

    tick_action = new fury_of_air_aoe_t( player );
  }
};

struct stonefist_strike_t : public shaman_attack_t
{
  stonefist_strike_t( shaman_t* player, const std::string& options_str ) :
    shaman_attack_t( "stonefist_strike", player, player -> talent.stonefist_strike )
  {
    weapon = &( player -> main_hand_weapon );
    weapon_multiplier *= 2.0; // TODO: Need a composite_weapon_multiplier() that takes a state object so we can differentiate properly between targets

    parse_options( options_str );
  }
};

struct earthen_spike_t : public shaman_attack_t
{
  earthen_spike_t( shaman_t* player, const std::string& options_str ) :
    shaman_attack_t( "earthen_spike", player, player -> talent.earthen_spike )
  {
    weapon = &( player -> main_hand_weapon );
    may_crit = true;

    parse_options( options_str );
  }

  void impact( action_state_t* s ) override
  {
    shaman_attack_t::impact( s );

    td( target ) -> debuff.earthen_spike -> trigger();
  }
};

// Lightning Shield Spell ===================================================

// TODO: Random targeting
struct lightning_shield_damage_t : public shaman_spell_t
{
  lightning_shield_damage_t( shaman_t* player ) :
    shaman_spell_t( "lightning_shield_damage", player, player -> find_spell( 192109 ) )
  {
    background = true;
  }
};

struct lightning_shield_t : public shaman_spell_t
{
  lightning_shield_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( "lightning_shield", player, player -> find_talent_spell( "Lightning Shield" ), options_str )
  {
    harmful = hasted_ticks = false;

    tick_action = new lightning_shield_damage_t( player );
  }

  virtual void execute() override
  {
    shaman_spell_t::execute();

    p() -> buff.lightning_shield -> trigger();
  }
};

// ==========================================================================
// Shaman Spells
// ==========================================================================

// Bloodlust Spell ==========================================================

struct bloodlust_t : public shaman_spell_t
{
  bloodlust_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( "bloodlust", player, player -> find_class_spell( "Bloodlust" ), options_str )
  {
    harmful = false;
  }

  virtual void execute() override
  {
    shaman_spell_t::execute();

    for ( size_t i = 0; i < sim -> player_non_sleeping_list.size(); ++i )
    {
      player_t* p = sim -> player_non_sleeping_list[ i ];
      if ( p -> buffs.exhaustion -> check() || p -> is_pet() )
        continue;
      p -> buffs.bloodlust -> trigger();
      p -> buffs.exhaustion -> trigger();
    }
  }

  virtual bool ready() override
  {
    if ( sim -> overrides.bloodlust )
      return false;

    if ( p() -> buffs.exhaustion -> check() )
      return false;

    if (  p() -> buffs.bloodlust -> cooldown -> down() )
      return false;

    return shaman_spell_t::ready();
  }
};

// Chain Lightning Spell ====================================================

struct chain_lightning_t: public shaman_spell_t
{
  double maelstrom_gain;
  elemental_overload_spell_t* overload;

  chain_lightning_t( shaman_t* player, const std::string& options_str ):
    shaman_spell_t( "chain_lightning", player, player -> find_specialization_spell( "Chain Lightning" ), options_str ),
    maelstrom_gain( data().effectN( 2 ).resource( RESOURCE_MAELSTROM ) )
  {
    aoe = 3;
    base_add_multiplier = data().effectN( 1 ).chain_multiplier();
    radius = 10.0;

    if ( player -> mastery.elemental_overload -> ok() )
    {
      overload = new elemental_overload_spell_t( player, "chain_lightning_overload", player -> find_spell( 45297 ) );
      add_child( overload );
    }
  }

  // Make Chain Lightning a single target spell for procs
  proc_types proc_type() const override
  { return PROC1_SPELL; }

  void execute() override
  {
    shaman_spell_t::execute();

    // Technically, should occur before impact, but zero-travel-time foils things
    double g = maelstrom_gain * execute_state -> n_targets;
    if ( p() -> buff.fire_empowered -> up() )
    {
      g *= 1.0 + p() -> buff.fire_empowered -> data().effectN( 1 ).percent();
    }

    player -> resource_gain( RESOURCE_MAELSTROM, g, gain, this );
  }

  void impact( action_state_t* state ) override
  {
    shaman_spell_t::impact( state );

    if ( result_is_hit( state -> result ) )
    {
      if ( p() -> spec.fulmination -> ok() )
      {
        player -> resource_gain( RESOURCE_MAELSTROM,
                                 p() -> spec.fulmination -> effectN( 2 ).resource( RESOURCE_MAELSTROM ),
                                 p() -> gain.fulmination,
                                 this );
      }
    }

    if ( p() -> mastery.elemental_overload -> ok() &&
         rng().roll( player -> cache.mastery_value() ) )
    {
      overload -> target = state -> target;
      overload -> schedule_execute();
    }

    p() -> trigger_tier15_2pc_caster( state );
    p() -> trigger_tier16_4pc_caster( state );
  }

  bool ready() override
  {
    if ( p() -> specialization() == SHAMAN_ELEMENTAL && p() -> buff.ascendance -> check() )
      return false;

    return shaman_spell_t::ready();
  }

  /**
    Check_distance_targeting is only called when distance_targeting_enabled is true. Otherwise,
    available_targets is called.  The following code is intended to generate a target list that
    properly accounts for range from each target during chain lightning.  On a very basic level, it
    starts at the original target, and then finds a path that will hit 4 more, if possible.  The
    code below randomly cycles through targets until it finds said path, or hits the maximum amount
    of attempts, in which it gives up and just returns the current best path.  I wouldn't be
    terribly surprised if Blizz did something like this in game.
  **/
  std::vector<player_t*> check_distance_targeting( std::vector< player_t* >& tl ) const override
  {
    player_t* last_chain; // We have to track the last target that it hit.
    last_chain = target;
    std::vector< player_t* > best_so_far; // Keeps track of the best chain path found so far, so we can use it if we give up.
    std::vector< player_t* > current_attempt;
    best_so_far.push_back( last_chain );
    current_attempt.push_back( last_chain );

    size_t num_targets = sim -> target_non_sleeping_list.size();
    size_t max_attempts = static_cast<size_t>( std::min( ( num_targets - 1.0 ) * 2.0 , 30.0 ) ); // With a lot of targets this can get pretty high. Cap it at 30.
    size_t local_attempts = 0, attempts = 0, chain_number = 1;
    std::vector<player_t*> targets_left_to_try( sim -> target_non_sleeping_list.data() ); // This list contains members of a vector that haven't been tried yet.
    auto position = std::find( targets_left_to_try.begin(), targets_left_to_try.end(), target );
    if ( position != targets_left_to_try.end() )
      targets_left_to_try.erase( position );

    std::vector<player_t*> original_targets( targets_left_to_try ); // This is just so we don't have to constantly remove the original target.

    bool stop_trying = false; // It's not you, it's me.

    while ( !stop_trying )
    {
      local_attempts = 0;
      attempts++;
      if ( attempts >= max_attempts )
        stop_trying = true;
      while ( targets_left_to_try.size() > 0 && local_attempts < num_targets * 2 )
      {
        player_t* possibletarget;
        size_t rng_target = static_cast<size_t>( rng().range( 0.0, ( static_cast<double>( targets_left_to_try.size() ) - 0.000001 ) ) );
        possibletarget = targets_left_to_try[rng_target];

        double distance_from_last_chain = last_chain -> get_player_distance( *possibletarget );
        if ( distance_from_last_chain <= radius )
        {
          last_chain = possibletarget;
          current_attempt.push_back( last_chain );
          targets_left_to_try.erase( targets_left_to_try.begin() + rng_target );
          chain_number++;
        }
        else
        {
          // If there is no hope of this target being chained to, there's no need to test it again
          // for other possibilities.
          if ( distance_from_last_chain > ( radius * ( aoe - chain_number ) ) )
            targets_left_to_try.erase( targets_left_to_try.begin() + rng_target );
          local_attempts++; // Only count failures towards the limit-cap.
        }
        // If we run out of targets to hit, or have hit 5 already. Break.
        if ( static_cast<int>( current_attempt.size() ) == aoe || current_attempt.size() == num_targets ) 
        {
          stop_trying = true;
          break;
        }
      }
      if ( current_attempt.size() > best_so_far.size() )
        best_so_far = current_attempt;

      current_attempt.clear();
      current_attempt.push_back( target );
      last_chain = target;
      targets_left_to_try = original_targets;
      chain_number = 1;
    }

    if ( sim -> log )
      sim -> out_debug.printf( "%s Total attempts at finding path: %.3f - %.3f targets found - %s target is first chain", 
        player -> name(), static_cast<double>(attempts), static_cast<double>( best_so_far.size() ), target -> name() );
    tl.swap( best_so_far );
    return tl;
  }
};

// Lava Beam Spell ==========================================================

struct lava_beam_t : public shaman_spell_t
{
  lava_beam_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( "lava_beam", player, player -> find_spell( 114074 ), options_str )
  {
    aoe                   = 5;
    base_add_multiplier   = data().effectN( 1 ).chain_multiplier();
  }

  void impact( action_state_t* state ) override
  {
    shaman_spell_t::impact( state );

    p() -> trigger_tier15_2pc_caster( state );
  }

  bool ready() override
  {
    if ( ! p() -> buff.ascendance -> check() )
      return false;

    return shaman_spell_t::ready();
  }
};

// Fire Nova Spell ==========================================================

struct fire_nova_explosion_t : public shaman_spell_t
{
  fire_nova_explosion_t( shaman_t* player ) :
    shaman_spell_t( "fire_nova_explosion", player, player -> find_spell( 8349 ) )
  {
    check_spec( SHAMAN_ENHANCEMENT );
    aoe        = -1;
    background = true;
    dual = true;
  }

  void init() override
  {
    shaman_spell_t::init();

    stats = player -> get_stats( "fire_nova" );
  }

  void execute() override
  {
    // The explosion has to regenerate target cache every time
    target_cache.is_valid = false;
    shaman_spell_t::execute();
  }

  // Fire nova does not damage the main target.
  size_t available_targets( std::vector< player_t* >& tl ) const override
  {
    shaman_spell_t::available_targets( tl );

    for ( size_t i = 0; i < tl.size(); i++ )
    {
      if ( tl[i] == target ) // Cannot hit the original target.
        tl.erase( tl.begin() + i );
    }

    return tl.size();
  }
};

struct fire_nova_t : public shaman_spell_t
{
  fire_nova_t( shaman_t* player, const std::string& options_str ):
    shaman_spell_t( "fire_nova", player, player -> find_specialization_spell( "Fire Nova" ), options_str )
  {
    may_crit = may_miss = callbacks = false;
    aoe = -1;

    impact_action = new fire_nova_explosion_t( player );
    uses_eoe = player -> talent.echo_of_the_elements -> ok();
  }

  // Override assess_damage, as fire_nova_explosion is going to do all the
  // damage for us.
  void assess_damage( dmg_e type, action_state_t* s ) override
  { if ( s -> result_amount > 0 ) shaman_spell_t::assess_damage( type, s ); }

  bool ready() override
  {
    if ( ! td( target ) -> dot.flame_shock -> is_ticking() )
      return false;

    return shaman_spell_t::ready();
  }

  // Fire nova is emitted on all targets with a flame shock from us .. so
  std::vector< player_t* >& target_list() const override
  {
    target_cache.list.clear();
    int fire_nova_targets = 0;
    // WoD Hotfix - 10-30-14
    // Fire Nova now has a maximum of 6 novas that can be triggered if more than 6 targets are affected by Flame Shock.
    for ( size_t i = 0; i < sim -> target_non_sleeping_list.size(); ++i )
    {
      if ( fire_nova_targets == 6 )
        break;
      player_t* e = sim -> target_non_sleeping_list[ i ];
      if ( ! e -> is_enemy() )
        continue;

      if ( td( e ) -> dot.flame_shock -> is_ticking() )
      {
        target_cache.list.push_back( e );
        fire_nova_targets++;
      }
    }

    return target_cache.list;
  }
};

// Lava Burst Spell =========================================================

struct lava_burst_overload_t : public elemental_overload_spell_t
{
  lava_burst_overload_t( shaman_t* p ) :
    elemental_overload_spell_t( p, "lava_burst_overload", p -> find_spell( 77451 ) )
  {
    base_execute_time = timespan_t::zero(); // TODO: Check later
  }

  double composite_target_multiplier( player_t* target ) const override
  {
    double m = shaman_spell_t::composite_target_multiplier( target );

    if ( td( target ) -> dot.flame_shock -> is_ticking() )
    {
      m *= 1.0 + p() -> spell.flame_shock -> effectN( 3 ).percent();
    }

    return m;
  }

  double composite_target_crit( player_t* ) const override
  { return 1.0; }
};

struct lava_burst_t : public shaman_spell_t
{
  double maelstrom_gain;
  lava_burst_overload_t* overload;

  lava_burst_t( shaman_t* player, const std::string& options_str ):
    shaman_spell_t( "lava_burst", player, player -> find_specialization_spell( "Lava Burst" ), options_str ),
    maelstrom_gain( data().effectN( 2 ).resource( RESOURCE_MAELSTROM ) )
  {
    uses_eoe = player -> talent.echo_of_the_elements -> ok();

    if ( player -> mastery.elemental_overload -> ok() )
    {
      overload = new lava_burst_overload_t( player );
      add_child( overload );
    }
  }

  double composite_target_multiplier( player_t* target ) const override
  {
    double m = shaman_spell_t::composite_target_multiplier( target );

    if ( td( target ) -> dot.flame_shock -> is_ticking() )
    {
      m *= 1.0 + p() -> spell.flame_shock -> effectN( 3 ).percent();
    }

    return m;
  }

  double composite_target_crit( player_t* ) const override
  { return 1.0; }

  void update_ready( timespan_t /* cd_duration */ ) override
  {
    timespan_t d = cooldown -> duration;

    // Lava Surge has procced during the cast of Lava Burst, the cooldown 
    // reset is deferred to the finished cast, instead of "eating" it.
    if ( p() -> lava_surge_during_lvb )
    {
      d = timespan_t::zero();
      cooldown -> last_charged = sim -> current_time();
    }

    shaman_spell_t::update_ready( d );
  }

  void execute() override
  {
    shaman_spell_t::execute();

    double g = maelstrom_gain;
    if ( p() -> buff.fire_empowered -> up() )
    {
      g *= 1.0 + p() -> buff.fire_empowered -> data().effectN( 1 ).percent();
    }

    player -> resource_gain( RESOURCE_MAELSTROM, g, gain, this );

    p() -> cooldown.ascendance -> ready -= p() -> sets.set( SET_CASTER, T15, B4 ) -> effectN( 1 ).time_value();

    // Lava Surge buff does not get eaten, if the Lava Surge proc happened
    // during the Lava Burst cast
    if ( ! p() -> lava_surge_during_lvb && p() -> buff.lava_surge -> check() )
      p() -> buff.lava_surge -> expire();

    p() -> lava_surge_during_lvb = false;
  }

  timespan_t execute_time() const override
  {
    if ( p() -> buff.lava_surge -> up() )
    {
      return timespan_t::zero();
    }

    return shaman_spell_t::execute_time();
  }

  void impact( action_state_t* state ) override
  {
    shaman_spell_t::impact( state );

    if ( result_is_hit( state -> result ) &&  p() -> spec.fulmination -> ok() )
    {
      player -> resource_gain( RESOURCE_MAELSTROM,
                               p() -> spec.fulmination -> effectN( 1 ).resource( RESOURCE_MAELSTROM ),
                               p() -> gain.fulmination,
                               this );
    }

    if ( p() -> mastery.elemental_overload -> ok() &&
         rng().roll( player -> cache.mastery_value() ) )
    {
      overload -> target = state -> target;
      overload -> schedule_execute();
    }

  }
};

// Lightning Bolt Spell =====================================================

struct lightning_bolt_t : public shaman_spell_t
{
  double max_cost, maelstrom_gain;
  elemental_overload_spell_t* overload;

  lightning_bolt_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( "lightning_bolt", player, player -> find_specialization_spell( "Lightning Bolt" ), options_str ),
    max_cost( data().powerN( POWER_MAELSTROM ).max_cost() ),
    maelstrom_gain( data().effectN( 2 ).resource( RESOURCE_MAELSTROM ) )
  {
    if ( player -> specialization() == SHAMAN_ENHANCEMENT )
    {
      base_costs[ RESOURCE_MAELSTROM ] = 1; // Fake a cost so action_t::cost() will be run (in consume_resource())
      attack_power_mod.direct = 0.1; // Hardcoded to tooltip
    }

    if ( player -> mastery.elemental_overload -> ok() )
    {
      overload = new elemental_overload_spell_t( player, "lightning_bolt_overload", player -> find_spell( 45284 ) );
      add_child( overload );
    }
  }

  double cost() const override
  {
    return std::min( p() -> resources.current[ RESOURCE_MAELSTROM ], max_cost );
  }

  double attack_direct_power_coefficient( const action_state_t* /* state */ ) const override
  {
    return attack_power_mod.direct * ( std::min( p() -> resources.current[ RESOURCE_MAELSTROM ], max_cost ) );
  }

  double composite_da_multiplier( const action_state_t* state ) const override
  {
    double m = shaman_spell_t::composite_da_multiplier( state );

    m *= 1.0 + p() -> sets.set( SET_CASTER, T14, B2 ) -> effectN( 1 ).percent();

    return m;
  }

  void execute() override
  {
    shaman_spell_t::execute();

    if ( maelstrom_gain > 0 )
    {
      double g = maelstrom_gain;
      if ( p() -> buff.fire_empowered -> up() )
      {
        g *= 1.0 + p() -> buff.fire_empowered -> data().effectN( 1 ).percent();
      }

      player -> resource_gain( RESOURCE_MAELSTROM, g, gain, this );
    }
  }

  void impact( action_state_t* state ) override
  {
    shaman_spell_t::impact( state );

    if ( result_is_hit( state -> result ) )
    {
      if ( p() -> spec.fulmination -> ok() )
      {
        player -> resource_gain( RESOURCE_MAELSTROM,
                                 p() -> spec.fulmination -> effectN( 1 ).resource( RESOURCE_MAELSTROM ),
                                 p() -> gain.fulmination,
                                 this );
      }
    }

    if ( p() -> mastery.elemental_overload -> ok() && rng().roll( player -> cache.mastery_value() ) )
    {
      overload -> target = state -> target;
      overload -> schedule_execute();
    }

    p() -> trigger_tier15_2pc_caster( state );
    p() -> trigger_tier16_4pc_caster( state );
  }
};

// Elemental Blast Spell ====================================================

struct elemental_blast_t : public shaman_spell_t
{
  elemental_blast_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( "elemental_blast", player, player -> talent.elemental_blast, options_str )
  { }

  result_e calculate_result( action_state_t* s ) const override
  {
    if ( ! s -> target )
      return RESULT_NONE;

    result_e result = RESULT_NONE;

    if ( rng().roll( miss_chance( composite_hit(), s -> target ) ) )
      result = RESULT_MISS;

    if ( result == RESULT_NONE )
    {
      result = RESULT_HIT;
      unsigned max_buffs = 4 + ( p() -> specialization() == SHAMAN_ENHANCEMENT ? 1 : 0 );

      unsigned b = static_cast< unsigned >( rng().range( 0, max_buffs ) );
      assert( b < max_buffs );

      p() -> buff.elemental_blast_agility -> expire();
      p() -> buff.elemental_blast_crit -> expire();
      p() -> buff.elemental_blast_haste -> expire();
      p() -> buff.elemental_blast_mastery -> expire();
      p() -> buff.elemental_blast_multistrike -> expire();

      if ( b == 0 )
        p() -> buff.elemental_blast_crit -> trigger();
      else if ( b == 1 )
        p() -> buff.elemental_blast_haste -> trigger();
      else if ( b == 2 )
        p() -> buff.elemental_blast_mastery -> trigger();
      else if ( b == 3 )
        p() -> buff.elemental_blast_multistrike -> trigger();
      else
        p() -> buff.elemental_blast_agility -> trigger();

      if ( rng().roll( std::max( composite_crit() + composite_target_crit( s -> target ), 0.0 ) ) )
        result = RESULT_CRIT;
    }

    if ( sim -> debug )
      sim -> out_debug.printf( "%s result for %s is %s", player -> name(), name(), util::result_type_string( result ) );

    // Re-snapshot state here, after we have procced a buff spell. The new state
    // is going to be used to calculate the damage of this spell already
    const_cast<elemental_blast_t*>(this) -> snapshot_state( s, DMG_DIRECT );
    if ( sim -> debug )
      s -> debug();

    return result;
  }
};


// Shamanistic Rage Spell ===================================================

struct shamanistic_rage_t : public shaman_spell_t
{
  shamanistic_rage_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( "shamanistic_rage", player, player -> find_specialization_spell( "Shamanistic Rage" ), options_str )
  {
    harmful   = false;
  }

  virtual void execute() override
  {
    shaman_spell_t::execute();

    p() -> buff.shamanistic_rage -> trigger();
  }
};

// Spirit Wolf Spell ========================================================

struct feral_spirit_spell_t : public shaman_spell_t
{
  feral_spirit_spell_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( "feral_spirit", player, player -> find_specialization_spell( "Feral Spirit" ), options_str )
  {
    harmful   = false;

    cooldown -> duration += player -> talent.spiritual_affinity -> effectN( 1 ).time_value();
  }

  virtual void execute() override
  {
    shaman_spell_t::execute();

    int n = 0;
    for ( size_t i = 0; i < p() -> pet_feral_spirit.size() && n < data().effectN( 1 ).base_value(); i++ )
    {
      if ( ! p() -> pet_feral_spirit[ i ] -> is_sleeping() )
        continue;

      p() -> pet_feral_spirit[ i ] -> summon( data().duration() );
      n++;
    }

    p() -> buff.feral_spirit -> trigger();
    p() -> buff.feral_spirit2 -> trigger();
  }

  bool ready() override
  {
    if ( p() -> talent.feral_kin -> ok() )
    {
      return false;
    }

    return shaman_spell_t::ready();
  }
};

struct feral_kin_t : public shaman_spell_t
{
  feral_kin_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( "feral_kin", player, player -> talent.feral_kin, options_str )
  {
    harmful   = false;

    cooldown -> duration += player -> talent.spiritual_affinity -> effectN( 1 ).time_value();
  }

  virtual void execute() override
  {
    shaman_spell_t::execute();

    // TODO: Summon kindred spirit

    p() -> buff.feral_spirit2 -> trigger();
  }
};

// Thunderstorm Spell =======================================================

struct thunderstorm_t : public shaman_spell_t
{
  thunderstorm_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( "thunderstorm", player, player -> find_specialization_spell( "Thunderstorm" ), options_str )
  {
    aoe = -1;
  }
};

// Earthquake Spell =========================================================

struct earthquake_rumble_t : public shaman_spell_t
{
  earthquake_rumble_t( shaman_t* player ) :
    shaman_spell_t( "earthquake_rumble", player, player -> find_spell( 77478 ) )
  {
    harmful = background = true;
    aoe = -1;
    school = SCHOOL_PHYSICAL;
    spell_power_mod.direct = 0.11; // Hardcoded into tooltip because it's cool
    ground_aoe = true;
  }

  virtual double composite_spell_power() const override
  {
    double sp = shaman_spell_t::composite_spell_power();

    sp += player -> cache.spell_power( SCHOOL_NATURE );

    return sp;
  }

  virtual double target_armor( player_t* ) const override
  { return 0; }
};

struct earthquake_t : public shaman_spell_t
{
  earthquake_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( "earthquake", player, player -> find_specialization_spell( "Earthquake" ), options_str )
  {
    harmful = hasted_ticks = true;
    may_miss = may_crit = callbacks = false;

    base_td = base_dd_min = base_dd_max = 0;
    spell_power_mod.direct = 0;
    ignore_false_positive = true;

    tick_action = new earthquake_rumble_t( player );

    uses_eoe = player -> talent.echo_of_the_elements -> ok();
  }

  void init() override
  {
    shaman_spell_t::init();

    update_flags &= ~STATE_HASTE;
  }

  //timespan_t composite_dot_duration( const action_state_t* state ) const
  //{ return dot_duration * state -> haste; }

  void consume_resource() override
  {
    shaman_spell_t::consume_resource();

    p() -> buff.enhanced_chain_lightning -> expire();
  }

  timespan_t execute_time() const override
  {
    timespan_t et = shaman_spell_t::execute_time();

    if ( p() -> buff.enhanced_chain_lightning -> check() )
    {
      et *= 1.0 + p() -> buff.enhanced_chain_lightning -> data().effectN( 2 ).percent();
    }

    return et;
  }

  double composite_persistent_multiplier( const action_state_t* state) const override
  {
    double m = shaman_spell_t::composite_persistent_multiplier( state );

    m *= 1.0 + p() -> buff.enhanced_chain_lightning -> check() * p() -> buff.enhanced_chain_lightning -> data().effectN( 1 ).percent();

    return m;
  }
};

struct spiritwalkers_grace_t : public shaman_spell_t
{
  spiritwalkers_grace_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( "spiritwalkers_grace", player, player -> find_specialization_spell( "Spiritwalker's Grace" ), options_str )
  {
    may_miss = may_crit = harmful = callbacks = false;
  }

  virtual void execute() override
  {
    shaman_spell_t::execute();

    p() -> buff.spiritwalkers_grace -> trigger();

    if ( p() -> sets.has_set_bonus( SET_HEALER, T13, B4 ) )
      p() -> buff.tier13_4pc_healer -> trigger();
  }
};

struct spirit_walk_t : public shaman_spell_t
{
  spirit_walk_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( "spirit_walk", player, player -> find_specialization_spell( "Spirit Walk" ), options_str )
  {
    may_miss = may_crit = harmful = callbacks = false;
  }

  void execute() override
  {
    shaman_spell_t::execute();

    p() -> buff.spirit_walk -> trigger();
  }
};

struct doom_winds_t : public shaman_spell_t
{
  doom_winds_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( "doom_winds", player, &( player -> artifact.doom_winds.data() ), options_str )
  {
    harmful = callbacks = false;
  }

  void execute()
  {
    shaman_spell_t::execute();

    p() -> buff.doom_winds -> trigger();
  }

  bool ready()
  {
    if ( ! player -> artifact_enabled() )
    {
      return false;
    }

    if ( p() -> artifact.doom_winds.rank() == 0 )
    {
      return false;
    }

    return shaman_spell_t::ready();
  }
};

// ==========================================================================
// Shaman Shock Spells
// ==========================================================================

// Earth Shock Spell ========================================================

struct earth_shock_t : public shaman_spell_t
{
  double max_cost, base_coefficient;

  earth_shock_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( "earth_shock", player, player -> find_specialization_spell( "Earth Shock" ), options_str ),
    max_cost( data().powerN( POWER_MAELSTROM ).cost() + data().powerN( POWER_MAELSTROM ).max_cost() ),
    base_coefficient( data().effectN( 1 ).sp_coeff() / max_cost )
  {
    base_costs[ RESOURCE_MAELSTROM ] = 1;
    cooldown -> duration += player -> spec.spiritual_insight -> effectN( 3 ).time_value();
  }

  double spell_direct_power_coefficient( const action_state_t* ) const override
  { return base_coefficient * std::min( player -> resources.current[ RESOURCE_MAELSTROM ], max_cost ); }

  double cost() const override
  {
    return std::min( player -> resources.current[ RESOURCE_MAELSTROM ], max_cost );
  }
};

// Flame Shock Spell ========================================================

struct flame_shock_t : public shaman_spell_t
{
  double duration_multiplier;
  double max_cost;

  flame_shock_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( "flame_shock", player, player -> find_specialization_spell( "Flame Shock" ), options_str ),
    duration_multiplier( 1.0 ),
    max_cost( data().powerN( POWER_MAELSTROM ).max_cost() )
  {
    tick_may_crit         = true;
    cooldown -> duration  += player -> spec.spiritual_insight -> effectN( 3 ).time_value();
    track_cd_waste        = false;
    base_costs[ RESOURCE_MAELSTROM ] = 1;

    // Elemental Tier 18 (WoD 6.2) trinket effect is in use, adjust Flame Shock based on spell data
    // of the special effect.
    if ( player -> elemental_bellows )
    {
      const spell_data_t* data = player -> elemental_bellows -> driver();
      double damage_value = data -> effectN( 1 ).average( player -> elemental_bellows -> item ) / 100.0;
      double duration_value = data -> effectN( 3 ).average( player -> elemental_bellows -> item ) / 100.0;

      base_multiplier *= 1.0 + damage_value;
      duration_multiplier = 1.0 + duration_value;
    }
  }

  timespan_t composite_dot_duration( const action_state_t* ) const override
  {
    timespan_t duration = dot_duration;
    duration += timespan_t::from_seconds( std::min( max_cost, player -> resources.current[ RESOURCE_MAELSTROM ] ) );
    duration *= duration_multiplier;
    return duration;
  }

  double cost() const override
  {
    return std::min( max_cost, player -> resources.current[ RESOURCE_MAELSTROM ] );
  }

  void tick( dot_t* d ) override
  {
    shaman_spell_t::tick( d );

    if ( rng().roll( p() -> spec.lava_surge -> proc_chance() ) )
    {
      if ( p() -> buff.lava_surge -> check() )
        p() -> proc.wasted_lava_surge -> occur();

      p() -> proc.lava_surge -> occur();
      if ( ! p() -> executing || p() -> executing -> id != 51505 )
        p() -> cooldown.lava_burst -> reset( true );
      else
      {
        p() -> proc.surge_during_lvb -> occur();
        p() -> lava_surge_during_lvb = true;
      }

      p() -> buff.lava_surge -> trigger();
    }

    p() -> trigger_tier16_4pc_melee( d -> state );
  }
};

// Frost Shock Spell ========================================================

struct frost_shock_t : public shaman_spell_t
{
  frost_shock_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( "frost_shock", player, player -> find_specialization_spell( "Frost Shock" ), options_str )
  { }
};

// Wind Shear Spell =========================================================

struct wind_shear_t : public shaman_spell_t
{
  wind_shear_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( "wind_shear", player, player -> find_specialization_spell( "Wind Shear" ), options_str )
  {
    may_miss = may_crit = false;
    ignore_false_positive = true;
  }

  virtual bool ready() override
  {
    if ( ! target -> debuffs.casting -> check() ) return false;
    return shaman_spell_t::ready();
  }
};

// Ascendancy Spell =========================================================

struct ascendance_t : public shaman_spell_t
{
  cooldown_t* strike_cd;

  ascendance_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( "ascendance", player, player -> talent.ascendance, options_str )
  {
    harmful = false;

    strike_cd = p() -> cooldown.strike;
  }

  virtual void execute() override
  {
    shaman_spell_t::execute();
    strike_cd -> reset( false );

    p() -> buff.ascendance -> trigger();
  }
};

// Healing Surge Spell ======================================================

struct healing_surge_t : public shaman_heal_t
{
  healing_surge_t( shaman_t* player, const std::string& options_str ) :
    shaman_heal_t( player, player -> find_specialization_spell( "Healing Surge" ), options_str )
  {
    resurgence_gain = 0.6 * p() -> spell.resurgence -> effectN( 1 ).average( player ) * p() -> spec.resurgence -> effectN( 1 ).percent();
  }

  double composite_crit() const override
  {
    double c = shaman_heal_t::composite_crit();

    if ( p() -> buff.tidal_waves -> up() )
    {
      c += p() -> spec.tidal_waves -> effectN( 1 ).percent() +
           p() -> sets.set( SET_HEALER, T14, B4 ) -> effectN( 1 ).percent();
    }

    return c;
  }
};

// Healing Wave Spell =======================================================

struct healing_wave_t : public shaman_heal_t
{
  healing_wave_t( shaman_t* player, const std::string& options_str ) :
    shaman_heal_t( player, player -> find_specialization_spell( "Healing Wave" ), options_str )
  {
    resurgence_gain = p() -> spell.resurgence -> effectN( 1 ).average( player ) * p() -> spec.resurgence -> effectN( 1 ).percent();
  }

  timespan_t execute_time() const override
  {
    timespan_t c = shaman_heal_t::execute_time();

    if ( p() -> buff.tidal_waves -> up() )
    {
      c *= 1.0 - ( p() -> spec.tidal_waves -> effectN( 1 ).percent() +
                   p() -> sets.set( SET_HEALER, T14, B4 ) -> effectN( 1 ).percent() );
    }

    return c;
  }
};

// Greater Healing Wave Spell ===============================================

struct greater_healing_wave_t : public shaman_heal_t
{
  greater_healing_wave_t( shaman_t* player, const std::string& options_str ) :
    shaman_heal_t( player, player -> find_specialization_spell( "Greater Healing Wave" ), options_str )
  {
    resurgence_gain = p() -> spell.resurgence -> effectN( 1 ).average( player ) * p() -> spec.resurgence -> effectN( 1 ).percent();
  }

  timespan_t execute_time() const override
  {
    timespan_t c = shaman_heal_t::execute_time();

    if ( p() -> buff.tidal_waves -> up() )
    {
      c *= 1.0 - ( p() -> spec.tidal_waves -> effectN( 1 ).percent() +
                   p() -> sets.set( SET_HEALER, T14, B4 ) -> effectN( 1 ).percent() );
    }

    return c;
  }
};

// Riptide Spell ============================================================

struct riptide_t : public shaman_heal_t
{
  riptide_t( shaman_t* player, const std::string& options_str ) :
    shaman_heal_t( player, player -> find_specialization_spell( "Riptide" ), options_str )
  {
    resurgence_gain = 0.6 * p() -> spell.resurgence -> effectN( 1 ).average( player ) * p() -> spec.resurgence -> effectN( 1 ).percent();

    uses_eoe = player -> talent.echo_of_the_elements -> ok();
  }
};

// Chain Heal Spell =========================================================

struct chain_heal_t : public shaman_heal_t
{
  chain_heal_t( shaman_t* player, const std::string& options_str ) :
    shaman_heal_t( player, player -> find_specialization_spell( "Chain Heal" ), options_str )
  {
    resurgence_gain = 0.333 * p() -> spell.resurgence -> effectN( 1 ).average( player ) * p() -> spec.resurgence -> effectN( 1 ).percent();
  }

  double composite_target_da_multiplier( player_t* t) const override
  {
    double m = shaman_heal_t::composite_target_da_multiplier( t );

    if ( td( t ) -> heal.riptide -> is_ticking() )
      m *= 1.0 + p() -> spec.riptide -> effectN( 3 ).percent();

    return m;
  }
};

// Healing Rain Spell =======================================================

struct healing_rain_t : public shaman_heal_t
{
  struct healing_rain_aoe_tick_t : public shaman_heal_t
  {
    healing_rain_aoe_tick_t( shaman_t* player ) :
      shaman_heal_t( "healing_rain_tick", player, player -> find_spell( 73921 ) )
    {
      background = true;
      aoe = -1;
    }
  };

  healing_rain_t( shaman_t* player, const std::string& options_str ) :
    shaman_heal_t( player, player -> find_specialization_spell( "Healing Rain" ), options_str )
  {
    base_tick_time = data().effectN( 2 ).period();
    dot_duration = data().duration();
    hasted_ticks = false;
    tick_action = new healing_rain_aoe_tick_t( player );
  }
};

// ==========================================================================
// Shaman Totem System
// ==========================================================================

struct shaman_totem_pet_t : public pet_t
{
  // Pulse related functionality
  totem_pulse_action_t* pulse_action;
  event_t*         pulse_event;
  timespan_t            pulse_amplitude;

  // Summon related functionality
  std::string           pet_name;
  pet_t*                summon_pet;

  shaman_totem_pet_t( shaman_t* p, const std::string& n ) :
    pet_t( p -> sim, p, n, true ),
    pulse_action( nullptr ), pulse_event( nullptr ), pulse_amplitude( timespan_t::zero() ),
    summon_pet( nullptr )
  {
    regen_type = REGEN_DISABLED;
    affects_wod_legendary_ring = false;
  }

  virtual void summon( timespan_t = timespan_t::zero() ) override;
  virtual void dismiss( bool expired = false ) override;

  bool init_finished() override
  {
    if ( ! pet_name.empty() )
    {
      summon_pet = owner -> find_pet( pet_name );
    }

    return pet_t::init_finished();
  }

  shaman_t* o()
  { return debug_cast< shaman_t* >( owner ); }

  virtual double composite_player_multiplier( school_e school ) const override
  {
    double m = owner -> cache.player_multiplier( school );

    // Shaman offensive totems double-dip on the legendary AOE ring damage buff, but do not
    // contribute to explosion damage.
    if ( owner -> buffs.legendary_aoe_ring && owner -> buffs.legendary_aoe_ring -> up() )
    {
      m *= 1.0 + owner -> buffs.legendary_aoe_ring -> default_value;
    }

    return m;
  }

  virtual double composite_spell_hit() const override
  { return owner -> cache.spell_hit(); }

  virtual double composite_spell_crit() const override
  { return owner -> cache.spell_crit(); }

  virtual double composite_spell_power( school_e school ) const override
  { return owner -> cache.spell_power( school ); }

  virtual double composite_spell_power_multiplier() const override
  { return owner -> composite_spell_power_multiplier(); }

  virtual expr_t* create_expression( action_t* a, const std::string& name ) override
  {
    if ( util::str_compare_ci( name, "duration" ) )
      return make_ref_expr( name, duration );

    return pet_t::create_expression( a, name );
  }
};

struct shaman_totem_t : public shaman_spell_t
{
  shaman_totem_pet_t* totem_pet;
  timespan_t totem_duration;

  shaman_totem_t( const std::string& totem_name, shaman_t* player, const std::string& options_str, const spell_data_t* spell_data ) :
    shaman_spell_t( totem_name, player, spell_data, options_str ),
    totem_duration( data().duration() )
  {
    totem = true;
    harmful = callbacks = may_miss = may_crit =  false;
    ignore_false_positive = true;
    dot_duration = timespan_t::zero();
  }

  bool init_finished() override
  {
    totem_pet = debug_cast< shaman_totem_pet_t* >( player -> find_pet( name() ) );

    return shaman_spell_t::init_finished();
  }

  virtual void execute() override
  {
    shaman_spell_t::execute();
    totem_pet -> summon( totem_duration );
  }

  virtual expr_t* create_expression( const std::string& name ) override
  {
    // Redirect active/remains to "pet.<totem name>.active/remains" so things work ok with the
    // pet initialization order shenanigans. Otherwise, at this point in time (when
    // create_expression is called), the pets don't actually exist yet.
    if ( util::str_compare_ci( name, "active" ) )
      return player -> create_expression( this, "pet." + name_str + ".active" );
    else if ( util::str_compare_ci( name, "remains" ) )
      return player -> create_expression( this, "pet." + name_str + ".remains" );
    else if ( util::str_compare_ci( name, "duration" ) )
      return make_ref_expr( name, totem_duration );

    return shaman_spell_t::create_expression( name );
  }

  bool ready() override
  {
    if ( ! totem_pet )
    {
      return false;
    }

    return shaman_spell_t::ready();
  }
};

struct totem_pulse_action_t : public spell_t
{
  bool hasted_pulse;
  double pulse_multiplier;
  shaman_totem_pet_t* totem;

  totem_pulse_action_t( const std::string& token, shaman_totem_pet_t* p, const spell_data_t* s ) :
    spell_t( token, p, s ), hasted_pulse( false ), pulse_multiplier( 1.0 ), totem( p )
  {
    may_crit = harmful = background = true;
    callbacks = false;

    crit_bonus_multiplier *= 1.0 + totem -> o() -> spec.elemental_fury -> effectN( 1 ).percent();
  }

  double action_multiplier() const override
  {
    double m = spell_t::action_multiplier();

    m *= pulse_multiplier;

    return m;
  }

  void init() override
  {
    spell_t::init();

    // Hacky, but constructor wont work.
    crit_multiplier *= util::crit_multiplier( totem -> o() -> meta_gem );
  }

  void reset() override
  {
    spell_t::reset();
    pulse_multiplier = 1.0;
  }
};

struct totem_pulse_event_t : public event_t
{
  shaman_totem_pet_t* totem;
  timespan_t real_amplitude;

  totem_pulse_event_t( shaman_totem_pet_t& t, timespan_t amplitude ) :
    event_t( t ),
    totem( &t ), real_amplitude( amplitude )
  {
    if ( totem -> pulse_action -> hasted_pulse )
      real_amplitude *= totem -> cache.spell_speed();

    add_event( real_amplitude );
  }
  virtual const char* name() const override
  { return  "totem_pulse"; }
  virtual void execute() override
  {
    if ( totem -> pulse_action )
      totem -> pulse_action -> execute();

    totem -> pulse_event = new ( sim() ) totem_pulse_event_t( *totem, totem -> pulse_amplitude );
  }
};

void shaman_totem_pet_t::summon( timespan_t duration )
{
  pet_t::summon( duration );

  if ( pulse_action )
  {
    pulse_action -> pulse_multiplier = 1.0;
    pulse_event = new ( *sim ) totem_pulse_event_t( *this, pulse_amplitude );
  }

  if ( summon_pet )
    summon_pet -> summon();
}

void shaman_totem_pet_t::dismiss( bool expired )
{
  // Disable last (partial) tick on dismiss, as it seems not to happen in game atm
  if ( pulse_action && pulse_event && expiration && expiration -> remains() == timespan_t::zero() )
  {
    if ( pulse_event -> remains() > timespan_t::zero() )
      pulse_action -> pulse_multiplier = pulse_event -> remains() / debug_cast<totem_pulse_event_t*>( pulse_event ) -> real_amplitude;
    pulse_action -> execute();
  }

  event_t::cancel( pulse_event );

  if ( summon_pet )
    summon_pet -> dismiss();

  pet_t::dismiss( expired );
}

// Earthquake totem =========================================================

struct earthquake_totem_pulse_t : public totem_pulse_action_t
{
  earthquake_totem_pulse_t( shaman_totem_pet_t* totem ) :
    totem_pulse_action_t( "earthquake", totem, totem -> find_spell( 77478 ) )
  {
    aoe = -1;
    background = ground_aoe = true;
    school = SCHOOL_PHYSICAL;
    spell_power_mod.direct = 0.11; // Hardcoded into tooltip because it's cool
    hasted_pulse = true;
  }

  double target_armor( player_t* ) const override
  { return 0; }
};

struct earthquake_totem_t : public shaman_totem_pet_t
{
  earthquake_totem_t( shaman_t* owner ):
    shaman_totem_pet_t( owner, "earthquake_totem" )
  {
    pulse_amplitude = owner -> find_spell( 61882 ) -> effectN( 2 ).period();
  }

  void init_spells() override
  {
    shaman_totem_pet_t::init_spells();

    pulse_action = new earthquake_totem_pulse_t( this );
  }
};

// Liquid Magma Spell =======================================================

struct liquid_magma_t: public shaman_spell_t
{
  struct liquid_magma_aoe_t : public shaman_spell_t
  {
    liquid_magma_aoe_t( shaman_t* player ) :
      shaman_spell_t( "liquid_magma_aoe", player, player -> find_spell( 177601 ) )
    {
      background = true;
      range = 0;
      radius = 40; // in tooltip
      aoe = -1;
    }
  };

  liquid_magma_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( "liquid_magma", player, player -> find_talent_spell( "Liquid Magma" ), options_str )
  {
    tick_zero = true;
    hasted_ticks = true;
    tick_action = new liquid_magma_aoe_t( player );
  }

  void tick( dot_t* d ) override
  {
    // Liquid Magma never has partial ticks
    d -> last_tick_factor = 1;

    shaman_spell_t::tick( d );
  }

  void execute() override
  {
    shaman_spell_t::execute();

    p() -> buff.liquid_magma -> trigger();
  }

  bool ready() override
  {
    if ( ! p() -> totems[ TOTEM_FIRE ] )
      return false;

    return shaman_spell_t::ready();
  }
};

// ==========================================================================
// Shaman Custom Buff implementation
// ==========================================================================

void ascendance_buff_t::ascendance( attack_t* mh, attack_t* oh, timespan_t lvb_cooldown )
{
  // Presume that ascendance trigger and expiration will not reset the swing
  // timer, so we need to cancel and reschedule autoattack with the
  // remaining swing time of main/off hands
  if ( player -> specialization() == SHAMAN_ENHANCEMENT )
  {
    bool executing = false;
    timespan_t time_to_hit = timespan_t::zero();
    if ( player -> main_hand_attack && player -> main_hand_attack -> execute_event )
    {
      executing = true;
      time_to_hit = player -> main_hand_attack -> execute_event -> remains();
#ifndef NDEBUG
      if ( time_to_hit < timespan_t::zero() )
      {
        util::fprintf( stderr, "Ascendance %s time_to_hit=%f", player -> main_hand_attack -> name(), time_to_hit.total_seconds() );
        assert( 0 );
      }
#endif
      event_t::cancel( player -> main_hand_attack -> execute_event );
    }

    player -> main_hand_attack = mh;
    if ( executing )
    {
      // Kick off the new main hand attack, by instantly scheduling
      // and rescheduling it to the remaining time to hit. We cannot use
      // normal reschedule mechanism here (i.e., simply use
      // event_t::reschedule() and leave it be), because the rescheduled
      // event would be triggered before the full swing time (of the new
      // auto attack) in most cases.
      player -> main_hand_attack -> base_execute_time = timespan_t::zero();
      player -> main_hand_attack -> schedule_execute();
      player -> main_hand_attack -> base_execute_time = player -> main_hand_attack -> weapon -> swing_time;
      player -> main_hand_attack -> execute_event -> reschedule( time_to_hit );
    }

    if ( player -> off_hand_attack )
    {
      time_to_hit = timespan_t::zero();
      executing = false;

      if ( player -> off_hand_attack -> execute_event )
      {
        executing = true;
        time_to_hit = player -> off_hand_attack -> execute_event -> remains();
#ifndef NDEBUG
        if ( time_to_hit < timespan_t::zero() )
        {
          util::fprintf( stderr, "Ascendance %s time_to_hit=%f", player -> off_hand_attack -> name(), time_to_hit.total_seconds() );
          assert( 0 );
        }
#endif
        event_t::cancel( player -> off_hand_attack -> execute_event );
      }

      player -> off_hand_attack = oh;
      if ( executing )
      {
        // Kick off the new off hand attack, by instantly scheduling
        // and rescheduling it to the remaining time to hit. We cannot use
        // normal reschedule mechanism here (i.e., simply use
        // event_t::reschedule() and leave it be), because the rescheduled
        // event would be triggered before the full swing time (of the new
        // auto attack) in most cases.
        player -> off_hand_attack -> base_execute_time = timespan_t::zero();
        player -> off_hand_attack -> schedule_execute();
        player -> off_hand_attack -> base_execute_time = player -> off_hand_attack -> weapon -> swing_time;
        player -> off_hand_attack -> execute_event -> reschedule( time_to_hit );
      }
    }
  }
  // Elemental simply changes the Lava Burst cooldown, Lava Beam replacement
  // will be handled by action list and ready() in Chain Lightning / Lava
  // Beam
  else if ( player -> specialization() == SHAMAN_ELEMENTAL )
  {
    if ( lava_burst )
    {
      lava_burst -> cooldown -> duration = lvb_cooldown;
      lava_burst -> cooldown -> reset( false );
    }
  }
}

inline bool ascendance_buff_t::trigger( int stacks, double value, double chance, timespan_t duration )
{
  shaman_t* p = debug_cast< shaman_t* >( player );

  if ( player -> specialization() == SHAMAN_ELEMENTAL && ! lava_burst )
  {
    lava_burst = player -> find_action( "lava_burst" );
  }

  ascendance( p -> ascendance_mh, p -> ascendance_oh, timespan_t::zero() );
  // Don't record CD waste during Ascendance.
  if ( lava_burst )
  {
    lava_burst -> cooldown -> last_charged = timespan_t::zero();
  }

  return buff_t::trigger( stacks, value, chance, duration );
}

inline void ascendance_buff_t::expire_override( int expiration_stacks, timespan_t remaining_duration )
{
  shaman_t* p = debug_cast< shaman_t* >( player );

  timespan_t lvbcd;
  lvbcd = lava_burst ? lava_burst -> data().charge_cooldown() : timespan_t::zero();

  ascendance( p -> melee_mh, p -> melee_oh, lvbcd );
  // Start CD waste recollection from when Ascendance buff fades, since Lava
  // Burst is guaranteed to be very much ready when Ascendance ends.
  if ( lava_burst )
  {
    lava_burst -> cooldown -> last_charged = sim -> current_time();
  }
  buff_t::expire_override( expiration_stacks, remaining_duration );
}

// ==========================================================================
// Shaman Character Definition
// ==========================================================================

// shaman_t::create_options =================================================

void shaman_t::create_options()
{
  player_t::create_options();

  add_option( opt_timespan( "uf_expiration_delay",        uf_expiration_delay        ) );
  add_option( opt_timespan( "uf_expiration_delay_stddev", uf_expiration_delay_stddev ) );
  add_option( opt_float( "mental_quickness", mental_quickness ) );
}

// shaman_t::create_action  =================================================

action_t* shaman_t::create_action( const std::string& name,
                                   const std::string& options_str )
{
  if ( name == "ascendance"              ) return new               ascendance_t( this, options_str );
  if ( name == "auto_attack"             ) return new              auto_attack_t( this, options_str );
  if ( name == "bloodlust"               ) return new                bloodlust_t( this, options_str );
  if ( name == "chain_lightning"         ) return new          chain_lightning_t( this, options_str );
  if ( name == "crash_lightning"         ) return new          crash_lightning_t( this, options_str );
  if ( name == "doom_winds"              ) return new               doom_winds_t( this, options_str );
  if ( name == "earthen_spike"           ) return new            earthen_spike_t( this, options_str );
  if ( name == "earth_shock"             ) return new              earth_shock_t( this, options_str );
  if ( name == "elemental_blast"         ) return new          elemental_blast_t( this, options_str );
  if ( name == "fire_elemental"          ) return new           fire_elemental_t( this, options_str );
  if ( name == "fire_nova"               ) return new                fire_nova_t( this, options_str );
  if ( name == "flametongue"             ) return new              flametongue_t( this, options_str );
  if ( name == "flame_shock"             ) return new              flame_shock_t( this, options_str );
  if ( name == "frost_shock"             ) return new              frost_shock_t( this, options_str );
  if ( name == "fury_of_air"             ) return new              fury_of_air_t( this, options_str );
  if ( name == "lava_beam"               ) return new                lava_beam_t( this, options_str );
  if ( name == "lava_burst"              ) return new               lava_burst_t( this, options_str );
  if ( name == "lava_lash"               ) return new                lava_lash_t( this, options_str );
  if ( name == "lightning_bolt"          ) return new           lightning_bolt_t( this, options_str );
  if ( name == "lightning_shield"        ) return new         lightning_shield_t( this, options_str );
  if ( name == "shamanistic_rage"        ) return new         shamanistic_rage_t( this, options_str );
  if ( name == "liquid_magma"            ) return new             liquid_magma_t( this, options_str );
  if ( name == "windstrike"              ) return new               windstrike_t( this, options_str );
  if ( name == "feral_kin"               ) return new                feral_kin_t( this, options_str );
  if ( name == "feral_spirit"            ) return new       feral_spirit_spell_t( this, options_str );
  if ( name == "fists_of_stone"          ) return new           fists_of_stone_t( this, options_str );
  if ( name == "rockbiter"               ) return new                rockbiter_t( this, options_str );
  if ( name == "spirit_walk"             ) return new              spirit_walk_t( this, options_str );
  if ( name == "spiritwalkers_grace"     ) return new      spiritwalkers_grace_t( this, options_str );
  if ( name == "stonefist_strike"        ) return new         stonefist_strike_t( this, options_str );
  if ( name == "stormstrike"             ) return new              stormstrike_t( this, options_str );
  if ( name == "sundering"               ) return new                sundering_t( this, options_str );
  if ( name == "thunderstorm"            ) return new             thunderstorm_t( this, options_str );
  if ( name == "wind_shear"              ) return new               wind_shear_t( this, options_str );
  if ( name == "windsong"                ) return new                 windsong_t( this, options_str );

  if ( name == "chain_heal"              ) return new               chain_heal_t( this, options_str );
  if ( name == "greater_healing_wave"    ) return new     greater_healing_wave_t( this, options_str );
  if ( name == "healing_rain"            ) return new             healing_rain_t( this, options_str );
  if ( name == "healing_surge"           ) return new            healing_surge_t( this, options_str );
  if ( name == "healing_wave"            ) return new             healing_wave_t( this, options_str );
  if ( name == "riptide"                 ) return new                  riptide_t( this, options_str );

  if ( name == "earthquake_totem" )
  {
    return new  shaman_totem_t( "earthquake_totem", this, options_str, find_specialization_spell( "Earthquake Totem" ) );
  }

  return player_t::create_action( name, options_str );
}

action_t* shaman_t::create_proc_action( const std::string& name, const special_effect_t& )
{
  if ( util::str_compare_ci( name, "flurry_of_xuen" ) ) return new shaman_flurry_of_xuen_t( this );

  return nullptr;
};

// shaman_t::create_pet =====================================================

pet_t* shaman_t::create_pet( const std::string& pet_name,
                             const std::string& /* pet_type */ )
{
  pet_t* p = find_pet( pet_name );

  if ( p ) return p;

  if ( pet_name == "fire_elemental_pet"       ) return new pet::fire_elemental_t( this, false );
  if ( pet_name == "fire_elemental_guardian"  ) return new pet::fire_elemental_t( this, true );
  if ( pet_name == "storm_elemental_pet"      ) return new pet::storm_elemental_t( this, false );
  if ( pet_name == "storm_elemental_guardian" ) return new pet::storm_elemental_t( this, true );
  if ( pet_name == "earth_elemental_pet"      ) return new pet::earth_elemental_t( this, false );
  if ( pet_name == "earth_elemental_guardian" ) return new pet::earth_elemental_t( this, true );
  if ( pet_name == "earthquake_totem"         ) return new earthquake_totem_t( this );

  return nullptr;
}

// shaman_t::create_pets ====================================================

void shaman_t::create_pets()
{
  if ( talent.primal_elementalist -> ok() )
  {
    if ( find_action( "fire_elemental" )  )
    {
      pet_fire_elemental = create_pet( "fire_elemental_pet" );
    }

    if ( find_action( "earth_elemental" ) )
    {
      pet_earth_elemental = create_pet( "earth_elemental_pet" );
    }

    if ( talent.storm_elemental -> ok() && find_action( "storm_elemental" ) )
    {
      pet_storm_elemental = create_pet( "storm_elemental_pet" );
    }
  }
  else
  {
    if ( find_action( "fire_elemental" ) )
    {
      guardian_fire_elemental = create_pet( "fire_elemental_guardian" );
    }

    if ( find_action( "earth_elemental" ) )
    {
      guardian_earth_elemental = create_pet( "earth_elemental_guardian" );
    }

    if ( talent.storm_elemental -> ok() && find_action( "storm_elemental" ) )
    {
      guardian_storm_elemental = create_pet( "storm_elemental_guardian" );
    }
  }

  if ( find_action( "earthquake_totem" ) )
  {
    create_pet( "earthquake_totem" );
  }

  if ( sets.has_set_bonus( SET_CASTER, T16, B4 ) )
  {
    guardian_lightning_elemental = new pet::lightning_elemental_t( this );
  }

  if ( specialization() == SHAMAN_ENHANCEMENT )
  {
    const spell_data_t* fs_data = find_specialization_spell( "Feral Spirit" );
    size_t n_feral_spirits = static_cast<size_t>( fs_data -> effectN( 1 ).base_value() );
    // Add two extra potential pets for the T15 4pc set bonus
    if ( sets.has_set_bonus( SET_MELEE, T15, B4 ) )
    {
      n_feral_spirits += 2;
    }

    for ( size_t i = 0; i < n_feral_spirits; i++ )
    {
      pet_feral_spirit.push_back( new pet::feral_spirit_pet_t( this ) );
    }
  }
}

// shaman_t::create_expression ==============================================

expr_t* shaman_t::create_expression( action_t* a, const std::string& name )
{
  std::vector<std::string> splits = util::string_split( name, "." );

  // totem.<kind>.<op>
  if ( splits.size() == 3 && util::str_compare_ci( splits[ 0 ], "totem" ) )
  {
    shaman_totem_pet_t* totem = nullptr;

    totem = static_cast< shaman_totem_pet_t* >( find_pet( splits[ 1 ] ) );

    // Nothing found
    if ( totem == nullptr )
      return player_t::create_expression( a, name );
    // A specific totem name given, and found
    else
      return totem -> create_expression( a, splits[ 2 ] );
  }

  return player_t::create_expression( a, name );
}

// shaman_t::init_spells ====================================================

void shaman_t::init_spells()
{
  // Generic
  spec.mail_specialization   = find_specialization_spell( "Mail Specialization" );
  constant.matching_gear_multiplier = spec.mail_specialization -> effectN( 1 ).percent();

  // Elemental / Restoration
  spec.spiritual_insight     = find_specialization_spell( "Spiritual Insight" );

  // Elemental
  spec.fulmination           = find_specialization_spell( "Fulmination" );
  spec.lava_surge            = find_specialization_spell( "Lava Surge" );
  spec.elemental_fury        = find_specialization_spell( "Elemental Fury" );
  spec.shamanism             = find_specialization_spell( "Shamanism" );

  // Enhancement
  spec.critical_strikes      = find_specialization_spell( "Critical Strikes" );
  spec.dual_wield            = find_specialization_spell( "Dual Wield" );
  spec.flurry                = find_specialization_spell( "Flurry" );
  spec.maelstrom_weapon      = find_specialization_spell( "Maelstrom Weapon" );
  spec.stormfury             = find_specialization_spell( "Stormfury" );
  spec.stormlash             = find_specialization_spell( "Stormlash" );
  spec.windfury              = find_specialization_spell( "Windfury" );

  // Restoration
  spec.ancestral_awakening   = find_specialization_spell( "Ancestral Awakening" );
  spec.ancestral_focus       = find_specialization_spell( "Ancestral Focus" );
  spec.earth_shield          = find_specialization_spell( "Earth Shield" );
  spec.meditation            = find_specialization_spell( "Meditation" );
  spec.purification          = find_specialization_spell( "Purification" );
  spec.resurgence            = find_specialization_spell( "Resurgence" );
  spec.riptide               = find_specialization_spell( "Riptide" );
  spec.tidal_waves           = find_specialization_spell( "Tidal Waves" );

  // Masteries
  mastery.elemental_overload         = find_mastery_spell( SHAMAN_ELEMENTAL   );
  mastery.enhanced_elements          = find_mastery_spell( SHAMAN_ENHANCEMENT );
  mastery.deep_healing               = find_mastery_spell( SHAMAN_RESTORATION );

  // Talents
  talent.ancestral_swiftness         = find_talent_spell( "Ancestral Swiftness"  );
  talent.gust_of_wind                = find_talent_spell( "Gust of Wind"         );
  talent.ascendance                  = find_talent_spell( "Ascendance"           );

  // Elemental
  talent.path_of_flame               = find_talent_spell( "Path of Flame"        );
  talent.path_of_elements            = find_talent_spell( "Path of Elements"     );
  talent.maelstrom_totem             = find_talent_spell( "Maelstrom Totem"      );

  talent.elemental_blast             = find_talent_spell( "Elemental Blast"      );
  talent.echo_of_the_elements        = find_talent_spell( "Echo of the Elements" );

  talent.elemental_fusion            = find_talent_spell( "Elemental Fusion"     );
  talent.sons_of_flame               = find_talent_spell( "Sons of Flame"        );
  talent.magnitude                   = find_talent_spell( "Magnitude"            );

  talent.lightning_rod               = find_talent_spell( "Lightning Rod"        );
  talent.storm_elemental             = find_talent_spell( "Storm Elemental"      );
  talent.liquid_magma_totem          = find_talent_spell( "Liquid Magma Totem"   );

  talent.primal_elementalist         = find_talent_spell( "Primal Elementalist"  );
  talent.totemic_fury                = find_talent_spell( "Totemic Fury"         );

  // Enhancement
  talent.windsong                    = find_talent_spell( "Windsong"             );
  talent.spiritual_resonance         = find_talent_spell( "Spiritual Resonance"  );
  talent.fists_of_stone              = find_talent_spell( "Fists of Stone"       );

  talent.feral_lunge                 = find_talent_spell( "Feral Lunge"          );

  talent.lightning_shield            = find_talent_spell( "Lightning Shield"     );
  talent.landslide                   = find_talent_spell( "Landslide"            );

  talent.tempest                     = find_talent_spell( "Tempest"              );
  talent.spiritual_affinity          = find_talent_spell( "Spiritual Affinity"   );
  talent.sundering                   = find_talent_spell( "Sundering"            );

  talent.fury_of_air                 = find_talent_spell( "Fury of Air"          );
  talent.crashing_storm              = find_talent_spell( "Crashing Storm"       );
  talent.stonefist_strike            = find_talent_spell( "Stonefist Strike"     );

  talent.feral_kin                   = find_talent_spell( "Feral Kin"            );
  talent.earthen_spike               = find_talent_spell( "Earthen Spike"        );

  // Artifact

  // Enhancement
  artifact.doom_winds                = find_artifact_spell( "Doom Winds"         );
  artifact.unleash_doom              = find_artifact_spell( "Unleash Doom"       );
  artifact.hurricane                 = find_artifact_spell( "Hurricane"          );
  artifact.stormflurry               = find_artifact_spell( "Stormflurry"        );
  artifact.hammer_of_storms          = find_artifact_spell( "Hammer of Storms"   );
  artifact.forged_in_lava            = find_artifact_spell( "Forged in Lava"     );
  artifact.surge_of_elements         = find_artifact_spell( "Surge of the Elements" );
  artifact.wind_strikes              = find_artifact_spell( "Wind Strikes"       );
  artifact.gathering_storms          = find_artifact_spell( "Gathering Storms"   );
  artifact.gathering_of_the_maelstrom= find_artifact_spell( "Gathering of the Maelstrom" );
  artifact.doom_vortex               = find_artifact_spell( "Doom Vortex"        );

  // Misc spells
  spell.resurgence                   = find_spell( 101033 );
  spell.flame_shock                  = find_class_spell( "Flame Shock" );
  spell.lightning_strike             = find_spell( 168557 );
  spell.eruption                     = find_spell( 168556 );
  spell.maelstrom_melee_gain         = find_spell( 187890 );
  if ( specialization() == SHAMAN_ELEMENTAL )
  {
    spell.echo_of_the_elements       = find_spell( 159101 );
  }
  else if ( specialization() == SHAMAN_ENHANCEMENT )
  {
    spell.echo_of_the_elements       = find_spell( 159103 );
  }
  else if ( specialization() == SHAMAN_RESTORATION )
  {
    spell.echo_of_the_elements       = find_spell( 159105 );
  }
  else
  {
    spell.echo_of_the_elements       = spell_data_t::not_found();
  }

  if ( artifact.unleash_doom.rank() )
  {
    unleash_doom[ 0 ] = new unleash_doom_spell_t( "unleash_lava", this, find_spell( 199053 ) );
    unleash_doom[ 1 ] = new unleash_doom_spell_t( "unleash_lightning", this, find_spell( 199054 ) );
  }

  if ( artifact.doom_vortex.rank() )
  {
    doom_vortex = new doom_vortex_t( this );
  }

  // Constants
  constant.speed_attack_ancestral_swiftness = 1.0 / ( 1.0 + talent.ancestral_swiftness -> effectN( 2 ).percent() );
  constant.haste_ancestral_swiftness  = 1.0 / ( 1.0 + talent.ancestral_swiftness -> effectN( 1 ).percent() );

  player_t::init_spells();
}

// shaman_t::init_base ======================================================

void shaman_t::init_base_stats()
{
  player_t::init_base_stats();

  base.attack_power_per_strength = 0.0;
  base.attack_power_per_agility  = 1.0;
  base.spell_power_per_intellect = 1.0;

  resources.initial_multiplier[ RESOURCE_MANA ] = 1.0 + spec.spiritual_insight -> effectN( 1 ).percent();
  if ( specialization() == SHAMAN_ELEMENTAL || specialization() == SHAMAN_ENHANCEMENT )
    resources.base[ RESOURCE_MAELSTROM ] = 100;

  base.distance = ( specialization() == SHAMAN_ENHANCEMENT ) ? 3 : 30;
  base.mana_regen_from_spirit_multiplier = spec.meditation -> effectN( 1 ).percent();

  //if ( specialization() == SHAMAN_ENHANCEMENT )
  //  ready_type = READY_TRIGGER;
}

void shaman_t::init_resources( bool force )
{
  player_t::init_resources( force );

  resources.current[ RESOURCE_MAELSTROM ] = 0;
}

// shaman_t::init_scaling ===================================================

void shaman_t::init_scaling()
{
  player_t::init_scaling();

  switch ( specialization() )
  {
    case SHAMAN_ENHANCEMENT:
      scales_with[ STAT_STRENGTH              ] = false;
      scales_with[ STAT_WEAPON_OFFHAND_DPS    ] = true;
      scales_with[ STAT_SPELL_POWER           ] = false;
      scales_with[ STAT_INTELLECT             ] = false;
      break;
    case SHAMAN_RESTORATION:
      scales_with[ STAT_MASTERY_RATING ] = false;
      break;
    default:
      break;
  }
}

// ==========================================================================
// Shaman Ability Triggers
// ==========================================================================

void shaman_t::trigger_stormfury( const action_state_t* state )
{
  assert( debug_cast< shaman_attack_t* >( state -> action ) != nullptr && "Stormfury called on invalid action type" );
  //shaman_attack_t* attack = debug_cast< shaman_attack_t* >( state -> action );

  if ( ! spec.stormfury -> ok() )
  {
    return;
  }

  double proc_chance = spec.stormfury -> proc_chance();
  proc_chance += cache.mastery() * mastery.enhanced_elements -> effectN( 3 ).mastery_value();

  if ( rng().roll( proc_chance ) )
  {
    buff.stormfury -> trigger( buff.stormfury -> max_stack() );
    cooldown.strike -> reset( true );
    buff.wind_strikes -> trigger();
  }
}

void shaman_t::trigger_unleash_doom( const action_state_t* state )
{
  if ( ! state -> action -> special )
  {
    return;
  }

  if ( state -> action -> background )
  {
    return;
  }

  if ( ! buff.unleash_doom -> up() )
  {
    return;
  }

  if ( ! state -> action -> callbacks )
  {
    return;
  }

  size_t spell_idx = rng().range( 0, unleash_doom.size() );
  unleash_doom[ spell_idx ] -> target = state -> target;
  unleash_doom[ spell_idx ] -> schedule_execute();
}

void shaman_t::trigger_windfury_weapon( const action_state_t* state )
{
  assert( debug_cast< shaman_attack_t* >( state -> action ) != nullptr && "Windfury Weapon called on invalid action type" );
  shaman_attack_t* attack = debug_cast< shaman_attack_t* >( state -> action );
  if ( ! attack -> may_proc_windfury )
    return;

  if ( ! attack -> weapon )
    return;

  if ( attack -> weapon -> slot != SLOT_MAIN_HAND && ! buff.doom_winds -> check() )
    return;

  double proc_chance = spec.windfury -> proc_chance();
  proc_chance += cache.mastery() * mastery.enhanced_elements -> effectN( 4 ).mastery_value();
  if ( buff.doom_winds -> up() )
  {
    proc_chance = 1.0;
  }

  if ( rng().roll( proc_chance ) )
  {
    cooldown.feral_spirits -> ready -= timespan_t::from_seconds( sets.set( SET_MELEE, T15, B4 ) -> effectN( 1 ).base_value() );

    windfury -> target = state -> target;
    windfury -> schedule_execute();
    windfury -> schedule_execute();
    windfury -> schedule_execute();
    if ( sets.has_set_bonus( SHAMAN_ENHANCEMENT, PVP, B4 ) )
    {
      windfury -> schedule_execute();
      windfury -> schedule_execute();
    }
  }
}

void shaman_t::trigger_tier16_2pc_melee( const action_state_t* state )
{
  if ( ! state -> action -> callbacks )
    return;

  if ( cooldown.t16_2pc_melee -> down() )
    return;

  proc.t16_2pc_melee -> occur();

  switch ( static_cast< int >( rng().range( 0, bugs ? 4 : 2 ) ) )
  {
    // Windfury
    case 0:
      break;
    // Flametongue
    case 1:
    case 2:
    case 3:
      break;
    default:
      assert( false );
      break;
  }
}

void shaman_t::trigger_tier15_2pc_caster( const action_state_t* s )
{
  if ( ! sets.has_set_bonus( SET_CASTER, T15, B2 ) )
    return;

  if ( ! s -> action -> result_is_hit( s -> result ) )
    return;

  if ( rng().roll( sets.set( SET_CASTER, T15, B2 ) -> proc_chance() ) )
  {
    action_lightning_strike -> target = s -> target;
    action_lightning_strike -> schedule_execute();
  }
}

void shaman_t::trigger_tier16_4pc_melee( const action_state_t* )
{
  if ( ! sets.has_set_bonus( SET_MELEE, T16, B4 ) )
    return;

  if ( cooldown.t16_4pc_melee -> down() )
    return;

  if ( ! rng().roll( sets.set( SET_MELEE, T16, B4 ) -> proc_chance() ) )
    return;

  cooldown.t16_4pc_melee -> start( sets.set( SET_MELEE, T16, B4 ) -> internal_cooldown() );

  proc.t16_4pc_melee -> occur();
  cooldown.lava_lash -> reset( true );
}

void shaman_t::trigger_tier16_4pc_caster( const action_state_t* )
{
  if ( ! sets.has_set_bonus( SET_CASTER, T16, B4 ) )
    return;

  if ( cooldown.t16_4pc_caster -> down() )
    return;

  if ( ! rng().roll( sets.set( SET_CASTER, T16, B4 ) -> proc_chance() ) )
    return;

  cooldown.t16_4pc_caster -> start( sets.set( SET_CASTER, T16, B4 ) -> internal_cooldown() );

  guardian_lightning_elemental -> summon( sets.set( SET_CASTER, T16, B4 ) -> effectN( 1 ).trigger() -> duration() );
  proc.t16_4pc_caster -> occur();
}

void shaman_t::trigger_tier17_2pc_elemental( int stacks )
{
  if ( ! sets.has_set_bonus( SHAMAN_ELEMENTAL, T17, B2 ) )
    return;

  buff.focus_of_the_elements -> trigger( 1, sets.set( SHAMAN_ELEMENTAL, T17, B2 ) -> effectN( 1 ).percent() * stacks );
}

void shaman_t::trigger_tier17_4pc_elemental( int stacks )
{
  if ( ! sets.has_set_bonus( SHAMAN_ELEMENTAL, T17, B4 ) )
    return;

  if ( stacks < sets.set( SHAMAN_ELEMENTAL, T17, B4 ) -> effectN( 1 ).base_value() )
    return;

  if ( buff.lava_surge -> check() )
    proc.wasted_lava_surge -> occur();

  proc.lava_surge -> occur();
  buff.lava_surge -> trigger();

  cooldown.lava_burst -> reset( false );
}

void shaman_t::trigger_tier18_4pc_elemental( int ls_stack )
{
  if ( ! sets.has_set_bonus( SHAMAN_ELEMENTAL, T18, B4 ) )
  {
    return;
  }

  int gathering_vortex_stacks = buff.gathering_vortex -> check();
  gathering_vortex_stacks += ls_stack;
  // Stacks have to go past max stack to trigger Lightning Vortex
  if ( gathering_vortex_stacks <= buff.gathering_vortex -> max_stack() )
  {
    buff.gathering_vortex -> trigger( ls_stack );
  }
  else
  {
    int remainder_stack = gathering_vortex_stacks % buff.gathering_vortex -> max_stack();
    buff.gathering_vortex -> expire();
    // Back to back 20 stack Lightning Shield fulminations will just set Gathering Vortex up so it's
    // again at 20 stacks, presumably.
    buff.gathering_vortex -> trigger( remainder_stack ? remainder_stack : buff.gathering_vortex -> max_stack() );
    buff.t18_4pc_elemental -> trigger();
  }
}

void shaman_t::trigger_flametongue_weapon( const action_state_t* state )
{
  assert( debug_cast< shaman_attack_t* >( state -> action ) != nullptr && "Flametongue Weapon called on invalid action type" );
  shaman_attack_t* attack = debug_cast< shaman_attack_t* >( state -> action );
  if ( ! attack -> may_proc_flametongue )
    return;

  if ( ! attack -> weapon )
    return;

  if ( ! buff.flametongue -> up() )
    return;

  flametongue -> target = state -> target;
  flametongue -> schedule_execute();
}

void shaman_t::trigger_improved_lava_lash( const action_state_t* state )
{
  // Do not spread the love when there is only poor Fluffy Pillow against you
  if ( sim -> enemy_targets == 1 )
    return;

  if ( ! state -> action -> result_is_hit( state -> result ) )
    return;

  if ( ! get_target_data( state -> target ) -> dot.flame_shock -> is_ticking() )
    return;

  // Splash from the action's target
  action_improved_lava_lash -> target = state -> target;
  action_improved_lava_lash -> schedule_execute();
}

// shaman_t::init_buffs =====================================================

void shaman_t::create_buffs()
{
  player_t::create_buffs();

  buff.ascendance              = new ascendance_buff_t( this );
  buff.echo_of_the_elements    = buff_creator_t( this, "echo_of_the_elements", talent.echo_of_the_elements )
                                 .chance( talent.echo_of_the_elements -> ok() );
  buff.lava_surge              = buff_creator_t( this, "lava_surge",        spec.lava_surge )
                                 .activated( false )
                                 .chance( 1.0 ); // Proc chance is handled externally
  buff.lightning_shield        = buff_creator_t( this, "lightning_shield", find_talent_spell( "Lightning Shield" ) )
                                 .period( timespan_t::zero() ) // Model ticks as a dot
                                 .cd( timespan_t::zero() );
  buff.shamanistic_rage        = buff_creator_t( this, "shamanistic_rage", find_specialization_spell( "Shamanistic Rage" ) );
  buff.spirit_walk             = buff_creator_t( this, "spirit_walk", find_specialization_spell( "Spirit Walk" ) );
  buff.spiritwalkers_grace     = buff_creator_t( this, "spiritwalkers_grace", find_specialization_spell( "Spiritwalker's Grace" ) )
                                 .chance( 1.0 )
                                 .duration( find_specialization_spell( "Spiritwalker's Grace" ) -> duration() +
                                            sets.set( SET_HEALER, T13, B4 ) -> effectN( 1 ).time_value() );
  buff.tidal_waves             = buff_creator_t( this, "tidal_waves", spec.tidal_waves -> ok() ? find_spell( 53390 ) : spell_data_t::not_found() );

  buff.tier13_4pc_healer       = haste_buff_creator_t( this, "tier13_4pc_healer", find_spell( 105877 ) ).add_invalidate( CACHE_HASTE );

  // Stat buffs
  buff.elemental_blast_crit    = stat_buff_creator_t( this, "elemental_blast_critical_strike", find_spell( 118522 ) )
                                 .max_stack( 1 );
  buff.elemental_blast_haste   = stat_buff_creator_t( this, "elemental_blast_haste", find_spell( 173183 ) )
                                 .max_stack( 1 );
  buff.elemental_blast_mastery = stat_buff_creator_t( this, "elemental_blast_mastery", find_spell( 173184 ) )
                                 .max_stack( 1 );
  buff.elemental_blast_multistrike = stat_buff_creator_t( this, "elemental_blast_multistrike", find_spell( 173185 ) )
                                 .max_stack( 1 );
  buff.elemental_blast_agility = stat_buff_creator_t( this, "elemental_blast_agility", find_spell( 173186 ) )
                                 .max_stack( 1 );
  buff.tier13_2pc_caster        = stat_buff_creator_t( this, "tier13_2pc_caster", find_spell( 105779 ) );
  buff.tier13_4pc_caster        = stat_buff_creator_t( this, "tier13_4pc_caster", find_spell( 105821 ) );

  buff.t18_4pc_elemental        = buff_creator_t( this, "lightning_vortex", find_spell( 189063 ) )
    .chance( sets.has_set_bonus( SHAMAN_ELEMENTAL, T18, B4 ) )
    .reverse( true )
    .add_invalidate( CACHE_HASTE )
    .default_value( find_spell( 189063 ) -> effectN( 2 ).percent() )
    .max_stack( 5 ); // Shows 3 stacks in spelldata, but the wording makes it seem that it is actually 5.

  buff.enhanced_chain_lightning = buff_creator_t( this, "enhanced_chain_lightning", find_spell( 157766 ) );

  buff.focus_of_the_elements = buff_creator_t( this, "focus_of_the_elements", find_spell( 167205 ) )
                               .chance( static_cast< double >( sets.has_set_bonus( SHAMAN_ELEMENTAL, T17, B2 ) ) );
  buff.feral_spirit          = buff_creator_t( this, "t17_4pc_melee", sets.set( SHAMAN_ENHANCEMENT, T17, B4 ) -> effectN( 1 ).trigger() );
  buff.feral_spirit2         = haste_buff_creator_t( this, "feral_spirit", find_specialization_spell( "Feral Spirit" ) -> effectN( 2 ).trigger() )
                               .tick_callback( [ this ]( buff_t* b, int, int ) {
                                     double g = b -> data().effectN( 1 ).base_value();
                                     g += this -> talent.spiritual_resonance -> effectN( 2 ).base_value();
                                     this -> resource_gain( RESOURCE_MAELSTROM, g, this -> gain.feral_spirit );
                                   });

  buff.gathering_vortex      = buff_creator_t( this, "gathering_vortex", find_spell( 189078 ) )
                               .max_stack( sets.has_set_bonus( SHAMAN_ELEMENTAL, T18, B4 )
                                           ? sets.set( SHAMAN_ELEMENTAL, T18, B4 ) -> effectN( 1 ).base_value()
                                           : 1 )
                               .chance( sets.has_set_bonus( SHAMAN_ELEMENTAL, T18, B4 ) );

  buff.flametongue = buff_creator_t( this, "flametongue", find_specialization_spell( "Flametongue" ) -> effectN( 1 ).trigger() );
  buff.stormfury = buff_creator_t( this, "stormfury", find_spell( 201846 ) )
                   .activated( false ) // TODO: Need a delay on this
                   .max_stack( find_spell( 201846 ) -> initial_stacks() + talent.tempest -> effectN( 1 ).base_value() );
  buff.crash_lightning = buff_creator_t( this, "crash_lightning", find_spell( 187878 ) );
  buff.windsong = haste_buff_creator_t( this, "windsong", talent.windsong )
                  .default_value( 1.0 / ( 1.0 + talent.windsong -> effectN( 2 ).percent() ) );
  buff.fists_of_stone = haste_buff_creator_t( this, "fists_of_stone", talent.fists_of_stone )
                        .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
                        .add_invalidate( CACHE_CRIT )
                        .default_value( 1.0 / ( 1.0 + talent.fists_of_stone -> effectN( 3 ).percent() ) );
  buff.rockbiter = buff_creator_t( this, "rockbiter", find_spell( 202004 ) )
                   .add_invalidate( CACHE_ATTACK_POWER )
                   .chance( talent.landslide -> ok() )
                   .default_value( find_spell( 202004 ) -> effectN( 1 ).percent() );
  buff.doom_winds = buff_creator_t( this, "doom_winds", &( artifact.doom_winds.data() ) )
                    .cd( timespan_t::zero() ); // handled by the action
  buff.unleash_doom = buff_creator_t( this, "unleash_doom", artifact.unleash_doom.data().effectN( 1 ).trigger() );
  buff.wind_strikes = haste_buff_creator_t( this, "wind_strikes", find_spell( 198293 ) )
                      .chance( artifact.wind_strikes.rank() > 0 )
                      .default_value( 1.0 / ( 1.0 + artifact.wind_strikes.percent() ) );
  buff.gathering_storms = buff_creator_t( this, "gathering_storms", find_spell( 198300 ) );
  buff.fire_empowered = buff_creator_t( this, "fire_empowered", find_spell( 193774 ) );
  buff.lava_dredger = buff_creator_t( this, "lava_dredger", find_spell( 198830 ) );
}

// shaman_t::init_gains =====================================================

void shaman_t::init_gains()
{
  player_t::init_gains();

  gain.resurgence           = get_gain( "resurgence"        );
  gain.feral_spirit         = get_gain( "Feral Spirit"      );
  gain.fulmination          = get_gain( "Fulmination"       );
}

// shaman_t::init_procs =====================================================

void shaman_t::init_procs()
{
  player_t::init_procs();

  proc.lava_surge         = get_proc( "lava_surge"              );
  proc.ls_fast            = get_proc( "lightning_shield_too_fast_fill" );
  proc.swings_clipped_mh  = get_proc( "swings_clipped_mh"       );
  proc.swings_clipped_oh  = get_proc( "swings_clipped_oh"       );
  proc.swings_reset_mh    = get_proc( "swings_reset_mh"         );
  proc.swings_reset_oh    = get_proc( "swings_reset_oh"         );
  proc.uf_flame_shock     = get_proc( "uf_flame_shock"          );
  proc.uf_fire_nova       = get_proc( "uf_fire_nova"            );
  proc.uf_lava_burst      = get_proc( "uf_lava_burst"           );
  proc.uf_elemental_blast = get_proc( "uf_elemental_blast"      );
  proc.uf_wasted          = get_proc( "uf_wasted"               );
  proc.t15_2pc_melee      = get_proc( "t15_2pc_melee"           );
  proc.t16_2pc_melee      = get_proc( "t16_2pc_melee"           );
  proc.t16_4pc_caster     = get_proc( "t16_4pc_caster"          );
  proc.t16_4pc_melee      = get_proc( "t16_4pc_melee"           );
  proc.wasted_t15_2pc_melee = get_proc( "wasted_t15_2pc_melee"  );
  proc.wasted_lava_surge  = get_proc( "wasted_lava_surge"       );
  proc.wasted_ls          = get_proc( "wasted_lightning_shield" );
  proc.wasted_ls_shock_cd = get_proc( "wasted_lightning_shield_shock_cd" );
  proc.windfury           = get_proc( "windfury"                );
  proc.surge_during_lvb   = get_proc( "lava_surge_during_lvb"   );
}

// shaman_t::init_rng =======================================================

void shaman_t::init_rng()
{
  player_t::init_rng();

  real_ppm.unleash_doom = real_ppm_t( *this, artifact.unleash_doom.data().real_ppm() );
}

// shaman_t::init_actions ===================================================

void shaman_t::init_action_list()
{
  if ( ! ( primary_role() == ROLE_ATTACK && specialization() == SHAMAN_ENHANCEMENT ) &&
       ! ( primary_role() == ROLE_SPELL  && specialization() == SHAMAN_ELEMENTAL   ) )
  {
    if ( ! quiet )
      sim -> errorf( "Player %s's role (%s) or spec(%s) isn't supported yet.",
                     name(), util::role_type_string( primary_role() ), dbc::specialization_string( specialization() ).c_str() );
    quiet = true;
    return;
  }

  if ( specialization() == SHAMAN_ENHANCEMENT && main_hand_weapon.type == WEAPON_NONE )
  {
    if ( ! quiet )
      sim -> errorf( "Player %s has no weapon equipped at the Main-Hand slot.", name() );
    quiet = true;
    return;
  }

  // Restoration isn't supported atm
  if ( specialization() == SHAMAN_RESTORATION && primary_role() == ROLE_HEAL )
  {
    if ( ! quiet )
      sim -> errorf( "Restoration Shaman healing for player %s is not currently supported.", name() );

    quiet = true;
    return;
  }

  // After error checks, initialize secondary actions for various things
  if ( specialization() == SHAMAN_ENHANCEMENT )
  {
    windfury = new windfury_weapon_melee_attack_t( "windfury_attack", this, &( main_hand_weapon ) );
    flametongue = new flametongue_weapon_spell_t( "flametongue_attack", this, &( off_hand_weapon ) );
    action_improved_lava_lash = new improved_lava_lash_t( this );
  }

  if ( sets.has_set_bonus( SET_CASTER, T15, B2 ) )
  {
    action_lightning_strike = new t15_2pc_caster_t( this );
  }

  if ( sets.has_set_bonus( SHAMAN_ENHANCEMENT, T18, B2 ) )
  {
    electrocute = new electrocute_t( this );
  }

  if ( ! action_list_str.empty() )
  {
    player_t::init_action_list();
    return;
  }

  clear_action_priority_lists();

  action_priority_list_t* precombat = get_action_priority_list( "precombat" );
  action_priority_list_t* def       = get_action_priority_list( "default"   );
  action_priority_list_t* single    = get_action_priority_list( "single", "Single target action priority list" );
  action_priority_list_t* aoe       = get_action_priority_list( "aoe", "Multi target action priority list" );

  // Flask
  if ( sim -> allow_flasks && true_level >= 80 )
  {
    std::string flask_action = "flask,type=";
    if ( primary_role() == ROLE_ATTACK )
      flask_action += ( ( true_level > 90 ) ? "greater_draenic_agility_flask" : ( true_level >= 85 ) ? "spring_blossoms" : ( true_level >= 80 ) ? "winds" : "" );
    else
      flask_action += ( ( true_level > 90 ) ? "greater_draenic_intellect_flask" : ( true_level >= 85 ) ? "warm_sun" : ( true_level >= 80 ) ? "draconic_mind" : "" );

    precombat -> add_action( flask_action );
  }

  // Food
  if ( sim -> allow_food && level() >= 80 )
  {
    std::string food_action = "food,type=";
    if ( specialization() == SHAMAN_ENHANCEMENT )
      food_action += ( ( level() >= 100 ) ? "buttered_sturgeon" : ( level() > 85 ) ? "sea_mist_rice_noodles" : ( level() > 80 ) ? "seafood_magnifique_feast" : "" );
    else
      food_action += ( ( level() >= 100 ) ? "salty_squid_roll" : ( level() > 85 ) ? "mogu_fish_stew" : ( level() > 80 ) ? "seafood_magnifique_feast" : "" );

    precombat -> add_action( food_action );
  }

  // Active Shield, presume any non-restoration / healer wants lightning shield
  if ( specialization() != SHAMAN_RESTORATION || primary_role() != ROLE_HEAL )
    precombat -> add_action( this, "Lightning Shield", "if=!buff.lightning_shield.up" );

  // Snapshot stats
  precombat -> add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );

  std::string potion_name;
  if ( sim -> allow_potions && true_level >= 80 )
  {
    if ( primary_role() == ROLE_ATTACK )
    {
      if ( true_level > 90 )
        potion_name = "draenic_agility";
      else if ( true_level > 85 )
        potion_name = "virmens_bite";
      else
        potion_name = "tolvir";
    }
    else
    {
      if ( true_level > 90 )
        potion_name = "draenic_intellect";
      else if ( true_level > 85 )
        potion_name = "jade_serpent";
      else
        potion_name = "volcanic";
    }

    precombat -> add_action( "potion,name=" + potion_name );
  }

  // All Shamans Bloodlust and Wind Shear by default
  def -> add_action( this, "Wind Shear" );

  std::string bloodlust_options = "if=";

  if ( sim -> bloodlust_percent > 0 )
    bloodlust_options += "target.health.pct<" + util::to_string( sim -> bloodlust_percent ) + "|";

  if ( sim -> bloodlust_time < timespan_t::zero() )
    bloodlust_options += "target.time_to_die<" + util::to_string( - sim -> bloodlust_time.total_seconds() ) + "|";

  if ( sim -> bloodlust_time > timespan_t::zero() )
    bloodlust_options += "time>" + util::to_string( sim -> bloodlust_time.total_seconds() ) + "|";
  bloodlust_options.erase( bloodlust_options.end() - 1 );

  if ( action_priority_t* a = def -> add_action( this, "Bloodlust", bloodlust_options ) )
    a -> comment( "Bloodlust casting behavior mirrors the simulator settings for proxy bloodlust. See options 'bloodlust_percent', and 'bloodlust_time'. " );

  // Melee turns on auto attack
  if ( primary_role() == ROLE_ATTACK )
    def -> add_action( "auto_attack" );

  int num_items = (int)items.size();
  for ( int i = 0; i < num_items; i++ )
  {
    if ( items[i].has_special_effect( SPECIAL_EFFECT_SOURCE_NONE, SPECIAL_EFFECT_USE ) )
    {
      def -> add_action( "use_item,name=" + items[i].name_str );
    }
  }

  if ( specialization() == SHAMAN_ENHANCEMENT && primary_role() == ROLE_ATTACK )
  {
    // In-combat potion
    if ( sim -> allow_potions && true_level >= 80  )
    {
      std::string potion_action = "potion,name=" + potion_name + ",if=(talent.storm_elemental_totem.enabled&(pet.storm_elemental_totem.remains>=25|(cooldown.storm_elemental_totem.remains>target.time_to_die&pet.fire_elemental_totem.remains>=25)))|(!talent.storm_elemental_totem.enabled&pet.fire_elemental_totem.remains>=25)|target.time_to_die<=30";

      def -> add_action( potion_action, "In-combat potion is preferentially linked to the Fire or Storm Elemental, depending on talents, unless combat will end shortly" );
    }

    def -> add_action( "blood_fury" );
    def -> add_action( "arcane_torrent" );
    def -> add_action( "berserking" );
    def -> add_talent( this, "Elemental Mastery" );
    def -> add_action( this, "Feral Spirit" );
    def -> add_talent( this, "Liquid Magma", "if=pet.searing_totem.remains>10|pet.magma_totem.remains>10|pet.fire_elemental_totem.remains>10" );
    def -> add_talent( this, "Ancestral Swiftness" );
    def -> add_action( this, "Ascendance" );

    def -> add_action( "call_action_list,name=aoe,if=spell_targets.chain_lightning>1", "On multiple enemies, the priority follows the 'aoe' action list." );
    def -> add_action( "call_action_list,name=single", "If only one enemy, priority follows the 'single' action list." );

    single -> add_action( this, find_specialization_spell( "Ascendance" ), "windstrike", "if=!talent.echo_of_the_elements.enabled|(talent.echo_of_the_elements.enabled&(charges=2|(action.windstrike.charges_fractional>1.75)|(charges=1&buff.ascendance.remains<1.5)))" );
    single -> add_action( this, "Stormstrike", "if=!talent.echo_of_the_elements.enabled|(talent.echo_of_the_elements.enabled&(charges=2|(action.stormstrike.charges_fractional>1.75)|target.time_to_die<6))" );
    single -> add_action( this, "Primal Strike" );
    single -> add_action( this, "Lava Lash", "if=!talent.echo_of_the_elements.enabled|(talent.echo_of_the_elements.enabled&(charges=2|(action.lava_lash.charges_fractional>1.8)|target.time_to_die<8))" );
    single -> add_action( this, find_specialization_spell( "Ascendance" ), "windstrike", "if=talent.echo_of_the_elements.enabled" );
    single -> add_action( this, "Lava Lash", "if=talent.echo_of_the_elements.enabled" );
    single -> add_action( this, "Stormstrike", "if=talent.echo_of_the_elements.enabled" );

    // AoE
    aoe -> add_action( this, "Fire Nova", "if=active_dot.flame_shock>=3&spell_targets.fire_nova_explosion>=3" );
    aoe -> add_action( "wait,sec=cooldown.fire_nova.remains,if=!talent.echo_of_the_elements.enabled&active_dot.flame_shock>=4&cooldown.fire_nova.remains<=action.fire_nova.gcd%2" );
    aoe -> add_action( this, "Lava Lash", "if=dot.flame_shock.ticking&active_dot.flame_shock<spell_targets.fire_nova_explosion" );
    aoe -> add_action( this, find_specialization_spell( "Ascendance" ), "windstrike" );
    aoe -> add_action( this, "Stormstrike" );
    aoe -> add_action( this, "Lava Lash" );
    aoe -> add_action( this, "Fire Nova", "if=active_dot.flame_shock>=2&spell_targets.fire_nova_explosion>=2" );
    aoe -> add_action( this, "Primal Strike" );
    aoe -> add_action( this, "Fire Nova", "if=active_dot.flame_shock>=1&spell_targets.fire_nova_explosion>=1" );
  }
  else if ( specialization() == SHAMAN_ELEMENTAL && ( primary_role() == ROLE_SPELL || primary_role() == ROLE_DPS ) )
  {
    // In-combat potion
    if ( sim -> allow_potions && true_level >= 80  )
    {
      std::string potion_action = "potion,name=" + potion_name + ",if=buff.ascendance.up|target.time_to_die<=30";

      def -> add_action( potion_action, "In-combat potion is preferentially linked to Ascendance, unless combat will end shortly" );
    }
    // Sync berserking with ascendance as they share a cooldown, but making sure
    // that no two haste cooldowns overlap, within reason
    def -> add_action( "berserking,if=!buff.bloodlust.up&(set_bonus.tier15_4pc_caster=1|(buff.ascendance.cooldown_remains=0&(dot.flame_shock.remains>buff.ascendance.duration|level<87)))" );
    // Sync blood fury with ascendance or fire elemental as long as one is ready
    // soon after blood fury is.
    def -> add_action( "blood_fury,if=buff.bloodlust.up|buff.ascendance.up|((cooldown.ascendance.remains>10|level<87)&cooldown.fire_elemental_totem.remains>10)" );
    def -> add_action( "arcane_torrent" );

    // Use Elemental Mastery on cooldown so long as it won't send you significantly under
    // the GCD cap.
    def -> add_talent( this, "Elemental Mastery", "if=action.lava_burst.cast_time>=1.2" );

    def -> add_talent( this, "Ancestral Swiftness", "if=!buff.ascendance.up" );
    def -> add_talent( this, "Storm Elemental Totem" );

    // Use Ascendance preferably with a haste CD up, but dont overdo the
    // delaying. Make absolutely sure that Ascendance can be used so that
    // only Lava Bursts need to be cast during it's duration
    std::string ascendance_opts = "if=spell_targets.chain_lightning>1|(dot.flame_shock.remains>buff.ascendance.duration&(target.time_to_die<20|buff.bloodlust.up";
    if ( race == RACE_TROLL )
      ascendance_opts += "|buff.berserking.up|set_bonus.tier15_4pc_caster=1";
    else
      ascendance_opts += "|time>=60";
    ascendance_opts += ")&cooldown.lava_burst.remains>0)";

    def -> add_action( this, "Ascendance", ascendance_opts );

    def -> add_talent( this, "Liquid Magma", "if=pet.searing_totem.remains>=15|pet.fire_elemental_totem.remains>=15" );

    // Need to remove the "/" in front of the profession action(s) for the new default action priority list stuff :/
    def -> add_action( init_use_profession_actions().erase( 0, 1 ) );

    def -> add_action( "call_action_list,name=aoe,if=spell_targets.chain_lightning>(2+t18_class_trinket)", "On multiple enemies, the priority follows the 'aoe' action list." );
    def -> add_action( "call_action_list,name=single", "If one or two enemies, priority follows the 'single' action list." );

    single -> add_action( this, "Unleash Flame", "moving=1" );
    single -> add_action( this, "Spiritwalker's Grace", "moving=1,if=buff.ascendance.up" );
    if ( find_item( "unerring_vision_of_lei_shen" ) )
      single -> add_action( this, "Flame Shock", "if=buff.perfect_aim.react&crit_pct<100" );
    single -> add_action( this, spec.fulmination, "earth_shock", "if=buff.lightning_shield.react=buff.lightning_shield.max_stack" );
    single -> add_action( this, "Lava Burst", "if=dot.flame_shock.remains>cast_time&(buff.ascendance.up|cooldown_react)" );
    single -> add_action( this, spec.fulmination, "earth_shock", "if=(set_bonus.tier17_4pc&buff.lightning_shield.react>=12&!buff.lava_surge.up)|(!set_bonus.tier17_4pc&buff.lightning_shield.react>15)" );
    single -> add_action( this, "Flame Shock", "cycle_targets=1,if=dot.flame_shock.remains<=(dot.flame_shock.duration*0.3)" );
    single -> add_talent( this, "Elemental Blast" );
    single -> add_action( this, "Flame Shock", "if=time>60&remains<=buff.ascendance.duration&cooldown.ascendance.remains+buff.ascendance.duration<duration",
                          "After the initial Ascendance, use Flame Shock pre-emptively just before Ascendance to guarantee Flame Shock staying up for the full duration of the Ascendance buff" );
    single -> add_action( this, "Spiritwalker's Grace", "moving=1,if=((talent.elemental_blast.enabled&cooldown.elemental_blast.remains=0)|(cooldown.lava_burst.remains=0&!buff.lava_surge.react))&cooldown.ascendance.remains>cooldown.spiritwalkers_grace.remains" );

    single -> add_action( this, "Earthquake", "cycle_targets=1,if=buff.enhanced_chain_lightning.up" );
    single -> add_action( this, "Chain Lightning", "if=spell_targets.chain_lightning>=2" );
    single -> add_action( this, "Lightning Bolt" );
    single -> add_action( this, "Earth Shock", "moving=1" );

    // AoE
    aoe -> add_action( this, "Earthquake", "cycle_targets=1,if=buff.enhanced_chain_lightning.up" );
    aoe -> add_action( this, find_specialization_spell( "Ascendance" ), "lava_beam" );
    aoe -> add_action( this, spec.fulmination, "earth_shock", "if=buff.lightning_shield.react=buff.lightning_shield.max_stack" );
    aoe -> add_action( this, "Chain Lightning", "if=spell_targets.chain_lightning>=2" );
    aoe -> add_action( this, "Lightning Bolt" );
  }
  else if ( primary_role() == ROLE_SPELL )
  {
    def -> add_action( this, "Spiritwalker's Grace", "moving=1" );
    def -> add_talent( this, "Elemental Mastery" );
    def -> add_talent( this, "Elemental Blast" );
    def -> add_talent( this, "Ancestral Swiftness" );
    def -> add_action( this, "Flame Shock", "if=!ticking|ticks_remain<2|((buff.bloodlust.react)&ticks_remain<3)" );
    def -> add_action( this, "Lava Burst", "if=dot.flame_shock.remains>cast_time" );
    def -> add_action( this, "Chain Lightning", "if=target.adds>2&mana.pct>25" );
    def -> add_action( this, "Lightning Bolt" );
  }
  else if ( primary_role() == ROLE_ATTACK )
  {
    def -> add_action( this, "Spiritwalker's Grace", "moving=1" );
    def -> add_talent( this, "Elemental Mastery" );
    def -> add_talent( this, "Elemental Blast" );
    def -> add_talent( this, "Ancestral Swiftness" );
    def -> add_talent( this, "Primal Strike" );
    def -> add_action( this, "Flame Shock", "if=!ticking|ticks_remain<2|((buff.bloodlust.react)&ticks_remain<3)" );
    def -> add_action( this, "Earth Shock" );
    def -> add_action( this, "Lightning Bolt", "moving=1" );
  }

  use_default_action_list = true;

  player_t::init_action_list();
}

// shaman_t::moving =========================================================

void shaman_t::moving()
{
  // Spiritwalker's Grace complicates things, as you can cast it while casting
  // anything. So, to model that, if a raid move event comes, we need to check
  // if we can trigger Spiritwalker's Grace. If so, conditionally execute it, to
  // allow the currently executing cast to finish.
  if ( true_level >= 85 )
  {
    action_t* swg = find_action( "spiritwalkers_grace" );

    // We need to bypass swg -> ready() check here, so whip up a special
    // readiness check that only checks for player skill, cooldown and resource
    // availability
    if ( swg && executing && swg -> ready() )
    {
      // Shaman executes SWG mid-cast during a movement event, if
      // 1) The profile does not have Glyph of Unleashed Lightning and is
      //    casting a Lighting Bolt (non-instant cast)
      // 2) The profile is casting Lava Burst (without Lava Surge)
      // 3) The profile is casting Chain Lightning
      // 4) The profile is casting Elemental Blast
      if ( ( executing -> id == 51505 ) ||
           ( executing -> id == 421 ) ||
           ( executing -> id == 117014 ) )
      {
        if ( sim -> log )
          sim -> out_log.printf( "%s spiritwalkers_grace during spell cast, next cast (%s) should finish",
                         name(), executing -> name() );
        swg -> execute();
      }
    }
    else
    {
      interrupt();
    }

    if ( main_hand_attack ) main_hand_attack -> cancel();
    if (  off_hand_attack )  off_hand_attack -> cancel();
  }
  else
  {
    halt();
  }
}

// shaman_t::matching_gear_multiplier =======================================

double shaman_t::matching_gear_multiplier( attribute_e attr ) const
{
  if ( attr == ATTR_AGILITY || attr == ATTR_INTELLECT )
    return constant.matching_gear_multiplier;

  return 0.0;
}

// shaman_t::composite_spell_haste ==========================================

double shaman_t::composite_spell_haste() const
{
  double h = player_t::composite_spell_haste();

  if ( talent.ancestral_swiftness -> ok() )
    h *= constant.haste_ancestral_swiftness;

  if ( buff.tier13_4pc_healer -> up() )
    h *= 1.0 / ( 1.0 + buff.tier13_4pc_healer -> data().effectN( 1 ).percent() );

  h *= 1.0 / ( 1.0 + buff.t18_4pc_elemental -> stack_value() );

  if ( buff.feral_spirit2 -> up() )
  {
    h *= 1.0 / ( 1.0 + buff.feral_spirit2 -> data().effectN( 2 ).percent() );
  }

  return h;
}

// shaman_t::composite_spell_crit ===========================================

double shaman_t::composite_spell_crit() const
{
  double m = player_t::composite_spell_crit();

  if ( buff.fists_of_stone -> up() )
  {
    m += buff.fists_of_stone -> data().effectN( 1 ).percent();
  }

  m += spec.critical_strikes -> effectN( 1 ).percent();

  return m;
}

// shaman_t::temporary_movement_modifier =======================================

double shaman_t::temporary_movement_modifier() const
{
  double ms = player_t::temporary_movement_modifier();

  if ( buff.spirit_walk -> up() )
    ms = std::max( buff.spirit_walk -> data().effectN( 1 ).percent(), ms );

  if ( buff.feral_spirit -> up() )
    ms = std::max( buff.feral_spirit -> data().effectN( 1 ).percent(), ms );

  if ( buff.fists_of_stone -> up() )
  {
    ms *= 1.0 + buff.fists_of_stone -> data().effectN( 4 ).percent();
  }

  return ms;
}

// shaman_t::composite_melee_crit ===========================================

double shaman_t::composite_melee_crit() const
{
  double m = player_t::composite_melee_crit();

  if ( buff.fists_of_stone -> up() )
  {
    m += buff.fists_of_stone -> data().effectN( 1 ).percent();
  }

  m += spec.critical_strikes -> effectN( 1 ).percent();

  return m;
}

// shaman_t::composite_attack_haste =========================================

double shaman_t::composite_melee_haste() const
{
  double h = player_t::composite_melee_haste();

  if ( talent.ancestral_swiftness -> ok() )
    h *= constant.haste_ancestral_swiftness;

  if ( buff.tier13_4pc_healer -> up() )
    h *= 1.0 / ( 1.0 + buff.tier13_4pc_healer -> data().effectN( 1 ).percent() );

  if ( buff.feral_spirit2 -> up() )
  {
    h *= 1.0 / ( 1.0 + buff.feral_spirit2 -> data().effectN( 2 ).percent() );
  }

  return h;
}

// shaman_t::composite_attack_speed =========================================

double shaman_t::composite_melee_speed() const
{
  double speed = player_t::composite_melee_speed();

  if ( talent.ancestral_swiftness -> ok() )
    speed *= constant.speed_attack_ancestral_swiftness;

  if ( buff.windsong -> up() )
  {
    speed *= buff.windsong -> check_value();
  }

  if ( buff.fists_of_stone -> up() )
  {
    speed *= buff.fists_of_stone -> check_value();
  }

  if ( buff.wind_strikes -> up() )
  {
    speed *= buff.wind_strikes -> check_value();
  }

  return speed;
}

// shaman_t::composite_spell_power ==========================================

double shaman_t::composite_spell_power( school_e school ) const
{
  double sp = 0;

  if ( specialization() == SHAMAN_ENHANCEMENT )
    sp = composite_attack_power_multiplier() * cache.attack_power() * mental_quickness;
  else
    sp = player_t::composite_spell_power( school );

  return sp;
}

// shaman_t::composite_spell_power_multiplier ===============================

double shaman_t::composite_spell_power_multiplier() const
{
  if ( specialization() == SHAMAN_ENHANCEMENT )
    return 1.0;

  return player_t::composite_spell_power_multiplier();
}

// shaman_t::composite_attack_power_multiplier ===============================

double shaman_t::composite_attack_power_multiplier() const
{
  double m = player_t::composite_attack_power_multiplier();

  if ( buff.rockbiter -> up() )
  {
    m *= 1.0 + buff.rockbiter -> check_value();
  }

  return m;
}

// shaman_t::composite_player_multiplier ====================================

double shaman_t::composite_player_multiplier( school_e school ) const
{
  double m = player_t::composite_player_multiplier( school );

  if ( mastery.enhanced_elements -> ok() &&
       ( dbc::is_school( school, SCHOOL_FIRE   ) ||
         dbc::is_school( school, SCHOOL_FROST  ) ||
         dbc::is_school( school, SCHOOL_NATURE ) ) )
  {
    m *= 1.0 + cache.mastery_value();
  }

  if ( buff.fists_of_stone -> up() )
  {
    m *= 1.0 + buff.fists_of_stone -> data().effectN( 2 ).percent();
  }

  return m;
}

// shaman_t::invalidate_cache ===============================================

void shaman_t::invalidate_cache( cache_e c )
{
  player_t::invalidate_cache( c );

  switch ( c )
  {
    case CACHE_AGILITY:
    case CACHE_STRENGTH:
    case CACHE_ATTACK_POWER:
      if ( specialization() == SHAMAN_ENHANCEMENT )
        player_t::invalidate_cache( CACHE_SPELL_POWER );
      break;
    case CACHE_MASTERY:
      if ( mastery.enhanced_elements -> ok() )
      {
        player_t::invalidate_cache( CACHE_PLAYER_DAMAGE_MULTIPLIER );
      }
      break;
    default: break;
  }
}

// shaman_t::arise() ========================================================

void shaman_t::arise()
{
  player_t::arise();

  assert( main_hand_attack == melee_mh && off_hand_attack == melee_oh );

  if ( main_hand_weapon.type != WEAPON_NONE )
    main_hand_weapon.buff_type = WINDFURY_IMBUE;

  if ( off_hand_weapon.type != WEAPON_NONE )
    off_hand_weapon.buff_type = FLAMETONGUE_IMBUE;
}

// shaman_t::reset ==========================================================

void shaman_t::reset()
{
  player_t::reset();

  ls_reset = timespan_t::zero();
  lava_surge_during_lvb = false;
  for (auto & elem : counters)
    elem -> reset();
  real_ppm.unleash_doom.reset();
}

// shaman_t::merge ==========================================================

void shaman_t::merge( player_t& other )
{
  player_t::merge( other );

  const shaman_t& s = static_cast<shaman_t&>( other );

  for ( size_t i = 0, end = counters.size(); i < end; i++ )
    counters[ i ] -> merge( *s.counters[ i ] );

  for ( size_t i = 0, end = cd_waste_exec.size(); i < end; i++ )
  {
    cd_waste_exec[ i ] -> second.merge( s.cd_waste_exec[ i ] -> second );
    cd_waste_cumulative[ i ] -> second.merge( s.cd_waste_cumulative[ i ] -> second );
  }
}

// shaman_t::datacollection_begin ===========================================

void shaman_t::datacollection_begin()
{
  if ( active_during_iteration )
  {
    for ( size_t i = 0, end = cd_waste_iter.size(); i < end; ++i )
    {
      cd_waste_iter[ i ] -> second.reset();
    }
  }

  player_t::datacollection_begin();
}

// shaman_t::datacollection_end =============================================

void shaman_t::datacollection_end()
{
  if ( requires_data_collection() )
  {
    for ( size_t i = 0, end = cd_waste_iter.size(); i < end; ++i )
    {
      cd_waste_cumulative[ i ] -> second.add( cd_waste_iter[ i ] -> second.sum() );
    }
  }

  player_t::datacollection_end();
}

// shaman_t::has_t18_class_trinket ==========================================

bool shaman_t::has_t18_class_trinket() const
{
  switch ( specialization() )
  {
    case SHAMAN_ENHANCEMENT: return furious_winds != nullptr;
    case SHAMAN_ELEMENTAL:   return elemental_bellows != nullptr;
    default:                 return false;
  }
}

// shaman_t::primary_role ===================================================

role_e shaman_t::primary_role() const
{
  if ( player_t::primary_role() == ROLE_HEAL )
    return ROLE_HYBRID;//To prevent spawning healing_target, as there is no support for healing.

  if ( specialization() == SHAMAN_RESTORATION )
  {
    if ( player_t::primary_role() == ROLE_DPS || player_t::primary_role() == ROLE_SPELL )
      return ROLE_SPELL;

    return ROLE_SPELL;
  }

  else if ( specialization() == SHAMAN_ENHANCEMENT )
    return ROLE_ATTACK;

  else if ( specialization() == SHAMAN_ELEMENTAL )
    return ROLE_SPELL;

  return player_t::primary_role();
}

// shaman_t::convert_hybrid_stat ===========================================
stat_e shaman_t::convert_hybrid_stat( stat_e s ) const
{
  switch ( s )
  {
  case STAT_STR_AGI_INT:
  case STAT_AGI_INT:
    if ( specialization() == SHAMAN_ENHANCEMENT )
      return STAT_AGILITY;
    else
      return STAT_INTELLECT;
  // This is a guess at how AGI/STR gear will work for Resto/Elemental, TODO: confirm
  case STAT_STR_AGI:
    return STAT_AGILITY;
  // This is a guess at how STR/INT gear will work for Enhance, TODO: confirm
  // this should probably never come up since shamans can't equip plate, but....
  case STAT_STR_INT:
    return STAT_INTELLECT;
  case STAT_SPIRIT:
    if ( specialization() == SHAMAN_RESTORATION )
      return s;
    else
      return STAT_NONE;
  case STAT_BONUS_ARMOR:
      return STAT_NONE;
  default: return s;
  }
}

/* Report Extension Class
 * Here you can define class specific report extensions/overrides
 */
class shaman_report_t : public player_report_extension_t
{
public:
  shaman_report_t( shaman_t& player ) :
      p( player )
  { }
  /*
  void mwgen_table_header( report::sc_html_stream& os )
  {
    os << "<table class=\"sc\" style=\"float: left;margin-right: 10px;\">\n"
         << "<tr>\n"
           << "<th>Ability</th>\n"
           << "<th>Generated</th>\n"
           << "<th>Wasted</th>\n"
         << "</tr>\n";
  }

  void mwuse_table_header( report::sc_html_stream& os )
  {
    const shaman_t& s = static_cast<const shaman_t&>( p );
    size_t n_mwstack = s.buff.maelstrom_weapon -> max_stack();
    os << "<table class=\"sc\" style=\"float: left;\">\n"
         << "<tr style=\"vertical-align: bottom;\">\n"
           << "<th rowspan=\"2\">Ability</th>\n"
           << "<th rowspan=\"2\">Event</th>\n";

    for ( size_t i = 0; i <= n_mwstack; ++i )
    {
      os   << "<th rowspan=\"2\">" << i << "</th>\n";
    }

    os     << "<th colspan=\"2\">Total</th>\n"
         << "</tr>\n"
         << "<tr><th>casts</th><th>charges</th></tr>\n";
  }
  */
  void cdwaste_table_header( report::sc_html_stream& os )
  {
    os << "<table class=\"sc\" style=\"float: left;margin-right: 10px;\">\n"
         << "<tr>\n"
           << "<th></th>\n"
           << "<th colspan=\"3\">Seconds per Execute</th>\n"
           << "<th colspan=\"3\">Seconds per Iteration</th>\n"
         << "</tr>\n"
         << "<tr>\n"
           << "<th>Ability</th>\n"
           << "<th>Average</th>\n"
           << "<th>Minimum</th>\n"
           << "<th>Maximum</th>\n"
           << "<th>Average</th>\n"
           << "<th>Minimum</th>\n"
           << "<th>Maximum</th>\n"
         << "</tr>\n";
  }

  /*
  void mwgen_table_footer( report::sc_html_stream& os )
  {
    os << "</table>\n";
  }

  void mwuse_table_footer( report::sc_html_stream& os )
  {
    os << "</table>\n";
  }
  */
  void cdwaste_table_footer( report::sc_html_stream& os )
  {
    os << "</table>\n";
  }
  /*
  void mwgen_table_contents( report::sc_html_stream& os )
  {
    double total_generated = 0, total_wasted = 0;
    int n = 0;

    for ( size_t i = 0, end = p.stats_list.size(); i < end; i++ )
    {
      stats_t* stats = p.stats_list[ i ];
      double n_generated = 0, n_wasted = 0;

      for (auto & elem : stats -> action_list)
      {
        shaman_attack_t* a = dynamic_cast<shaman_attack_t*>( elem );
        if ( ! a )
          continue;

        if ( ! a -> may_proc_maelstrom )
          continue;

        n_generated += a -> maelstrom_procs -> mean();
        total_generated += a -> maelstrom_procs -> mean();
        n_wasted += a -> maelstrom_procs_wasted -> mean();
        total_wasted += a -> maelstrom_procs_wasted -> mean();
      }

      if ( n_generated > 0 || n_wasted > 0 )
      {
        std::string name_str = report::decorated_action_name( stats -> action_list[ 0 ] );
        std::string row_class_str = "";
        if ( ++n & 1 )
          row_class_str = " class=\"odd\"";

        os.format("<tr%s><td class=\"left\">%s</td><td class=\"right\">%.2f</td><td class=\"right\">%.2f (%.2f%%)</td></tr>\n",
            row_class_str.c_str(),
            name_str.c_str(),
            util::round( n_generated, 2 ),
            util::round( n_wasted, 2 ), util::round( 100.0 * n_wasted / n_generated, 2 ) );
      }
    }

    os.format("<tr><td class=\"left\">Total</td><td class=\"right\">%.2f</td><td class=\"right\">%.2f (%.2f%%)</td></tr>\n",
        total_generated, total_wasted, 100.0 * total_wasted / total_generated );
  }

  void mwuse_table_contents( report::sc_html_stream& os )
  {
    const shaman_t& s = static_cast<const shaman_t&>( p );
    size_t n_mwstack = s.buff.maelstrom_weapon -> max_stack();
    std::vector<double> total_mw_cast( n_mwstack + 2 );
    std::vector<double> total_mw_executed( n_mwstack + 2 );
    int n = 0;
    std::string row_class_str = "";

    for ( size_t i = 0, end = p.action_list.size(); i < end; i++ )
    {
      if ( shaman_spell_t* s = dynamic_cast<shaman_spell_t*>( p.action_list[ i ] ) )
      {
        for ( size_t j = 0, end2 = s -> maelstrom_weapon_cast.size() - 1; j < end2; j++ )
        {
          total_mw_cast[ j ] += s -> maelstrom_weapon_cast[ j ] -> mean();
          total_mw_cast[ n_mwstack + 1 ] += s -> maelstrom_weapon_cast[ j ] -> mean();

          total_mw_executed[ j ] += s -> maelstrom_weapon_executed[ j ] -> mean();
          total_mw_executed[ n_mwstack + 1 ] += s -> maelstrom_weapon_executed[ j ] -> mean();
        }
      }
    }

    for ( size_t i = 0, end = p.stats_list.size(); i < end; i++ )
    {
      stats_t* stats = p.stats_list[ i ];
      std::vector<double> n_cast( n_mwstack + 2 );
      std::vector<double> n_executed( n_mwstack + 2 );
      double n_cast_charges = 0, n_executed_charges = 0;
      bool has_data = false;

      for (auto & elem : stats -> action_list)
      {
        if ( shaman_spell_t* s = dynamic_cast<shaman_spell_t*>( elem ) )
        {
          for ( size_t k = 0, end3 = s -> maelstrom_weapon_cast.size() - 1; k < end3; k++ )
          {
            if ( s -> maelstrom_weapon_cast[ k ] -> mean() > 0 || s -> maelstrom_weapon_executed[ k ] -> mean() > 0 )
              has_data = true;

            n_cast[ k ] += s -> maelstrom_weapon_cast[ k ] -> mean();
            n_cast[ n_mwstack + 1 ] += s -> maelstrom_weapon_cast[ k ] -> mean();

            n_cast_charges += s -> maelstrom_weapon_cast[ k ] -> mean() * k;

            n_executed[ k ] += s -> maelstrom_weapon_executed[ k ] -> mean();
            n_executed[ n_mwstack + 1 ] += s -> maelstrom_weapon_executed[ k ] -> mean();

            n_executed_charges += s -> maelstrom_weapon_executed[ k ] -> mean() * k;
          }
        }
      }

      if ( has_data )
      {
        row_class_str = "";
        if ( ++n & 1 )
          row_class_str = " class=\"odd\"";

        std::string name_str = report::decorated_action_name( stats -> action_list[ 0 ] );

        os.format("<tr%s><td rowspan=\"2\" class=\"left\" style=\"vertical-align: top;\">%s</td>",
            row_class_str.c_str(), name_str.c_str() );

        os << "<td class=\"left\">Cast</td>";

        for ( size_t j = 0, end2 = n_cast.size(); j < end2; j++ )
        {
          double pct = 0;
          if ( total_mw_cast[ j ] > 0 )
            pct = 100.0 * n_cast[ j ] / n_cast[ n_mwstack + 1 ];

          if ( j < end2 - 1 )
            os.format("<td class=\"right\">%.1f (%.1f%%)</td>", util::round( n_cast[ j ], 1 ), util::round( pct, 1 ) );
          else
          {
            os.format("<td class=\"right\">%.1f</td>", util::round( n_cast[ j ], 1 ) );
            os.format("<td class=\"right\">%.1f</td>", util::round( n_cast_charges, 1 ) );
          }
        }

        os << "</tr>\n";

        os.format("<tr%s>", row_class_str.c_str() );

        os << "<td class=\"left\">Execute</td>";

        for ( size_t j = 0, end2 = n_executed.size(); j < end2; j++ )
        {
          double pct = 0;
          if ( total_mw_executed[ j ] > 0 )
            pct = 100.0 * n_executed[ j ] / n_executed[ n_mwstack + 1 ];

          if ( j < end2 - 1 )
            os.format("<td class=\"right\">%.1f (%.1f%%)</td>", util::round( n_executed[ j ], 1 ), util::round( pct, 1 ) );
          else
          {
            os.format("<td class=\"right\">%.1f</td>", util::round( n_executed[ j ], 1 ) );
            os.format("<td class=\"right\">%.1f</td>", util::round( n_executed_charges, 1 ) );
          }
        }

        os << "</tr>\n";
      }
    }
  }
  */
  void cdwaste_table_contents( report::sc_html_stream& os )
  {
    size_t n = 0;
    for ( size_t i = 0; i < p.cd_waste_exec.size(); i++ )
    {
      const data_t* entry = p.cd_waste_exec[ i ];
      if ( entry -> second.count() == 0 )
      {
        continue;
      }

      const data_t* iter_entry = p.cd_waste_cumulative[ i ];

      action_t* a = p.find_action( entry -> first );
      std::string name_str = entry -> first;
      if ( a )
      {
        name_str = report::decorated_action_name( a );
      }

      std::string row_class_str = "";
      if ( ++n & 1 )
        row_class_str = " class=\"odd\"";

      os.format( "<tr%s>", row_class_str.c_str() );
      os << "<td class=\"left\">" << name_str << "</td>";
      os.format("<td class=\"right\">%.3f</td>", entry -> second.mean() );
      os.format("<td class=\"right\">%.3f</td>", entry -> second.min() );
      os.format("<td class=\"right\">%.3f</td>", entry -> second.max() );
      os.format("<td class=\"right\">%.3f</td>", iter_entry -> second.mean() );
      os.format("<td class=\"right\">%.3f</td>", iter_entry -> second.min() );
      os.format("<td class=\"right\">%.3f</td>", iter_entry -> second.max() );
      os << "</tr>\n";
    }
  }

  virtual void html_customsection( report::sc_html_stream& os ) override
  {
    // Custom Class Section
    os << "\t\t\t\t<div class=\"player-section custom_section\">\n";
/*
    if ( p.specialization() == SHAMAN_ENHANCEMENT )
    {
      os << "\t\t\t\t\t<h3 class=\"toggle open\">Maelstrom Weapon details</h3>\n"
         << "\t\t\t\t\t<div class=\"toggle-content\">\n";

      mwgen_table_header( os );
      mwgen_table_contents( os );
      mwgen_table_footer( os );

      mwuse_table_header( os );
      mwuse_table_contents( os );
      mwuse_table_footer( os );

      os << "\t\t\t\t\t\t</div>\n";

      os << "<div class=\"clear\"></div>\n";
    }
*/
    if ( p.cd_waste_exec.size() > 0 )
    {
      os << "\t\t\t\t\t<h3 class=\"toggle open\">Cooldown waste details</h3>\n"
         << "\t\t\t\t\t<div class=\"toggle-content\">\n";

      cdwaste_table_header( os );
      cdwaste_table_contents( os );
      cdwaste_table_footer( os );

      os << "\t\t\t\t\t</div>\n";

      os << "<div class=\"clear\"></div>\n";
    }

    os << "\t\t\t\t\t</div>\n";
  }
private:
  shaman_t& p;
};

// SHAMAN MODULE INTERFACE ==================================================

static void do_trinket_init( shaman_t*                player,
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

// Elemental T18 (WoD 6.2) trinket effect
static void elemental_bellows( special_effect_t& effect )
{
  shaman_t* s = debug_cast<shaman_t*>( effect.player );
  do_trinket_init( s, SHAMAN_ELEMENTAL, s -> elemental_bellows, effect );
}

// Enhancement T18 (WoD 6.2) trinket effect
static void furious_winds( special_effect_t& effect )
{
  shaman_t* s = debug_cast<shaman_t*>( effect.player );
  do_trinket_init( s, SHAMAN_ENHANCEMENT, s -> furious_winds, effect );
}

// Enhancement Doomhammer of Doom of Legion fame!
static void doomhammer( special_effect_t& effect )
{
  struct doomhammer_proc_callback_t : public dbc_proc_callback_t
  {
    earthen_might_t* earthen_might;

    doomhammer_proc_callback_t( const item_t* i, const special_effect_t& effect ) :
      dbc_proc_callback_t( i, effect ),
      earthen_might( new earthen_might_t( debug_cast<shaman_t*>( i -> player ) ) )
    { }

    void trigger( action_t* a, void* call_data ) override
    {
      // Proc on Rockbiter and Flametongue (primary buttonpress) only
      if ( a -> id != 193786 && a -> id != 193796 )
      {
        return;
      }

      dbc_proc_callback_t::trigger( a, call_data );
    }

    void execute( action_t* a, action_state_t* state ) override
    {
      shaman_t* shaman = debug_cast<shaman_t*>( state -> action -> player );
      switch ( a -> id )
      {
        case 193786:
          earthen_might -> target = state -> target;
          earthen_might -> schedule_execute();
          break;
        case 193796:
          shaman -> buff.lava_dredger -> trigger();
          break;
        default:
          break;
      }
    }
  };

  new doomhammer_proc_callback_t( effect.item, effect );
}

struct shaman_module_t : public module_t
{
  shaman_module_t() : module_t( SHAMAN ) {}

  virtual player_t* create_player( sim_t* sim, const std::string& name, race_e r = RACE_NONE ) const override
  {
    auto  p = new shaman_t( sim, name, r );
    p -> report_extension = std::unique_ptr<player_report_extension_t>( new shaman_report_t( *p ) );
    return p;
  }
  virtual bool valid() const override { return true; }
  virtual void init( player_t* p ) const override
  {
    p -> buffs.bloodlust  = haste_buff_creator_t( p, "bloodlust", p -> find_spell( 2825 ) )
                            .max_stack( 1 );

    p -> buffs.earth_shield = buff_creator_t( p, "earth_shield", p -> find_spell( 974 ) )
                              .cd( timespan_t::from_seconds( 2.0 ) );

    p -> buffs.exhaustion = buff_creator_t( p, "exhaustion", p -> find_spell( 57723 ) )
                            .max_stack( 1 )
                            .quiet( true );
  }

  virtual void static_init() const override
  {
    unique_gear::register_special_effect( 184919, elemental_bellows );
    unique_gear::register_special_effect( 184920, furious_winds     );
    unique_gear::register_special_effect( 198735, doomhammer        );
  }

  virtual void register_hotfixes() const override
  {
  }

  virtual void combat_begin( sim_t* ) const override {}
  virtual void combat_end( sim_t* ) const override {}
};

} // UNNAMED NAMESPACE

const module_t* module_t::shaman()
{
  static shaman_module_t m;
  return &m;
}
