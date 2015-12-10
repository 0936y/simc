// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

// ==========================================================================
//
// TODO:
// Pets
//
// Affliction -
// UA ignite, totally not right.
// Seed of Corruption
// Haunt reset
// GoSac
// Soul Effigy
//
// Destruction - Everything
//
// Demo - Everything
// 
// ==========================================================================
namespace { // unnamed namespace

// ==========================================================================
// Warlock
// ==========================================================================

struct warlock_t;

namespace pets {
struct wild_imp_pet_t;
struct t18_illidari_satyr_t;
struct t18_prince_malchezaar_t;
struct t18_vicious_hellhound_t;
}

struct actives_t
{
  action_t*           unstable_affliction;
} active;

struct warlock_td_t: public actor_target_data_t
{
  dot_t*  dots_agony;
  dot_t*  dots_corruption;
  dot_t*  dots_doom;
  dot_t*  dots_drain_soul;
  dot_t*  dots_immolate;
  dot_t*  dots_seed_of_corruption;
  dot_t*  dots_shadowflame;
  dot_t*  dots_unstable_affliction;
  dot_t*  dots_siphon_life;
  dot_t*  dots_phantom_singularity;

  buff_t* debuffs_haunt;
  buff_t* debuffs_shadowflame;
  buff_t* debuffs_agony;
  buff_t* debuffs_flamelicked;

  int agony_stack;
  double soc_trigger;

  warlock_t& warlock;
  warlock_td_t( player_t* target, warlock_t& p );

  void reset()
  {
    agony_stack = 1;
    soc_trigger = 0;
  }

  void target_demise();
};

struct warlock_t: public player_t
{
public:
  player_t* havoc_target;
  int double_nightfall;


  // Active Pet
  struct pets_t
  {
    pet_t* active;
    pet_t* last;
    static const int WILD_IMP_LIMIT = 25;
    static const int T18_PET_LIMIT = 6 ;
    std::array<pets::wild_imp_pet_t*, WILD_IMP_LIMIT> wild_imps;
    pet_t* inner_demon;
    std::array<pets::t18_illidari_satyr_t*, T18_PET_LIMIT> t18_illidari_satyr;
    std::array<pets::t18_prince_malchezaar_t*, T18_PET_LIMIT> t18_prince_malchezaar;
    std::array<pets::t18_vicious_hellhound_t*, T18_PET_LIMIT> t18_vicious_hellhound;
  } pets;

  std::vector<std::string> pet_name_list;

  // Talents
  struct talents_t
  {

    const spell_data_t* haunt;
    const spell_data_t* writhe_in_agony;
    const spell_data_t* drain_soul;

    const spell_data_t* backdraft;
    const spell_data_t* fire_and_brimstone;
    const spell_data_t* shadowburn;

    const spell_data_t* contagion;
    const spell_data_t* absolute_corruption;
    const spell_data_t* mana_tap;

    const spell_data_t* soul_leech;
    const spell_data_t* mortal_coil;
    const spell_data_t* howl_of_terror;

    const spell_data_t* siphon_life;
    const spell_data_t* sow_the_seeds;
    const spell_data_t* soul_harvest;

    const spell_data_t* demonic_circle;
    const spell_data_t* burning_rush;
    const spell_data_t* dark_pact;

    const spell_data_t* grimoire_of_supremacy;
    const spell_data_t* grimoire_of_service;
    const spell_data_t* grimoire_of_sacrifice;
    const spell_data_t* grimoire_of_synergy;

    const spell_data_t* soul_effigy;
    const spell_data_t* phantom_singularity;
    const spell_data_t* demonic_servitude;

    const spell_data_t* demonbolt; // Demonology only
    const spell_data_t* cataclysm;
    const spell_data_t* shadowfury;
  } talents;

  // Glyphs
  struct glyphs_t
  {
  } glyphs;

  // Mastery Spells
  struct mastery_spells_t
  {
    const spell_data_t* potent_afflictions;
    const spell_data_t* master_demonologist;
    const spell_data_t* emberstorm;
  } mastery_spells;

  //Procs and RNG
  real_ppm_t demonic_power_rppm; // grimoire of sacrifice
  real_ppm_t grimoire_of_synergy; //caster ppm, i.e., if it procs, the wl will create a buff for the pet.
  real_ppm_t grimoire_of_synergy_pet; //pet ppm, i.e., if it procs, the pet will create a buff for the wl.

  // Cooldowns
  struct cooldowns_t
  {
    cooldown_t* infernal;
    cooldown_t* doomguard;
    cooldown_t* hand_of_guldan;
    cooldown_t* haunt;
  } cooldowns;

  // Passives
  struct specs_t
  {
    // All Specs
    const spell_data_t* fel_armor;
    const spell_data_t* nethermancy;

    // Affliction only
    const spell_data_t* nightfall;
    const spell_data_t* unstable_affliction;

    // Demonology only
    const spell_data_t* doom;
    const spell_data_t* wild_imps;

    // Destruction only
    const spell_data_t* immolate;

  } spec;

  // Buffs
  struct buffs_t
  {
    buff_t* backdraft;
    buff_t* demonic_synergy;
    buff_t* fire_and_brimstone;
    buff_t* demonic_power;
    buff_t* havoc;
    buff_t* mana_tap;

    buff_t* tier18_2pc_demonology;
  } buffs;

  // Gains
  struct gains_t
  {
    gain_t* life_tap;
    gain_t* soul_leech;
    gain_t* nightfall;
    gain_t* conflagrate;
    gain_t* immolate;
    gain_t* shadowburn_shard;
    gain_t* miss_refund;
    gain_t* seed_of_corruption;
    gain_t* drain_soul;
    gain_t* soul_harvest;
    gain_t* mana_tap;
  } gains;

  // Procs
  struct procs_t
  {
    proc_t* wild_imp;
    proc_t* havoc_waste;
    proc_t* fragment_wild_imp;
    proc_t* t18_4pc_destruction;
    proc_t* t18_illidari_satyr;
    proc_t* t18_vicious_hellhound;
    proc_t* t18_prince_malchezaar;
  } procs;

  struct spells_t
  {
    spell_t* seed_of_corruption_aoe;
    spell_t* melee;

  } spells;

  int initial_soul_shards;
  std::string default_pet;

  timespan_t shard_react;

    // Tier 18 (WoD 6.2) trinket effects
  const special_effect_t* affliction_trinket;
  const special_effect_t* demonology_trinket;
  const special_effect_t* destruction_trinket;

  warlock_t( sim_t* sim, const std::string& name, race_e r = RACE_UNDEAD );

  // Character Definition
  virtual void      init_spells() override;
  virtual void      init_base_stats() override;
  virtual void      init_scaling() override;
  virtual void      create_buffs() override;
  virtual void      init_gains() override;
  virtual void      init_procs() override;
  virtual void      init_rng() override;
  virtual void      init_action_list() override;
  virtual void      init_resources( bool force ) override;
  virtual void      reset() override;
  virtual void      create_options() override;
  virtual action_t* create_action( const std::string& name, const std::string& options ) override;
  virtual pet_t*    create_pet( const std::string& name, const std::string& type = std::string() ) override;
  virtual void      create_pets() override;
  virtual std::string      create_profile( save_e = SAVE_ALL ) override;
  virtual void      copy_from( player_t* source ) override;
  virtual resource_e primary_resource() const override { return RESOURCE_MANA; }
  virtual role_e    primary_role() const override     { return ROLE_SPELL; }
  virtual stat_e    convert_hybrid_stat( stat_e s ) const override;
  virtual double    matching_gear_multiplier( attribute_e attr ) const override;
  virtual double    composite_player_multiplier( school_e school ) const override;
  virtual double    composite_rating_multiplier( rating_e rating ) const override;
  virtual void      invalidate_cache( cache_e ) override;
  virtual double    composite_spell_crit() const override;
  virtual double    composite_spell_haste() const override;
  virtual double    composite_melee_crit() const override;
  virtual double    composite_mastery() const override;
  virtual double    resource_gain( resource_e, double, gain_t* = nullptr, action_t* = nullptr ) override;
  virtual double    mana_regen_per_second() const override;
  virtual double    composite_armor() const override;

  virtual void      halt() override;
  virtual void      combat_begin() override;
  virtual expr_t*   create_expression( action_t* a, const std::string& name_str ) override;

  target_specific_t<warlock_td_t> target_data;

  virtual warlock_td_t* get_target_data( player_t* target ) const override
  {
    warlock_td_t*& td = target_data[target];
    if ( ! td )
    {
      td = new warlock_td_t( target, const_cast<warlock_t&>( *this ) );
    }
    return td;
  }

private:
  void apl_precombat();
  void apl_default();
  void apl_affliction();
  void apl_demonology();
  void apl_destruction();
  void apl_global_filler();
};

static void do_trinket_init(  warlock_t*               player,
                              specialization_e         spec,
                              const special_effect_t*& ptr,
                              const special_effect_t&  effect )
{
  // Ensure we have the spell data. This will prevent the trinket effect from working on live
  // Simulationcraft. Also ensure correct specialization.
  if ( !player -> find_spell( effect.spell_id ) -> ok() ||
    player -> specialization() != spec )
  {
    return;
  }
  // Set pointer, module considers non-null pointer to mean the effect is "enabled"
  ptr = &( effect );
}

static void affliction_trinket( special_effect_t& effect )
{
  warlock_t* warlock = debug_cast<warlock_t*>( effect.player );
  do_trinket_init( warlock, WARLOCK_AFFLICTION, warlock -> affliction_trinket, effect );
}

static void demonology_trinket( special_effect_t& effect )
{
  warlock_t* warlock = debug_cast<warlock_t*>( effect.player );
  do_trinket_init( warlock, WARLOCK_DEMONOLOGY, warlock -> demonology_trinket, effect );
}

static void destruction_trinket( special_effect_t& effect )
{
  warlock_t* warlock = debug_cast<warlock_t*>( effect.player );
  do_trinket_init( warlock, WARLOCK_DESTRUCTION, warlock -> destruction_trinket, effect);
}

void parse_spell_coefficient( action_t& a )
{
  for ( size_t i = 1; i <= a.data()._effects -> size(); i++ )
  {
    if ( a.data().effectN( i ).type() == E_SCHOOL_DAMAGE )
      a.spell_power_mod.direct = a.data().effectN( i ).sp_coeff();
    else if ( a.data().effectN( i ).type() == E_APPLY_AURA && a.data().effectN( i ).subtype() == A_PERIODIC_DAMAGE )
      a.spell_power_mod.tick = a.data().effectN( i ).sp_coeff();
  }
}

// Pets
namespace pets {

  struct warlock_pet_t: public pet_t
  {
    action_t* special_action;
    action_t* special_action_two;
    melee_attack_t* melee_attack;
    stats_t* summon_stats;
    const spell_data_t* supremacy;
    const spell_data_t* command;

    warlock_pet_t( sim_t* sim, warlock_t* owner, const std::string& pet_name, pet_e pt, bool guardian = false );
    virtual void init_base_stats() override;
    virtual void init_action_list() override;
    virtual void create_buffs() override;
    virtual void schedule_ready( timespan_t delta_time = timespan_t::zero(),
      bool   waiting = false ) override;
    virtual double composite_player_multiplier( school_e school ) const override;
    virtual double composite_melee_crit() const override;
    virtual double composite_spell_crit() const override;
    virtual double composite_melee_haste() const override;
    virtual double composite_spell_haste() const override;
    virtual double composite_melee_speed() const override;
    virtual double composite_spell_speed() const override;
    virtual resource_e primary_resource() const override { return RESOURCE_ENERGY; }
    warlock_t* o()
    {
      return static_cast<warlock_t*>( owner );
    }
    const warlock_t* o() const
    {
      return static_cast<warlock_t*>( owner );
    }

    struct buffs_t
    {
      buff_t* demonic_synergy;
    } buffs;

    struct travel_t: public action_t
    {
      travel_t( player_t* player ): action_t( ACTION_OTHER, "travel", player ) {}
      void execute() override { player -> current.distance = 1; }
      timespan_t execute_time() const override { return timespan_t::from_seconds( player -> current.distance / 10.0 ); }
      bool ready() override { return ( player -> current.distance > 1 ); }
      bool usable_moving() const override { return true; }
    };

    action_t* create_action( const std::string& name,
      const std::string& options_str ) override
    {
      if ( name == "travel" ) return new travel_t( this );

      return pet_t::create_action( name, options_str );
    }
  };

namespace actions {

// Template for common warlock pet action code. See priest_action_t.
template <class ACTION_BASE>
struct warlock_pet_action_t: public ACTION_BASE
{
public:
private:
  typedef ACTION_BASE ab; // action base, eg. spell_t
public:
  typedef warlock_pet_action_t base_t;

  warlock_pet_action_t( const std::string& n, warlock_pet_t* p,
                        const spell_data_t* s = spell_data_t::nil() ):
                        ab( n, p, s )
  {
    ab::may_crit = true;
  }
  virtual ~warlock_pet_action_t() {}

  warlock_pet_t* p()
  {
    return static_cast<warlock_pet_t*>( ab::player );
  }
  const warlock_pet_t* p() const
  {
    return static_cast<warlock_pet_t*>( ab::player );
  }

  virtual void execute()
  {
    ab::execute();

    if ( ab::result_is_hit( ab::execute_state -> result ) && p() -> o() -> talents.grimoire_of_synergy -> ok() )
    {
      bool procced = p() -> o() -> grimoire_of_synergy_pet.trigger(); //check for RPPM
      if ( procced ) p() -> o() -> buffs.demonic_synergy -> trigger(); //trigger the buff
    }
  }
};

struct warlock_pet_melee_t: public melee_attack_t
{
  struct off_hand_swing: public melee_attack_t
  {
    off_hand_swing( warlock_pet_t* p, const char* name = "melee_oh" ):
      melee_attack_t( name, p, spell_data_t::nil() )
    {
      school = SCHOOL_PHYSICAL;
      weapon = &( p -> off_hand_weapon );
      base_execute_time = weapon -> swing_time;
      may_crit = true;
      background = true;
      base_multiplier = 0.5;
    }
  };

  off_hand_swing* oh;

  warlock_pet_melee_t( warlock_pet_t* p, const char* name = "melee" ):
    melee_attack_t( name, p, spell_data_t::nil() ), oh( nullptr )
  {
    school = SCHOOL_PHYSICAL;
    weapon = &( p -> main_hand_weapon );
    base_execute_time = weapon -> swing_time;
    may_crit = background = repeating = true;

    if ( p -> dual_wield() )
      oh = new off_hand_swing( p );
  }

  virtual void execute() override
  {
    if ( ! player -> executing && ! player -> channeling )
    {
      melee_attack_t::execute();
      if ( oh )
      {
        oh -> time_to_execute = time_to_execute;
        oh -> execute();
      }
    }
    else
    {
      schedule_execute();
    }
  }
};

struct warlock_pet_melee_attack_t: public warlock_pet_action_t < melee_attack_t >
{
private:
  void _init_warlock_pet_melee_attack_t()
  {
    weapon = &( player -> main_hand_weapon );
    special = true;
  }

public:
  warlock_pet_melee_attack_t( warlock_pet_t* p, const std::string& n ):
    base_t( n, p, p -> find_pet_spell( n ) )
  {
    _init_warlock_pet_melee_attack_t();
  }

  warlock_pet_melee_attack_t( const std::string& token, warlock_pet_t* p, const spell_data_t* s = spell_data_t::nil() ):
    base_t( token, p, s )
  {
    _init_warlock_pet_melee_attack_t();
  }
};

struct warlock_pet_spell_t: public warlock_pet_action_t < spell_t >
{
private:
  void _init_warlock_pet_spell_t()
  {
    parse_spell_coefficient( *this );
  }

public:
  warlock_pet_spell_t( warlock_pet_t* p, const std::string& n ):
    base_t( n, p, p -> find_pet_spell( n ) )
  {
    _init_warlock_pet_spell_t();
  }

  warlock_pet_spell_t( const std::string& token, warlock_pet_t* p, const spell_data_t* s = spell_data_t::nil() ):
    base_t( token, p, s )
  {
    _init_warlock_pet_spell_t();
  }
};

struct firebolt_t: public warlock_pet_spell_t
{
  firebolt_t( warlock_pet_t* p ):
    warlock_pet_spell_t( p, "Firebolt" )
  {
    if ( p -> owner -> bugs )
      min_gcd = timespan_t::from_seconds( 1.5 );
  }
};

struct legion_strike_t: public warlock_pet_melee_attack_t
{
  legion_strike_t( warlock_pet_t* p ):
    warlock_pet_melee_attack_t( p, "Legion Strike" )
  {
    aoe = -1;
    split_aoe_damage = true;
    weapon           = &( p -> main_hand_weapon );
  }

  virtual bool ready() override
  {
    if ( p() -> special_action -> get_dot() -> is_ticking() ) return false;

    return warlock_pet_melee_attack_t::ready();
  }
};

struct felstorm_tick_t: public warlock_pet_melee_attack_t
{
  felstorm_tick_t( warlock_pet_t* p, const spell_data_t& s ):
    warlock_pet_melee_attack_t( "felstorm_tick", p, s.effectN( 1 ).trigger() )
  {
    aoe = -1;
    background = true;
    weapon = &( p -> main_hand_weapon );
  }
};

struct felstorm_t: public warlock_pet_melee_attack_t
{
  felstorm_t( warlock_pet_t* p ):
    warlock_pet_melee_attack_t( "felstorm", p, p -> find_spell( 89751 ) )
  {
    tick_zero = true;
    hasted_ticks = false;
    may_miss = false;
    may_crit = false;
    weapon_multiplier = 0;

    dynamic_tick_action = true;
    tick_action = new felstorm_tick_t( p, data() );
  }

  virtual void cancel() override
  {
    warlock_pet_melee_attack_t::cancel();

    get_dot() -> cancel();
  }

  virtual void execute() override
  {
    warlock_pet_melee_attack_t::execute();

    p() -> melee_attack -> cancel();
  }

  virtual void last_tick( dot_t* d ) override
  {
    warlock_pet_melee_attack_t::last_tick( d );

    if ( ! p() -> is_sleeping() && ! p() -> melee_attack -> target -> is_sleeping() )
      p() -> melee_attack -> execute();
  }
};

struct shadow_bite_t: public warlock_pet_spell_t
{
  shadow_bite_t( warlock_pet_t* p ):
    warlock_pet_spell_t( p, "Shadow Bite" )
  { }
};

struct lash_of_pain_t: public warlock_pet_spell_t
{
  lash_of_pain_t( warlock_pet_t* p ):
    warlock_pet_spell_t( p, "Lash of Pain" )
  {
    if ( p -> owner -> bugs ) min_gcd = timespan_t::from_seconds( 1.5 );
  }
};

struct whiplash_t: public warlock_pet_spell_t
{
  whiplash_t( warlock_pet_t* p ):
    warlock_pet_spell_t( p, "Whiplash" )
  {
    aoe = -1;
  }
};

struct torment_t: public warlock_pet_spell_t
{
  torment_t( warlock_pet_t* p ):
    warlock_pet_spell_t( p, "Torment" )
  { }
};

struct immolation_tick_t: public warlock_pet_spell_t
{
  immolation_tick_t( warlock_pet_t* p, const spell_data_t& s ):
    warlock_pet_spell_t( "immolation_tick", p, s.effectN( 1 ).trigger() )
  {
    aoe = -1;
    background = true;
    may_crit = true;
    may_multistrike = true;
  }
};

struct immolation_t: public warlock_pet_spell_t
{
  immolation_t( warlock_pet_t* p, const std::string& options_str ):
    warlock_pet_spell_t( "immolation", p, p -> find_spell( 19483 ) )
  {
    parse_options( options_str );

    dynamic_tick_action = hasted_ticks = true;
    tick_action = new immolation_tick_t( p, data() );
  }

  void init() override
  {
    warlock_pet_spell_t::init();

    // Explicitly snapshot haste, as the spell actually has no duration in spell data
    snapshot_flags |= STATE_HASTE;
  }

  timespan_t composite_dot_duration( const action_state_t* ) const override
  {
    return player -> sim -> expected_iteration_time * 2;
  }

  virtual void cancel() override
  {
    dot_t* dot = find_dot( target );
    if ( dot && dot -> is_ticking() )
    {
      dot -> cancel();
    }
    action_t::cancel();
  }
};

struct doom_bolt_t: public warlock_pet_spell_t
{
  doom_bolt_t( warlock_pet_t* p ):
    warlock_pet_spell_t( "Doom Bolt", p, p -> find_spell( 85692 ) )
  {
  }

  virtual double composite_target_multiplier( player_t* target ) const override
  {
    double m = warlock_pet_spell_t::composite_target_multiplier( target );

    if ( target -> health_percentage() < 20 )
    {
      m *= 1.0 + data().effectN( 2 ).percent();
    }
    return m;
  }
};

struct meteor_strike_t: public warlock_pet_spell_t
{
  meteor_strike_t( warlock_pet_t* p, const std::string& options_str ):
    warlock_pet_spell_t( "Meteor Strike", p, p -> find_spell( 171018 ) )
  {
    parse_options( options_str );
    aoe = -1;
  }
};

struct wild_firebolt_t: public warlock_pet_spell_t
{
  wild_firebolt_t( warlock_pet_t* p ):
    warlock_pet_spell_t( "fel_firebolt", p, p -> find_spell( 104318 ) )
  {
  }

  virtual void execute() override
  {
    warlock_pet_spell_t::execute();

    if ( player -> resources.current[RESOURCE_ENERGY] <= 0 )
    {
      p() -> dismiss();
      return;
    }
  }

  virtual bool ready() override
  {
    return spell_t::ready();
  }
};

} // pets::actions

warlock_pet_t::warlock_pet_t( sim_t* sim, warlock_t* owner, const std::string& pet_name, pet_e pt, bool guardian ):
pet_t( sim, owner, pet_name, pt, guardian ), special_action( nullptr ), special_action_two( nullptr ), melee_attack( nullptr ), summon_stats( nullptr )
{
  owner_coeff.ap_from_sp = 1.0;
  owner_coeff.sp_from_sp = 1.0;
  supremacy = find_spell( 115578 );
  command = find_spell( 21563 );
}

void warlock_pet_t::init_base_stats()
{
  pet_t::init_base_stats();

  resources.base[RESOURCE_ENERGY] = 200;
  base_energy_regen_per_second = 10;

  base.spell_power_per_intellect = 1;

  intellect_per_owner = 0;
  stamina_per_owner = 0;

  main_hand_weapon.type = WEAPON_BEAST;

  //double dmg = dbc.spell_scaling( owner -> type, owner -> level );

  main_hand_weapon.swing_time = timespan_t::from_seconds( 2.0 );
}

void warlock_pet_t::init_action_list()
{
  if ( special_action )
  {
    if ( type == PLAYER_PET )
      special_action -> background = true;
    else
      special_action -> action_list = get_action_priority_list( "default" );
  }

  if ( special_action_two )
  {
    if ( type == PLAYER_PET )
      special_action_two -> background = true;
    else
      special_action_two -> action_list = get_action_priority_list( "default" );
  }

  pet_t::init_action_list();

  if ( summon_stats )
    for ( size_t i = 0; i < action_list.size(); ++i )
      summon_stats -> add_child( action_list[i] -> stats );
}

void warlock_pet_t::create_buffs()
{
  pet_t::create_buffs();

  buffs.demonic_synergy = buff_creator_t( this, "demonic_synergy", find_spell( 171982 ) )
    .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
    .chance( 1 );
}

void warlock_pet_t::schedule_ready( timespan_t delta_time, bool waiting )
{
  dot_t* d;
  if ( melee_attack && ! melee_attack -> execute_event && ! ( special_action && ( d = special_action -> get_dot() ) && d -> is_ticking() ) )
  {
    melee_attack -> schedule_execute();
  }

  pet_t::schedule_ready( delta_time, waiting );
}

double warlock_pet_t::composite_player_multiplier( school_e school ) const
{
  double m = pet_t::composite_player_multiplier( school );

  if ( o() -> race == RACE_ORC )
    m *= 1.0 + command -> effectN( 1 ).percent();

  m *= 1.0 + o() -> buffs.tier18_2pc_demonology -> stack_value();

  if ( o() -> talents.grimoire_of_supremacy -> ok() && pet_type != PET_WILD_IMP )
    m *= 1.0 + supremacy -> effectN( 1 ).percent(); // The relevant effect is not attatched to the talent spell, weirdly enough

  if ( buffs.demonic_synergy -> up() )
    m *= 1.0 + buffs.demonic_synergy -> data().effectN( 1 ).percent();

  return m;
}

double warlock_pet_t::composite_melee_crit() const
{
  double mc = pet_t::composite_melee_crit();

  return mc;
}

double warlock_pet_t::composite_spell_crit() const
{
  double sc = pet_t::composite_spell_crit();

  return sc;
}

double warlock_pet_t::composite_melee_haste() const
{
  double mh = pet_t::composite_melee_haste();

  return mh;
}

double warlock_pet_t::composite_spell_haste() const
{
  double sh = pet_t::composite_spell_haste();

  return sh;
}

double warlock_pet_t::composite_melee_speed() const
{
  // Make sure we get our overridden haste values applied to melee_speed
  return player_t::composite_melee_speed();
}

double warlock_pet_t::composite_spell_speed() const
{
  // Make sure we get our overridden haste values applied to spell_speed
  return player_t::composite_spell_speed();
}

struct imp_pet_t: public warlock_pet_t
{
  imp_pet_t( sim_t* sim, warlock_t* owner, const std::string& name = "imp" ):
    warlock_pet_t( sim, owner, name, PET_IMP, name != "imp" )
  {
    action_list_str = "firebolt";
    owner_coeff.ap_from_sp = 1.0;
  }

  virtual action_t* create_action( const std::string& name, const std::string& options_str ) override
  {
    if ( name == "firebolt" ) return new actions::firebolt_t( this );

    return warlock_pet_t::create_action( name, options_str );
  }
};

struct felguard_pet_t: public warlock_pet_t
{
  felguard_pet_t( sim_t* sim, warlock_t* owner, const std::string& name = "felguard" ):
    warlock_pet_t( sim, owner, name, PET_FELGUARD, name != "felguard" )
  {
    action_list_str = "legion_strike";
    owner_coeff.ap_from_sp = 1.0;
  }

  virtual void init_base_stats() override
  {
    warlock_pet_t::init_base_stats();

    melee_attack = new actions::warlock_pet_melee_t( this );
    special_action = new actions::felstorm_t( this );
  }

  virtual action_t* create_action( const std::string& name, const std::string& options_str ) override
  {
    if ( name == "legion_strike" ) return new actions::legion_strike_t( this );

    return warlock_pet_t::create_action( name, options_str );
  }
};

struct t18_illidari_satyr_t: public warlock_pet_t
{
  t18_illidari_satyr_t(sim_t* sim, warlock_t* owner ):
    warlock_pet_t( sim, owner, "illidari_satyr", PET_FELGUARD, true )
  {
    owner_coeff.ap_from_sp = 1;
    regen_type = REGEN_DISABLED;
    action_list_str = "travel";
  }

  void init_base_stats() override
  {
    warlock_pet_t::init_base_stats();
    base_energy_regen_per_second = 0;
    melee_attack = new actions::warlock_pet_melee_t( this );
    if ( o() -> pets.t18_illidari_satyr[0] )
      melee_attack -> stats = o() -> pets.t18_illidari_satyr[0] -> get_stats( "melee" );
  }
};

struct t18_prince_malchezaar_t: public warlock_pet_t
{
  t18_prince_malchezaar_t(  sim_t* sim, warlock_t* owner ):
    warlock_pet_t( sim, owner, "prince_malchezaar", PET_GHOUL, true )
  {
    owner_coeff.ap_from_sp = 1;
    regen_type = REGEN_DISABLED;
    action_list_str = "travel";
  }

  void init_base_stats() override
  {
    warlock_pet_t::init_base_stats();
    base_energy_regen_per_second = 0;
    melee_attack = new actions::warlock_pet_melee_t( this );
    if ( o() -> pets.t18_prince_malchezaar[0] )
      melee_attack -> stats = o() -> pets.t18_prince_malchezaar[0] -> get_stats( "melee" );
  }

  double composite_player_multiplier( school_e school ) const override
  {
    double m = warlock_pet_t::composite_player_multiplier( school );
    m *= 9.45; // Prince deals 9.45 times normal damage.. you know.. for reasons.
    return m;
  }
};

struct t18_vicious_hellhound_t: public warlock_pet_t
{
  t18_vicious_hellhound_t( sim_t* sim, warlock_t* owner ):
    warlock_pet_t( sim, owner, "vicious_hellhound", PET_DOG, true )
  {
    owner_coeff.ap_from_sp = 1;
    regen_type = REGEN_DISABLED;
    action_list_str = "travel";
  }

  void init_base_stats() override
  {
    warlock_pet_t::init_base_stats();
    base_energy_regen_per_second = 0;
    main_hand_weapon.swing_time = timespan_t::from_seconds( 1.0 );
    melee_attack = new actions::warlock_pet_melee_t( this );
    melee_attack -> base_execute_time = timespan_t::from_seconds( 1.0 );
    if ( o() -> pets.t18_vicious_hellhound[0] )
      melee_attack -> stats = o() -> pets.t18_vicious_hellhound[0] -> get_stats( "melee" );
  }
};

struct felhunter_pet_t: public warlock_pet_t
{
  felhunter_pet_t( sim_t* sim, warlock_t* owner, const std::string& name = "felhunter" ):
    warlock_pet_t( sim, owner, name, PET_FELHUNTER, name != "felhunter" )
  {
    action_list_str = "shadow_bite";
    owner_coeff.ap_from_sp = 1.0;
  }

  virtual void init_base_stats() override
  {
    warlock_pet_t::init_base_stats();

    melee_attack = new actions::warlock_pet_melee_t( this );
  }

  virtual action_t* create_action( const std::string& name, const std::string& options_str ) override
  {
    if ( name == "shadow_bite" ) return new actions::shadow_bite_t( this );

    return warlock_pet_t::create_action( name, options_str );
  }
};

struct succubus_pet_t: public warlock_pet_t
{
  succubus_pet_t( sim_t* sim, warlock_t* owner, const std::string& name = "succubus" ):
    warlock_pet_t( sim, owner, name, PET_SUCCUBUS, name != "succubus" )
  {
    action_list_str = "lash_of_pain";
    owner_coeff.ap_from_sp = 0.5;
  }

  virtual void init_base_stats() override
  {
    warlock_pet_t::init_base_stats();

    main_hand_weapon.swing_time = timespan_t::from_seconds( 3.0 );
    melee_attack = new actions::warlock_pet_melee_t( this );
    if ( ! util::str_compare_ci( name_str, "service_succubus" ) )
      special_action = new actions::whiplash_t( this );
  }

  virtual action_t* create_action( const std::string& name, const std::string& options_str ) override
  {
    if ( name == "lash_of_pain" ) return new actions::lash_of_pain_t( this );

    return warlock_pet_t::create_action( name, options_str );
  }
};

struct voidwalker_pet_t: public warlock_pet_t
{
  voidwalker_pet_t( sim_t* sim, warlock_t* owner, const std::string& name = "voidwalker" ):
    warlock_pet_t( sim, owner, name, PET_VOIDWALKER, name != "voidwalker" )
  {
    action_list_str = "torment";
    owner_coeff.ap_from_sp = 1.0;
  }

  virtual void init_base_stats() override
  {
    warlock_pet_t::init_base_stats();

    melee_attack = new actions::warlock_pet_melee_t( this );
  }

  virtual action_t* create_action( const std::string& name, const std::string& options_str ) override
  {
    if ( name == "torment" ) return new actions::torment_t( this );

    return warlock_pet_t::create_action( name, options_str );
  }
};

struct infernal_pet_t: public warlock_pet_t
{
  infernal_pet_t( sim_t* sim, warlock_t* owner, const std::string& name = "infernal"   ):
    warlock_pet_t( sim, owner, name, PET_INFERNAL, name != "infernal" )
  {
    owner_coeff.ap_from_sp = 0.065934;
  }

  virtual void init_base_stats() override
  {
    warlock_pet_t::init_base_stats();
    action_list_str = "immolation,if=!ticking";
    if ( o() -> talents.demonic_servitude -> ok() )
      action_list_str += "/meteor_strike";
    resources.base[RESOURCE_ENERGY] = 100;
    melee_attack = new actions::warlock_pet_melee_t( this );
  }

  virtual action_t* create_action( const std::string& name, const std::string& options_str ) override
  {
    if ( name == "immolation" ) return new actions::immolation_t( this, options_str );
    if ( name == "meteor_strike" ) return new actions::meteor_strike_t( this, options_str );

    return warlock_pet_t::create_action( name, options_str );
  }
};

struct doomguard_pet_t: public warlock_pet_t
{
    doomguard_pet_t( sim_t* sim, warlock_t* owner, const std::string& name = "doomguard"  ):
    warlock_pet_t( sim, owner, name, PET_DOOMGUARD, name != "doomguard" )
  {
    owner_coeff.ap_from_sp = 0.065934;
    action_list_str = "doom_bolt";
  }

  virtual void init_base_stats() override
  {
    warlock_pet_t::init_base_stats();

    resources.base[RESOURCE_ENERGY] = 100;
  }

  virtual action_t* create_action( const std::string& name, const std::string& options_str ) override
  {
    if ( name == "doom_bolt" ) return new actions::doom_bolt_t( this );

    return warlock_pet_t::create_action( name, options_str );
  }
};

struct wild_imp_pet_t: public warlock_pet_t
{
  stats_t** firebolt_stats;
  stats_t* regular_stats;
  stats_t* swarm_stats;

  wild_imp_pet_t( sim_t* sim, warlock_t* owner ):
    warlock_pet_t( sim, owner, "wild_imp", PET_WILD_IMP, true ), firebolt_stats( nullptr )
  {
    owner_coeff.sp_from_sp = 0.75;
  }

  virtual void init_base_stats() override
  {
    warlock_pet_t::init_base_stats();

    action_list_str = "firebolt";

    resources.base[RESOURCE_ENERGY] = 10;
    base_energy_regen_per_second = 0;
  }

  virtual action_t* create_action( const std::string& name,
                                   const std::string& options_str ) override
  {
    if ( name == "firebolt" )
    {
      action_t* a = new actions::wild_firebolt_t( this );
      firebolt_stats = &( a -> stats );
      if ( this == o() -> pets.wild_imps[ 0 ] || sim -> report_pets_separately )
      {
        regular_stats = a -> stats;
        swarm_stats = get_stats( "fel_firebolt_swarm", a );
        swarm_stats -> school = a -> school;
      }
      else
      {
        regular_stats = o() -> pets.wild_imps[ 0 ] -> get_stats( "fel_firebolt" );
        swarm_stats = o() -> pets.wild_imps[ 0 ] -> get_stats( "fel_firebolt_swarm" );
      }
      return a;
    }

    return warlock_pet_t::create_action( name, options_str );
  }

  void trigger( bool swarm = false )
  {
    if ( swarm )
      *firebolt_stats = swarm_stats;
    else
      *firebolt_stats = regular_stats;

    summon();
  }
};


} // end namespace pets

// Spells
namespace actions {

struct warlock_heal_t: public heal_t
{
  warlock_heal_t( const std::string& n, warlock_t* p, const uint32_t id ):
    heal_t( n, p, p -> find_spell( id ) )
  {
    target = p;
  }

  warlock_t* p()
  {
    return static_cast<warlock_t*>( player );
  }
};

struct warlock_spell_t: public spell_t
{
  bool demo_mastery;
private:
  void _init_warlock_spell_t()
  {
    may_crit = true;
    tick_may_crit = true;
    weapon_multiplier = 0.0;
    gain = player -> get_gain( name_str );
    cost_event = nullptr;
    havoc_consume = backdraft_consume = 0;

    havoc_proc = nullptr;

    if ( p() -> destruction_trinket )
    {
      affected_by_flamelicked = data().affected_by( p() -> destruction_trinket -> driver() -> effectN( 1 ).trigger() -> effectN( 1 ) );
    }
    else
    {
      affected_by_flamelicked = false;
    }

    affected_by_contagion = data().affected_by( p() -> find_spell( 30108 ) -> effectN( 2 ) );

    parse_spell_coefficient( *this );

  }

public:
  gain_t* gain;
  mutable std::vector< player_t* > havoc_targets;

  int havoc_consume, backdraft_consume;

  proc_t* havoc_proc;

  bool affected_by_flamelicked;
  bool affected_by_contagion;

  struct cost_event_t: player_event_t
  {
    warlock_spell_t* spell;
    resource_e resource;

    cost_event_t( player_t* p, warlock_spell_t* s, resource_e r = RESOURCE_NONE ):
      player_event_t( *p ), spell( s ), resource( r )
    {
      if ( resource == RESOURCE_NONE ) resource = spell -> current_resource();
      add_event( timespan_t::from_seconds( 1 ) );
    }
    virtual const char* name() const override
    { return  "cost_event"; }
    virtual void execute() override
    {
      spell -> cost_event = new ( sim() ) cost_event_t( p(), spell, resource );
      p() -> resource_loss( resource, spell -> base_costs_per_tick[resource], spell -> gain );
    }
  };

  event_t* cost_event;

  warlock_spell_t( warlock_t* p, const std::string& n ):
    spell_t( n, p, p -> find_class_spell( n ) )
  {
    _init_warlock_spell_t();
  }

  warlock_spell_t( const std::string& token, warlock_t* p, const spell_data_t* s = spell_data_t::nil() ):
    spell_t( token, p, s )
  {
    _init_warlock_spell_t();
  }

  warlock_t* p()
  {
    return static_cast<warlock_t*>( player );
  }
  const warlock_t* p() const
  {
    return static_cast<warlock_t*>( player );
  }

  warlock_td_t* td( player_t* t ) const
  {
    return p() -> get_target_data( t );
  }

  bool use_havoc() const
  {
    if ( ! p() -> havoc_target || target == p() -> havoc_target || ! havoc_consume )
      return false;

    if ( p() -> buffs.havoc -> check() < havoc_consume )
      return false;

    return true;
  }

  bool use_backdraft() const
  {
    if ( ! backdraft_consume )
      return false;

    if ( p() -> buffs.backdraft -> check() < backdraft_consume )
      return false;

    return true;
  }

  virtual void init() override
  {
    spell_t::init();

    if ( havoc_consume > 0 )
    {
      havoc_proc = player -> get_proc( "Havoc: " + ( data().id() ? std::string( data().name_cstr() ) : name_str ) );
    }
  }

  virtual void reset() override
  {
    spell_t::reset();

    event_t::cancel( cost_event );
  }

  virtual int n_targets() const override
  {
    if ( aoe == 0 && use_havoc() )
      return 2;

    return spell_t::n_targets();
  }

  virtual std::vector< player_t* >& target_list() const override
  {
    if ( use_havoc() )
    {
      if ( ! target_cache.is_valid )
        available_targets( target_cache.list );

      havoc_targets.clear();
      if ( std::find( target_cache.list.begin(), target_cache.list.end(), target ) != target_cache.list.end() )
        havoc_targets.push_back( target );

      if ( ! p() -> havoc_target -> is_sleeping() &&
           std::find( target_cache.list.begin(), target_cache.list.end(), p() -> havoc_target ) != target_cache.list.end() )
           havoc_targets.push_back( p() -> havoc_target );
      return havoc_targets;
    }
    else
      return spell_t::target_list();
  }

  virtual double cost() const override
  {
    double c = spell_t::cost();

    if ( use_backdraft() && current_resource() == RESOURCE_MANA )
      c *= 1.0 + p() -> buffs.backdraft -> data().effectN( 1 ).percent();

    return c;
  }

  virtual void execute() override
  {
    spell_t::execute();

    if ( result_is_hit( execute_state -> result ) && p() -> talents.grimoire_of_synergy -> ok() )
    {
      pets::warlock_pet_t* my_pet = static_cast<pets::warlock_pet_t*>( p() -> pets.active ); //get active pet
      if ( my_pet != nullptr )
      {
        bool procced = p() -> grimoire_of_synergy.trigger();
        if ( procced ) my_pet -> buffs.demonic_synergy -> trigger();
      }
    }
  }

  virtual timespan_t execute_time() const override
  {
    timespan_t h = spell_t::execute_time();

    if ( use_backdraft() )
      h *= 1.0 + p() -> buffs.backdraft -> data().effectN( 1 ).percent();

    return h;
  }

  void consume_resource() override
  {
    spell_t::consume_resource();

    if ( use_havoc() )
    {
      havoc_proc -> occur();

      p() -> buffs.havoc -> decrement( havoc_consume );
      if ( p() -> buffs.havoc -> check() == 0 )
        p() -> havoc_target = nullptr;
    }

    if ( use_backdraft() )
      p() -> buffs.backdraft -> decrement( backdraft_consume );
  }

  virtual void tick( dot_t* d ) override
  {
    spell_t::tick( d );

    trigger_seed_of_corruption( td( d -> state -> target ), p(), d -> state -> result_amount );
  }

  virtual void impact( action_state_t* s ) override
  {
    spell_t::impact( s );

    trigger_seed_of_corruption( td( s -> target ), p(), s -> result_amount );
  }

  virtual double composite_target_multiplier( player_t* t ) const override
  {
    double m = 1.0;

    warlock_td_t* td = this -> td( t );

    // Contagion - change to effect 2 spelldata list
    if ( affected_by_contagion && td -> dots_unstable_affliction -> is_ticking() )
      m *= 1.0 + p() -> talents.contagion -> effectN( 1 ).percent();

    return spell_t::composite_target_multiplier( t ) * m;
  }

  virtual double action_multiplier() const override
  {
    double pm = spell_t::action_multiplier();

    return pm;
  }

  virtual resource_e current_resource() const override
  {
    return spell_t::current_resource();
  }

  virtual double composite_target_crit( player_t* target ) const override
  {
    double c = spell_t::composite_target_crit( target );
    if ( affected_by_flamelicked && p() -> destruction_trinket )
      c += td( target ) -> debuffs_flamelicked -> stack_value();

    return c;
  }

  void trigger_seed_of_corruption( warlock_td_t* td, warlock_t* p, double amount )
  {
    if ( ( ( td -> dots_seed_of_corruption -> current_action && id == td -> dots_seed_of_corruption -> current_action -> id )
      || td -> dots_seed_of_corruption -> is_ticking() ) && td -> soc_trigger > 0 )
    {
      td -> soc_trigger -= amount;
      if ( td -> soc_trigger <= 0 )
      {
        p -> spells.seed_of_corruption_aoe -> execute();
        td -> dots_seed_of_corruption -> cancel();
      }
    }
  }

  bool consume_cost_per_tick( const dot_t& dot ) override
  {
    bool consume = spell_t::consume_cost_per_tick( dot );

    resource_e r = current_resource();

    return consume;
  }

  void extend_dot( dot_t* dot, timespan_t extend_duration )
  {
    if ( dot -> is_ticking() )
    {
      //FIXME: This is roughly how it works, but we need more testing
      dot -> extend_duration( extend_duration, dot -> current_action -> dot_duration * 1.5 );
    }
  }

  static void trigger_soul_leech( warlock_t* p, double amount )
  {
    if ( p -> talents.soul_leech -> ok() )
    {
      p -> resource_gain( RESOURCE_HEALTH, amount, p -> gains.soul_leech );
    }
  }

  static void trigger_wild_imp( warlock_t* p )
  {
    for ( size_t i = 0; i < p -> pets.wild_imps.size() ; i++ )
    {
      if ( p -> pets.wild_imps[i] -> is_sleeping() )
      {
        p -> pets.wild_imps[i] -> trigger();
        p -> procs.wild_imp -> occur();
        return;
      }
    }
    p -> sim -> errorf( "Player %s ran out of wild imps.\n", p -> name() );
    assert( false ); // Will only get here if there are no available imps
  }
};

// Affliction Spells

struct agony_t: public warlock_spell_t
{
  agony_t( warlock_t* p ):
    warlock_spell_t( p, "Agony" )
  {
    may_crit = false;

    if ( p -> affliction_trinket )
    {
      const spell_data_t* data = p -> affliction_trinket -> driver();
      double period_value = data -> effectN( 1 ).average( p -> affliction_trinket -> item ) / 100.0;
      double duration_value = data -> effectN( 2 ).average( p -> affliction_trinket -> item ) / 100.0;

      base_tick_time *= 1.0 + period_value;
      dot_duration *= 1.0 + duration_value;
    }
  }

  virtual void last_tick( dot_t* d ) override
  {
    td( d -> state -> target ) -> agony_stack = 1;
    warlock_spell_t::last_tick( d );
  }

  virtual void tick( dot_t* d ) override
  {
    if ( p() -> talents.writhe_in_agony -> ok() && td( d -> state -> target ) -> agony_stack < ( 20 ) )
      td( d -> state -> target ) -> agony_stack++;
    else if ( td( d -> state -> target ) -> agony_stack < ( 10 ) ) 
      td( d -> state -> target ) -> agony_stack++;

    td( d -> target ) -> debuffs_agony -> trigger();

    if ( p() -> spec.nightfall -> ok() )
    {

      double nightfall_chance = p() -> spec.nightfall -> effectN( 1 ).percent() / 10;

      if ( rng().roll( nightfall_chance ) ) // Change to nightfall_chance once data exists
      {
          p() -> resource_gain( RESOURCE_SOUL_SHARD, 1, p() -> gains.nightfall );

        // If going from 0 to 1 shard was a surprise, the player would have to react to it
        if ( p() -> resources.current[RESOURCE_SOUL_SHARD] == 1 )
          p() -> shard_react = p() -> sim -> current_time() + p() -> total_reaction_time();
        else if ( p() -> resources.current[RESOURCE_SOUL_SHARD] >= 1 )
          p() -> shard_react = p() -> sim -> current_time();
        else
          p() -> shard_react = timespan_t::max();
      }
    }

    warlock_spell_t::tick( d );
  }

  double composite_target_multiplier( player_t* target ) const override
  {
    double m = warlock_spell_t::composite_target_multiplier( target );

    m *= td( target ) -> agony_stack;

    return m;
  }

  virtual double action_multiplier() const override
  {
    double m = warlock_spell_t::action_multiplier();

    if ( p() -> mastery_spells.potent_afflictions -> ok() )
      m *= 1.0 + p() -> cache.mastery_value();

    return m;
  }
};

struct unstable_affliction_t : public warlock_spell_t
{
  struct unstable_affliction_dot_t : public residual_action::residual_periodic_action_t <warlock_spell_t>
  {
    unstable_affliction_dot_t( warlock_t* p ) :
      base_t( "unstable_affliction", p, p -> spec.unstable_affliction )
    {
      dual = true;
    }
  };

  unstable_affliction_dot_t* ua_dot;

  unstable_affliction_t( warlock_t* p ) :
    warlock_spell_t( "unstable_affliction", p, p -> spec.unstable_affliction )
  {
    ua_dot = new unstable_affliction_dot_t( p );

    if ( p -> affliction_trinket )
    {
      const spell_data_t* data = p -> affliction_trinket -> driver();
      double period_value = data -> effectN( 1 ).average( p -> affliction_trinket -> item ) / 100.0;
      double duration_value = data -> effectN( 2 ).average( p -> affliction_trinket -> item ) / 100.0;

      base_tick_time *= 1.0 + period_value;
      dot_duration *= 1.0 + duration_value;
    }
  }

  virtual double action_multiplier() const override
  {
    double m = warlock_spell_t::action_multiplier();

    if ( p() -> mastery_spells.potent_afflictions -> ok() )
      m *= 1.0 + p() -> cache.mastery_value();

    return m;
  }

  virtual void impact( action_state_t* s ) override
  {
    if ( result_is_hit( s -> result ) )
    {
      const spell_data_t* ua_tick = p() -> find_spell( 30108 );
      double damage = s -> spell_power * ( ua_tick -> effectN( 3 ).sp_coeff() * ( ua_tick -> duration() / ua_tick -> effectN( 3 ).period() ) );

      residual_action::trigger( ua_dot, s -> target, damage );
    }
  }
};

struct corruption_t: public warlock_spell_t
{
  corruption_t( warlock_t* p ):
    warlock_spell_t( "Corruption", p, p -> find_spell( 172 ) ) //Use original corruption until DBC acts more friendly.
  {
    may_crit = false;
    dot_duration = data().effectN( 1 ).trigger() -> duration();
    spell_power_mod.tick = data().effectN( 1 ).trigger() -> effectN( 1 ).sp_coeff();
    base_tick_time = data().effectN( 1 ).trigger() -> effectN( 1 ).period();

    if ( p -> affliction_trinket )
    {
      const spell_data_t* data = p -> affliction_trinket ->  driver();
      double period_value = data -> effectN( 1 ).average( p -> affliction_trinket -> item ) / 100.0;
      double duration_value = data -> effectN( 2 ).average( p -> affliction_trinket -> item ) / 100.0;

      base_tick_time *= 1.0 + period_value;
      dot_duration *= 1.0 + duration_value;
    }

    if ( p -> talents.absolute_corruption -> ok() )
    {
      dot_duration = sim -> expected_iteration_time > timespan_t::zero() ?
        2 * sim -> expected_iteration_time :
        2 * sim -> max_time * ( 1.0 + sim -> vary_combat_length ); // "infinite" duration
    }
  }

  timespan_t travel_time() const override
  {
    return timespan_t::from_millis( 100 );
  }

  virtual double action_multiplier() const override
  {
    double m = warlock_spell_t::action_multiplier();

    if ( p() -> mastery_spells.potent_afflictions -> ok() )
      m *= 1.0 + p() -> cache.mastery_value();

    return m;
  }
};

struct drain_life_t: public warlock_spell_t
{
  drain_life_t( warlock_t* p ):
    warlock_spell_t( p, "Drain Life" )
  {
    channeled = true;
    hasted_ticks = false;
    may_crit = false;
  }

  void tick( dot_t* d ) override
  {
    spell_t::tick( d );

    trigger_seed_of_corruption( td( d -> state -> target ), p(), d -> state -> result_amount );
  }

  virtual bool ready() override
  {
    if ( p() -> talents.drain_soul -> ok() )
      return false;

    return warlock_spell_t::ready();
  }
};

struct life_tap_t: public warlock_spell_t
{
  life_tap_t( warlock_t* p ):
    warlock_spell_t( p, "Life Tap" )
  {
    harmful = false;
    ignore_false_positive = true;
  }

  virtual void execute() override
  {
    warlock_spell_t::execute();

    double health = player -> resources.max[RESOURCE_HEALTH];
    double mana = player -> resources.max[RESOURCE_MANA];

    player -> resource_loss( RESOURCE_HEALTH, health * data().effectN( 2 ).percent() );
    // FIXME run through resource usage
    player -> resource_gain( RESOURCE_MANA, mana * data().effectN( 1 ).percent(), p() -> gains.life_tap );
  }
};

struct doom_t: public warlock_spell_t
{
  doom_t( warlock_t* p ):
    warlock_spell_t( "doom", p, p -> spec.doom )
  {
    may_crit = false;
  }

  double action_multiplier() const override
  {
    double am = spell_t::action_multiplier();

    double mastery = p() -> cache.mastery();
    am *= 1.0 + mastery * p() -> mastery_spells.master_demonologist -> effectN( 3 ).mastery_value();

    return am;
  }

  virtual void tick( dot_t* d ) override
  {
    warlock_spell_t::tick( d );

    if ( d -> state -> result == RESULT_CRIT ) trigger_wild_imp( p() );

  }
};

struct havoc_t: public warlock_spell_t
{
  havoc_t( warlock_t* p ): warlock_spell_t( p, "Havoc" )
  {
    may_crit = false;
  }

  virtual void execute() override
  {
    warlock_spell_t::execute();

    p() -> buffs.havoc -> trigger( p() -> buffs.havoc -> max_stack() );
    p() -> havoc_target = execute_state -> target;
  }
};

struct hand_of_guldan_t: public warlock_spell_t
{

  double demonology_trinket_chance;

  hand_of_guldan_t( warlock_t* p ):
    warlock_spell_t( p, "Hand of Gul'dan" ),
    demonology_trinket_chance( 0.0 )
  {
    aoe = -1;

    cooldown -> duration = timespan_t::from_seconds( 15 );

    parse_effect_data( p -> find_spell( 86040 ) -> effectN( 1 ) );

    if ( p -> demonology_trinket && p -> specialization() == WARLOCK_DEMONOLOGY )
    {
      const spell_data_t* data = p -> find_spell( p -> demonology_trinket -> spell_id );
      demonology_trinket_chance = data -> effectN( 1 ).average( p -> demonology_trinket -> item );
      demonology_trinket_chance /= 100.0;
    }
  }

  virtual timespan_t travel_time() const override
  {
    return timespan_t::from_seconds( 1.5 );
  }

  virtual void execute() override
  {
    warlock_spell_t::execute();

    if ( p() -> demonology_trinket && p() -> rng().roll( demonology_trinket_chance ) )
    {
      trigger_wild_imp( p() );
      trigger_wild_imp( p() );
      trigger_wild_imp( p() );
      p() -> procs.fragment_wild_imp -> occur();
      p() -> procs.fragment_wild_imp -> occur();
      p() -> procs.fragment_wild_imp -> occur();
    }
  }
};

struct shadow_bolt_t: public warlock_spell_t
{
  hand_of_guldan_t* hand_of_guldan;
  shadow_bolt_t( warlock_t* p ):
    warlock_spell_t( p, "Shadow Bolt" ), hand_of_guldan( new hand_of_guldan_t( p ) )
  {
    base_multiplier *= 1.0 + p -> sets.set( SET_CASTER, T14, B2 ) -> effectN( 3 ).percent();
    hand_of_guldan               -> background = true;
    hand_of_guldan               -> base_costs[RESOURCE_MANA] = 0;
  }

  virtual void impact( action_state_t* s ) override
  {
    warlock_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      trigger_soul_leech( p(), s -> result_amount * p() -> talents.soul_leech -> effectN( 1 ).percent() );
    }
  }
};

struct immolate_t: public warlock_spell_t
{
  immolate_t* fnb;

  immolate_t( warlock_t* p ):
    warlock_spell_t( p, "Immolate" ),
    fnb( new immolate_t( "immolate", p, p -> find_spell( 108686 ) ) )
  {
    havoc_consume = 1;
    base_tick_time = p -> find_spell( 157736 ) -> effectN( 1 ).period();
    dot_duration = p -> find_spell( 157736 ) -> duration();
    spell_power_mod.tick = p -> spec.immolate -> effectN( 1 ).sp_coeff();
    hasted_ticks = true;
    tick_may_crit = true;
  }

  immolate_t( const std::string& n, warlock_t* p, const spell_data_t* spell ):
    warlock_spell_t( n, p, spell ),
    fnb( nullptr )
  {
    base_tick_time = p -> find_spell( 157736 ) -> effectN( 1 ).period();
    dot_duration = p -> find_spell( 157736 ) -> duration();
    hasted_ticks = true;
    tick_may_crit = true;
    spell_power_mod.tick = data().effectN( 1 ).sp_coeff();
    aoe = -1;
    stats = p -> get_stats( "immolate_fnb", this );
    gain = p -> get_gain( "immolate_fnb" );
  }

  void schedule_execute( action_state_t* state ) override
  {
    if ( fnb && p() -> buffs.fire_and_brimstone -> up() )
      fnb -> schedule_execute( state );
    else
      warlock_spell_t::schedule_execute( state );
  }

  double cost() const override
  {
    if ( fnb && p() -> buffs.fire_and_brimstone -> check() )
      return fnb -> cost();

    return warlock_spell_t::cost();
  }

  virtual double composite_crit() const override
  {
    double cc = warlock_spell_t::composite_crit();

    return cc;
  }

  virtual void tick( dot_t* d ) override
  {
    warlock_spell_t::tick( d );

      if ( d -> state -> result == RESULT_CRIT )
        p() -> resource_gain( RESOURCE_SOUL_SHARD, 1, p() -> gains.immolate );;
  }
};

struct conflagrate_t: public warlock_spell_t
{
  conflagrate_t* fnb;

  conflagrate_t( warlock_t* p ):
    warlock_spell_t( p, "Conflagrate" ),
    fnb( new conflagrate_t( "conflagrate", p, p -> find_spell( 108685 ) ) )
  {
    havoc_consume = 1;
  }

  conflagrate_t( const std::string& n, warlock_t* p, const spell_data_t* spell ):
    warlock_spell_t( n, p, spell ),
    fnb( nullptr )
  {
    aoe = -1;
    stats = p -> get_stats( "conflagrate_fnb", this );
    gain = p -> get_gain( "conflagrate_fnb" );
  }

  void schedule_execute( action_state_t* state ) override
  {
    if ( fnb && p() -> buffs.fire_and_brimstone -> up() )
      fnb -> schedule_execute( state );
    else
      warlock_spell_t::schedule_execute( state );
  }

  double cost() const override
  {
    if ( fnb && p() -> buffs.fire_and_brimstone -> check() )
      return fnb -> cost();

    return warlock_spell_t::cost();
  }

  void init() override
  {
    warlock_spell_t::init();

    cooldown -> duration = timespan_t::from_seconds( 12.0 );
    cooldown -> charges = 2;
  }

  void schedule_travel( action_state_t* s ) override
  {

    warlock_spell_t::schedule_travel( s );
  }

  void execute() override
  {
    warlock_spell_t::execute();

    if ( result_is_hit( execute_state -> result ) && p() -> talents.backdraft -> ok() )
      p() -> buffs.backdraft -> trigger( 3 );
  }

  virtual bool ready() override
  {
    if ( fnb && p() -> buffs.fire_and_brimstone -> check() )
      return fnb -> ready();
    return warlock_spell_t::ready();
  }
};

struct incinerate_t: public warlock_spell_t
{
  incinerate_t* fnb;
  // Normal incinerate
  incinerate_t( warlock_t* p ):
    warlock_spell_t( p, "Incinerate" ),
    fnb( new incinerate_t( "incinerate", p, p -> find_spell( 114654 ) ) )
  {
    havoc_consume = 1;
  }

  // Fire and Brimstone incinerate
  incinerate_t( const std::string& n, warlock_t* p, const spell_data_t* spell ):
    warlock_spell_t( n, p, spell ),
    fnb( nullptr )
  {
    aoe = -1;
    stats = p -> get_stats( "incinerate_fnb", this );
    gain = p -> get_gain( "incinerate_fnb" );
  }

  void init() override
  {
    warlock_spell_t::init();

    backdraft_consume = 1;
    base_multiplier *= 1.0 + p() -> sets.set( SET_CASTER, T14, B2 ) -> effectN( 2 ).percent();
  }

  void schedule_execute( action_state_t* state ) override
  {
    if ( fnb && p() -> buffs.fire_and_brimstone -> up() )
      fnb -> schedule_execute( state );
    else
      warlock_spell_t::schedule_execute( state );
  }

  double cost() const override
  {
    if ( fnb && p() -> buffs.fire_and_brimstone -> check() )
      return fnb -> cost();

    return warlock_spell_t::cost();
  }

  virtual double composite_crit() const override
  {
    double cc = warlock_spell_t::composite_crit();

    return cc;
  }

  void impact( action_state_t* s ) override
  {
    warlock_spell_t::impact( s );

    // TODO: FIXME
    /*
    gain_t* gain;
    if ( ! fnb && p() -> spec.fire_and_brimstone -> ok() )
      gain = p() -> gains.incinerate_fnb;
    else
      gain = p() -> gains.incinerate;
      */

    if ( result_is_hit( s -> result ) )
      trigger_soul_leech( p(), s -> result_amount * p() -> talents.soul_leech -> effectN( 1 ).percent() );

    if ( p() -> destruction_trinket )
    {
      td( s -> target ) -> debuffs_flamelicked -> trigger( 1 );
    }
  }

  virtual bool ready() override
  {
    if ( fnb && p() -> buffs.fire_and_brimstone -> check() )
      return fnb -> ready();
    return warlock_spell_t::ready();
  }
};

struct chaos_bolt_t: public warlock_spell_t
{
  chaos_bolt_t* fnb;
  chaos_bolt_t( warlock_t* p ):
    warlock_spell_t( p, "Chaos Bolt" ),
    fnb( new chaos_bolt_t( "chaos_bolt", p, p -> find_spell( 157701 ) ) )
  {
    havoc_consume = 3;
    backdraft_consume = 3;

    base_multiplier *= 1.0 + ( p -> sets.set( WARLOCK_DESTRUCTION, T18, B2 ) -> effectN( 2 ).percent() );
    base_execute_time += p -> sets.set( WARLOCK_DESTRUCTION, T18, B2 ) -> effectN( 1 ).time_value();

  }

  chaos_bolt_t( const std::string& n, warlock_t* p, const spell_data_t* spell ):
    warlock_spell_t( n, p, spell ),
    fnb( nullptr )
  {
    aoe = -1;
    backdraft_consume = 3;
    radius = 10;
    range = 40;

    base_multiplier *= 1.0 + ( p -> sets.set( WARLOCK_DESTRUCTION, T18, B2 ) -> effectN( 2 ).percent() );
    base_execute_time += ( p -> sets.set( WARLOCK_DESTRUCTION, T18, B2 ) -> effectN( 1 ).time_value() );

    stats = p -> get_stats( "chaos_bolt_fnb", this );
    gain = p -> get_gain( "chaos_bolt_fnb" );
  }

  void schedule_execute( action_state_t* state ) override
  {
    if ( fnb && p() -> buffs.fire_and_brimstone -> up() )
      fnb -> schedule_execute( state );
    else
      warlock_spell_t::schedule_execute( state );
  }

  void consume_resource() override
  {
    bool t18_procced = rng().roll( p() -> sets.set( WARLOCK_DESTRUCTION, T18, B4 ) -> effectN( 1 ).percent() );
    double base_cost = 0;

    if ( t18_procced )
    {
      base_cost = base_costs[ RESOURCE_SOUL_SHARD ];
      base_costs[ RESOURCE_SOUL_SHARD ] = p() -> buffs.fire_and_brimstone -> check() ? 1 : 0;
      p() -> procs.t18_4pc_destruction -> occur();
    }

    warlock_spell_t::consume_resource();

    if ( t18_procced )
    {
      base_costs[ RESOURCE_SOUL_SHARD ] = base_cost;
    }
  }

  // Force spell to always crit
  double composite_crit() const override
  {
    return 1.0;
  }

  // Record non-crit suppressed target-based crit% to state object
  double composite_target_crit( player_t* target ) const override
  {
    double c = warlock_spell_t::composite_target_crit( target );

    int level_delta = player -> level() - target -> level();
    if ( level_delta < 0 )
    {
      c += abs( level_delta ) / 100.0;
    }

    return c;
  }

  double calculate_direct_amount( action_state_t* state ) const override
  {
    warlock_spell_t::calculate_direct_amount( state );

    // Can't use player-based crit chance from the state object as it's hardcoded to 1.0. Use cached
    // player spell crit instead. The state target crit chance of the state object is correct.
    // Targeted Crit debuffs function as a separate multiplier.
    state -> result_total *= 1.0 + player -> cache.spell_crit() + state -> target_crit;

    return state -> result_total;
  }

  void multistrike_direct( const action_state_t* source_state, action_state_t* ms_state ) override
  {
    warlock_spell_t::multistrike_direct( source_state, ms_state );

    // Can't use player-based crit chance from the state object as it's hardcoded to 1.0. Use cached
    // player spell crit instead. The state target crit chance of the state object is correct.
    // Targeted Crit debuffs function as a separate multiplier.
    ms_state -> result_total *= 1.0 + player-> cache.spell_crit() + source_state -> target_crit;
    ms_state -> result_amount = ms_state -> result_total;
  }

  double cost() const override
  {
    double c = warlock_spell_t::cost();

    if ( fnb && p() -> buffs.fire_and_brimstone -> check() )
      return fnb -> cost();

    return c;
  }
};

// AOE SPELLS

struct seed_of_corruption_aoe_t: public warlock_spell_t
{
  seed_of_corruption_aoe_t( warlock_t* p ):
    warlock_spell_t( "seed_of_corruption_aoe", p, p -> find_spell( 27285 ) )
  {
    aoe = -1;
    dual = true;
    background = true;
    callbacks = false;
  }
};

struct seed_of_corruption_t: public warlock_spell_t
{
  seed_of_corruption_t( warlock_t* p ):
    warlock_spell_t( "seed_of_corruption", p, p -> find_spell( 27243 ) )
  {
    may_crit = false;

    if ( ! p -> spells.seed_of_corruption_aoe ) p -> spells.seed_of_corruption_aoe = new seed_of_corruption_aoe_t( p );
  }

  virtual void impact( action_state_t* s ) override
  {
    warlock_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      td( s -> target ) -> soc_trigger = s -> composite_spell_power() * data().effectN( 1 ).sp_coeff() * 3;
    }
  }
};

struct rain_of_fire_tick_t: public warlock_spell_t
{
  const spell_data_t& parent_data;

  rain_of_fire_tick_t( warlock_t* p, const spell_data_t& pd ):
    warlock_spell_t( "rain_of_fire_tick", p, pd.effectN( 2 ).trigger() ), parent_data( pd )
  {
    aoe = -1;
    background = true;
  }

  virtual proc_types proc_type() const override
  {
    return PROC1_PERIODIC;
  }
};

struct rain_of_fire_t: public warlock_spell_t
{
  rain_of_fire_t( warlock_t* p ):
    warlock_spell_t( "rain_of_fire", p, ( p -> specialization() == WARLOCK_DESTRUCTION ) ? p -> find_spell( 104232 ) : ( p -> specialization() == WARLOCK_AFFLICTION ) ? p -> find_spell( 5740 ) : spell_data_t::not_found() )
  {
    dot_behavior = DOT_CLIP;
    may_miss = false;
    may_crit = false;
    ignore_false_positive = true;

    tick_action = new rain_of_fire_tick_t( p, data() );
  }

  bool consume_cost_per_tick( const dot_t& dot ) override
  {
    if ( channeled )
      return false;
    return warlock_spell_t::consume_cost_per_tick( dot );
  }

  timespan_t composite_dot_duration( const action_state_t* state ) const override
  { return tick_time( state -> haste ) * ( data().duration() / base_tick_time ); }

  // TODO: Bring Back dot duration haste scaling ?

  virtual double composite_target_ta_multiplier( player_t* t ) const override
  {
    double m = warlock_spell_t::composite_target_ta_multiplier( t );

    if ( td( t ) -> dots_immolate -> is_ticking() )
      m *= 1.0 + data().effectN( 1 ).percent();

    return m;

  }
};

struct hellfire_tick_t: public warlock_spell_t
{
  hellfire_tick_t( warlock_t* p, const spell_data_t& s ):
    warlock_spell_t( "hellfire_tick", p, s.effectN( 1 ).trigger() )
  {
    aoe = -1;
    background = true;
  }
};

struct hellfire_t: public warlock_spell_t
{
  hellfire_t( warlock_t* p ):
    warlock_spell_t( p, "Hellfire" )
  {
    tick_zero = false;
    may_miss = false;
    channeled = true;
    may_crit = false;

    spell_power_mod.tick = base_td = 0;

    dynamic_tick_action = true;
    tick_action = new hellfire_tick_t( p, data() );
  }

  virtual bool usable_moving() const override
  {
    return true;
  }
};

// SUMMONING SPELLS

struct summon_pet_t: public warlock_spell_t
{
  timespan_t summoning_duration;
  std::string pet_name;
  pets::warlock_pet_t* pet;

private:
  void _init_summon_pet_t()
  {
    util::tokenize( pet_name );
    harmful = false;

    if ( data().ok() &&
         std::find( p() -> pet_name_list.begin(), p() -> pet_name_list.end(), pet_name ) ==
         p() -> pet_name_list.end() )
    {
      p() -> pet_name_list.push_back( pet_name );
    }
  }

public:
  summon_pet_t( const std::string& n, warlock_t* p, const std::string& sname = "" ):
    warlock_spell_t( p, sname.empty() ? "Summon " + n : sname ),
    summoning_duration( timespan_t::zero() ),
    pet_name( sname.empty() ? n : sname ), pet( nullptr )
  {
    _init_summon_pet_t();
  }

  summon_pet_t( const std::string& n, warlock_t* p, int id ):
    warlock_spell_t( n, p, p -> find_spell( id ) ),
    summoning_duration( timespan_t::zero() ),
    pet_name( n ), pet( nullptr )
  {
    _init_summon_pet_t();
  }

  summon_pet_t( const std::string& n, warlock_t* p, const spell_data_t* sd ):
    warlock_spell_t( n, p, sd ),
    summoning_duration( timespan_t::zero() ),
    pet_name( n ), pet( nullptr )
  {
    _init_summon_pet_t();
  }

  bool init_finished() override
  {
    pet = debug_cast<pets::warlock_pet_t*>( player -> find_pet( pet_name ) );
    return warlock_spell_t::init_finished();
  }

  virtual void execute() override
  {
    pet -> summon( summoning_duration );

    warlock_spell_t::execute();
  }

  bool ready() override
  {
    if ( ! pet )
    {
      return false;
    }

    return warlock_spell_t::ready();
  }
};

struct summon_main_pet_t: public summon_pet_t
{
  cooldown_t* instant_cooldown;

  summon_main_pet_t( const std::string& n, warlock_t* p ):
    summon_pet_t( n, p ), instant_cooldown( p -> get_cooldown( "instant_summon_pet" ) )
  {
    instant_cooldown -> duration = timespan_t::from_seconds( 60 );
    ignore_false_positive = true;
  }

  virtual void schedule_execute( action_state_t* state = nullptr ) override
  {
    warlock_spell_t::schedule_execute( state );

    if ( p() -> pets.active )
    {
      p() -> pets.active -> dismiss();
      p() -> pets.active = nullptr;
    }
  }

  virtual bool ready() override
  {
    if ( p() -> pets.active == pet )
      return false;

    if ( p() -> talents.demonic_servitude -> ok() ) //if we have the uberpets, we can't summon our standard pets
      return false;
    return summon_pet_t::ready();
  }

  virtual void execute() override
  {
    summon_pet_t::execute();

    p() -> pets.active = p() -> pets.last = pet;

    if ( p() -> buffs.demonic_power -> check() )
      p() -> buffs.demonic_power -> expire();
  }
};

struct infernal_awakening_t: public warlock_spell_t
{
  infernal_awakening_t( warlock_t* p, spell_data_t* spell ):
    warlock_spell_t( "infernal_awakening", p, spell )
  {
    aoe = -1;
    background = true;
    dual = true;
    trigger_gcd = timespan_t::zero();
  }
};

struct summon_infernal_t: public summon_pet_t
{
  infernal_awakening_t* infernal_awakening;

  summon_infernal_t( warlock_t* p ):
    summon_pet_t( "infernal", p ),
    infernal_awakening( nullptr )
  {
    harmful = false;

    cooldown = p -> cooldowns.infernal;
    cooldown -> duration = data().cooldown();

    if ( p -> talents.demonic_servitude -> ok() )
      summoning_duration = timespan_t::from_seconds( -1 );
    else
    {
      summoning_duration = p -> find_spell( 111685 ) -> duration();
      infernal_awakening = new infernal_awakening_t( p, data().effectN( 1 ).trigger() );
      infernal_awakening -> stats = stats;
    }
  }

  virtual void execute() override
  {
    summon_pet_t::execute();

    p() -> cooldowns.doomguard -> start();
    if ( infernal_awakening )
      infernal_awakening -> execute();
  }
};

struct summon_doomguard2_t: public summon_pet_t
{
  summon_doomguard2_t( warlock_t* p, spell_data_t* spell ):
    summon_pet_t( "doomguard", p, spell )
  {
    harmful = false;
    background = true;
    dual = true;
    callbacks = false;
    if ( p -> talents.demonic_servitude -> ok() ){
      summoning_duration = timespan_t::from_seconds( -1 );
    }
    else 
      summoning_duration = p -> find_spell( 60478 ) -> duration();
  }
};

struct summon_doomguard_t: public warlock_spell_t
{
  summon_doomguard2_t* summon_doomguard2;

  summon_doomguard_t( warlock_t* p ):
    warlock_spell_t( p, "Summon Doomguard" ),
    summon_doomguard2( nullptr )
  {
    cooldown = p -> cooldowns.doomguard;
    cooldown -> duration = data().cooldown();

    harmful = false;
    summon_doomguard2 = new summon_doomguard2_t( p, data().effectN( 2 ).trigger() );
    summon_doomguard2 -> stats = stats;
  }

  bool init_finished() override
  {
    if ( summon_doomguard2 -> pet )
    {
      summon_doomguard2 -> pet -> summon_stats = stats;
    }

    return warlock_spell_t::init_finished();
  }

  virtual void execute() override
  {
    warlock_spell_t::execute();

    p() -> cooldowns.infernal -> start();
    summon_doomguard2 -> execute();
  }
};

// TALENT SPELLS

struct shadowflame_t: public warlock_spell_t
{
  shadowflame_t( warlock_t* p ):
    warlock_spell_t( "shadowflame", p, p -> find_spell( 47960 ) )
  {
    background = true;
    may_miss = false;
    spell_power_mod.tick *= 0.8; // Check
  }

  virtual timespan_t travel_time() const override
  {
    return timespan_t::from_seconds( 1.5 );
  }

  double composite_target_multiplier( player_t* target ) const override
  {
    double m = warlock_spell_t::composite_target_multiplier( target );

    m *= td( target ) -> debuffs_shadowflame -> stack();

    return m;
  }

  virtual void last_tick( dot_t* d ) override
  {
    warlock_spell_t::last_tick( d );

    td ( d -> state -> target ) -> debuffs_shadowflame -> expire();
  }
};

struct drain_soul_t: public warlock_spell_t
{
  drain_soul_t( warlock_t* p ):
    warlock_spell_t( "drain_soul", p, p -> talents.drain_soul )
  {
    channeled = true;
    hasted_ticks = false;
    may_crit = false;
  }

  virtual bool ready() override
  {
   if ( !p() -> talents.drain_soul -> ok() ) 
      return false;

    return warlock_spell_t::ready();
  }
};

struct demonbolt_t: public warlock_spell_t
{
  demonbolt_t( warlock_t* p ):
    warlock_spell_t( "demonbolt", p, p -> talents.demonbolt )
  {
  }

  virtual bool ready() override
  {
    bool r = warlock_spell_t::ready();

    if ( !p() -> talents.demonbolt -> ok() ) 
      r = false;

    return r;
  }
};

struct cataclysm_t: public warlock_spell_t
{
  cataclysm_t( warlock_t* p ):
    warlock_spell_t( "cataclysm", p, p -> talents.cataclysm )
  {
    aoe = -1;
  }
  virtual bool ready() override
  {
    bool r = warlock_spell_t::ready();

    if ( !p() -> talents.cataclysm -> ok() ) r = false;

    return r;
  }
};

struct fire_and_brimstone_t: public warlock_spell_t
{
  fire_and_brimstone_t( warlock_t* p ):
    warlock_spell_t( "fire_and_brimstone", p, p -> talents.fire_and_brimstone )
  {
    harmful = false;
  }
};

struct shadowburn_t: public warlock_spell_t
{
  struct resource_event_t: public player_event_t
  {
    shadowburn_t* spell;
    gain_t* ember_gain;
    player_t* target;

    resource_event_t( warlock_t* p, shadowburn_t* s, player_t* t ):
      player_event_t( *p ), spell( s ), ember_gain( p -> gains.shadowburn_shard), target(t)
    {
      add_event( spell -> delay );
    }
    virtual const char* name() const override
    { return "shadowburn_execute_gain"; }
    virtual void execute() override
    {
      if ( target -> is_sleeping() )
      {
        p() -> resource_gain( RESOURCE_SOUL_SHARD, 2, ember_gain ); //TODO look up ember amount in shadowburn spell
      }
    }
  };

  resource_event_t* resource_event;
  timespan_t delay;
  shadowburn_t( warlock_t* p ):
    warlock_spell_t( "shadowburn", p, p -> talents.shadowburn ), resource_event( nullptr )
  {
    min_gcd = timespan_t::from_millis( 500 );
    havoc_consume = 1;
    delay = data().effectN( 1 ).trigger() -> duration();
  }

  virtual void impact( action_state_t* s ) override
  {
    warlock_spell_t::impact( s );

    resource_event = new ( *sim ) resource_event_t( p(), this, s -> target );
  }

  virtual double action_multiplier() const override
  {
    double m = warlock_spell_t::action_multiplier();

    if ( p() -> mastery_spells.emberstorm -> ok() )
      m *= 1.0 + p() -> cache.mastery_value();

    return m;
  }

  virtual bool ready() override
  {
    bool r = warlock_spell_t::ready();

    if ( target -> health_percentage() >= 20 ) 
      r = false;
    if ( !p() -> talents.shadowburn -> ok() )
      r = false;

    return r;
  }
};

struct haunt_t: public warlock_spell_t
{
  haunt_t( warlock_t* p ):
    warlock_spell_t( "haunt", p, p -> talents.haunt )
  {
  }

  virtual bool ready() override
  {
    if ( !p() -> talents.haunt -> ok() )
      return false;

    return warlock_spell_t::ready();
  }
};

struct phantom_singularity_tick_t : public warlock_spell_t
{
  phantom_singularity_tick_t( warlock_t* p ):
    warlock_spell_t( "phantom_singularity_tick", p, p -> find_spell( 205246 ) )
  {
    background = true;
    may_miss = false;
    dual = true;
    aoe = -1;
  }
};

struct phantom_singularity_t : public warlock_spell_t
{
  phantom_singularity_tick_t* phantom_singularity;

  phantom_singularity_t( warlock_t* p ):
    warlock_spell_t( "phantom_singularity", p, p -> talents.phantom_singularity )
  {
    ignore_false_positive = true;
    hasted_ticks = callbacks = false; // FIXME check for hasted ticks.

    phantom_singularity = new phantom_singularity_tick_t( p );
    add_child( phantom_singularity );
  }

  void tick( dot_t* d ) override
  {
    phantom_singularity -> execute();
    warlock_spell_t::tick( d );
  }

  virtual bool ready() override
  {
    if ( !p() -> talents.phantom_singularity -> ok() )
      return false;

    return warlock_spell_t::ready();
  }
};

struct mana_tap_t : public warlock_spell_t
{
  mana_tap_t( warlock_t* p ) :
    warlock_spell_t( "mana_tap", p, p -> talents.mana_tap )
  {
    harmful = false;
    ignore_false_positive = true;
  }

  void execute() override
  {
    warlock_spell_t::execute();

      p() -> buffs.mana_tap -> trigger();
      
      double mana = player -> resources.current[RESOURCE_MANA];

      player -> resource_loss( RESOURCE_MANA, mana * data().effectN( 2 ).percent(), p() -> gains.mana_tap );
  }

  virtual bool ready() override
  {
    if ( !p() -> talents.mana_tap -> ok() )
      return false;

    return warlock_spell_t::ready();
  }
};

struct siphon_life_t : public warlock_spell_t
{
  siphon_life_t(warlock_t* p) :
    warlock_spell_t( "siphon_life", p, p -> talents.siphon_life )
  {
    may_crit = false;
  }

  virtual bool ready() override
  {
    if ( !p() -> talents.siphon_life -> ok() )
      return false;

    return warlock_spell_t::ready();
  }
};

struct soul_harvest_t : public warlock_spell_t
{
  soul_harvest_t( warlock_t* p ) :
    warlock_spell_t( "soul_harvest", p, p -> talents.soul_harvest )
  {
    harmful = false;
  }

  virtual void execute() override
  {
    warlock_spell_t::execute();

    p() -> resource_gain( RESOURCE_SOUL_SHARD, 5, p() -> gains.soul_harvest );
  }

  virtual bool ready() override
  {
    if ( !p() -> talents.soul_harvest -> ok() )
      return false;

    return warlock_spell_t::ready();
  }
};

struct grimoire_of_sacrifice_t: public warlock_spell_t
{
  grimoire_of_sacrifice_t( warlock_t* p ):
    warlock_spell_t( "grimoire_of_sacrifice", p, p -> talents.grimoire_of_sacrifice )
  {
    harmful = false;
    ignore_false_positive = true;
  }

  virtual bool ready() override
  {
    if ( ! p() -> pets.active ) return false;

    return warlock_spell_t::ready();
  }

  virtual void execute() override
  {
    if ( p() -> pets.active )
    {
      warlock_spell_t::execute();

      p() -> pets.active -> dismiss();
      p() -> pets.active = nullptr;
      p() -> buffs.demonic_power -> trigger();

    }
  }
};

struct demonic_power_damage_t : public warlock_spell_t
{
  demonic_power_damage_t( warlock_t* p ) :
    warlock_spell_t( "demonic_power", p, p -> find_spell( 196100 ) )
  {
    background = true;
    proc = true;
  }
};

struct grimoire_of_service_t: public summon_pet_t
{
  grimoire_of_service_t( warlock_t* p, const std::string& pet_name ):
    summon_pet_t( "service_" + pet_name, p, p -> talents.grimoire_of_service -> ok() ? p -> find_class_spell( "Grimoire: " + pet_name ) : spell_data_t::not_found() )
  {
    cooldown = p -> get_cooldown( "grimoire_of_service" );
    cooldown -> duration = data().cooldown();
    summoning_duration = data().duration();
  }

  bool init_finished() override
  {
    if ( pet )
      pet -> summon_stats = stats;

    return summon_pet_t::init_finished();
  }
};

struct mortal_coil_heal_t: public warlock_heal_t
{
  mortal_coil_heal_t( warlock_t* p, const spell_data_t& s ):
    warlock_heal_t( "mortal_coil_heal", p, s.effectN( 3 ).trigger_spell_id() )
  {
    background = true;
    may_miss = false;
  }

  virtual void execute() override
  {
    double heal_pct = data().effectN( 1 ).percent();
    base_dd_min = base_dd_max = player -> resources.max[RESOURCE_HEALTH] * heal_pct;

    warlock_heal_t::execute();
  }
};

struct mortal_coil_t: public warlock_spell_t
{
  mortal_coil_heal_t* heal;

  mortal_coil_t( warlock_t* p ):
    warlock_spell_t( "mortal_coil", p, p -> talents.mortal_coil ), heal( nullptr )
  {
    havoc_consume = 1;
    base_dd_min = base_dd_max = 0;
    heal = new mortal_coil_heal_t( p, data() );
  }

  virtual void impact( action_state_t* s ) override
  {
    warlock_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
      heal -> execute();
  }
};

} // end actions namespace

warlock_td_t::warlock_td_t( player_t* target, warlock_t& p ):
actor_target_data_t( target, &p ),
agony_stack( 1 ),
soc_trigger( 0 ),
warlock( p )
{
  dots_corruption = target -> get_dot( "corruption", &p );
  dots_unstable_affliction = target -> get_dot( "unstable_affliction", &p );
  dots_agony = target -> get_dot( "agony", &p );
  dots_doom = target -> get_dot( "doom", &p );
  dots_drain_soul = target -> get_dot( "drain_soul", &p );
  dots_immolate = target -> get_dot( "immolate", &p );
  dots_shadowflame = target -> get_dot( "shadowflame", &p );
  dots_seed_of_corruption = target -> get_dot( "seed_of_corruption", &p );
  dots_phantom_singularity = target -> get_dot( "phantom_singularity", &p );
  
  debuffs_haunt = buff_creator_t( *this, "haunt", source -> find_class_spell( "Haunt" ) )
    .refresh_behavior( BUFF_REFRESH_PANDEMIC );
  debuffs_shadowflame = buff_creator_t( *this, "shadowflame", source -> find_spell( 47960 ) )
    .refresh_behavior( BUFF_REFRESH_PANDEMIC );
  debuffs_agony = buff_creator_t( *this, "agony", source -> find_spell( 980 ) )
    .refresh_behavior( BUFF_REFRESH_PANDEMIC );
  if ( warlock.destruction_trinket )
  {
    debuffs_flamelicked = buff_creator_t( *this, "flamelicked", warlock.destruction_trinket -> driver() -> effectN( 1 ).trigger() )
      .default_value( warlock.destruction_trinket -> driver() -> effectN( 1 ).trigger() -> effectN( 1 ).average( warlock.destruction_trinket -> item ) / 100.0 );
  }
  else
  {
    debuffs_flamelicked = buff_creator_t( *this, "flamelicked" )
      .chance( 0 );
  }

  target -> callbacks_on_demise.push_back( std::bind( &warlock_td_t::target_demise, this ) );
}

void warlock_td_t::target_demise()
{
  if ( warlock.specialization() == WARLOCK_AFFLICTION && dots_drain_soul -> is_ticking() )
  {
    if ( warlock.sim -> log )
    {
      warlock.sim -> out_debug.printf( "Player %s demised. Warlock %s gains a shard by channeling drain soul during this.", target -> name(), warlock.name() );
    }
    warlock.resource_gain( RESOURCE_SOUL_SHARD, 1, warlock.gains.drain_soul );
  }
}

warlock_t::warlock_t( sim_t* sim, const std::string& name, race_e r ):
  player_t( sim, WARLOCK, name, r ),
    havoc_target( nullptr ),
    double_nightfall( 0 ),
    pets( pets_t() ),
    talents( talents_t() ),
    glyphs( glyphs_t() ),
    mastery_spells( mastery_spells_t() ),
    demonic_power_rppm( *this ),
    grimoire_of_synergy( *this ),
    grimoire_of_synergy_pet( *this ),
    cooldowns( cooldowns_t() ),
    spec( specs_t() ),
    buffs( buffs_t() ),
    gains( gains_t() ),
    procs( procs_t() ),
    spells( spells_t() ),
    default_pet( "" ),
    shard_react( timespan_t::zero() ),
    initial_soul_shards( 1 ),
    affliction_trinket( nullptr ),
    demonology_trinket( nullptr ),
    destruction_trinket( nullptr )
{
  base.distance = 40;

  cooldowns.infernal = get_cooldown( "summon_infernal" );
  cooldowns.doomguard = get_cooldown( "summon_doomguard" );
  cooldowns.hand_of_guldan = get_cooldown( "hand_of_guldan" );

  regen_type = REGEN_DYNAMIC;
  regen_caches[CACHE_HASTE] = true;
  regen_caches[CACHE_SPELL_HASTE] = true;
}


double warlock_t::composite_player_multiplier( school_e school ) const
{
  double m = player_t::composite_player_multiplier( school );

  if ( buffs.demonic_synergy -> up() )
    m *= 1.0 + buffs.demonic_synergy -> data().effectN( 1 ).percent();

  if ( buffs.mana_tap -> up() )
    m *= 1.0 + talents.mana_tap -> effectN( 1 ).percent();

  return m;
}

void warlock_t::invalidate_cache( cache_e c )
{
  player_t::invalidate_cache( c );

  switch ( c )
  {
  case CACHE_MASTERY:
    if ( mastery_spells.master_demonologist -> ok() )
      player_t::invalidate_cache( CACHE_PLAYER_DAMAGE_MULTIPLIER );
    break;
  default: break;
  }
}

double warlock_t::composite_spell_crit() const
{
  double sc = player_t::composite_spell_crit();

  return sc;
}

double warlock_t::composite_spell_haste() const
{
  double h = player_t::composite_spell_haste();

  return h;
}

double warlock_t::composite_melee_crit() const
{
  double mc = player_t::composite_melee_crit();

  return mc;
}

double warlock_t::composite_mastery() const
{
  double m = player_t::composite_mastery();

  return m;
}

double warlock_t::composite_rating_multiplier( rating_e rating ) const
{
  double m = player_t::composite_rating_multiplier( rating );

  return m;
}

double warlock_t::resource_gain( resource_e resource_type, double amount, gain_t* source, action_t* action )
{

  return player_t::resource_gain( resource_type, amount, source, action );
}

double warlock_t::mana_regen_per_second() const
{
  double mp5 = player_t::mana_regen_per_second();

  mp5 /= cache.spell_haste();

  return mp5;
}

double warlock_t::composite_armor() const
{
  return player_t::composite_armor() + spec.fel_armor -> effectN( 2 ).base_value();
}

void warlock_t::halt()
{
  player_t::halt();

  if ( spells.melee ) spells.melee -> cancel();
}

double warlock_t::matching_gear_multiplier( attribute_e attr ) const
{
  if ( attr == ATTR_INTELLECT )
    return spec.nethermancy -> effectN( 1 ).percent();

  return 0.0;
}

action_t* warlock_t::create_action( const std::string& action_name,
                                    const std::string& options_str )
{
  action_t* a;

  if ( ( action_name == "summon_pet" || action_name == "service_pet" ) && default_pet.empty() )
  {
    sim -> errorf( "Player %s used a generic pet summoning action without specifying a default_pet.\n", name() );
    return nullptr;
  }

  using namespace actions;

  if      ( action_name == "conflagrate"           ) a = new           conflagrate_t( this );
  else if ( action_name == "corruption"            ) a = new            corruption_t( this );
  else if ( action_name == "agony"                 ) a = new                 agony_t( this );
  else if ( action_name == "demonbolt"             ) a = new             demonbolt_t( this );
  else if ( action_name == "doom"                  ) a = new                  doom_t( this );
  else if ( action_name == "chaos_bolt"            ) a = new            chaos_bolt_t( this );
  else if ( action_name == "drain_life"            ) a = new            drain_life_t( this );
  else if ( action_name == "drain_soul"            ) a = new            drain_soul_t( this );
  else if ( action_name == "grimoire_of_sacrifice" ) a = new grimoire_of_sacrifice_t( this );
  else if ( action_name == "haunt"                 ) a = new                 haunt_t( this );
  else if ( action_name == "phantom_singularity"   ) a = new   phantom_singularity_t( this );
  else if ( action_name == "soul_harvest"          ) a = new          soul_harvest_t( this );
  else if ( action_name == "siphon_life"           ) a = new           siphon_life_t( this );
  else if ( action_name == "immolate"              ) a = new              immolate_t( this );
  else if ( action_name == "incinerate"            ) a = new            incinerate_t( this );
  else if ( action_name == "life_tap"              ) a = new              life_tap_t( this );
  else if ( action_name == "mana_tap"              ) a = new              mana_tap_t( this );
  else if ( action_name == "mortal_coil"           ) a = new           mortal_coil_t( this );
  else if ( action_name == "shadow_bolt"           ) a = new           shadow_bolt_t( this );
  else if ( action_name == "shadowburn"            ) a = new            shadowburn_t( this );
  else if ( action_name == "unstable_affliction"   ) a = new   unstable_affliction_t( this );
  else if ( action_name == "hand_of_guldan"        ) a = new        hand_of_guldan_t( this );
  else if ( action_name == "havoc"                 ) a = new                 havoc_t( this );
  else if ( action_name == "seed_of_corruption"    ) a = new    seed_of_corruption_t( this );
  else if ( action_name == "cataclysm"             ) a = new             cataclysm_t( this );
  else if ( action_name == "rain_of_fire"          ) a = new          rain_of_fire_t( this );
  else if ( action_name == "hellfire"              ) a = new              hellfire_t( this );
  else if ( action_name == "fire_and_brimstone"    ) a = new    fire_and_brimstone_t( this );
  else if ( action_name == "summon_infernal"       ) a = new       summon_infernal_t( this );
  else if ( action_name == "summon_doomguard"      ) a = new      summon_doomguard_t( this );
  else if ( action_name == "summon_felhunter"      ) a = new summon_main_pet_t( "felhunter", this );
  else if ( action_name == "summon_felguard"       ) a = new summon_main_pet_t( "felguard", this );
  else if ( action_name == "summon_succubus"       ) a = new summon_main_pet_t( "succubus", this );
  else if ( action_name == "summon_voidwalker"     ) a = new summon_main_pet_t( "voidwalker", this );
  else if ( action_name == "summon_imp"            ) a = new summon_main_pet_t( "imp", this );
  else if ( action_name == "summon_pet"            ) a = new summon_main_pet_t( default_pet, this );
  else if ( action_name == "service_felguard"      ) a = new grimoire_of_service_t( this, "felguard" );
  else if ( action_name == "service_felhunter"     ) a = new grimoire_of_service_t( this, "felhunter" );
  else if ( action_name == "service_imp"           ) a = new grimoire_of_service_t( this, "imp" );
  else if ( action_name == "service_succubus"      ) a = new grimoire_of_service_t( this, "succubus" );
  else if ( action_name == "service_voidwalker"    ) a = new grimoire_of_service_t( this, "voidwalker" );
  else if ( action_name == "service_infernal"      ) a = new grimoire_of_service_t( this, "infernal" );
  else if ( action_name == "service_doomguard"     ) a = new grimoire_of_service_t( this, "doomguard" );
  else if ( action_name == "service_pet"           ) a = new grimoire_of_service_t( this,  talents.demonic_servitude -> ok() ? "doomguard" : default_pet );
  else return player_t::create_action( action_name, options_str );

  a -> parse_options( options_str );

  return a;
}

pet_t* warlock_t::create_pet( const std::string& pet_name,
                              const std::string& /* pet_type */ )
{
  pet_t* p = find_pet( pet_name );

  if ( p ) return p;

  using namespace pets;

  if ( pet_name == "felguard"     ) return new    felguard_pet_t( sim, this );
  if ( pet_name == "felhunter"    ) return new   felhunter_pet_t( sim, this );
  if ( pet_name == "imp"          ) return new         imp_pet_t( sim, this );
  if ( pet_name == "succubus"     ) return new    succubus_pet_t( sim, this );
  if ( pet_name == "voidwalker"   ) return new  voidwalker_pet_t( sim, this );
  if ( pet_name == "infernal"     ) return new    infernal_pet_t( sim, this );
  if ( pet_name == "doomguard"    ) return new   doomguard_pet_t( sim, this );

  if ( pet_name == "service_felguard"     ) return new    felguard_pet_t( sim, this, pet_name );
  if ( pet_name == "service_felhunter"    ) return new   felhunter_pet_t( sim, this, pet_name );
  if ( pet_name == "service_imp"          ) return new         imp_pet_t( sim, this, pet_name );
  if ( pet_name == "service_succubus"     ) return new    succubus_pet_t( sim, this, pet_name );
  if ( pet_name == "service_voidwalker"   ) return new  voidwalker_pet_t( sim, this, pet_name );
  if ( pet_name == "service_doomguard"    ) return new   doomguard_pet_t( sim, this, pet_name );
  if ( pet_name == "service_infernal"     ) return new    infernal_pet_t( sim, this, pet_name );

  return nullptr;
}

void warlock_t::create_pets()
{
  for ( size_t i = 0; i < pet_name_list.size(); ++i )
  {
    create_pet( pet_name_list[ i ] );
  }

  if ( specialization() == WARLOCK_DEMONOLOGY )
  {
    for ( size_t i = 0; i < pets.wild_imps.size(); i++ )
    {
      pets.wild_imps[ i ] = new pets::wild_imp_pet_t( sim, this );
      if ( i > 0 )
        pets.wild_imps[ i ] -> quiet = 1;
    }
    if ( sets.has_set_bonus( WARLOCK_DEMONOLOGY, T18, B4 ) )
    {
      for ( size_t i = 0; i < pets.t18_illidari_satyr.size(); i++ )
      {
        pets.t18_illidari_satyr[i] = new pets::t18_illidari_satyr_t( sim, this );
      }
      for ( size_t i = 0; i < pets.t18_prince_malchezaar.size(); i++ )
      {
        pets.t18_prince_malchezaar[i] = new pets::t18_prince_malchezaar_t( sim, this );
      }
      for ( size_t i = 0; i < pets.t18_vicious_hellhound.size(); i++ )
      {
        pets.t18_vicious_hellhound[i] = new pets::t18_vicious_hellhound_t( sim, this );
      }
    }
  }
}

void warlock_t::init_spells()
{
  player_t::init_spells();

  // General
  spec.fel_armor   = find_spell( 104938 );
  spec.nethermancy = find_spell( 86091 );

  // Spezialization Spells
  spec.immolate               = find_specialization_spell( "Immolate" );
  spec.nightfall              = find_specialization_spell( "Nightfall" );
  spec.wild_imps              = find_specialization_spell( "Wild Imps" );
  spec.unstable_affliction    = find_specialization_spell( "Unstable Affliction" );

  // Removed terniary for compat.
  spec.doom                   = find_spell( 603 );

  // Mastery
  mastery_spells.emberstorm          = find_mastery_spell( WARLOCK_DESTRUCTION );
  mastery_spells.potent_afflictions  = find_mastery_spell( WARLOCK_AFFLICTION );
  mastery_spells.master_demonologist = find_mastery_spell( WARLOCK_DEMONOLOGY );

  // Talents

  talents.haunt                 = find_talent_spell( "Haunt" );
  talents.writhe_in_agony       = find_talent_spell( "Writhe in Agony" );
  talents.drain_soul            = find_talent_spell( "Drain Soul" );

  talents.backdraft             = find_talent_spell( "Backdraft" );
  talents.fire_and_brimstone    = find_talent_spell( "Fire and Brimstone" );
  talents.shadowburn            = find_talent_spell( "Shadowburn" );

  talents.contagion             = find_talent_spell( "Contagion" );
  talents.absolute_corruption   = find_talent_spell( "Absolute Corruption" );
  talents.mana_tap              = find_talent_spell( "Mana Tap" );

  talents.soul_leech            = find_talent_spell( "Soul Leech" );
  talents.mortal_coil           = find_talent_spell( "Mortal Coil" );
  talents.howl_of_terror        = find_talent_spell( "Howl of Terror" );

  talents.siphon_life           = find_talent_spell( "Siphon Life" );
  talents.sow_the_seeds         = find_talent_spell( "Sow the Seeds" );
  talents.soul_harvest          = find_talent_spell( "Soul Harvest" );

  talents.demonic_circle        = find_talent_spell( "Demonic Circle" );
  talents.burning_rush          = find_talent_spell( "Burning Rush" );
  talents.dark_pact             = find_talent_spell( "Dark Pact" );

  talents.grimoire_of_supremacy = find_talent_spell( "Grimoire of Supremacy" );
  talents.grimoire_of_service   = find_talent_spell( "Grimoire of Service" );
  talents.grimoire_of_sacrifice = find_talent_spell( "Grimoire of Sacrifice" );
  talents.grimoire_of_synergy   = find_talent_spell( "Grimoire of Synergy" );

  talents.soul_effigy           = find_talent_spell( "Soul Effigy" );
  talents.phantom_singularity   = find_talent_spell( "Phantom Singularity" );
  talents.demonic_servitude     = find_talent_spell( "Demonic Servitude" );

  talents.demonbolt             = find_talent_spell( "Demonbolt" );
  talents.cataclysm             = find_talent_spell( "Cataclysm" );
  talents.shadowfury            = find_talent_spell( "Shadowfury" );
  
  // Glyphs
}

void warlock_t::init_base_stats()
{
  player_t::init_base_stats();

  base.attack_power_per_strength = 0.0;
  base.attack_power_per_agility = 0.0;
  base.spell_power_per_intellect = 1.0;

  base.attribute_multiplier[ATTR_STAMINA] *= 1.0 + spec.fel_armor -> effectN( 1 ).percent();

  base.mana_regen_per_second = resources.base[RESOURCE_MANA] * 0.01;

  resources.base[RESOURCE_SOUL_SHARD] = 5;

  if ( default_pet.empty() )
  {
    if ( specialization() == WARLOCK_DEMONOLOGY )
      default_pet = "felguard";
    else
      default_pet = "felhunter";
  }
}

void warlock_t::init_scaling()
{
  player_t::init_scaling();
}

struct havoc_buff_t : public buff_t
{
  havoc_buff_t( warlock_t* p ) :
    buff_t( buff_creator_t( p, "havoc", p -> find_specialization_spell( "Havoc" ) ).cd( timespan_t::zero() ) )
  { }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    buff_t::expire_override( expiration_stacks, remaining_duration );

    if ( remaining_duration == timespan_t::zero() )
    {
      debug_cast<warlock_t*>( player ) -> procs.havoc_waste -> occur();
    }
  }
};

void warlock_t::create_buffs()
{
  player_t::create_buffs();

  buffs.backdraft = buff_creator_t( this, "backdraft", talents.backdraft -> effectN( 1 ).trigger() );
  buffs.demonic_power = buff_creator_t( this, "demonic_power", talents.grimoire_of_sacrifice -> effectN( 2 ).trigger() );
  buffs.demonic_synergy = buff_creator_t( this, "demonic_synergy", find_spell( 171982 ) )
    .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
    .chance( 1 );
  buffs.mana_tap = buff_creator_t( this, "mana_tap", talents.mana_tap )
    .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
    .refresh_behavior( BUFF_REFRESH_PANDEMIC );
  buffs.havoc = new havoc_buff_t( this );

  buffs.tier18_2pc_demonology = buff_creator_t( this, "demon_rush", sets.set( WARLOCK_DEMONOLOGY, T18, B2 ) -> effectN( 1 ).trigger() )
    .default_value( sets.set( WARLOCK_DEMONOLOGY, T18, B2 ) -> effectN( 1 ).trigger() -> effectN( 1 ).percent() );
}

void warlock_t::init_rng()
{
  player_t::init_rng();
  demonic_power_rppm.set_frequency( find_spell( 196099 ) -> real_ppm() );
  grimoire_of_synergy.set_frequency( find_spell( 171975 ) -> real_ppm() );
  grimoire_of_synergy_pet.set_frequency( find_spell( 171975 ) -> real_ppm() );
}

void warlock_t::init_gains()
{
  player_t::init_gains();

  gains.life_tap            = get_gain( "life_tap" );
  gains.soul_leech          = get_gain( "soul_leech" );
  gains.nightfall           = get_gain( "nightfall" );
  gains.conflagrate         = get_gain( "conflagrate" );
  gains.immolate            = get_gain( "immolate" );
  gains.shadowburn_shard    = get_gain( "shadowburn_shard" );
  gains.miss_refund         = get_gain( "miss_refund" );
  gains.seed_of_corruption  = get_gain( "seed_of_corruption" );
  gains.drain_soul          = get_gain( "drain_soul" );
  gains.soul_harvest        = get_gain( "soul_harvest" );
  gains.mana_tap            = get_gain( "mana_tap" );
}

// warlock_t::init_procs ===============================================

void warlock_t::init_procs()
{
  player_t::init_procs();

  procs.wild_imp = get_proc( "wild_imp" );
  procs.havoc_waste = get_proc( "Havoc: Buff expiration" );
  procs.fragment_wild_imp = get_proc( "fragment_wild_imp" );
  procs.t18_4pc_destruction = get_proc( "t18_4pc_destruction" );
  procs.t18_prince_malchezaar = get_proc( "t18_prince_malchezaar" );
  procs.t18_vicious_hellhound = get_proc( "t18_vicious_hellhound" );
  procs.t18_illidari_satyr = get_proc( "t18_illidari_satyr" );
}

void warlock_t::apl_precombat()
{
  std::string& precombat_list =
    get_action_priority_list( "precombat" )->action_list_str;

  if ( sim-> allow_flasks )
  {
    // Flask
    if ( true_level == 100 )
      precombat_list = "flask,type=greater_draenic_intellect_flask";
    else if ( true_level >= 85 )
      precombat_list = "flask,type=warm_sun";
  }

  if ( sim -> allow_food )
  {
    // Food
    if ( level() == 100 && specialization() == WARLOCK_DESTRUCTION )
      precombat_list += "/food,type=pickled_eel";
    else if ( level() == 100 && specialization() == WARLOCK_DEMONOLOGY)
      precombat_list += "/food,type=sleeper_sushi";
    else if ( level() == 100 && specialization() == WARLOCK_AFFLICTION )
      precombat_list += "/food,type=felmouth_frenzy";
    else if ( level() >= 85 )
      precombat_list += "/food,type=mogu_fish_stew";
  }

  if ( sim -> allow_potions )
  {
    // Pre-potion
  if ( true_level == 100 )
    precombat_list += "/potion,name=draenic_intellect";
  else if ( true_level >= 85 )
    precombat_list += "/potion,name=jade_serpent";
  }

  action_list_str += init_use_profession_actions();

  for ( int i = as< int >( items.size() ) - 1; i >= 0; i-- )
  {
    if ( items[i].has_special_effect( SPECIAL_EFFECT_SOURCE_NONE, SPECIAL_EFFECT_USE ) )
    {
        action_list_str += "/use_item,name=";
        action_list_str += items[i].name();
    }
  }
}

void warlock_t::apl_global_filler()
{
  add_action( "Life Tap" );
}

void warlock_t::apl_default()
{
}

void warlock_t::apl_affliction()
{
  
}

void warlock_t::apl_demonology()
{

}

void warlock_t::apl_destruction()
{
  action_priority_list_t* single_target       = get_action_priority_list( "single_target" );    
  action_priority_list_t* aoe                 = get_action_priority_list( "aoe" );
}

void warlock_t::init_action_list()
{
  if ( action_list_str.empty() )
  {
    clear_action_priority_lists();

    apl_precombat();

    switch ( specialization() )
    {
    case WARLOCK_AFFLICTION:
      apl_affliction();
      break;
    case WARLOCK_DESTRUCTION:
      apl_destruction();
      break;
    case WARLOCK_DEMONOLOGY:
      apl_demonology();
      break;
    default:
      apl_default();
      break;
    }

    apl_global_filler();

    use_default_action_list = true;
  }

  player_t::init_action_list();
}

void warlock_t::init_resources( bool force )
{
  player_t::init_resources( force );

  resources.current[RESOURCE_SOUL_SHARD] = initial_soul_shards;

  if ( pets.active )
    pets.active -> init_resources( force );
}

void warlock_t::combat_begin()
{

  player_t::combat_begin();
}

void warlock_t::reset()
{
  player_t::reset();

  for ( size_t i = 0; i < sim -> actor_list.size(); i++ )
  {
    warlock_td_t* td = target_data[sim -> actor_list[i]];
    if ( td ) td -> reset();
  }

  pets.active = nullptr;
  shard_react = timespan_t::zero();
  havoc_target = nullptr;
  double_nightfall = 0;

  demonic_power_rppm.reset();
  grimoire_of_synergy.reset();
  grimoire_of_synergy_pet.reset();
}

void warlock_t::create_options()
{
  player_t::create_options();

  add_option( opt_int( "soul_shards", initial_soul_shards ) );
  add_option( opt_string( "default_pet", default_pet ) );
}

std::string warlock_t::create_profile( save_e stype )
{
  std::string profile_str = player_t::create_profile( stype );

  if ( stype == SAVE_ALL )
  {
    if ( initial_soul_shards != 1 )    profile_str += "soul_shards=" + util::to_string( initial_soul_shards ) + "\n";
    if ( ! default_pet.empty() )       profile_str += "default_pet=" + default_pet + "\n";
  }

  return profile_str;
}

void warlock_t::copy_from( player_t* source )
{
  player_t::copy_from( source );

  warlock_t* p = debug_cast<warlock_t*>( source );

  initial_soul_shards = p -> initial_soul_shards;
  default_pet = p -> default_pet;
}

// warlock_t::convert_hybrid_stat ==============================================

stat_e warlock_t::convert_hybrid_stat( stat_e s ) const
{
  // this converts hybrid stats that either morph based on spec or only work
  // for certain specs into the appropriate "basic" stats
  switch ( s )
  {
    // This is all a guess at how the hybrid primaries will work, since they
    // don't actually appear on cloth gear yet. TODO: confirm behavior
  case STAT_STR_AGI_INT:
  case STAT_AGI_INT:
  case STAT_STR_INT:
    return STAT_INTELLECT;
  case STAT_STR_AGI:
    return STAT_NONE;
  case STAT_SPIRIT:
    return STAT_NONE;
  case STAT_BONUS_ARMOR:
    return STAT_NONE;
  default: return s;
  }
}

expr_t* warlock_t::create_expression( action_t* a, const std::string& name_str )
{
  if ( name_str == "shard_react" )
  {
    struct shard_react_expr_t: public expr_t
    {
      warlock_t& player;
      shard_react_expr_t( warlock_t& p ):
        expr_t( "shard_react" ), player( p ) { }
      virtual double evaluate() override { return player.resources.current[RESOURCE_SOUL_SHARD] >= 1 && player.sim -> current_time() >= player.shard_react; }
    };
    return new shard_react_expr_t( *this );
  }
  else if ( name_str == "felstorm_is_ticking" )
  {
    struct felstorm_is_ticking_expr_t: public expr_t
    {
      pets::warlock_pet_t* felguard;
      felstorm_is_ticking_expr_t( pets::warlock_pet_t* f ):
        expr_t( "felstorm_is_ticking" ), felguard( f ) { }
      virtual double evaluate() override { return ( felguard ) ? felguard -> special_action -> get_dot() -> is_ticking() : false; }
    };
    return new felstorm_is_ticking_expr_t( debug_cast<pets::warlock_pet_t*>( find_pet( "felguard" ) ) );
  }
  else
  {
    return player_t::create_expression( a, name_str );
  }
}


/* Report Extension Class
 * Here you can define class specific report extensions/overrides
 */
class warlock_report_t: public player_report_extension_t
{
public:
  warlock_report_t( warlock_t& player ):
    p( player )
  {

  }

  virtual void html_customsection( report::sc_html_stream& /* os*/ ) override
  {
    (void)p;
    /*// Custom Class Section
    os << "\t\t\t\t<div class=\"player-section custom_section\">\n"
    << "\t\t\t\t\t<h3 class=\"toggle open\">Custom Section</h3>\n"
    << "\t\t\t\t\t<div class=\"toggle-content\">\n";

    os << p.name();

    os << "\t\t\t\t\t\t</div>\n" << "\t\t\t\t\t</div>\n";*/
  }
private:
  warlock_t& p;
};

// WARLOCK MODULE INTERFACE =================================================

struct warlock_module_t: public module_t
{
  warlock_module_t(): module_t( WARLOCK ) {}

  virtual player_t* create_player( sim_t* sim, const std::string& name, race_e r = RACE_NONE ) const override
  {
    auto  p = new warlock_t( sim, name, r );
    p -> report_extension = std::unique_ptr<player_report_extension_t>( new warlock_report_t( *p ) );
    return p;
  }

  virtual void static_init() const override
  {
    unique_gear::register_special_effect( 184922, affliction_trinket);
    unique_gear::register_special_effect( 184923, demonology_trinket);
    unique_gear::register_special_effect( 184924, destruction_trinket);
  }

  virtual void register_hotfixes() const override
  {
  }

  virtual bool valid() const override { return true; }
  virtual void init( player_t* ) const override {}
  virtual void combat_begin( sim_t* ) const override {}
  virtual void combat_end( sim_t* ) const override {}
};

} // end unnamed namespace

const module_t* module_t::warlock()
{
  static warlock_module_t m;
  return &m;
}
