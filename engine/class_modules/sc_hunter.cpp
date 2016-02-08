//==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

namespace
{ // UNNAMED NAMESPACE

// ==========================================================================
// Hunter
// ==========================================================================

struct hunter_t;

namespace pets
{
struct hunter_main_pet_t;
}

enum aspect_type { ASPECT_NONE = 0, ASPECT_MAX };

enum exotic_munitions { NO_AMMO = 0, FROZEN_AMMO = 2, INCENDIARY_AMMO = 4, POISONED_AMMO = 8 };

struct hunter_td_t: public actor_target_data_t
{
  buff_t* debuffs_pvp_surv_4p_black_fire;
  buff_t* debuffs_survival_t18_2p;
  struct dots_t
  {
    dot_t* serpent_sting;
    dot_t* poisoned_ammo;
  } dots;

  hunter_td_t( player_t* target, hunter_t* p );
};

struct hunter_t: public player_t
{
public:
  timespan_t movement_ended;

  // Active
  std::vector<pets::hunter_main_pet_t*> hunter_main_pets;
  struct actives_t
  {
    pets::hunter_main_pet_t* pet;
    aspect_type         aspect;
    exotic_munitions    ammo;
    action_t*           serpent_sting;
    action_t*           frozen_ammo;
    action_t*           incendiary_ammo;
    action_t*           poisoned_ammo;
  } active;

  // Secondary pets
  std::array<pet_t*, 2>  pet_dire_beasts;
  // Tier 15 2-piece bonus: need 10 slots (just to be safe) because each
  // Steady Shot or Cobra Shot can trigger a Thunderhawk, which stays
  // for 10 seconds.
  std::array< pet_t*, 10 > thunderhawk;

  // Tier 15 4-piece bonus
  attack_t* action_lightning_arrow_aimed_shot;
  attack_t* action_lightning_arrow_arcane_shot;
  attack_t* action_lightning_arrow_multi_shot;

  // Tier 18 (WoD 6.2) trinket effects
  const special_effect_t* beastlord;
  const special_effect_t* longview;
  const special_effect_t* blackness;

  // Buffs
  struct buffs_t
  {
    buff_t* aspect_of_the_fox;
    buff_t* aspect_of_the_pack;
    buff_t* beast_cleave;
    buff_t* bestial_wrath;
    buff_t* bombardment;
    buff_t* careful_aim;
    buff_t* cobra_strikes;
    buff_t* focus_fire;
    buff_t* steady_focus;
    buff_t* pre_steady_focus;
    buff_t* lock_and_load;
    buff_t* thrill_of_the_hunt;
    buff_t* stampede;
    buff_t* rapid_fire;
    buff_t* sniper_training;
    buff_t* tier13_4pc;
    buff_t* tier16_2pc;
    buff_t* tier16_4pc;
    buff_t* tier16_4pc_mm_keen_eye;
    buff_t* tier16_4pc_bm_brutal_kinship;
    buff_t* heavy_shot; // t17 SV 4pc
    buff_t* t18_2p_rapid_fire;
  } buffs;

  // Cooldowns
  struct cooldowns_t
  {
    cooldown_t* explosive_shot;
    cooldown_t* black_arrow;
    cooldown_t* bestial_wrath;
    cooldown_t* kill_shot_reset;
    cooldown_t* rapid_fire;
  } cooldowns;

  // Custom Parameters
  std::string summon_pet_str;

  // Gains
  struct gains_t
  {
    gain_t* invigoration;
    gain_t* focus_fire;
    gain_t* thrill_of_the_hunt;
    gain_t* steady_shot;
    gain_t* steady_focus;
    gain_t* focusing_shot;
    gain_t* cobra_shot;
    gain_t* aimed_shot;
    gain_t* dire_beast;
  } gains;

  // Procs
  struct procs_t
  {
    proc_t* invigoration;
    proc_t* thrill_of_the_hunt;
    proc_t* lock_and_load;
    proc_t* tier15_2pc_melee;
    proc_t* tier15_4pc_melee_aimed_shot;
    proc_t* tier15_4pc_melee_arcane_shot;
    proc_t* tier15_4pc_melee_multi_shot;
    proc_t* tier16_2pc_melee;
    proc_t* tier16_4pc_melee;
    proc_t* tier17_2pc_bm;
    proc_t* black_arrow_trinket_reset;
    proc_t* tier18_2pc_mm_wasted_proc;
    proc_t* tier18_2pc_mm_wasted_overwrite;
  } procs;

  real_ppm_t ppm_tier15_2pc_melee;
  real_ppm_t ppm_tier15_4pc_melee;

  // Talents
  struct talents_t
  {
    // tier 1
    const spell_data_t* one_with_the_pack;
    const spell_data_t* way_of_the_cobra;
    const spell_data_t* dire_stable;
    
    const spell_data_t* black_arrow;
    const spell_data_t* momentum;
    const spell_data_t* true_aim;

    const spell_data_t* animal_instincts;
    const spell_data_t* improved_traps;
    const spell_data_t* way_of_moknathal;

    // tier 2
    const spell_data_t* posthaste;
    const spell_data_t* farstrider;
    const spell_data_t* narrow_escape;
    const spell_data_t* dash;

    // tier 3
    const spell_data_t* binding_shot;
    const spell_data_t* wyvern_sting;
    const spell_data_t* intimidation;
    const spell_data_t* camouflage;
    
    const spell_data_t* caltrops;
    const spell_data_t* steel_trap;
    const spell_data_t* rangers_net;

    // tier 4
    const spell_data_t* stomp;
    const spell_data_t* exotic_munitions;
    const spell_data_t* chimaera_shot;

    const spell_data_t* piercing_shots;
    const spell_data_t* broadside;

    const spell_data_t* sticky_bomb;
    // const spell_data_t* camouflage;
    const spell_data_t* throwing_axes;
    
    // tier 5
    const spell_data_t* big_game_hunter;
    const spell_data_t* bestial_fury;
    const spell_data_t* blink_strikes;
    
    const spell_data_t* light_em_up;
    const spell_data_t* steady_focus;
    const spell_data_t* headshot;

    const spell_data_t* thrill_of_the_hunt;
    const spell_data_t* dragonsfire_trap;
    const spell_data_t* snake_hunter;

    // tier 6
    const spell_data_t* a_murder_of_crows;
    const spell_data_t* barrage;
    const spell_data_t* volley;

    const spell_data_t* butchery;
    const spell_data_t* mortal_wounds;
    const spell_data_t* serpent_sting;

    // tier 7
    const spell_data_t* stampede;
    const spell_data_t* killer_cobra;
    const spell_data_t* aspect_of_the_beast;

    const spell_data_t* dark_ranger;
    const spell_data_t* kill_flip;
    const spell_data_t* lock_and_load;

    const spell_data_t* sentry_turrent;
    const spell_data_t* expert_trapper;
  } talents;

  // Perks
  struct
  {
    const spell_data_t* enhanced_camouflage;

    //Beast Mastery
    const spell_data_t* improved_focus_fire;
    const spell_data_t* improved_beast_cleave;
    const spell_data_t* enhanced_basic_attacks;
    // Marks
    const spell_data_t* enhanced_kill_shot;
    const spell_data_t* enhanced_aimed_shot;
    const spell_data_t* improved_focus;
    // Survival
    const spell_data_t* empowered_explosive_shot;
    const spell_data_t* enhanced_traps;
    const spell_data_t* enhanced_entrapment;
  } perks;

  // Specialization Spells
  struct specs_t
  {
    // Baseline
    const spell_data_t* trueshot_aura;
    const spell_data_t* critical_strikes;

    // Shared
    const spell_data_t* lethal_shots;
    const spell_data_t* lightning_reflexes;
    const spell_data_t* animal_handler;

    const spell_data_t* exhilaration;

    // Beast Mastery
    const spell_data_t* cobra_shot;
    const spell_data_t* kill_command;
    const spell_data_t* dire_beast;
    const spell_data_t* aspect_of_the_wild;
    const spell_data_t* go_for_the_throat;
    const spell_data_t* beast_cleave;
    const spell_data_t* wild_call;

    const spell_data_t* frenzy;
    const spell_data_t* focus_fire;
    const spell_data_t* bestial_wrath;
    const spell_data_t* cobra_strikes;
    const spell_data_t* the_beast_within;
    const spell_data_t* kindred_spirits;
    const spell_data_t* invigoration;
    const spell_data_t* exotic_beasts;
    
    // Marksmanship
    const spell_data_t* aimed_shot;
    const spell_data_t* careful_aim;
    const spell_data_t* bombardment;
    const spell_data_t* chimaera_shot;
    const spell_data_t* rapid_fire;
    const spell_data_t* lock_and_load;

    // Survival
    const spell_data_t* flanking_strike;
    const spell_data_t* raptor_strike;
    const spell_data_t* wing_clip;
    const spell_data_t* mongoose_bite;
    const spell_data_t* hatchet_toss;
    const spell_data_t* harpoon;
    const spell_data_t* muzzle;
    const spell_data_t* laceration;
    const spell_data_t* aspect_of_the_eagle;
    const spell_data_t* carve;
    const spell_data_t* tar_trap;
    const spell_data_t* survivalist;
  } specs;

  // Glyphs
  struct glyphs_t
  {
    // Major
    const spell_data_t* aimed_shot;
    const spell_data_t* animal_bond;
    const spell_data_t* black_ice;
    const spell_data_t* camouflage;
    const spell_data_t* chimaera_shot;
    const spell_data_t* deterrence;
    const spell_data_t* disengage;
    const spell_data_t* endless_wrath;
    const spell_data_t* explosive_trap;
    const spell_data_t* freezing_trap;
    const spell_data_t* ice_trap;
    const spell_data_t* icy_solace;
    const spell_data_t* marked_for_death;
    const spell_data_t* masters_call;
    const spell_data_t* mend_pet;
    const spell_data_t* mending;
    const spell_data_t* mirrored_blades;
    const spell_data_t* misdirection;
    const spell_data_t* no_escape;
    const spell_data_t* pathfinding;
    const spell_data_t* scattering;
    const spell_data_t* snake_trap;
    const spell_data_t* tranquilizing_shot;

    // Minor
    const spell_data_t* aspects;
    const spell_data_t* aspect_of_the_pack;
    const spell_data_t* direction;
    const spell_data_t* fetch;
    const spell_data_t* fireworks;
    const spell_data_t* lesser_proportion;
    const spell_data_t* marking;
    const spell_data_t* revive_pet;
    const spell_data_t* stampede;
    const spell_data_t* tame_beast;
    const spell_data_t* the_cheetah;
  } glyphs;

  struct mastery_spells_t
  {
    const spell_data_t* master_of_beasts;
    const spell_data_t* sniper_training;
    const spell_data_t* essence_of_the_viper;
  } mastery;

  stats_t* stats_stampede;
  stats_t* stats_tier17_4pc_bm;
  stats_t* stats_tier18_4pc_bm;

  double pet_multiplier;

  hunter_t( sim_t* sim, const std::string& name, race_e r = RACE_NONE ):
    player_t( sim, HUNTER, name, r ),
    active( actives_t() ),
    pet_dire_beasts(),
    thunderhawk(),
    action_lightning_arrow_aimed_shot(),
    action_lightning_arrow_arcane_shot(),
    action_lightning_arrow_multi_shot(),
    beastlord( nullptr ),
    longview( nullptr ),
    blackness( nullptr ),
    buffs( buffs_t() ),
    cooldowns( cooldowns_t() ),
    gains( gains_t() ),
    procs( procs_t() ),
    ppm_tier15_2pc_melee( *this, std::numeric_limits<double>::min(), RPPM_HASTE ),
    ppm_tier15_4pc_melee( *this, std::numeric_limits<double>::min(), RPPM_HASTE ),
    talents( talents_t() ),
    specs( specs_t() ),
    glyphs( glyphs_t() ),
    mastery( mastery_spells_t() ),
    stats_stampede( nullptr ),
    stats_tier17_4pc_bm( nullptr ),
    stats_tier18_4pc_bm( nullptr ),
    pet_multiplier( 1.0 )
  {
    // Cooldowns
    cooldowns.explosive_shot  = get_cooldown( "explosive_shot" );
    cooldowns.black_arrow = get_cooldown( "black_arrow" );
    cooldowns.bestial_wrath   = get_cooldown( "bestial_wrath" );
    cooldowns.kill_shot_reset = get_cooldown( "kill_shot_reset" );
    cooldowns.kill_shot_reset -> duration = find_spell( 90967 ) -> duration();
    cooldowns.rapid_fire      = get_cooldown( "rapid_fire" );

    summon_pet_str = "";
    base.distance = 40;
    base_gcd = timespan_t::from_seconds( 1.0 );

    regen_type = REGEN_DYNAMIC;
    regen_caches[ CACHE_HASTE ] = true;
    regen_caches[ CACHE_ATTACK_HASTE ] = true;
  }

  // Character Definition
  virtual void      init_spells() override;
  virtual void      init_base_stats() override;
  virtual void      create_buffs() override;
  virtual void      init_gains() override;
  virtual void      init_position() override;
  virtual void      init_procs() override;
  virtual void      init_rng() override;
  virtual void      init_scaling() override;
  virtual void      init_action_list() override;
  virtual void      arise() override;
  virtual void      reset() override;

  virtual void      regen( timespan_t periodicity = timespan_t::from_seconds( 0.25 ) ) override;
  virtual double    composite_attack_power_multiplier() const override;
  virtual double    composite_melee_crit() const override;
  virtual double    composite_spell_crit() const override;
  virtual double    composite_melee_haste() const override;
  virtual double    composite_player_critical_damage_multiplier() const override;
  virtual double    composite_rating_multiplier( rating_e rating ) const override;
  virtual double    composite_player_multiplier( school_e school ) const override;
  virtual double    matching_gear_multiplier( attribute_e attr ) const override;
  virtual void      invalidate_cache( cache_e ) override;
  virtual void      create_options() override;
  virtual expr_t*   create_expression( action_t*, const std::string& name ) override;
  virtual action_t* create_action( const std::string& name, const std::string& options ) override;
  virtual pet_t*    create_pet( const std::string& name, const std::string& type = std::string() ) override;
  virtual void      create_pets() override;
  virtual resource_e primary_resource() const override { return RESOURCE_FOCUS; }
  virtual role_e    primary_role() const override { return ROLE_ATTACK; }
  virtual stat_e    convert_hybrid_stat( stat_e s ) const override;
  virtual std::string      create_profile( save_e = SAVE_ALL ) override;
  virtual void      copy_from( player_t* source ) override;
  virtual void      armory_extensions( const std::string& r, const std::string& s, const std::string& c, cache::behavior_e ) override;

  virtual void      schedule_ready( timespan_t delta_time = timespan_t::zero( ), bool waiting = false ) override;
  virtual void      moving( ) override;
  virtual void      finish_moving() override;

  void              apl_default();
  void              apl_surv();
  void              apl_bm();
  void              apl_mm();

  void              add_item_actions( action_priority_list_t* list );
  void              add_racial_actions( action_priority_list_t* list );
  void              add_potion_action( action_priority_list_t* list, const std::string big_potion, const std::string little_potion, const std::string options = std::string() );

  target_specific_t<hunter_td_t> target_data;

  virtual hunter_td_t* get_target_data( player_t* target ) const override
  {
    hunter_td_t*& td = target_data[target];
    if ( !td ) td = new hunter_td_t( target, const_cast<hunter_t*>( this ) );
    return td;
  }

  double beast_multiplier()
  {
    double pm = pet_multiplier;
    if ( mastery.master_of_beasts -> ok() )
    {
      pm *= 1.0 + cache.mastery_value();
    }

    return pm;
  }

  // shared code to interrupt a steady focus pair if the ability is cast between steady/cobras
  virtual void no_steady_focus()
  {
    buffs.pre_steady_focus -> expire();
  }
};

static void do_trinket_init( hunter_t*                 r,
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

static void beastlord( special_effect_t& effect )
{
  hunter_t* hunter = debug_cast<hunter_t*>( effect.player );
  do_trinket_init( hunter, HUNTER_BEAST_MASTERY, hunter -> beastlord, effect );
}

static void longview( special_effect_t& effect )
{
  hunter_t* hunter = debug_cast<hunter_t*>( effect.player );
  do_trinket_init( hunter, HUNTER_MARKSMANSHIP, hunter -> longview, effect );
}

static void blackness( special_effect_t& effect )
{
  hunter_t* hunter = debug_cast<hunter_t*>( effect.player );
  do_trinket_init( hunter, HUNTER_SURVIVAL, hunter -> blackness, effect );
}

// Template for common hunter action code. See priest_action_t.
template <class Base>
struct hunter_action_t: public Base
{
private:
  typedef Base ab;
public:
  typedef hunter_action_t base_t;

  hunter_action_t( const std::string& n, hunter_t* player,
                   const spell_data_t* s = spell_data_t::nil() ):
                   ab( n, player, s )
  {
  }

  virtual ~hunter_action_t() {}

  hunter_t* p()
  {
    return static_cast<hunter_t*>( ab::player );
  }
  const hunter_t* p() const
  {
    return static_cast<hunter_t*>( ab::player );
  }

  hunter_td_t* td( player_t* t ) const
  {
    return p() -> get_target_data( t );
  }

  virtual double action_multiplier() const
  {
    double am = ab::action_multiplier();
    return am;
  }

  virtual double composite_target_multiplier( player_t* t ) const
  {
    double m = ab::composite_target_multiplier( t );

    if ( dbc::is_school( ab::school, SCHOOL_FIRE ) )
    {
      if ( td( t ) -> debuffs_pvp_surv_4p_black_fire -> check() )
      {
        m *= 1.0 + ( p() -> sets.set( HUNTER_SURVIVAL, PVP, B4 ) -> effectN( 1 ).trigger() -> effectN( 1 ).percent() * td( t ) -> debuffs_pvp_surv_4p_black_fire -> current_stack );
      }
    }

    if ( td( t ) -> debuffs_survival_t18_2p -> check() )
      m *= 1.0 + td( t ) -> debuffs_survival_t18_2p -> data().effectN( 1 ).percent();

    return m;
  }

  virtual double cost() const 
  {
    double c = ab::cost();

    if ( c == 0 )
      return 0;

    if ( p() -> buffs.bestial_wrath -> check() )
      c *= ( 1.0 + p() -> buffs.bestial_wrath -> data().effectN( 6 ).percent() );

    return c;
  }

  virtual double cast_regen() const
  {
    double cast_seconds = ab::execute_time().total_seconds();
    double sf_seconds = std::min( cast_seconds, p() -> buffs.steady_focus -> remains().total_seconds() );
    double regen = p() -> focus_regen_per_second();
    return ( regen * cast_seconds ) + ( regen * p() -> buffs.steady_focus -> current_value * sf_seconds );
  }

// action list expressions
  
  virtual expr_t* create_expression( const std::string& name )
  {
    if ( util::str_compare_ci( name, "cast_regen" ) )
    {
      // Return the focus that will be regenerated during the cast time or GCD of the target action.
      // This includes additional focus for the steady_shot buff if present, but does not include 
      // focus generated by dire beast.
      struct cast_regen_expr_t : public expr_t
      {
        hunter_action_t* action;
        cast_regen_expr_t( action_t* a ) :
          expr_t( "cast_regen" ),  action( debug_cast<hunter_action_t*>( a ) ) { }
        virtual double evaluate() override
        { return action -> cast_regen(); }
      };
      return new cast_regen_expr_t( this );
    }

    return ab::create_expression( name );
  }

  void trigger_blackness()
  {
    if ( p() -> blackness && p() -> specialization() == HUNTER_SURVIVAL )
    {
      const spell_data_t* data = p() -> find_spell( p() -> blackness -> spell_id );
      double chance = data -> effectN( 1 ).average( p() -> blackness -> item ) / 100.0;
      if ( ab::rng().roll( chance ) )
      {
        p() -> cooldowns.black_arrow -> reset( true );
        p() -> procs.black_arrow_trinket_reset -> occur();
      }
    }
  }

// thrill_of_the_hunt support ===============================================

  void trigger_thrill_of_the_hunt()
  {
    double c = cost();
    if ( p() -> talents.thrill_of_the_hunt -> ok() && c > 0 )
    {
      // Stacks: 3 initial, 3 maximum
      int stacks = p() -> buffs.thrill_of_the_hunt -> data().initial_stacks();
      // "You have a $s1% chance per 10 Focus spent"
      double chance = p() -> talents.thrill_of_the_hunt -> effectN( 1 ).percent() * c / 10.0;
      if ( p() -> buffs.thrill_of_the_hunt -> trigger( stacks, buff_t::DEFAULT_VALUE(), chance ) )
        p() -> procs.thrill_of_the_hunt -> occur();
    }
  }

  double thrill_discount( double cost ) const
  {
    double result = cost;

    if ( p() -> buffs.thrill_of_the_hunt -> check() )
      result += p() -> buffs.thrill_of_the_hunt -> data().effectN( 1 ).base_value();

    return std::max( 0.0, result );
  }

  void consume_thrill_of_the_hunt()
  {
    // "up" is required to correctly track the buff benefit in reporting.
    // focus cost savings is reported as a "gain" so that comparison with steady focus is easy
    if ( p() -> buffs.thrill_of_the_hunt -> up() ) 
    {
      double benefit = -( p() -> buffs.thrill_of_the_hunt -> data().effectN( 1 ).base_value() );
      // NOTE the focus discount has already been applied. This just records it. Don't use resource_gain
      // here or it will double-dip.
      p() -> gains.thrill_of_the_hunt -> add( RESOURCE_FOCUS, benefit );
      p() -> buffs.thrill_of_the_hunt -> decrement();
    }
  }

  void trigger_tier16_bm_4pc_melee()
  {
    if ( ab::background )
      return;

    trigger_tier16_bm_4pc_brutal_kinship( p() );
  }

  virtual void try_steady_focus()
  {
    if ( p() -> talents.steady_focus -> ok() )
      p() -> buffs.pre_steady_focus -> expire();
  }
};

// SV Explosive Shot casts have a 40% chance to not consume a charge of Lock and Load.
// MM Instant Aimed shots reduce the cast time of your next Aimed Shot by 50%. (uses keen eye buff)
// TODO BM Offensive abilities used during Bestial Wrath increase all damage you deal by 2% and all
// damage dealt by your pet by 2%, stacking up to 15 times.
// 1 stack for MoC, Lynx Rush, Glaive Toss, Barrage, Powershot, Focus Fire
// no stack for Dire Beast
void trigger_tier16_bm_4pc_brutal_kinship( hunter_t* p )
{
  if ( p -> specialization() != HUNTER_BEAST_MASTERY )
    return;
  if ( !p -> sets.has_set_bonus( SET_MELEE, T16, B4 ) )
    return;
  if ( p -> buffs.bestial_wrath -> check() )
    p -> buffs.tier16_4pc_bm_brutal_kinship -> trigger( 1, 0, 1, p -> buffs.bestial_wrath -> remains() );
}

namespace pets
{

// ==========================================================================
// Hunter Pet
// ==========================================================================

struct hunter_pet_t: public pet_t
{
public:
  typedef pet_t base_t;

  hunter_pet_t( sim_t& sim, hunter_t& owner, const std::string& pet_name, pet_e pt = PET_HUNTER, bool guardian = false, bool dynamic = false ) :
    base_t( &sim, &owner, pet_name, pt, guardian, dynamic )
  {
  }

  hunter_t* o() const
  {
    return static_cast<hunter_t*>( owner );
  }

  virtual double composite_player_multiplier( school_e school ) const override
  {
    double m = base_t::composite_player_multiplier( school );
    m *= o() -> beast_multiplier();
    
    return m;
  }
};

// Template for common hunter pet action code. See priest_action_t.
template <class T_PET, class Base>
struct hunter_pet_action_t: public Base
{
private:
  typedef Base ab;
public:
  typedef hunter_pet_action_t base_t;

  hunter_pet_action_t( const std::string& n, T_PET& p,
                       const spell_data_t* s = spell_data_t::nil() ):
                       ab( n, &p, s )
  {

  }

  T_PET* p()
  {
    return static_cast<T_PET*>( ab::player );
  }
  const T_PET* p() const
  {
    return static_cast<T_PET*>( ab::player );
  }

  hunter_t* o()
  {
    return static_cast<hunter_t*>( p() -> o() );
  }
  const hunter_t* o() const
  {
    return static_cast<hunter_t*>( p() -> o() );
  }
};

// COPY PASTE of blademaster trinket code so we can support mastery for beastmaster
const std::string BLADEMASTER_PET_NAME = "mirror_image_(trinket)";

struct felstorm_tick_t : public melee_attack_t
{
  felstorm_tick_t( pet_t* p ) :
    melee_attack_t( "felstorm_tick", p, p -> find_spell( 184280 ) )
  {
    school = SCHOOL_PHYSICAL;

    background = special = may_crit = true;
    callbacks = false;
    aoe = -1;
    range = data().effectN( 1 ).radius();

    weapon = &( p -> main_hand_weapon );
  }

  bool init_finished() override
  {
    // Find first blademaster pet, it'll be the first trinket-created pet
    pet_t* main_pet = player -> cast_pet() -> owner -> find_pet( BLADEMASTER_PET_NAME );
    if ( player != main_pet )
    {
      stats = main_pet -> find_action( "felstorm_tick" ) -> stats;
    }

    return melee_attack_t::init_finished();
  }
};

struct felstorm_t : public melee_attack_t
{
  felstorm_t( pet_t* p, const std::string& opts ) :
    melee_attack_t( "felstorm", p, p -> find_spell( 184279 ) )
  {
    parse_options( opts );

    callbacks = may_miss = may_block = may_parry = false;
    dynamic_tick_action = hasted_ticks = true;
    trigger_gcd = timespan_t::from_seconds( 1.0 );

    tick_action = new felstorm_tick_t( p );
  }

  // Make dot long enough to last for the duration of the summon
  timespan_t composite_dot_duration( const action_state_t* ) const override
  { return sim -> expected_iteration_time; }

  bool init_finished() override
  {
    pet_t* main_pet = player -> cast_pet() -> owner -> find_pet( BLADEMASTER_PET_NAME );
    if ( player != main_pet )
    {
      stats = main_pet -> find_action( "felstorm" ) -> stats;
    }

    return melee_attack_t::init_finished();
  }
};

struct blademaster_pet_t : public hunter_pet_t
{
  action_t* felstorm;

  blademaster_pet_t( player_t* owner ) :
    hunter_pet_t( *(owner -> sim), *static_cast<hunter_t*>(owner), BLADEMASTER_PET_NAME, PET_NONE, true, true ),
    felstorm( nullptr )
  {
    main_hand_weapon.type = WEAPON_BEAST;
    // Verified 5/11/15, TODO: Check if this is still the same on live
    owner_coeff.ap_from_ap = 1.0;

    // Magical constants for base damage
    double damage_range = 0.4;
    double base_dps = owner -> dbc.spell_scaling( PLAYER_SPECIAL_SCALE, owner -> level() ) * 4.725;
    double min_dps = base_dps * ( 1 - damage_range / 2.0 );
    double max_dps = base_dps * ( 1 + damage_range / 2.0 );
    main_hand_weapon.swing_time = timespan_t::from_seconds( 2.0 );
    main_hand_weapon.min_dmg =  min_dps * main_hand_weapon.swing_time.total_seconds();
    main_hand_weapon.max_dmg =  max_dps * main_hand_weapon.swing_time.total_seconds();
  }

  timespan_t available() const override
  { return timespan_t::from_seconds( 20.0 ); }

  void init_action_list() override
  {
    action_list_str = "felstorm,if=!ticking";

    pet_t::init_action_list();
  }

  void dismiss( bool expired = false ) override
  {
    hunter_pet_t::dismiss( expired );

    if ( dot_t* d = felstorm -> find_dot( felstorm -> target ) )
    {
      d -> cancel();
    }
  }

  action_t* create_action( const std::string& name,
                           const std::string& options_str ) override
  {
    if ( name == "felstorm" )
    {
      felstorm = new felstorm_t( this, options_str );
      return felstorm;
    }

    return pet_t::create_action( name, options_str );
  }
};

// ==========================================================================
// Hunter Main Pet
// ==========================================================================

struct hunter_main_pet_td_t: public actor_target_data_t
{
public:
  hunter_main_pet_td_t( player_t* target, hunter_main_pet_t* p );
};

struct hunter_main_pet_t: public hunter_pet_t
{
public:
  typedef hunter_pet_t base_t;

  struct actives_t
  {
    action_t* kill_command;
    attack_t* beast_cleave;
  } active;

  struct specs_t
  {
    // ferocity
    const spell_data_t* hearth_of_the_phoenix;
    const spell_data_t* spiked_collar;
    // tenacity
    const spell_data_t* last_stand;
    const spell_data_t* charge;
    const spell_data_t* thunderstomp;
    const spell_data_t* blood_of_the_rhino;
    const spell_data_t* great_stamina;
    // cunning
    const spell_data_t* roar_of_sacrifice;
    const spell_data_t* bullhead;
    const spell_data_t* cornered;
    const spell_data_t* boars_speed;

    const spell_data_t* dash; // ferocity, cunning

    // base for all pets
    const spell_data_t* wild_hunt;
    const spell_data_t* combat_experience;
  } specs;

  // Buffs
  struct buffs_t
  {
    buff_t* bestial_wrath;
    buff_t* frenzy;
    buff_t* stampede;
    buff_t* beast_cleave;
    buff_t* enhanced_basic_attacks;
    buff_t* tier16_4pc_bm_brutal_kinship;
    buff_t* tier17_4pc_bm;
    buff_t* tier18_4pc_bm;
  } buffs;

  // Gains
  struct gains_t
  {
    gain_t* focus_fire;
    gain_t* go_for_the_throat;
    gain_t* steady_focus;
  } gains;

  // Benefits
  struct benefits_t
  {
    benefit_t* wild_hunt;
  } benefits;

  hunter_main_pet_t( sim_t& sim, hunter_t& owner, const std::string& pet_name, pet_e pt ):
    base_t( sim, owner, pet_name, pt ),
    active( actives_t() ),
    specs( specs_t() ),
    buffs( buffs_t() ),
    gains( gains_t() ),
    benefits( benefits_t() )
  {
    owner.hunter_main_pets.push_back( this );

    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.min_dmg    = dbc.spell_scaling( owner.type, owner.level() ) * 0.25;
    main_hand_weapon.max_dmg    = dbc.spell_scaling( owner.type, owner.level() ) * 0.25;
    main_hand_weapon.damage     = ( main_hand_weapon.min_dmg + main_hand_weapon.max_dmg ) / 2;
    main_hand_weapon.swing_time = timespan_t::from_seconds( 2.0 );

    stamina_per_owner = 0.7;

    initial.armor_multiplier *= 1.05;

    // FIXME work around assert in pet specs
    // Set default specs
    _spec = default_spec();
  }

  specialization_e default_spec()
  {
    if ( pet_type > PET_NONE          && pet_type < PET_FEROCITY_TYPE ) return PET_FEROCITY;
    if ( pet_type > PET_FEROCITY_TYPE && pet_type < PET_TENACITY_TYPE ) return PET_TENACITY;
    if ( pet_type > PET_TENACITY_TYPE && pet_type < PET_CUNNING_TYPE ) return PET_CUNNING;
    return PET_FEROCITY;
  }

  std::string unique_special()
  {
    switch ( pet_type )
    {
    case PET_CARRION_BIRD: return "demoralizing_screech";
    case PET_CAT:          return "roar_of_courage";
    case PET_CORE_HOUND:   return "double_bite";
    case PET_DEVILSAUR:    return "furious_howl/monstrous_bite";
    case PET_HYENA:        return "cackling_howl";
    case PET_MOTH:         return "";
    case PET_RAPTOR:       return "";
    case PET_SPIRIT_BEAST: return "roar_of_courage";
    case PET_TALLSTRIDER:  return "";
    case PET_WASP:         return "";
    case PET_WOLF:         return "furious_howl";
    case PET_BEAR:         return "";
    case PET_BOAR:         return "";
    case PET_CRAB:         return "";
    case PET_CROCOLISK:    return "";
    case PET_GORILLA:      return "";
    case PET_RHINO:        return "wild_strength";
    case PET_SCORPID:      return "";
    case PET_SHALE_SPIDER: return "";
    case PET_TURTLE:       return "";
    case PET_WARP_STALKER: return "";
    case PET_WORM:         return "";
    case PET_BAT:          return "";
    case PET_BIRD_OF_PREY: return "tenacity";
    case PET_CHIMAERA:      return "froststorm_breath";
    case PET_DRAGONHAWK:   return "spry_attacks";
    case PET_NETHER_RAY:   return "";
    case PET_RAVAGER:      return "chitinous_armor";
    case PET_SERPENT:      return "corrosive_spit";
    case PET_SILITHID:     return "qiraji_fortitude";
    case PET_SPIDER:       return "";
    case PET_SPOREBAT:     return "";
    case PET_WIND_SERPENT: return "breath_of_the_winds";
    case PET_FOX:          return "tailspin";
    default: break;
    }
    return nullptr;
  }

  virtual void init_base_stats() override
  {
    base_t::init_base_stats();

    resources.base[RESOURCE_HEALTH] = util::interpolate( level(), 0, 4253, 6373 );
    resources.base[RESOURCE_FOCUS] = 100 + o() -> specs.kindred_spirits -> effectN( 1 ).resource( RESOURCE_FOCUS );

    base_gcd = timespan_t::from_seconds( 1.00 );

    resources.infinite_resource[RESOURCE_FOCUS] = o() -> resources.infinite_resource[RESOURCE_FOCUS];

    owner_coeff.ap_from_ap = 0.6;
    owner_coeff.sp_from_ap = 0.6;
  }

  virtual void create_buffs() override
  {
    base_t::create_buffs();

    buffs.bestial_wrath = buff_creator_t( this, 19574, "bestial_wrath" ).activated( true );
    buffs.bestial_wrath -> cooldown -> duration = timespan_t::zero();
    buffs.bestial_wrath -> buff_duration += owner -> sets.set( SET_MELEE, T14, B4 ) -> effectN( 1 ).time_value();
    buffs.bestial_wrath -> default_value = buffs.bestial_wrath -> data().effectN( 2 ).percent();

    buffs.frenzy            = buff_creator_t( this, 19615, "frenzy" ).chance( o() -> specs.frenzy -> effectN( 2 ).percent() );

    // Use buff to indicate whether the pet is a stampede summon
    buffs.stampede          = buff_creator_t( this, 130201, "stampede" )
      .duration( timespan_t::from_millis( 40027 ) )
      // Added 0.027 seconds to properly reflect haste threshholds seen in game.
      .activated( true )
      /*.quiet( true )*/;

    double cleave_value     = o() -> find_specialization_spell( "Beast Cleave" ) -> effectN( 1 ).percent() + o() -> perks.improved_beast_cleave -> effectN( 1 ).percent();
    buffs.beast_cleave      = buff_creator_t( this, 118455, "beast_cleave" ).activated( true ).default_value( cleave_value );

    buffs.enhanced_basic_attacks = buff_creator_t( this, "enhanced_basic_attacks", o() -> perks.enhanced_basic_attacks );

    buffs.tier16_4pc_bm_brutal_kinship = buff_creator_t( this, 145737, "tier16_4pc_brutal_kinship" );
    buffs.tier17_4pc_bm = buff_creator_t( this, 178875, "tier17_4pc_bm" )
      .default_value( owner -> find_spell( 178875 ) -> effectN( 2 ).percent() )
      .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
    buffs.tier18_4pc_bm = buff_creator_t( this, "tier18_4pc_bm" )
      .default_value( owner -> find_spell( 178875 ) -> effectN( 2 ).percent() )
      .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  }

  virtual void init_special_effects() override
  {
    base_t::init_special_effects();
    hunter_t* hunter = debug_cast<hunter_t*>( owner );
    const special_effect_t* bl = hunter -> beastlord;
    if ( bl )
    {
      const spell_data_t* data = hunter -> find_spell( bl -> spell_id );
      buffs.bestial_wrath -> buff_duration += timespan_t::from_seconds( data -> effectN( 1 ).base_value() );
      double increase = data -> effectN( 2 ).average( bl -> item ) + data -> effectN( 3 ).average( bl -> item );
      buffs.bestial_wrath -> default_value += increase / 100.0;
    }
  }

  virtual void init_gains() override
  {
    base_t::init_gains();

    gains.focus_fire        = get_gain( "focus_fire" );
    gains.go_for_the_throat = get_gain( "go_for_the_throat" );
    gains.steady_focus      = get_gain( "steady_focus" );
  }

  virtual void init_benefits() override
  {
    base_t::init_benefits();

    benefits.wild_hunt = get_benefit( "wild_hunt" );
  }

  virtual void init_action_list() override
  {
    if ( action_list_str.empty() )
    {
      action_list_str += "/auto_attack";
      action_list_str += "/snapshot_stats";

      std::string special = unique_special();
      if ( !special.empty() )
      {
        action_list_str += "/";
        action_list_str += special;
      }
      action_list_str += "/claw";
      action_list_str += "/wait_until_ready";
      use_default_action_list = true;
    }

    base_t::init_action_list();
  }

  virtual double composite_melee_crit() const override
  {
    double ac = base_t::composite_melee_crit();
    ac += specs.spiked_collar -> effectN( 3 ).percent();
    return ac;
  }

  void regen( timespan_t periodicity ) override
  {
    player_t::regen( periodicity );
    if ( o() -> buffs.steady_focus -> up() )
    {
      double base = focus_regen_per_second() * o() -> buffs.steady_focus -> current_value;
      resource_gain( RESOURCE_FOCUS, base * periodicity.total_seconds(), gains.steady_focus );
    }
  }

  double focus_regen_per_second() const override
  {
    return o() -> focus_regen_per_second() * 1.25;
  }

  virtual double composite_melee_speed() const override
  {
    double ah = base_t::composite_melee_speed();

    ah *= 1.0 / ( 1.0 + o() -> specs.frenzy -> effectN( 1 ).percent() * buffs.frenzy -> stack() );
    ah *= 1.0 / ( 1.0 + specs.spiked_collar -> effectN( 2 ).percent() );

    return ah;
  }

  virtual void summon( timespan_t duration = timespan_t::zero() ) override
  {
    base_t::summon( duration );

    o() -> active.pet = this;
  }

  bool tier_pet_summon( timespan_t duration )
  {
    // More than one 4pc pet could be up at a time
    if ( this == o() -> active.pet || buffs.stampede -> check() || buffs.tier17_4pc_bm -> check() || buffs.tier18_4pc_bm -> check() )
      return false;

    type = PLAYER_GUARDIAN;

    bool is_t18 = o() -> sets.has_set_bonus( HUNTER_BEAST_MASTERY, T18, B4 );

    for ( size_t i = 0; i < stats_list.size(); ++i )
    {
      if ( !( stats_list[i] -> parent ) )
      {
        if ( is_t18 )
          o() -> stats_tier18_4pc_bm -> add_child( stats_list[i] );
        else
          o() -> stats_tier17_4pc_bm -> add_child( stats_list[i] );
      }
    }
    base_t::summon( duration );
    
    double ap_coeff = 0.6; // default coefficient for pets
    if ( is_t18 )
    {
      ap_coeff = 0.4; //  (1.0 + o() -> buffs.focus_fire -> current_value) 
      buffs.tier18_4pc_bm -> trigger();
    }
    else if ( o() -> sets.has_set_bonus( HUNTER_BEAST_MASTERY, T18, B4 ) )
    {
      // pet appears at the target
      current.distance = 0;
      buffs.tier17_4pc_bm -> trigger( 1, buff_t::DEFAULT_VALUE(), 1.0, duration );
    }

    owner_coeff.ap_from_ap = ap_coeff;
    owner_coeff.sp_from_ap = ap_coeff;

    // pet swings immediately (without an execute time)
    if ( !main_hand_attack -> execute_event ) main_hand_attack -> execute();
    return true;
  }

  void stampede_summon( timespan_t duration )
  {
    if ( this == o() -> active.pet || buffs.tier17_4pc_bm -> check() || buffs.tier18_4pc_bm -> check() )
      return;

    type = PLAYER_GUARDIAN;

    for ( size_t i = 0; i < stats_list.size(); ++i )
      if ( !( stats_list[i] -> parent ) )
        o() -> stats_stampede -> add_child( stats_list[i] );

    base_t::summon( duration );
    // pet appears at the target
    current.distance = 0;
    owner_coeff.ap_from_ap = 0.6;
    owner_coeff.sp_from_ap = 0.6;

    buffs.stampede -> trigger( 1, buff_t::DEFAULT_VALUE(), 1.0, duration );

    // pet swings immediately (without an execute time)
    if ( !main_hand_attack -> execute_event ) main_hand_attack -> execute();
  }

  virtual void demise() override
  {
    base_t::demise();

    if ( o() -> active.pet == this )
      o() -> active.pet = nullptr;
  }

  virtual double composite_player_multiplier( school_e school ) const override
  {
    double m = base_t::composite_player_multiplier( school );

    if ( !buffs.stampede -> check() && buffs.bestial_wrath -> up() )
      m *= 1.0 + buffs.bestial_wrath -> current_value;

    if ( o() -> sets.has_set_bonus( SET_MELEE, T16, B4 ) )
      m *= 1.0 + buffs.tier16_4pc_bm_brutal_kinship -> stack() * buffs.tier16_4pc_bm_brutal_kinship -> data().effectN( 1 ).percent();

    // from Nimox: 178875 is the 4pc BM pet damage buff
    if ( buffs.tier17_4pc_bm -> up() )
      m *= 1.0 + buffs.tier17_4pc_bm -> current_value;

    // Pet combat experience
    m *= 1.0 + specs.combat_experience -> effectN( 2 ).percent();

    return m;
  }

  target_specific_t<hunter_main_pet_td_t> target_data;

  virtual hunter_main_pet_td_t* get_target_data( player_t* target ) const override
  {
    hunter_main_pet_td_t*& td = target_data[target];
    if ( !td )
      td = new hunter_main_pet_td_t( target, const_cast<hunter_main_pet_t*>( this ) );
    return td;
  }

  virtual resource_e primary_resource() const override { return RESOURCE_FOCUS; }

  virtual action_t* create_action( const std::string& name, const std::string& options_str ) override;

  virtual void init_spells() override;

  void moving() override
  {
    return;
  }
};

namespace actions
{

// Template for common hunter main pet action code. See priest_action_t.
template <class Base>
struct hunter_main_pet_action_t: public hunter_pet_action_t < hunter_main_pet_t, Base >
{
private:
  typedef hunter_pet_action_t<hunter_main_pet_t, Base> ab;
public:
  typedef hunter_main_pet_action_t base_t;

  bool special_ability;

  hunter_main_pet_action_t( const std::string& n, hunter_main_pet_t* player,
                            const spell_data_t* s = spell_data_t::nil() ):
                            ab( n, *player, s ),
                            special_ability( false )
  {
    if ( ab::data().rank_str() && !strcmp( ab::data().rank_str(), "Special Ability" ) )
      special_ability = true;
  }

  hunter_main_pet_t* p() const
  {
    return static_cast<hunter_main_pet_t*>( ab::player );
  }

  hunter_t* o() const
  {
    return static_cast<hunter_t*>( p() -> o() );
  }

  hunter_main_pet_td_t* td( player_t* t = nullptr ) const
  {
    return p() -> get_target_data( t ? t : ab::target );
  }
};

// ==========================================================================
// Hunter Pet Attacks
// ==========================================================================

struct hunter_main_pet_attack_t: public hunter_main_pet_action_t < melee_attack_t >
{
  hunter_main_pet_attack_t( const std::string& n, hunter_main_pet_t* player,
                            const spell_data_t* s = spell_data_t::nil() ):
                            base_t( n, player, s )
  {
    special = true;
    may_crit = true;
  }

  virtual bool ready() override
  {
    // Stampede pets don't use abilities or spells
    if ( p() -> buffs.stampede -> check() || p() -> buffs.tier17_4pc_bm -> check() || p() -> buffs.tier18_4pc_bm -> check() )
      return false;

    return base_t::ready();
  }

  void trigger_tier16_bm_4pc_melee()
  {
    if ( o() -> specialization() != HUNTER_BEAST_MASTERY )
      return;
    if ( !o() -> sets.has_set_bonus( SET_MELEE, T16, B4 ) )
      return;
    if ( p() -> buffs.bestial_wrath -> check() )
      p() -> buffs.tier16_4pc_bm_brutal_kinship -> trigger( 1, 0, 1, p() -> buffs.bestial_wrath -> remains() );
  }
};

// Beast Cleave ==============================================================

struct beast_cleave_attack_t: public hunter_main_pet_attack_t
{
  beast_cleave_attack_t( hunter_main_pet_t* p ):
    hunter_main_pet_attack_t( "beast_cleave_attack", p, p -> find_spell( 22482 ) )
  {
    may_miss = false;
    may_crit = false;
    proc = false;
    callbacks = false;
    background = true;
    school = SCHOOL_PHYSICAL;
    aoe = -1;
    range = -1.0;
    radius = 10.0;
    // The starting damage includes all the buffs
    weapon_multiplier = 0;
  }

  size_t available_targets( std::vector< player_t* >& tl ) const override
  {
    hunter_main_pet_attack_t::available_targets( tl );

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

static void trigger_beast_cleave( action_state_t* s )
{
  if ( !s -> action -> result_is_hit( s -> result ) )
    return;

  if ( s -> action -> sim -> active_enemies == 1 )
    return;

  hunter_main_pet_t* p = debug_cast<hunter_main_pet_t*>( s -> action -> player );

  if ( !p -> buffs.beast_cleave -> check() )
    return;

  if ( p -> active.beast_cleave -> target != s -> target )
    p -> active.beast_cleave -> target_cache.is_valid = false;

  p -> active.beast_cleave -> target = s -> target;
  double cleave = s -> result_total * p -> buffs.beast_cleave -> current_value;

  p -> active.beast_cleave -> base_dd_min = cleave;
  p -> active.beast_cleave -> base_dd_max = cleave;
  p -> active.beast_cleave -> execute();
}

// Pet Melee ================================================================

struct pet_melee_t: public hunter_main_pet_attack_t
{
  pet_melee_t( hunter_main_pet_t* player ):
    hunter_main_pet_attack_t( "melee", player )
  {
    special = false;
    weapon = &( player -> main_hand_weapon );
    base_execute_time = weapon -> swing_time;
    background = true;
    repeating = true;
    auto_attack = true;
    school = SCHOOL_PHYSICAL;

  }

  virtual void impact( action_state_t* s ) override
  {
    hunter_main_pet_attack_t::impact( s );

    trigger_beast_cleave( s );
  }
};

// Pet Auto Attack ==========================================================

struct pet_auto_attack_t: public hunter_main_pet_attack_t
{
  pet_auto_attack_t( hunter_main_pet_t* player, const std::string& options_str ):
    hunter_main_pet_attack_t( "auto_attack", player, nullptr )
  {
    parse_options( options_str );

    p() -> main_hand_attack = new pet_melee_t( player );
    trigger_gcd = timespan_t::zero();
    school = SCHOOL_PHYSICAL;
  }

  virtual void execute() override
  {
    p() -> main_hand_attack -> schedule_execute();
  }

  virtual bool ready() override
  {
    return( p() -> main_hand_attack -> execute_event == nullptr ); // not swinging
  }
};

// Pet Claw/Bite/Smack ======================================================

struct basic_attack_t: public hunter_main_pet_attack_t
{
  double chance_invigoration;
  double gain_invigoration;

  basic_attack_t( hunter_main_pet_t* p, const std::string& name, const std::string& options_str ):
    hunter_main_pet_attack_t( name, p, p -> find_pet_spell( name ) )
  {
    parse_options( options_str );
    school = SCHOOL_PHYSICAL;

    attack_power_mod.direct = 1.0 / 3.0;
    base_multiplier *= 1.0 + p -> specs.spiked_collar -> effectN( 1 ).percent();
    chance_invigoration = p -> find_spell( 53397 ) -> proc_chance();
    gain_invigoration = p -> find_spell( 53398 ) -> effectN( 1 ).resource( RESOURCE_FOCUS );
  }

  void execute() override
  {
    hunter_main_pet_attack_t::execute();
    trigger_tier16_bm_4pc_melee();
    if ( p() == o() -> active.pet )
      o() -> buffs.cobra_strikes -> decrement();

    if ( o() -> specs.frenzy -> ok() )
      p() -> buffs.frenzy -> trigger( 1 );
  }

  // Override behavior so that Basic Attacks use hunter's attack power rather than the pet's
  double composite_attack_power() const override
  {
    return o() -> cache.attack_power() * o()->composite_attack_power_multiplier();
  }

  void impact( action_state_t* s ) override
  {
    hunter_main_pet_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      if ( o() -> specs.invigoration -> ok() && rng().roll( chance_invigoration ) )
      {
        o() -> resource_gain( RESOURCE_FOCUS, gain_invigoration, o() -> gains.invigoration );
        o() -> procs.invigoration -> occur();
      }
      trigger_beast_cleave( s );
    }
  }

  void consume_resource() override
  {
    if ( p() -> buffs.enhanced_basic_attacks -> up() )
      p() -> buffs.enhanced_basic_attacks -> expire();
    hunter_main_pet_attack_t::consume_resource();
  }

  double composite_crit() const override
  {
    double cc = hunter_main_pet_attack_t::composite_crit();

    if ( o() -> buffs.cobra_strikes -> up() && p() == o() -> active.pet )
      cc += o() -> buffs.cobra_strikes -> data().effectN( 1 ).percent();

    return cc;
  }

  bool use_wild_hunt() const
  {
    // comment out to avoid procs
    return p() -> resources.current[RESOURCE_FOCUS] > 50;
  }

  double action_multiplier() const override
  {
    double am = hunter_main_pet_attack_t::action_multiplier();

    if ( o() -> talents.blink_strikes -> ok() && p() == o() -> active.pet )
      am *= 1.0 + o() -> talents.blink_strikes -> effectN( 1 ).percent();

    if ( use_wild_hunt() )
    {
      p() -> benefits.wild_hunt -> update( true );
      am *= 1.0 + p() -> specs.wild_hunt -> effectN( 1 ).percent();
    }
    else
      p() -> benefits.wild_hunt -> update( false );

    return am;
  }

  double cost() const override
  {
    double base_cost = hunter_main_pet_attack_t::cost();
    double c = base_cost;

    if ( use_wild_hunt() )
      c *= 1.0 + p() -> specs.wild_hunt -> effectN( 2 ).percent();

    if ( p() -> buffs.enhanced_basic_attacks -> check() )
      c -= base_cost;

    return c;
  }

  void update_ready( timespan_t cd_duration ) override
  {
    hunter_main_pet_attack_t::update_ready( cd_duration );
    if ( o() -> rng().roll( o() -> perks.enhanced_basic_attacks -> effectN( 1 ).percent() ) )
    {
      cooldown -> reset( true );
      p() -> buffs.enhanced_basic_attacks -> trigger();
    }
  }
};

// Devilsaur Monstrous Bite =================================================

struct monstrous_bite_t: public hunter_main_pet_attack_t
{
  monstrous_bite_t( hunter_main_pet_t* p, const std::string& options_str ):
    hunter_main_pet_attack_t( "monstrous_bite", p, p -> find_spell( 54680 ) )
  {
    parse_options( options_str );
    school = SCHOOL_PHYSICAL;
    stats -> school = SCHOOL_PHYSICAL;
  }
};

// Kill Command (pet) =======================================================

struct kill_command_t: public hunter_main_pet_attack_t
{
  kill_command_t( hunter_main_pet_t* p ):
    hunter_main_pet_attack_t( "kill_command", p, p -> find_spell( 83381 ) )
  {
    background = true;
    proc = true;
    school = SCHOOL_PHYSICAL;
    range = 25;

    // The hardcoded parameter is taken from the $damage value in teh tooltip. e.g., 1.36 below
    // $damage = ${ 1.5*($83381m1 + ($RAP*  1.632   ))*$<bmMastery> }
    attack_power_mod.direct  = 1.632; // Hard-coded in tooltip.
  }

  // Override behavior so that Kill Command uses hunter's attack power rather than the pet's
  double composite_attack_power() const override
  {
    return o() -> cache.attack_power() * o()->composite_attack_power_multiplier();
  }

  bool usable_moving() const override
  {
    return true;
  }
};

// ==========================================================================
// Hunter Pet Spells
// ==========================================================================

struct hunter_main_pet_spell_t: public hunter_main_pet_action_t < spell_t >
{
  hunter_main_pet_spell_t( const std::string& n, hunter_main_pet_t* player,
                           const spell_data_t* s = spell_data_t::nil() ):
                           base_t( n, player, s )
  {
  }

  virtual bool ready() override
  {
    // Stampede pets don't use abilities or spells
    if ( p() -> buffs.stampede -> check() || p() -> buffs.tier17_4pc_bm -> check() || p() -> buffs.tier18_4pc_bm -> check() )
      return false;

    return base_t::ready();
  }
};

// ==========================================================================
// Unique Pet Specials
// ==========================================================================

// Wolf/Devilsaur Furious Howl ==============================================

struct furious_howl_t: public hunter_main_pet_spell_t
{
  furious_howl_t( hunter_main_pet_t* player, const std::string& options_str ):
    hunter_main_pet_spell_t( "furious_howl", player, player -> find_pet_spell( "Furious Howl" ) )
  {
    parse_options( options_str );

    harmful = false;
  }
};

// chimaera Froststorm Breath ================================================

struct froststorm_breath_t: public hunter_main_pet_spell_t
{
  struct froststorm_breath_tick_t: public hunter_main_pet_spell_t
  {
    froststorm_breath_tick_t( hunter_main_pet_t* player ):
      hunter_main_pet_spell_t( "froststorm_breath_tick", player, player -> find_spell( 95725 ) )
    {
      attack_power_mod.direct = 0.144; // hardcoded into tooltip, 2012/08 checked 2015/02/21
      background = true;
      direct_tick = true;
    }
  };

  froststorm_breath_tick_t* tick_spell;
  froststorm_breath_t( hunter_main_pet_t* player, const std::string& options_str ):
    hunter_main_pet_spell_t( "froststorm_breath", player, player -> find_pet_spell( "Froststorm Breath" ) )
  {
    parse_options( options_str );
    channeled = true;
    tick_spell = new froststorm_breath_tick_t( player );
    add_child( tick_spell );
  }

  virtual void init() override
  {
    hunter_main_pet_spell_t::init();

    tick_spell -> stats = stats;
  }

  virtual void tick( dot_t* d ) override
  {
    tick_spell -> execute();
    stats -> add_tick( d -> time_to_tick, d -> state -> target );
  }
};

} // end namespace pets::actions


hunter_main_pet_td_t::hunter_main_pet_td_t( player_t* target, hunter_main_pet_t* p ):
actor_target_data_t( target, p )
{
}
// hunter_pet_t::create_action ==============================================

action_t* hunter_main_pet_t::create_action( const std::string& name,
                                            const std::string& options_str )
{
  using namespace actions;

  if ( name == "auto_attack" ) return new          pet_auto_attack_t( this, options_str );
  if ( name == "claw" ) return new                 basic_attack_t( this, "Claw", options_str );
  if ( name == "bite" ) return new                 basic_attack_t( this, "Bite", options_str );
  if ( name == "smack" ) return new                basic_attack_t( this, "Smack", options_str );
  if ( name == "froststorm_breath" ) return new    froststorm_breath_t( this, options_str );
  if ( name == "furious_howl" ) return new         furious_howl_t( this, options_str );
  if ( name == "monstrous_bite" ) return new       monstrous_bite_t( this, options_str );
  return base_t::create_action( name, options_str );
}
// hunter_t::init_spells ====================================================

void hunter_main_pet_t::init_spells()
{
  base_t::init_spells();

  // ferocity
  active.kill_command = new actions::kill_command_t( this );
  specs.hearth_of_the_phoenix = spell_data_t::not_found();
  specs.spiked_collar = spell_data_t::not_found();
  // tenacity
  specs.last_stand = spell_data_t::not_found();
  specs.charge = spell_data_t::not_found();
  specs.thunderstomp = spell_data_t::not_found();
  specs.blood_of_the_rhino = spell_data_t::not_found();
  specs.great_stamina = spell_data_t::not_found();
  // cunning
  specs.roar_of_sacrifice = spell_data_t::not_found();
  specs.bullhead = spell_data_t::not_found();
  specs.cornered = spell_data_t::not_found();
  specs.boars_speed = spell_data_t::not_found();
  specs.dash = spell_data_t::not_found(); // ferocity, cunning

  // ferocity
  specs.spiked_collar    = find_specialization_spell( "Spiked Collar" );
  specs.wild_hunt = find_spell( 62762 );
  specs.combat_experience = find_specialization_spell( "Combat Experience" );
  if ( o() -> specs.beast_cleave -> ok() )
  {
    active.beast_cleave = new actions::beast_cleave_attack_t( this );
  }
}

// ==========================================================================
// Dire Critter
// ==========================================================================

struct dire_critter_t: public hunter_pet_t
{
  struct melee_t: public hunter_pet_action_t < dire_critter_t, melee_attack_t >
  {
    int focus_gain;
    melee_t( dire_critter_t& p ):
      base_t( "dire_beast_melee", p )
    {
      weapon = &( player -> main_hand_weapon );
      weapon_multiplier = 0;
      base_execute_time = weapon -> swing_time;
      base_dd_min = base_dd_max = player -> dbc.spell_scaling( p.o() -> type, p.o() -> level() );
      attack_power_mod.direct = 0.5714;
      school = SCHOOL_PHYSICAL;
      trigger_gcd = timespan_t::zero();
      background = true;
      repeating = true;
      may_glance = true;
      may_crit = true;
      special = false;
      focus_gain = player -> find_spell( 120694 ) -> effectN( 1 ).base_value();
      base_multiplier *= 1.15; // Hotfix
    }

    bool init_finished() override
    {
      if ( p() -> o() -> pet_dire_beasts[ 0 ] )
        stats = p() -> o() -> pet_dire_beasts[ 0 ] -> get_stats( "dire_beast_melee" );

      return hunter_pet_action_t<dire_critter_t, melee_attack_t>::init_finished();
    }

    virtual void impact( action_state_t* s ) override
    {
      melee_attack_t::impact( s );

      if ( result_is_hit( s -> result ) )
        p() -> o() -> resource_gain( RESOURCE_FOCUS, focus_gain, p() -> o() -> gains.dire_beast );
    }
  };

  dire_critter_t( hunter_t& owner, size_t index ):
    hunter_pet_t( *owner.sim, owner, std::string( "dire_beast_" ) + util::to_string( index ), PET_HUNTER, true /*GUARDIAN*/ )
  {
    owner_coeff.ap_from_ap = 1.0;
    regen_type = REGEN_DISABLED;
  }

  virtual void init_base_stats() override
  {
    hunter_pet_t::init_base_stats();

    // FIXME numbers are from treant. correct them.
    resources.base[RESOURCE_HEALTH] = 9999; // Level 85 value
    resources.base[RESOURCE_MANA] = 0;

    stamina_per_owner = 0;
    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.min_dmg    = dbc.spell_scaling( o() -> type, o() -> level() );
    main_hand_weapon.max_dmg    = main_hand_weapon.min_dmg;
    main_hand_weapon.swing_time = timespan_t::from_seconds( 2 );

    main_hand_attack = new melee_t( *this );
  }

  virtual void summon( timespan_t duration = timespan_t::zero() ) override
  {
    hunter_pet_t::summon( duration );

    // attack immediately on summons
    main_hand_attack -> execute();
  }
};

// ==========================================================================
// Tier 15 2-piece bonus temporary pet
// ==========================================================================

struct tier15_thunderhawk_t: public hunter_pet_t
{
  tier15_thunderhawk_t( hunter_t& owner ):
    hunter_pet_t( *owner.sim, owner, "tier15_thunderhawk", PET_HUNTER, true /*GUARDIAN*/ )
  {
    regen_type = REGEN_DISABLED;
  }

  virtual void init_base_stats() override
  {
    hunter_pet_t::init_base_stats();
    action_list_str = "lightning_blast";
  }

  struct lightning_blast_t: public hunter_pet_action_t < tier15_thunderhawk_t, spell_t >
  {
    lightning_blast_t( tier15_thunderhawk_t& p ):
      base_t( "lightning_blast", p, p.find_spell( 138374 ) )
    {
      may_crit = true;
      base_costs[RESOURCE_MANA] = 0;
    }
    virtual double composite_haste() const override { return 1.0; }
  };

  virtual action_t* create_action( const std::string& name,
                                   const std::string& options_str ) override
  {
    if ( name == "lightning_blast" ) return new lightning_blast_t( *this );
    return hunter_pet_t::create_action( name, options_str );
  }
};

} // end namespace pets

namespace attacks
{

// ==========================================================================
// Hunter Attack
// ==========================================================================

struct hunter_ranged_attack_t: public hunter_action_t < ranged_attack_t >
{
  hunter_ranged_attack_t( const std::string& n, hunter_t* player,
                          const spell_data_t* s = spell_data_t::nil() ):
                          base_t( n, player, s )
  {
    if ( player -> main_hand_weapon.type == WEAPON_NONE )
      background = true;

    special = true;
    may_crit = true;
    tick_may_crit = true;
    may_parry = false;
    may_block = false;
  }

  virtual void init() override
  {
    if ( player -> main_hand_weapon.group() != WEAPON_RANGED )
      background = true;
    base_t::init();
  }

  virtual bool usable_moving() const override
  {
    return true;
  }

  virtual void execute() override
  {
    ranged_attack_t::execute();
    try_steady_focus();
    trigger_thrill_of_the_hunt();
    trigger_tier16_bm_4pc_melee();
  }

  virtual timespan_t execute_time() const override
  {
    timespan_t t = base_t::execute_time();

    if ( t == timespan_t::zero() || base_execute_time < timespan_t::zero() )
      return timespan_t::zero();

    return t;
  }

  void trigger_go_for_the_throat()
  {
    if ( !p() -> specs.go_for_the_throat -> ok() )
      return;

    if ( !p() -> active.pet )
      return;

    int gain = p() -> specs.go_for_the_throat -> effectN( 1 ).trigger() -> effectN( 1 ).base_value();
    p() -> active.pet -> resource_gain( RESOURCE_FOCUS, gain, p() -> active.pet -> gains.go_for_the_throat );
  }

  virtual void trigger_steady_focus( bool require_pre )
  {
    if ( !p() -> talents.steady_focus -> ok() )
      return;

    if ( require_pre )
    {
      p() -> buffs.pre_steady_focus -> trigger( 1 );
      if ( p() -> buffs.pre_steady_focus -> stack() < 2 )
        return;
    }

    // either two required shots have happened or we don't require them
    double regen_buff = p() -> buffs.steady_focus -> data().effectN( 1 ).percent();
    p() -> buffs.steady_focus -> trigger( 1, regen_buff );
    p() -> buffs.pre_steady_focus -> expire();
  }

  void trigger_tier15_2pc_melee()
  {
    if ( !p() -> sets.has_set_bonus( SET_MELEE, T15, B2 ) )
      return;

    if ( ( p() -> ppm_tier15_2pc_melee.trigger() ) )
    {
      p() -> procs.tier15_2pc_melee -> occur();
      size_t i;

      for ( i = 0; i < p() -> thunderhawk.size(); i++ )
      {
        if ( !p() -> thunderhawk[i] -> is_sleeping() )
          continue;

        p() -> thunderhawk[i] -> summon( timespan_t::from_seconds( 10 ) );
        break;
      }

      assert( i < p() -> thunderhawk.size() );
    }
  }

  void trigger_tier15_4pc_melee( proc_t* proc, attack_t* attack );
};

struct hunter_melee_attack_t: public hunter_action_t < melee_attack_t >
{
  hunter_melee_attack_t( const std::string& n, hunter_t* player,
                          const spell_data_t* s = spell_data_t::nil() ):
                          base_t( n, player, s )
  {
    if ( player -> main_hand_weapon.type == WEAPON_NONE )
      background = true;

    special = true;
    may_crit = true;
    tick_may_crit = true;
    may_parry = false;
    may_block = false;
  }

  virtual void init() override
  {
    if ( player -> main_hand_weapon.group() != WEAPON_1H || player -> main_hand_weapon.group() != WEAPON_2H )
      background = true;
    base_t::init();
  }

  virtual bool usable_moving() const override
  {
    return true;
  } 

  virtual void execute() override
  {
    melee_attack_t::execute();
    trigger_thrill_of_the_hunt();
    trigger_tier16_bm_4pc_melee();
  }

  virtual timespan_t execute_time() const override
  {
    timespan_t t = base_t::execute_time();

    if ( t == timespan_t::zero() || base_execute_time < timespan_t::zero() )
      return timespan_t::zero();

    return t;
  }

  void trigger_go_for_the_throat()
  {
    if ( !p() -> specs.go_for_the_throat -> ok() )
      return;

    if ( !p() -> active.pet )
      return;

    int gain = p() -> specs.go_for_the_throat -> effectN( 1 ).trigger() -> effectN( 1 ).base_value();
    p() -> active.pet -> resource_gain( RESOURCE_FOCUS, gain, p() -> active.pet -> gains.go_for_the_throat );
  }

  virtual void trigger_steady_focus( bool require_pre )
  {
    if ( !p() -> talents.steady_focus -> ok() )
      return;

    if ( require_pre )
    {
      p() -> buffs.pre_steady_focus -> trigger( 1 );
      if ( p() -> buffs.pre_steady_focus -> stack() < 2 )
        return;
    }

    // either two required shots have happened or we don't require them
    double regen_buff = p() -> buffs.steady_focus -> data().effectN( 1 ).percent();
    p() -> buffs.steady_focus -> trigger( 1, regen_buff );
    p() -> buffs.pre_steady_focus -> expire();
  }

  void trigger_tier15_2pc_melee()
  {
    if ( !p() -> sets.has_set_bonus( SET_MELEE, T15, B2 ) )
      return;

    if ( ( p() -> ppm_tier15_2pc_melee.trigger() ) )
    {
      p() -> procs.tier15_2pc_melee -> occur();
      size_t i;

      for ( i = 0; i < p() -> thunderhawk.size(); i++ )
      {
        if ( !p() -> thunderhawk[i] -> is_sleeping() )
          continue;

        p() -> thunderhawk[i] -> summon( timespan_t::from_seconds( 10 ) );
        break;
      }

      assert( i < p() -> thunderhawk.size() );
    }
  }

  void trigger_tier15_4pc_melee( proc_t* proc, attack_t* attack );
};

// Ranged Attack ============================================================

struct ranged_t: public hunter_ranged_attack_t
{
  bool first_shot;
  ranged_t( hunter_t* player, const char* name = "ranged", const spell_data_t* s = spell_data_t::nil() ):
    hunter_ranged_attack_t( name, player, s ), first_shot( true )
  {
    school = SCHOOL_PHYSICAL;
    weapon = &( player -> main_hand_weapon );
    base_execute_time = weapon -> swing_time;
    background = true;
    repeating = true;
    special = false;
  }

  void reset() override
  {
    hunter_ranged_attack_t::reset();

    first_shot = true;
  }

  virtual timespan_t execute_time() const override
  {
    timespan_t t = hunter_ranged_attack_t::execute_time();
    if ( first_shot )
      return timespan_t::from_millis( 100 );
    else
      return t;
  }

  virtual void execute() override
  {
    if ( first_shot )
      first_shot = false;
    hunter_ranged_attack_t::execute();
  }

  virtual void try_steady_focus() override
  {
  }

  virtual void impact( action_state_t* s ) override
  {
    hunter_ranged_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      if ( p() -> active.ammo != NO_AMMO )
      {
        if ( p() -> active.ammo == POISONED_AMMO )
        {
          const spell_data_t* poisoned_tick = p() -> find_spell( 170661 );
          double damage = s -> attack_power * ( poisoned_tick -> effectN( 1 ).ap_coeff() *
                                                ( poisoned_tick -> duration() / poisoned_tick -> effectN( 1 ).period() ) );
          residual_action::trigger(
            p() -> active.poisoned_ammo, // ignite spell
            s -> target, // target
            damage );
        }
        else if ( p() -> active.ammo == FROZEN_AMMO )
          p() -> active.frozen_ammo -> execute();
        else if ( p() -> active.ammo == INCENDIARY_AMMO )
          p() -> active.incendiary_ammo -> execute();
      }
      if ( s -> result == RESULT_CRIT )
        trigger_go_for_the_throat();
    }
  }
};

// Auto Shot ================================================================

struct auto_shot_t: public ranged_t
{
  auto_shot_t( hunter_t* p ): ranged_t( p, "auto_shot", spell_data_t::nil() )
  {
    school = SCHOOL_PHYSICAL;
    range = 40.0;
  }
};

struct start_attack_t: public hunter_ranged_attack_t
{
  start_attack_t( hunter_t* p, const std::string& options_str ):
    hunter_ranged_attack_t( "start_auto_shot", p, spell_data_t::nil() )
  {
    parse_options( options_str );

    p -> main_hand_attack = new auto_shot_t( p );
    stats = p -> main_hand_attack -> stats;
    ignore_false_positive = true;

    trigger_gcd = timespan_t::zero();
  }

  virtual void execute() override
  {
    p() -> main_hand_attack -> schedule_execute();
  }

  virtual bool ready() override
  {
    return( player -> main_hand_attack -> execute_event == nullptr ); // not swinging
  }
};

// Exotic Munitions ==================================================================

struct exotic_munitions_poisoned_ammo_t: public residual_action::residual_periodic_action_t < hunter_ranged_attack_t >
{
  exotic_munitions_poisoned_ammo_t( hunter_t* p, const char* name, const spell_data_t* s ):
    base_t( name, p, s )
  {
    may_crit = true;
    tick_may_crit = true;
  }

  void init() override
  {
    base_t::init();

    snapshot_flags |= STATE_CRIT | STATE_TGT_CRIT ;
  }
};

struct exotic_munitions_incendiary_ammo_t: public hunter_ranged_attack_t
{
  exotic_munitions_incendiary_ammo_t( hunter_t* p, const char* name, const spell_data_t* s ):
    hunter_ranged_attack_t( name, p, s )
  {
    aoe = -1;
  }
};

struct exotic_munitions_frozen_ammo_t: public hunter_ranged_attack_t
{
  exotic_munitions_frozen_ammo_t( hunter_t* p, const char* name, const spell_data_t* s ):
    hunter_ranged_attack_t( name, p, s )
  {
  }
};

struct exotic_munitions_t: public hunter_ranged_attack_t
{
  std::string ammo_type;
  exotic_munitions swap_ammo;
  exotic_munitions_t( hunter_t* player, const std::string& options_str ):
    hunter_ranged_attack_t( "exotic_munitions", player, player -> talents.exotic_munitions )
  {
    add_option( opt_string( "ammo_type", ammo_type ) );
    parse_options( options_str );

    callbacks = false;
    harmful = false;
    ignore_false_positive = true;

    if ( !ammo_type.empty() )
    {
      if ( ammo_type == "incendiary" || ammo_type == "incen" || ammo_type == "incendiary_ammo" || ammo_type == "fire" )
        swap_ammo = INCENDIARY_AMMO;
      else if ( ammo_type == "poisoned" || ammo_type == "poison" || ammo_type == "poisoned_ammo" )
        swap_ammo = POISONED_AMMO;
      else if ( ammo_type == "frozen" || ammo_type == "frozen_ammo" || ammo_type == "ice"  )
        swap_ammo = FROZEN_AMMO;
    }
  }

  void execute() override
  {
    hunter_ranged_attack_t::execute();
    p() -> active.ammo = swap_ammo;
  }

  bool ready() override
  {
    if ( p() -> active.ammo == swap_ammo )
      return false;

    return hunter_ranged_attack_t::ready();
  }
};

// Aimed Shot ===============================================================

struct aimed_shot_t: public hunter_ranged_attack_t
{
  // focus gained on every Aimed Shot crit
  double crit_gain;
  benefit_t* aimed_in_ca;
  aimed_shot_t( hunter_t* p, const std::string& options_str ):
    hunter_ranged_attack_t( "aimed_shot", p, p -> find_specialization_spell( "Aimed Shot" ) ),
    aimed_in_ca( p -> get_benefit( "aimed_in_careful_aim" ) )
  {
    parse_options( options_str );

    crit_gain = p -> perks.enhanced_aimed_shot -> effectN( 1 ).resource( RESOURCE_FOCUS );
    crit_gain += p -> sets.set( HUNTER_MARKSMANSHIP, T17, B2 ) -> effectN( 1 ).resource( RESOURCE_FOCUS );
    base_multiplier *= 1.0 + p -> sets.set( SET_MELEE, T16, B2 ) -> effectN( 1 ).percent();
    base_execute_time *= 1.0 - ( p -> sets.set( HUNTER_MARKSMANSHIP, T18, B4 ) -> effectN( 2 ).percent() );
  }

  virtual double cost() const override
  {
    return thrill_discount( hunter_ranged_attack_t::cost() );
  }

  virtual double composite_target_crit( player_t* t ) const override
  {
    double cc = hunter_ranged_attack_t::composite_target_crit( t );

    cc += p() -> buffs.careful_aim -> value();
    cc += p() -> sets.set( SET_MELEE, T16, B4 ) -> effectN( 2 ).percent();

    return cc;
  }

  virtual void impact( action_state_t* s ) override
  {
    hunter_ranged_attack_t::impact( s );

    if ( s -> result == RESULT_CRIT && crit_gain > 0.0 )
      p() -> resource_gain( RESOURCE_FOCUS, crit_gain, p() -> gains.aimed_shot );
  }

  virtual void execute() override
  {
    hunter_ranged_attack_t::execute();
    aimed_in_ca -> update( p() -> buffs.careful_aim -> check() != 0 );
    consume_thrill_of_the_hunt();
    if ( p() -> sets.has_set_bonus( HUNTER_MARKSMANSHIP, PVP, B4 ) )
    {
      p() -> cooldowns.rapid_fire -> adjust( -p() -> sets.set( HUNTER_MARKSMANSHIP, PVP, B4 ) -> effectN( 1 ).time_value() );
    }
  }
};

// Freezing Trap ==============================================================
// Implemented here because often there are buffs associated with it

struct freezing_trap_t: public hunter_ranged_attack_t
{
  gain_t* freezing_trap_gain;
  freezing_trap_t( hunter_t* player, const std::string& options_str ):
    hunter_ranged_attack_t( "freezing_trap", player, player -> find_class_spell( "Freezing Trap" ) )
  {
    parse_options( options_str );

    cooldown -> duration = data().cooldown();
    if ( p() -> perks.enhanced_traps -> ok() )
      cooldown -> duration *= ( 1.0 + p() -> perks.enhanced_traps -> effectN( 1 ).percent() );

    freezing_trap_gain = player -> get_gain( "freezing_trap" );
    
    // BUG simulate slow velocity of launch
    travel_speed = 18.0;
  }
   
  virtual void impact( action_state_t* s ) override
  {
    hunter_ranged_attack_t::impact( s );
    if ( p() -> sets.has_set_bonus( p() -> specialization(), PVP, B2 ) && result_is_hit( s -> result ) )
    {
      int focus = p() -> sets.set( p() -> specialization(), PVP, B2 ) ->  effectN( 1 ).trigger() -> effectN( 1 ).base_value();
      p() -> resource_gain( RESOURCE_FOCUS, focus, freezing_trap_gain );
    }
  }
};

// Explosive Trap ==============================================================

struct explosive_trap_tick_t: public hunter_ranged_attack_t
{
  explosive_trap_tick_t( hunter_t* player ):
    hunter_ranged_attack_t( "explosive_trap_tick", player, player -> find_spell( 13812 ) )
  {
    aoe = -1;
    hasted_ticks = false;
    dot_duration = data().duration();
    base_tick_time = data().effectN( 2 ).period();
    attack_power_mod.direct = data().effectN( 1 ).ap_coeff();
    // BUG in game it uses the direct damage AP mltiplier for ticks as well.
    attack_power_mod.tick = attack_power_mod.direct;
    ignore_false_positive = ground_aoe = background = true;
  }
};

struct explosive_trap_t: public hunter_ranged_attack_t
{
  explosive_trap_tick_t* tick;
  explosive_trap_t( hunter_t* player, const std::string& options_str ):
    hunter_ranged_attack_t( "explosive_trap", player, player -> find_class_spell( "Explosive Trap" ) ),
    tick( new explosive_trap_tick_t( player ) )
  {
    parse_options( options_str );

    cooldown -> duration = data().cooldown();
    if ( p() -> perks.enhanced_traps -> ok() )
      cooldown -> duration *= ( 1.0 + p() -> perks.enhanced_traps -> effectN( 1 ).percent() );
    range = 40.0;
    // BUG simulate slow velocity of launch
    travel_speed = 18.0;
    add_child( tick );
    impact_action = tick;
 }
};

// Chimaera Shot =============================================================

struct chimaera_shot_impact_t: public hunter_ranged_attack_t
{
  chimaera_shot_impact_t( hunter_t* p, const char* name, const spell_data_t* s ):
    hunter_ranged_attack_t( name, p, s )
  {
    dual = true;
    aoe = 2;
    radius = 5.0;
  }

  void impact( action_state_t* s ) override
  {
    hunter_ranged_attack_t::impact( s );
    if ( p() -> sets.has_set_bonus( HUNTER_MARKSMANSHIP, T18, B2 ) )
    {
      if ( s -> result == RESULT_CRIT && p() -> buffs.rapid_fire -> remains() < p() -> buffs.t18_2p_rapid_fire -> buff_duration )
      {
        p() -> buffs.t18_2p_rapid_fire -> trigger();

        if ( p() -> buffs.rapid_fire -> check() )
        {
          p() -> buffs.rapid_fire -> expire();
          p() -> procs.tier18_2pc_mm_wasted_proc -> occur();
        }
      }
    }
  }

  double action_multiplier() const override
  {
    double am = hunter_ranged_attack_t::action_multiplier();
    am *= 1.0 + p() -> sets.set( SET_MELEE, T14, B2 ) -> effectN( 2 ).percent();

    return am;
  }
};

struct chimaera_shot_t: public hunter_ranged_attack_t
{
  chimaera_shot_impact_t* frost;
  chimaera_shot_impact_t* nature;

  chimaera_shot_t( hunter_t* player, const std::string& options_str ):
    hunter_ranged_attack_t( "chimaera_shot", player, player -> specs.chimaera_shot ),
    frost( nullptr ), nature( nullptr )
  {
    parse_options( options_str );
    callbacks = false;
    frost = new chimaera_shot_impact_t( player, "chimaera_shot_frost", player -> find_spell( 171454 ) );
    add_child( frost );
    nature = new chimaera_shot_impact_t( player, "chimaera_shot_nature", player -> find_spell( 171457 ) );
    add_child( nature );
    school = SCHOOL_FROSTSTRIKE; // Just so the report shows a mixture of the two colors.
    starved_proc = player -> get_proc( "starved: chimaera_shot" );
  }

  void execute() override
  {
    hunter_ranged_attack_t::execute();

    if ( rng().roll( 0.5 ) ) // Chimaera shot has a 50/50 chance to roll frost or nature damage... for the flavorz.
      frost -> execute();
    else
      nature -> execute();
  }
};

// Cobra Shot Attack ========================================================

struct cobra_shot_t: public hunter_ranged_attack_t
{
  double focus_gain;
  proc_t* cast_in_BW;

  cobra_shot_t( hunter_t* player, const std::string& options_str ):
    hunter_ranged_attack_t( "cobra_shot", player, player -> find_specialization_spell( "Cobra Shot" ) )
  {
    parse_options( options_str );
    focus_gain = p() -> find_spell( 91954 ) -> effectN( 1 ).base_value();

    if ( p() -> sets.has_set_bonus( SET_MELEE, T13, B2 ) )
      focus_gain *= 2.0;

    cast_in_BW = player -> get_proc( "cobra_shot during bestial_wrath" );
  }

  virtual void try_steady_focus() override
  {
    trigger_steady_focus( true );
  }

  virtual void execute() override
  {
    hunter_ranged_attack_t::execute();

    if (p() -> buffs.bestial_wrath -> check() )
      cast_in_BW -> occur();

    if ( result_is_hit( execute_state -> result ) )
      trigger_tier15_2pc_melee();
  }

  virtual void impact( action_state_t* s ) override
  {
    hunter_ranged_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
      p() -> resource_gain( RESOURCE_FOCUS, focus_gain, p() -> gains.cobra_shot );
  }
};

// Kill Shot ================================================================

struct kill_shot_t: public hunter_ranged_attack_t
{
  kill_shot_t( hunter_t* player, const std::string& options_str ):
    hunter_ranged_attack_t( "kill_shot", player, player -> find_specialization_spell( "Kill Shot" ) )
  {
    parse_options( options_str );
  }

  virtual void execute() override
  {
    hunter_ranged_attack_t::execute();

    if ( p() -> cooldowns.kill_shot_reset -> up() )
    {
      cooldown -> reset( true );
      p() -> cooldowns.kill_shot_reset -> start();
    }
  }

  virtual bool ready() override
  {
    if ( target -> health_percentage() > ( p() -> perks.enhanced_kill_shot -> ok() ? 35 : 20 ) )
      return false;

    return hunter_ranged_attack_t::ready();
  }
};

// Serpent Sting Attack =====================================================

struct serpent_sting_t: public hunter_ranged_attack_t
{
  serpent_sting_t( hunter_t* player ):
    hunter_ranged_attack_t( "serpent_sting", player, player -> find_spell( 118253 ) )
  {
    background = true;
    proc = true;
    tick_may_crit = true;
    hasted_ticks = false;
  }
};

// Arcane Shot Attack =======================================================

struct arcane_shot_t: public hunter_ranged_attack_t
{
  arcane_shot_t( hunter_t* player, const std::string& options_str ):
    hunter_ranged_attack_t( "arcane_shot", player, player -> find_spell( "Arcane Shot" ) )
  {
    parse_options( options_str );
    base_multiplier *= 1.0 + player -> sets.set( SET_MELEE, T16, B2 ) -> effectN( 1 ).percent();
  }

  virtual double cost() const override
  {
    return thrill_discount( hunter_ranged_attack_t::cost() );
  }
  
  virtual void try_steady_focus() override
  {
    trigger_steady_focus( true );
  }

  virtual void execute() override
  {
    hunter_ranged_attack_t::execute();
    consume_thrill_of_the_hunt();

    if ( p() -> sets.has_set_bonus( HUNTER_BEAST_MASTERY, T18, B2 ) && p() -> buffs.focus_fire -> check() )
      p() -> buffs.focus_fire -> extend_duration( p(), p() -> sets.set( HUNTER_BEAST_MASTERY, T18, B2 ) -> effectN( 1 ).time_value() );

    if ( result_is_hit( execute_state -> result ) )
      trigger_tier15_4pc_melee( p() -> procs.tier15_4pc_melee_arcane_shot, p() -> action_lightning_arrow_arcane_shot );

    trigger_blackness();
  }

  virtual void impact( action_state_t* s ) override
  {
    hunter_ranged_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      if ( p() -> talents.serpent_sting -> ok() )
      {
        p() -> active.serpent_sting -> target = s -> target;
        p() -> active.serpent_sting -> execute();
      }
    }
    if ( result_is_hit( s -> result ) )
    {
      p() -> buffs.cobra_strikes -> trigger();
      p() -> buffs.tier13_4pc -> trigger();
    }
  }
};

// Multi Shot Attack ========================================================

struct multi_shot_t: public hunter_ranged_attack_t
{
  multi_shot_t( hunter_t* player, const std::string& options_str ):
    hunter_ranged_attack_t( "multi_shot", player, player -> find_class_spell( "Multi-Shot" ) )
  {
    parse_options( options_str );

    aoe = -1;
    base_multiplier *= 1.0 + player -> sets.set( SET_MELEE, T16, B2 ) -> effectN( 1 ).percent();
  }

  virtual double cost() const override
  {
    double cost = hunter_ranged_attack_t::cost();

    cost = thrill_discount( cost );
    if ( p() -> buffs.bombardment -> check() )
      cost += 1 + p() -> buffs.bombardment -> data().effectN( 1 ).base_value();

    return cost;
  }

  virtual double action_multiplier() const override
  {
    double am = hunter_ranged_attack_t::action_multiplier();
    if ( p() -> buffs.bombardment -> up() )
      am *= 1 + p() -> buffs.bombardment -> data().effectN( 2 ).percent();
    return am;
  }

  virtual void execute() override
  {
    hunter_ranged_attack_t::execute();
    consume_thrill_of_the_hunt();
    trigger_blackness();

    if ( p() -> sets.has_set_bonus( HUNTER_BEAST_MASTERY, T18, B2 ) && p() -> buffs.focus_fire -> check() )
      p() -> buffs.focus_fire -> extend_duration( p(), p() -> sets.set( HUNTER_BEAST_MASTERY, T18, B2 ) -> effectN( 1 ).time_value() );

    pets::hunter_main_pet_t* pet = p() -> active.pet;
    if ( pet && p() -> specs.beast_cleave -> ok() )
      pet -> buffs.beast_cleave -> trigger();

    if ( result_is_hit( execute_state -> result ) )
      trigger_tier15_4pc_melee( p() -> procs.tier15_4pc_melee_multi_shot, p() -> action_lightning_arrow_multi_shot );
  }

  virtual void impact( action_state_t* s ) override
  {
    hunter_ranged_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      if ( p() -> talents.serpent_sting -> ok() )
      {
        p() -> active.serpent_sting -> target = s -> target;
        p() -> active.serpent_sting -> execute();
      }
    }
    if ( result_is_hit( s -> result ) )
      if ( s -> result == RESULT_CRIT && p() -> specs.bombardment -> ok() )
        p() -> buffs.bombardment -> trigger();
  }
};

// Steady Shot Attack =======================================================

struct steady_shot_t: public hunter_ranged_attack_t
{
  double focus_gain;
  steady_shot_t( hunter_t* player, const std::string& options_str ):
    hunter_ranged_attack_t( "steady_shot", player, player -> find_class_spell( "Steady Shot" ) )
  {
    parse_options( options_str );

    focus_gain = p() -> find_spell( 77443 ) -> effectN( 1 ).base_value();
    // Needs testing
    if ( p() -> sets.has_set_bonus( SET_MELEE, T13, B2 ) )
      focus_gain *= 2.0;
  }


  virtual void execute() override
  {
    hunter_ranged_attack_t::execute();

    if ( result_is_hit( execute_state -> result ) )
    {
      trigger_tier15_2pc_melee();
      p() -> resource_gain( RESOURCE_FOCUS, focus_gain, p() -> gains.steady_shot );
    }
  }

  virtual double composite_target_crit( player_t* t ) const override
  {
    double cc = hunter_ranged_attack_t::composite_target_crit( t );

    cc += p() -> buffs.careful_aim -> value();

    return cc;
  }
};

// Lightning Arrow (Tier 15 4-piece bonus) ==================================

struct lightning_arrow_t: public hunter_ranged_attack_t
{
  lightning_arrow_t( hunter_t* p, const std::string& attack_suffix ):
    hunter_ranged_attack_t( "lightning_arrow" + attack_suffix, p, p -> find_spell( 138366 ) )
  {
    school = SCHOOL_NATURE;
    proc = true;
    background = true;
  }
};

void hunter_ranged_attack_t::trigger_tier15_4pc_melee( proc_t* proc, attack_t* attack )
{
  if ( !p() -> sets.has_set_bonus( SET_MELEE, T15, B4 ) )
    return;

  if ( p() -> ppm_tier15_4pc_melee.trigger() )
  {
    proc -> occur();
    attack -> execute();
  }
}

} // end attacks

// ==========================================================================
// Hunter Spells
// ==========================================================================

namespace spells
{
struct hunter_spell_t: public hunter_action_t < spell_t >
{
public:
  hunter_spell_t( const std::string& n, hunter_t* player,
                  const spell_data_t* s = spell_data_t::nil() ):
                  base_t( n, player, s )
  {
  }

  virtual timespan_t gcd() const override
  {
    if ( !harmful && !player -> in_combat )
      return timespan_t::zero();

    // Hunter gcd unaffected by haste
    return trigger_gcd;
  }

  virtual void execute() override
  {
    hunter_action_t<spell_t>::execute();
    
    this -> try_steady_focus();
  }
};

struct aspect_of_the_fox_t: public hunter_spell_t
{
  aspect_of_the_fox_t( hunter_t* player, const std::string& options_str ):
    hunter_spell_t( "aspect_of_the_fox", player, player -> find_class_spell( "Aspect of the Fox" ) )
  {
    parse_options( options_str );
    harmful = false;
    trigger_gcd = timespan_t::zero();
  }

  virtual void execute() override
  {
    hunter_spell_t::execute();
    for ( size_t i = 0; i < sim -> player_non_sleeping_list.size(); ++i )
    {
      player_t* p = sim -> player_non_sleeping_list[i];
      if ( p -> type == PLAYER_GUARDIAN )
        continue;
      p -> buffs.aspect_of_the_fox -> trigger();
    }
  }
};


struct aspect_of_the_pack_t: public hunter_spell_t
{
  std::string toggle;
  bool tog;
  aspect_of_the_pack_t( hunter_t* player, const std::string& options_str ):
    hunter_spell_t( "aspect_of_the_pack", player, player -> find_class_spell( "Aspect of the Pack" ) ),
    toggle( "" ), tog( true )
  {
    add_option( opt_string( "toggle", toggle ) );
    parse_options( options_str );

    if ( !toggle.empty() )
    {
      if ( toggle == "on" )
        tog = true;
      else if ( toggle == "off" )
        tog = false;
    }
    harmful = false;
    trigger_gcd = timespan_t::zero();
  }

  virtual void execute() override
  {
    if ( tog == true )
    {
      for ( size_t i = 0; i < sim -> player_non_sleeping_list.size(); ++i )
      {
        player_t* p = sim -> player_non_sleeping_list[i];
        if ( p -> type == PLAYER_GUARDIAN )
          continue;
        p -> buffs.aspect_of_the_pack -> trigger();
      }
    }
    else if ( tog == false )
    {
      for ( size_t i = 0; i < sim -> player_non_sleeping_list.size(); ++i )
      {
        player_t* p = sim -> player_non_sleeping_list[i];
        if ( p -> type == PLAYER_GUARDIAN )
          continue;
        p -> buffs.aspect_of_the_pack -> expire();
      }
    }
  }

  virtual bool ready() override
  {
    if ( ( p() -> buffs.aspect_of_the_pack -> check() && tog == true ) ||
         ( !p() -> buffs.aspect_of_the_pack -> check() && tog == false ) )
         return false;

    return hunter_spell_t::ready();
  }
};

// Barrage ==================================================================
// This is a spell because that's the only way to support "channeled" effects

struct barrage_t: public hunter_spell_t
{
  struct barrage_damage_t: public attacks::hunter_ranged_attack_t
  {
    barrage_damage_t( hunter_t* player ):
      attacks::hunter_ranged_attack_t( "barrage_primary", player, player -> talents.barrage -> effectN( 2 ).trigger() )
    {
      background = true;
      may_crit = true;
      weapon = &( player -> main_hand_weapon );
      base_execute_time = weapon -> swing_time;
      aoe = -1;
      base_aoe_multiplier = 0.5;
      range = radius;
      range = 0;
    }
  };

  barrage_t( hunter_t* player, const std::string& options_str ):
    hunter_spell_t( "barrage", player, player -> talents.barrage )
  {
    parse_options( options_str );

    may_block = false;
    hasted_ticks = false;
    channeled = true;

    tick_zero = true;
    dynamic_tick_action = true;
    travel_speed = 0.0;
    tick_action = new barrage_damage_t( player );

    starved_proc = player -> get_proc( "starved: barrage" );
  }

 void schedule_execute( action_state_t* state = nullptr ) override
  {
    hunter_spell_t::schedule_execute( state );

    // To suppress autoshot, just delay it by the execute time of the barrage
    if ( p() -> main_hand_attack && p() -> main_hand_attack -> execute_event )
    {
      timespan_t time_to_next_hit = p() -> main_hand_attack -> execute_event -> remains();
      time_to_next_hit += dot_duration;
      p() -> main_hand_attack -> execute_event -> reschedule( time_to_next_hit );
    }
    trigger_tier16_bm_4pc_melee();
    trigger_thrill_of_the_hunt();
    p() -> no_steady_focus();
  }

 bool usable_moving() const override
  {
    return true;
  }
};

// A Murder of Crows ========================================================

struct peck_t: public ranged_attack_t
{
  peck_t( hunter_t* player, const std::string& name ):
    ranged_attack_t( name, player, player -> find_spell( 131900 ) )
  {
    dual = true;
    may_crit = true;
    may_parry = false;
    may_block = false;
    may_dodge = false;
    travel_speed = 0.0;
  }

  hunter_t* p() const { return static_cast<hunter_t*>( player ); }

  virtual double action_multiplier() const override
  {
    double am = ranged_attack_t::action_multiplier();
    am *= p() -> beast_multiplier();
    return am;
  }
};

// TODO this should reset CD if the target dies
struct moc_t: public ranged_attack_t
{
  peck_t* peck;
  moc_t( hunter_t* player, const std::string& options_str ):
    ranged_attack_t( "a_murder_of_crows", player, player -> talents.a_murder_of_crows ),
    peck( new peck_t( player, "crow_peck" ) )
  {
    parse_options( options_str );
    add_child( peck );
    hasted_ticks = false;
    callbacks = false;
    may_crit = false;
    may_miss = false;
    may_parry = false;
    may_block = false;
    may_dodge = false;

    starved_proc = player -> get_proc( "starved: a_murder_of_crows" );
  }

  hunter_t* p() const { return static_cast<hunter_t*>( player ); }

  void tick( dot_t*d ) override
  {
    ranged_attack_t::tick( d );
    peck -> execute();
  }

  virtual double cost() const override
  {
    double c = ranged_attack_t::cost();
    if ( p() -> buffs.bestial_wrath -> check() )
      c *= 1.0 + p() -> buffs.bestial_wrath -> data().effectN( 6 ).percent();
    return c;
  }

  virtual void execute() override
  {
    trigger_tier16_bm_4pc_brutal_kinship( p() );
    
    p() -> no_steady_focus();
    ranged_attack_t::execute();
  }
};

// Dire Beast ===============================================================

struct dire_beast_t: public hunter_spell_t
{
  dire_beast_t( hunter_t* player, const std::string& options_str ):
    hunter_spell_t( "dire_beast", player, player -> specs.dire_beast )
  {
    parse_options( options_str );
    harmful = false;
    hasted_ticks = false;
    may_crit = false;
    may_miss = false;
    school = SCHOOL_PHYSICAL;
  }

  bool init_finished() override
  {
    if ( p() -> pet_dire_beasts[ 0 ] )
    {
      stats -> add_child( p() -> pet_dire_beasts[ 0 ] -> get_stats( "dire_beast_melee" ) );
    }

    return hunter_spell_t::init_finished();
  }

  virtual void execute() override
  {
    hunter_spell_t::execute();
    p() -> no_steady_focus();

    pet_t* beast = p() -> pet_dire_beasts[0];
    if ( !beast -> is_sleeping() )
      beast = p() -> pet_dire_beasts[1];

    // Dire beast gets a chance for an extra attack based on haste
    // rather than discrete plateaus.  At integer numbers of attacks,
    // the beast actually has a 50% chance of n-1 attacks and 50%
    // chance of n.  It (apparently) scales linearly between n-0.5
    // attacks to n+0.5 attacks.  This uses beast duration to
    // effectively alter the number of attacks as the duration itself
    // isn't important and combat log testing shows some variation in
    // attack speeds.  This is not quite perfect but more accurate
    // than plateaus.
    const timespan_t base_duration = timespan_t::from_seconds( 15.0 );
    const timespan_t swing_time = beast -> main_hand_weapon.swing_time * beast -> composite_melee_speed();
    double partial_attacks_per_summon = base_duration / swing_time;
    double base_attacks_per_summon = floor( partial_attacks_per_summon - 0.5 ); // 8.4 -> 7, 8.5 -> 8, 8.6 -> 8, etc

    if ( rng().roll( partial_attacks_per_summon - base_attacks_per_summon - 0.5 ) )
      base_attacks_per_summon += 1;

    timespan_t duration = base_attacks_per_summon * swing_time;
    beast -> summon( duration );
  }
};

// Bestial Wrath ============================================================

struct bestial_wrath_t: public hunter_spell_t
{
  bestial_wrath_t( hunter_t* player, const std::string& options_str ):
    hunter_spell_t( "bestial_wrath", player, player -> specs.bestial_wrath )
  {
    parse_options( options_str );
    harmful = false;
  }

  virtual void execute() override
  {
    p() -> buffs.bestial_wrath  -> trigger();
    p() -> active.pet -> buffs.bestial_wrath -> trigger();
    if ( p() -> sets.has_set_bonus( HUNTER_BEAST_MASTERY, T17, B4 ) )
    {
      const timespan_t duration = p() -> buffs.bestial_wrath -> buff_duration;
      // start from the back so we don't overlap stampede pets in reporting
      for ( size_t i = p() -> hunter_main_pets.size(); i-- > 0; )
      {
        if ( p() -> hunter_main_pets[i] -> tier_pet_summon( duration ) )
          break;
      }
    }
    hunter_spell_t::execute();
  }

  virtual bool ready() override
  {
    if ( !p() -> active.pet )
      return false;

    return hunter_spell_t::ready();
  }
};

// Focus Fire ===============================================================

struct focus_fire_buff_t : public buff_t
{
  focus_fire_buff_t( const buff_creator_t& creator ) : buff_t( creator ) {}

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    buff_t::expire_override( expiration_stacks, remaining_duration );
    
    hunter_t* p = debug_cast<hunter_t*>( player );
    if ( p -> sets.has_set_bonus( HUNTER_BEAST_MASTERY, T18, B4 ) )
    {
      // unsummon an active t18 pet
      for ( size_t i = p -> hunter_main_pets.size(); i-- > 0; )
      {
        pets::hunter_main_pet_t* pet = p -> hunter_main_pets[i];
        // we manually dismiss the pets when the buff expires because it focus fir emay be extended
        if ( pet -> buffs.tier18_4pc_bm -> check() )
        {
          pet -> buffs.tier18_4pc_bm -> expire();
          pet -> dismiss();
        }
      }
    }
  }
};

struct focus_fire_t: public hunter_spell_t
{
  int min_frenzy;

  focus_fire_t( hunter_t* player, const std::string& options_str ):
    hunter_spell_t( "focus_fire", player, player -> find_spell( 82692 ) ),
    min_frenzy( 1 )
  {
    int five_stacks = 0;
    int min_stacks = 0;
    add_option( opt_bool( "five_stacks", five_stacks ) );
    add_option( opt_int( "stacks", min_stacks ) );
    add_option( opt_int( "min_frenzy", min_frenzy ) );
    parse_options( options_str );
    if ( five_stacks )
      min_frenzy = std::max(min_frenzy, 5);
    else if ( min_stacks )
      min_frenzy = std::max(min_frenzy, min_stacks);

    harmful = false;
  }

  virtual void execute() override
  {
    int stacks = p() -> active.pet -> buffs.frenzy -> stack();
    double value = stacks * p() -> specs.focus_fire -> effectN( 1 ).percent();
    p() -> buffs.focus_fire -> trigger( stacks, value );
    if ( p() -> sets.has_set_bonus( HUNTER_BEAST_MASTERY, T18, B4 ) )
    {
      // start from the back so we don't overlap stampede pets in reporting
      for ( size_t i = p() -> hunter_main_pets.size(); i-- > 0; )
      {
        // we manually unsummon the pets when the buff expires because it focus fir emay be extended
        if ( p() -> hunter_main_pets[i] -> tier_pet_summon( timespan_t::zero() ) )
          break;
      }
    }

    double gain = stacks * p() -> specs.focus_fire -> effectN( 2 ).resource( RESOURCE_FOCUS );
    p() -> active.pet -> resource_gain( RESOURCE_FOCUS, gain, p() -> active.pet -> gains.focus_fire );

    hunter_spell_t::execute();
    p() -> active.pet -> buffs.frenzy -> expire();
    trigger_tier16_bm_4pc_melee();
    p() -> no_steady_focus();
  }

  virtual bool ready() override
  {
    if ( !p() -> active.pet )
      return false;

    if ( !p() -> active.pet -> buffs.frenzy -> check() )
      return false;
    
    // Focus fire cannot be triggered while the focus_fire buff is active.
    if ( p() -> buffs.focus_fire -> check() )
      return false;

    if ( p() -> active.pet -> buffs.frenzy -> stack_react() < min_frenzy )
      return false;

    return hunter_spell_t::ready();
  }
};

// Kill Command =============================================================

struct kill_command_t: public hunter_spell_t
{
  kill_command_t( hunter_t* player, const std::string& options_str ):
    hunter_spell_t( "kill_command", player, player -> specs.kill_command )
  {
    parse_options( options_str );
    harmful = false;
  }

  bool init_finished() override
  {
    for (auto pet : p() -> pet_list)
    {
      
      stats -> add_child( pet -> get_stats( "kill_command" ) );
    }

    return hunter_spell_t::init_finished();
  }

  virtual void execute() override
  {
    hunter_spell_t::execute();
    trigger_tier16_bm_4pc_melee();

    if ( p() -> active.pet )
    {
      p() -> active.pet -> active.kill_command -> execute();
      trigger_tier17_2pc_bm();
    }
    p() -> no_steady_focus();
  }

  virtual bool ready() override
  {
    if ( p() -> active.pet && p() -> active.pet -> active.kill_command -> ready() ) // Range check from the pet.
      return hunter_spell_t::ready();

    return false;
  }

  bool trigger_tier17_2pc_bm()
  {
    if ( !p() -> sets.has_set_bonus( HUNTER_BEAST_MASTERY, T17, B2 ) )
      return false;

    bool procced = rng().roll( p() -> sets.set( HUNTER_BEAST_MASTERY, T17, B2 ) -> proc_chance() );
    if ( procced )
    {
      p() -> cooldowns.bestial_wrath -> reset( true );
      p() -> procs.tier17_2pc_bm -> occur();
    }

    return procced;
  }
};

// Rapid Fire ===============================================================

struct rapid_fire_t: public hunter_spell_t
{
  double value;
  rapid_fire_t( hunter_t* player, const std::string& options_str ):
    hunter_spell_t( "rapid_fire", player, player -> specs.rapid_fire )
  {
    parse_options( options_str );
    value = data().effectN( 1 ).percent();
  }

  virtual void execute() override
  {
    p() -> buffs.rapid_fire -> trigger( 1, value );
    {
      p() -> procs.tier18_2pc_mm_wasted_overwrite -> occur();
      p() -> buffs.t18_2p_rapid_fire -> expire();
    }

    hunter_spell_t::execute();
  }
};

// Summon Pet ===============================================================

struct summon_pet_t: public hunter_spell_t
{
  std::string pet_name;
  pet_t* pet;
  summon_pet_t( hunter_t* player, const std::string& options_str ):
    hunter_spell_t( "summon_pet", player ),
    pet( nullptr )
  {
    harmful = false;
    callbacks = false;
    ignore_false_positive = true;
    pet_name = options_str.empty() ? p() -> summon_pet_str : options_str;
  }

  bool init_finished() override
  {
    if ( ! pet )
    {
      pet = player -> find_pet( pet_name );
    }

    if ( ! pet && p() -> specialization() != HUNTER_MARKSMANSHIP )
    {
      sim -> errorf( "Player %s unable to find pet %s for summons.\n", p() -> name(), pet_name.c_str() );
      sim -> cancel();
    }

    return hunter_spell_t::init_finished();
  }

  virtual void execute() override
  {
    hunter_spell_t::execute();
    pet -> type = PLAYER_PET;
    pet -> summon();

    if ( p() -> main_hand_attack ) p() -> main_hand_attack -> cancel();
  }

  virtual bool ready() override
  {
    if ( p() -> active.pet == pet || p() -> specialization() == HUNTER_MARKSMANSHIP ) 
      return false;

    return hunter_spell_t::ready();
  }
};

// Stampede =================================================================

struct stampede_t: public hunter_spell_t
{
  stampede_t( hunter_t* p, const std::string& options_str ):
    hunter_spell_t( "stampede", p, p -> talents.stampede )
  {
    parse_options( options_str );
    harmful = false;
    callbacks = false;
    school = SCHOOL_PHYSICAL;
  }

  virtual void execute() override
  {
    hunter_spell_t::execute();
    trigger_tier16_bm_4pc_melee();
    p() -> buffs.stampede -> trigger();

    for ( unsigned int i = 0; i < p() -> hunter_main_pets.size() && i < 5; ++i )
    {
      p() -> hunter_main_pets[i] -> stampede_summon( p() -> buffs.stampede -> buff_duration + timespan_t::from_millis( 27 ) );
      // Added 0.027 seconds to properly reflect haste threshholds seen in game.
    }
    p() -> no_steady_focus();
  }
};
}

hunter_td_t::hunter_td_t( player_t* target, hunter_t* p ):
actor_target_data_t( target, p ),
dots( dots_t() )
{
  dots.serpent_sting = target -> get_dot( "serpent_sting", p );
  dots.poisoned_ammo = target -> get_dot( "poisoned_ammo", p );

  debuffs_survival_t18_2p = buff_creator_t( *this, "black_decay", p -> sets.set( HUNTER_SURVIVAL, T18, B2 ) -> effectN( 1 ).trigger() );
  debuffs_pvp_surv_4p_black_fire = buff_creator_t( *this, "debuffs_pvp_surv_4p_black_fire", p -> sets.set( HUNTER_SURVIVAL, PVP, B4 ) -> effectN( 1 ).trigger() );
}
 

expr_t* hunter_t::create_expression( action_t* a, const std::string& name )
{
  return player_t::create_expression( a, name );
}


// hunter_t::create_action ==================================================

action_t* hunter_t::create_action( const std::string& name,
                                   const std::string& options_str )
{
  using namespace attacks;
  using namespace spells;

  if ( name == "auto_shot"             ) return new           start_attack_t( this, options_str );
  if ( name == "aimed_shot"            ) return new             aimed_shot_t( this, options_str );
  if ( name == "arcane_shot"           ) return new            arcane_shot_t( this, options_str );
  if ( name == "bestial_wrath"         ) return new          bestial_wrath_t( this, options_str );
  if ( name == "chimaera_shot"         ) return new          chimaera_shot_t( this, options_str );
  if ( name == "exotic_munitions"      ) return new       exotic_munitions_t( this, options_str );
  if ( name == "explosive_trap"        ) return new         explosive_trap_t( this, options_str );
  if ( name == "freezing_trap"         ) return new          freezing_trap_t( this, options_str );
  if ( name == "focus_fire"            ) return new             focus_fire_t( this, options_str );
  if ( name == "kill_command"          ) return new           kill_command_t( this, options_str );
  if ( name == "kill_shot"             ) return new              kill_shot_t( this, options_str );
  if ( name == "multishot"             ) return new             multi_shot_t( this, options_str );
  if ( name == "multi_shot"            ) return new             multi_shot_t( this, options_str );
  if ( name == "rapid_fire"            ) return new             rapid_fire_t( this, options_str );
  if ( name == "steady_shot"           ) return new            steady_shot_t( this, options_str );
  if ( name == "summon_pet"            ) return new             summon_pet_t( this, options_str );
  if ( name == "a_murder_of_crows"     ) return new                    moc_t( this, options_str );
  if ( name == "aspect_of_the_fox"     ) return new      aspect_of_the_fox_t( this, options_str );
  if ( name == "aspect_of_the_pack"    ) return new     aspect_of_the_pack_t( this, options_str );
  if ( name == "stampede"              ) return new               stampede_t( this, options_str );
  if ( name == "dire_beast"            ) return new             dire_beast_t( this, options_str );
  if ( name == "barrage"               ) return new                barrage_t( this, options_str );

#if 0
  if ( name == "binding_shot"          ) return new           binding_shot_t( this, options_str );
  if ( name == "wyvern_sting"          ) return new           wyvern_sting_t( this, options_str );
#endif
  return player_t::create_action( name, options_str );
}

// hunter_t::create_pet =====================================================

pet_t* hunter_t::create_pet( const std::string& pet_name,
                             const std::string& pet_type )
{
  using namespace pets;
  // Blademaster pets have to always be explicitly created, cannot re-use the same pet as there are
  // many of them.
  if (pet_name == BLADEMASTER_PET_NAME) return new blademaster_pet_t(this);

  pet_t* p = find_pet( pet_name );

  if ( p )
    return p;

  pet_e type = util::parse_pet_type( pet_type );
  if ( type > PET_NONE && type < PET_HUNTER )
    return new pets::hunter_main_pet_t( *sim, *this, pet_name, type );
  else if ( pet_type != "" )
  {
    sim -> errorf( "Player %s with pet %s has unknown type %s\n", name(), pet_name.c_str(), pet_type.c_str() );
    sim -> cancel();
  }

  return nullptr;
}

// hunter_t::create_pets ====================================================

void hunter_t::create_pets()
{
  size_t create_pets = 1;
  if ( talents.stampede -> ok() ) // We only need to create 1 main pet if we are not using the stampede talent.
    create_pets += 4;
  for ( size_t i = 0; i < create_pets; i++ )
  {
    if ( glyphs.stampede -> ok() && talents.stampede -> ok() )
      create_pet( summon_pet_str + ( i > 0 ? util::to_string( i ) : "" ), summon_pet_str );
    else if ( !talents.stampede -> ok() )
      create_pet( summon_pet_str, summon_pet_str );
    else
    {
      create_pet( "raptor", "raptor" );
      create_pet( "wolf", "wolf" );
      create_pet( "hyena", "hyena" );
      create_pet( "devilsaur", "devilsaur" );
      create_pet( "cat", "cat" );
      break;
    }
  }

  if ( sets.has_set_bonus( HUNTER_BEAST_MASTERY, T17, B4 ) )
  {
    create_pet( "t17_pet_2", "wolf" );
    create_pet( "t17_pet_1", "wolf" );
  }
  if ( sets.has_set_bonus( HUNTER_BEAST_MASTERY, T18, B4 ) )
    create_pet( "t18_fel_boar", "boar" );

  if ( specs.dire_beast -> ok() )
  {
    for ( size_t i = 0; i < pet_dire_beasts.size(); ++i )
    {
      pet_dire_beasts[i] = new pets::dire_critter_t( *this, i + 1 );
    }
  }

  if ( sets.has_set_bonus( SET_MELEE, T15, B2 ) )
  {
    for ( int i = 0; i < 10; i++ )
    {
      thunderhawk[i] = new pets::tier15_thunderhawk_t( *this );
    }
  }
}

// hunter_t::init_spells ====================================================

void hunter_t::init_spells()
{
  player_t::init_spells();

  // Baseline
  // Talents
  talents.posthaste                         = find_talent_spell( "Posthaste" );
  talents.narrow_escape                     = find_talent_spell( "Narrow Escape" );

  talents.intimidation                      = find_talent_spell( "Intimidation" );
  talents.wyvern_sting                      = find_talent_spell( "Wyvern Sting" );
  talents.binding_shot                      = find_talent_spell( "Binding Shot" );

  talents.steady_focus                      = find_talent_spell( "Steady Focus" );
  talents.thrill_of_the_hunt                = find_talent_spell( "Thrill of the Hunt" );

  talents.a_murder_of_crows                 = find_talent_spell( "A Murder of Crows" );
  talents.blink_strikes                     = find_talent_spell( "Blink Strikes" );
  talents.stampede                          = find_talent_spell( "Stampede" );

  talents.barrage                           = find_talent_spell( "Barrage" );

  talents.exotic_munitions                  = find_talent_spell( "Exotic Munitions" );

  // Perks
  perks.enhanced_camouflage               = find_perk_spell( "Enhanced Camouflage" );
  perks.improved_focus_fire               = find_perk_spell( "Improved Focus Fire" );
  perks.improved_beast_cleave             = find_perk_spell( "Improved Beast Cleave" );
  perks.enhanced_basic_attacks            = find_perk_spell( "Enhanced Basic Attacks" );
  perks.enhanced_kill_shot                = find_perk_spell( "Enhanced Kill Shot" );
  perks.enhanced_aimed_shot               = find_perk_spell( "Enhanced Aimed Shot" );
  perks.improved_focus                    = find_perk_spell( "Improved Focus" );
  perks.empowered_explosive_shot          = find_perk_spell( "Empowered Explosive Shot" );
  perks.enhanced_traps                    = find_perk_spell( "Enhanced Traps" );
  perks.enhanced_entrapment               = find_perk_spell( "Enhanced Entrapment" );

  // Mastery
  mastery.master_of_beasts     = find_mastery_spell( HUNTER_BEAST_MASTERY );
  mastery.sniper_training      = find_mastery_spell( HUNTER_MARKSMANSHIP );
  mastery.essence_of_the_viper = find_mastery_spell( HUNTER_SURVIVAL );

  // Major
  glyphs.aimed_shot          = find_glyph_spell( "Glyph of Aimed Shot" ); // id=119465
  glyphs.endless_wrath       = find_glyph_spell( "Glyph of Endless Wrath" );
  glyphs.animal_bond         = find_glyph_spell( "Glyph of Animal Bond" );
  glyphs.black_ice           = find_glyph_spell( "Glyph of Block Ice" );
  glyphs.camouflage          = find_glyph_spell( "Glyph of Camoflage" );
  glyphs.chimaera_shot       = find_glyph_spell( "Glyph of Chimaera Shot" );
  glyphs.deterrence          = find_glyph_spell( "Glyph of Deterrence" );
  glyphs.disengage           = find_glyph_spell( "Glyph of Disengage" );
  glyphs.explosive_trap      = find_glyph_spell( "Glyph of Explosive Trap" );
  glyphs.freezing_trap       = find_glyph_spell( "Glyph of Freezing Trap" );
  glyphs.ice_trap            = find_glyph_spell( "Glyph of Ice Trap" );
  glyphs.icy_solace          = find_glyph_spell( "Glyph of Icy Solace" );
  glyphs.marked_for_death    = find_glyph_spell( "Glyph of Marked for Death" );
  glyphs.masters_call        = find_glyph_spell( "Glyph of Master's Call" );
  glyphs.mend_pet            = find_glyph_spell( "Glyph of Mend Pet" );
  glyphs.mending             = find_glyph_spell( "Glyph of Mending" );
  glyphs.mirrored_blades     = find_glyph_spell( "Glyph of Mirrored Blades" );
  glyphs.misdirection        = find_glyph_spell( "Glyph of Misdirection" );
  glyphs.no_escape           = find_glyph_spell( "Glyph of No Escape" );
  glyphs.pathfinding         = find_glyph_spell( "Glyph of Pathfinding" );
  glyphs.scattering          = find_glyph_spell( "Glyph of Scattering" );
  glyphs.snake_trap          = find_glyph_spell( "Glyph of Snake Trap" );
  glyphs.tranquilizing_shot  = find_glyph_spell( "Glyph of Tranquilizing Shot" );

  // Minor
  glyphs.aspects             = find_glyph_spell( "Glyph of Aspects" );
  glyphs.aspect_of_the_pack  = find_glyph_spell( "Glyph of Aspect of the Pack" );
  glyphs.direction           = find_glyph_spell( "Glyph of Direction" );
  glyphs.fetch               = find_glyph_spell( "Glyph of Fetch" );
  glyphs.fireworks           = find_glyph_spell( "Glyph of Fireworks" );
  glyphs.lesser_proportion   = find_glyph_spell( "Glyph of Lesser Proportion" );
  glyphs.marking             = find_glyph_spell( "Glyph of Marking" );
  glyphs.revive_pet          = find_glyph_spell( "Glyph of Revive Pet" );
  glyphs.stampede            = find_glyph_spell( "Glyph of Stampede" );
  glyphs.tame_beast          = find_glyph_spell( "Glyph of Tame Beast" );
  glyphs.the_cheetah         = find_glyph_spell( "Glyph of the Cheetah" );

  // Attunments
  specs.lethal_shots         = find_specialization_spell( "Lethal Shots" );
  specs.animal_handler       = find_specialization_spell( "Animal Handler" );
  specs.lightning_reflexes   = find_specialization_spell( "Lightning Reflexes" );

  // Spec spells
  specs.critical_strikes     = find_spell( 157443 );
  specs.go_for_the_throat    = find_specialization_spell( "Go for the Throat" );
  specs.careful_aim          = find_specialization_spell( "Careful Aim" );
  specs.beast_cleave         = find_specialization_spell( "Beast Cleave" );
  specs.frenzy               = find_specialization_spell( "Frenzy" );
  specs.focus_fire           = find_specialization_spell( "Focus Fire" );
  specs.cobra_strikes        = find_specialization_spell( "Cobra Strikes" );
  specs.the_beast_within     = find_specialization_spell( "The Beast Within" );
  specs.exotic_beasts        = find_specialization_spell( "Exotic Beasts" );
  specs.invigoration         = find_specialization_spell( "Invigoration" );
  specs.kindred_spirits      = find_specialization_spell( "Kindred Spirits" );
  specs.careful_aim          = find_specialization_spell( "Careful Aim" );
  specs.lock_and_load        = find_specialization_spell( "Lock and Load" );
  specs.bombardment          = find_specialization_spell( "Bombardment" );
  specs.trueshot_aura        = find_spell( 19506 );
  specs.chimaera_shot        = find_specialization_spell( "Chimaera Shot" );
  specs.aimed_shot           = find_specialization_spell( "Aimed Shot" );
  specs.bestial_wrath        = find_specialization_spell( "Bestial Wrath" );
  specs.kill_command         = find_specialization_spell( "Kill Command" );
  specs.rapid_fire           = find_specialization_spell( "Rapid Fire" );
  specs.survivalist          = find_specialization_spell( "Survivalist" );

  if ( talents.serpent_sting -> ok() )
    active.serpent_sting = new attacks::serpent_sting_t( this );

  if ( talents.exotic_munitions -> ok() )
  {
    active.incendiary_ammo = new attacks::exotic_munitions_incendiary_ammo_t( this, "incendiary_ammo", find_spell( 162541 ) );
    active.frozen_ammo = new attacks::exotic_munitions_frozen_ammo_t( this, "frozen_ammo", find_spell( 162546 ) );
    active.poisoned_ammo = new attacks::exotic_munitions_poisoned_ammo_t( this, "poisoned_ammo", find_spell( 170661 ) );
  }

  action_lightning_arrow_aimed_shot = new attacks::lightning_arrow_t( this, "_aimed_shot" );
  action_lightning_arrow_arcane_shot = new attacks::lightning_arrow_t( this, "_arcane_shot" );
  action_lightning_arrow_multi_shot = new attacks::lightning_arrow_t( this, "_multi_shot" );
}

// hunter_t::init_base ======================================================

void hunter_t::init_base_stats()
{
  player_t::init_base_stats();

  base.attack_power_per_strength = 0.0;
  base.attack_power_per_agility  = 1.0;

  base_focus_regen_per_second = 4;

  resources.base[RESOURCE_FOCUS] = 100 + specs.kindred_spirits -> effectN( 1 ).resource( RESOURCE_FOCUS ) + perks.improved_focus -> effectN( 1 ).resource( RESOURCE_FOCUS );

  // Orc racial
  if ( race == RACE_ORC )
    pet_multiplier *= 1.0 + find_racial_spell( "Command" ) -> effectN( 1 ).percent();

  stats_stampede = get_stats( "stampede" );
  stats_tier17_4pc_bm = get_stats( "tier17_4pc_bm" );
  stats_tier18_4pc_bm = get_stats( "tier18_4pc_bm" );
}

// hunter_t::init_buffs =====================================================

void hunter_t::create_buffs()
{
  player_t::create_buffs();

  buffs.bestial_wrath                = buff_creator_t( this, "bestial_wrath", specs.bestial_wrath )
    .cd( timespan_t::zero() )
    .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
    buffs.bestial_wrath -> default_value = buffs.bestial_wrath -> data().effectN( 7 ).percent();


  buffs.bestial_wrath -> buff_duration += sets.set( SET_MELEE, T14, B4 ) -> effectN( 1 ).time_value();

  buffs.bombardment                 = buff_creator_t( this, "bombardment", specs.bombardment -> effectN( 1 ).trigger() );

  double careful_aim_crit           = specs.careful_aim -> effectN( 1 ).percent( );
  buffs.careful_aim                 = buff_creator_t( this, "careful_aim", specs.careful_aim ).activated( true ).default_value( careful_aim_crit );

  buffs.cobra_strikes               = buff_creator_t( this, 53257, "cobra_strikes" ).chance( specs.cobra_strikes -> proc_chance() );

  buffs.focus_fire                  = new spells::focus_fire_buff_t( buff_creator_t( this, 82692, "focus_fire" )
                                                                .max_stack( find_spell( 19615 ) -> max_stacks() )
                                                                .add_invalidate( CACHE_ATTACK_HASTE )
                                                                .add_invalidate( CACHE_ATTACK_POWER ));

  buffs.thrill_of_the_hunt          = buff_creator_t( this, 34720, "thrill_of_the_hunt" ).chance( talents.thrill_of_the_hunt -> proc_chance() );
  buffs.steady_focus                = buff_creator_t( this, 177668, "steady_focus" ).chance( talents.steady_focus -> ok() );
  buffs.pre_steady_focus            = buff_creator_t( this, "pre_steady_focus" ).max_stack( 2 ).quiet( true );

  buffs.lock_and_load               = buff_creator_t( this, 168980, "lock_and_load" );

  buffs.rapid_fire                  = buff_creator_t( this, "rapid_fire", specs.rapid_fire )
    .add_invalidate( CACHE_ATTACK_HASTE );

  buffs.rapid_fire -> cooldown -> duration = timespan_t::zero();

  buffs.sniper_training   = buff_creator_t( this, "sniper_training", mastery.sniper_training )
                            .duration( timespan_t::from_seconds( mastery.sniper_training -> effectN( 3 ).base_value() ) )
                            .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );

  buffs.stampede = buff_creator_t( this, 130201, "stampede" ) // To allow action lists to react to stampede, rather than doing it in a roundabout way.
    .activated( true )
    .duration( timespan_t::from_millis( 40027 ) );
  // Added 0.027 seconds to properly reflect haste threshholds seen in game.
  /*.quiet( true )*/;

  buffs.tier13_4pc        = buff_creator_t( this, 105919, "tier13_4pc" )
    .chance( sets.set( SET_MELEE, T13, B4 ) -> proc_chance() )
    .cd( timespan_t::from_seconds( 105.0 ) )
    .add_invalidate( CACHE_ATTACK_HASTE );

  buffs.tier16_2pc       = buff_creator_t( this, "tier16_2pc", sets.set( SET_MELEE, T16, B2 ) )
    .activated( sets.has_set_bonus( SET_MELEE, T16, B2 ) );

  buffs.tier16_4pc       = buff_creator_t( this, "tier16_4pc", sets.set( SET_MELEE, T16, B4 ) )
    .activated( sets.has_set_bonus( SET_MELEE, T16, B4 ) );

  buffs.tier16_4pc_bm_brutal_kinship = buff_creator_t( this, 144670, "tier16_4pc_brutal_kinship" )
    .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );

  buffs.heavy_shot   = buff_creator_t( this, 167165, "heavy_shot" )
    .default_value( find_spell( 167165 ) -> effectN( 1 ).percent() )
    .refresh_behavior( BUFF_REFRESH_EXTEND );

  buffs.t18_2p_rapid_fire = buff_creator_t( this, "rapid_fire_t18", sets.set( HUNTER_MARKSMANSHIP, T18, B2 ) -> effectN( 1 ).trigger() )
    .chance( sets.set( HUNTER_MARKSMANSHIP, T18, B2 ) -> proc_chance() )
    .default_value( sets.set( HUNTER_MARKSMANSHIP, T18, B2 ) -> effectN( 1 ).trigger() -> effectN( 1 ).percent() )
    .add_invalidate( CACHE_HASTE );
}

// hunter_t::init_gains =====================================================

void hunter_t::init_gains()
{
  player_t::init_gains();

  gains.invigoration         = get_gain( "invigoration" );
  gains.focus_fire           = get_gain( "focus_fire" );
  gains.thrill_of_the_hunt   = get_gain( "thrill_of_the_hunt_savings" );
  gains.steady_shot          = get_gain( "steady_shot" );
  gains.steady_focus         = get_gain( "steady_focus" );
  gains.focusing_shot        = get_gain( "focusing_shot" );
  gains.cobra_shot           = get_gain( "cobra_shot" );
  gains.aimed_shot           = get_gain( "aimed_shot" );
  gains.dire_beast           = get_gain( "dire_beast" );
}

// hunter_t::init_position ==================================================

void hunter_t::init_position()
{
  player_t::init_position();

  if ( base.position == POSITION_FRONT )
  {
    base.position = POSITION_RANGED_FRONT;
    position_str = util::position_type_string( base.position );
  }
  else if ( initial.position == POSITION_BACK )
  {
    base.position = POSITION_RANGED_BACK;
    position_str = util::position_type_string( base.position );
  }

  if ( sim -> debug )
    sim -> out_debug.printf( "%s: Position adjusted to %s", name(), position_str.c_str() );
}

// hunter_t::init_procs =====================================================

void hunter_t::init_procs()
{
  player_t::init_procs();

  procs.invigoration                 = get_proc( "invigoration" );
  procs.thrill_of_the_hunt           = get_proc( "thrill_of_the_hunt" );
  procs.lock_and_load                = get_proc( "lock_and_load" );
  procs.tier15_2pc_melee             = get_proc( "tier15_2pc_melee" );
  procs.tier15_4pc_melee_aimed_shot  = get_proc( "tier15_4pc_melee_aimed_shot" );
  procs.tier15_4pc_melee_arcane_shot = get_proc( "tier15_4pc_melee_arcane_shot" );
  procs.tier15_4pc_melee_multi_shot  = get_proc( "tier15_4pc_melee_multi_shot" );
  procs.tier16_2pc_melee             = get_proc( "tier16_2pc_melee" );
  procs.tier16_4pc_melee             = get_proc( "tier16_4pc_melee" );
  procs.tier17_2pc_bm                = get_proc( "tier17_2pc_bm" );
  procs.black_arrow_trinket_reset    = get_proc( "black_arrow_trinket_reset" );
  procs.tier18_2pc_mm_wasted_proc    = get_proc( "tier18_2pc_mm_wasted_proc" );
  procs.tier18_2pc_mm_wasted_overwrite     = get_proc ("tier18_2pc_mm_wasted_overwrite");
}

// hunter_t::init_rng =======================================================

void hunter_t::init_rng()
{
  // RPPMS
  double tier15_2pc_melee_rppm;

  if ( specialization() == HUNTER_BEAST_MASTERY )
    tier15_2pc_melee_rppm = 0.7;
  else if ( specialization() == HUNTER_MARKSMANSHIP )
    tier15_2pc_melee_rppm = 1.0;
  else // HUNTER_SURVIVAL
    tier15_2pc_melee_rppm = 1.2;

  ppm_tier15_2pc_melee.set_frequency( tier15_2pc_melee_rppm );
  ppm_tier15_4pc_melee.set_frequency( 3.0 );

  player_t::init_rng();
}

// hunter_t::init_scaling ===================================================

void hunter_t::init_scaling()
{
  player_t::init_scaling();

  scales_with[STAT_STRENGTH] = false;
}

// hunter_t::init_actions ===================================================

void hunter_t::init_action_list()
{
  if ( main_hand_weapon.group() != WEAPON_RANGED )
  {
    sim -> errorf( "Player %s does not have a ranged weapon at the Main Hand slot.", name() );
  }

  if ( action_list_str.empty() )
  {
    clear_action_priority_lists();

    action_priority_list_t* precombat = get_action_priority_list( "precombat" );

    // Flask
    if ( sim -> allow_flasks && true_level >= 80 )
    {
      std::string flask_action = "flask,type=";
      if ( true_level > 90 )
        flask_action += "greater_draenic_agility_flask";
      else
        flask_action += "spring_blossoms";
      precombat -> add_action( flask_action );
    }

    // Food
    if ( sim -> allow_food )
    {
      std::string food_action = "food,type=";
      if ( level() <= 90 )
        food_action += ( level() > 85 ) ? "sea_mist_rice_noodles" : "seafood_magnifique_feast";
      else if ( specialization() == HUNTER_BEAST_MASTERY )
        food_action += "sleeper_sushi";
      else if ( specialization() == HUNTER_MARKSMANSHIP )
        food_action += "pickled_eel";
      else if ( specialization() == HUNTER_SURVIVAL )
        food_action += "salty_squid_roll";
      else
        food_action += "salty_squid_roll";
      precombat -> add_action( food_action );
    }

    precombat -> add_action( "summon_pet" );
    precombat -> add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );
    precombat -> add_action( "exotic_munitions,ammo_type=poisoned,if=spell_targets.multi_shot<3" );
    precombat -> add_action( "exotic_munitions,ammo_type=incendiary,if=spell_targets.multi_shot>=3" );

    //Pre-pot
    add_potion_action( precombat, "draenic_agility", "virmens_bite" );

    switch ( specialization() )
    {
    case HUNTER_SURVIVAL:
      talent_overrides_str += "exotic_munitions,if=raid_event.adds.count>=3|enemies>2";
      apl_surv();
      break;
    case HUNTER_BEAST_MASTERY:
      talent_overrides_str += "steady_focus,if=raid_event.adds.count>=1|enemies>1";
      apl_bm();
      break;
    case HUNTER_MARKSMANSHIP:
      //talent_overrides_str += "lone_wolf,if=raid_event.adds.count>=3|enemies>1";
      apl_mm();
      break;
    default:
      apl_default(); // DEFAULT
      break;
    }

    if ( summon_pet_str.empty() )
      summon_pet_str = "cat";
    // Default
    use_default_action_list = true;
    player_t::init_action_list();
  }
}


// Item Actions =======================================================================

void hunter_t::add_item_actions( action_priority_list_t* list )
{
  int num_items = (int)items.size();
  for ( int i = 0; i < num_items; i++ )
  {
    if ( items[i].has_special_effect( SPECIAL_EFFECT_SOURCE_NONE, SPECIAL_EFFECT_USE ) )
      list -> add_action( "use_item,name=" + items[i].name_str );
  }
}

// Racial Actions =======================================================================

void hunter_t::add_racial_actions( action_priority_list_t* list )
{
    list -> add_action( "arcane_torrent,if=focus.deficit>=30");
    list -> add_action( "blood_fury" );
    list -> add_action( "berserking" );
}

// Potions Actions =======================================================================

void hunter_t::add_potion_action( action_priority_list_t* list, const std::string big_potion, const std::string little_potion, const std::string options )
{
  std::string action_options = options.empty() ? options : "," + options;
  if ( sim -> allow_potions )
  {
    if ( true_level > 90 )
      list -> add_action( "potion,name=" + big_potion + action_options );
    else if ( true_level >= 85 )
      list -> add_action( "potion,name=" + little_potion + action_options );
  }
}

// Beastmastery Action List =============================================================

void hunter_t::apl_bm()
{
  action_priority_list_t* default_list = get_action_priority_list( "default" );

  default_list -> add_action( "auto_shot" );

  add_item_actions( default_list );
  add_racial_actions( default_list );
  add_potion_action( default_list, "draenic_agility", "virmens_bite",
   "if=!talent.stampede.enabled&((buff.bestial_wrath.up&(legendary_ring.up|!legendary_ring.has_cooldown)&target.health.pct<=20)|target.time_to_die<=20)" );
  add_potion_action( default_list, "draenic_agility", "virmens_bite",
   "if=talent.stampede.enabled&((buff.stampede.remains&(legendary_ring.up|!legendary_ring.has_cooldown)&(buff.bloodlust.up|buff.focus_fire.up))|target.time_to_die<=40)" );
  default_list -> add_talent( this, "Stampede", "if=((buff.bloodlust.up|buff.focus_fire.up)&(legendary_ring.up|!legendary_ring.has_cooldown))|target.time_to_die<=25" );

  default_list -> add_talent( this, "Dire Beast" );
  default_list -> add_action( this, "Focus Fire", "if=buff.focus_fire.down&((cooldown.bestial_wrath.remains<1&buff.bestial_wrath.down)|(talent.stampede.enabled&buff.stampede.remains)|pet.cat.buff.frenzy.remains<1)" );
  default_list -> add_action( this, "Bestial Wrath", "if=focus>30&!buff.bestial_wrath.up" );
  default_list -> add_action( this, "Multi-Shot", "if=spell_targets.multi_shot>1&pet.cat.buff.beast_cleave.remains<0.5" );
  default_list -> add_action( this, "Focus Fire", "min_frenzy=5" );
  default_list -> add_talent( this, "Barrage", "if=spell_targets.barrage>1" );
  default_list -> add_action( this, "Explosive Trap", "if=spell_targets.explosive_trap_tick>5" );
  default_list -> add_action( this, "Multi-Shot", "if=spell_targets.multi_shot>5" );
  default_list -> add_action( this, "Kill Command" );
  default_list -> add_talent( this, "A Murder of Crows" );
  default_list -> add_action( this, "Kill Shot","if=focus.time_to_max>gcd" );
  default_list -> add_talent( this, "Focusing Shot", "if=focus<50" );
  default_list -> add_action( this, "Cobra Shot", "if=buff.pre_steady_focus.up&buff.steady_focus.remains<7&(14+cast_regen)<focus.deficit", "Cast a second shot for steady focus if that won't cap us." );
  default_list -> add_action( this, "Explosive Trap", "if=spell_targets.explosive_trap_tick>1" );
  default_list -> add_action( this, "Cobra Shot", "if=talent.steady_focus.enabled&buff.steady_focus.remains<4&focus<50", "Prepare for steady focus refresh if it is running out." );
  default_list -> add_talent( this, "Glaive Toss" );
  default_list -> add_talent( this, "Barrage" );
  default_list -> add_talent( this, "Powershot", "if=focus.time_to_max>cast_time" );
  default_list -> add_action( this, "Cobra Shot", "if=spell_targets.multi_shot>5" );
  default_list -> add_action( this, "Arcane Shot", "if=(buff.thrill_of_the_hunt.react&focus>35)|buff.bestial_wrath.up" );
  default_list -> add_action( this, "Arcane Shot", "if=focus>=75" );
  if ( true_level >= 81 )
    default_list -> add_action( this, "Cobra Shot");
  else
    default_list -> add_action(this, "Steady Shot");
}

// Marksman Action List ======================================================================

void hunter_t::apl_mm()
{
  action_priority_list_t* default_list        = get_action_priority_list( "default" );
  action_priority_list_t* careful_aim         = get_action_priority_list( "careful_aim" );

  default_list -> add_action( "auto_shot" );

  add_item_actions( default_list );
  add_racial_actions( default_list );

  add_potion_action( default_list, "draenic_agility", "virmens_bite",
    "if=((buff.rapid_fire.up|buff.bloodlust.up)&(cooldown.stampede.remains<1))|target.time_to_die<=25" );

  default_list -> add_action( this, "Chimaera Shot" );
  // "if=cast_regen+action.aimed_shot.cast_regen<focus.deficit"
  default_list -> add_action( this, "Kill Shot" );

  default_list -> add_action( this, "Rapid Fire");
  default_list -> add_talent( this, "Stampede", "if=buff.rapid_fire.up|buff.bloodlust.up|target.time_to_die<=25" );
  default_list -> add_action( "call_action_list,name=careful_aim,if=buff.careful_aim.up" );
  {
    careful_aim -> add_talent( this, "Glaive Toss", "if=active_enemies>2" );
    careful_aim -> add_talent( this, "Powershot", "if=spell_targets.powershot>1&cast_regen<focus.deficit" );
    careful_aim -> add_talent( this, "Barrage", "if=spell_targets.barrage>1" );
    // careful_aim -> add_action( this, "Steady Shot", "if=buff.pre_steady_focus.up&if=buff.pre_steady_focus.up&(14+cast_regen+action.aimed_shot.cast_regen)<=focus.deficit)" );
    careful_aim -> add_action( this, "Aimed Shot" );
    careful_aim -> add_talent( this, "Focusing Shot", "if=50+cast_regen<focus.deficit" );
    careful_aim -> add_action( this, "Steady Shot" );
  }
  default_list -> add_action( this, "Explosive Trap", "if=spell_targets.explosive_trap_tick>1" );

  default_list -> add_talent( this, "A Murder of Crows" );
  default_list -> add_talent( this, "Dire Beast", "if=cast_regen+action.aimed_shot.cast_regen<focus.deficit" );

  default_list -> add_talent( this, "Glaive Toss" );
  default_list -> add_talent( this, "Powershot", "if=cast_regen<focus.deficit" );
  default_list -> add_talent( this, "Barrage" );
  default_list -> add_action( this, "Steady Shot", "if=focus.deficit*cast_time%(14+cast_regen)>cooldown.rapid_fire.remains", "Pool max focus for rapid fire so we can spam AimedShot with Careful Aim buff" );
  default_list -> add_talent( this, "Focusing Shot", "if=focus.deficit*cast_time%(50+cast_regen)>cooldown.rapid_fire.remains&focus<100" );
  default_list -> add_action( this, "Steady Shot", "if=buff.pre_steady_focus.up&(14+cast_regen+action.aimed_shot.cast_regen)<=focus.deficit", "Cast a second shot for steady focus if that won't cap us." );
  default_list -> add_action( this, "Multi-Shot", "if=spell_targets.multi_shot>6" );
  default_list -> add_action( this, "Aimed Shot", "if=talent.focusing_shot.enabled" );
  default_list -> add_action( this, "Aimed Shot", "if=focus+cast_regen>=85" );
  default_list -> add_action( this, "Aimed Shot", "if=buff.thrill_of_the_hunt.react&focus+cast_regen>=65" );
  default_list -> add_talent( this, "Focusing Shot", "if=50+cast_regen-10<focus.deficit", "Allow FS to over-cap by 10 if we have nothing else to do" );
  default_list -> add_action( this, "Steady Shot" );
}

// Survival Action List ===================================================================

void hunter_t::apl_surv()
{
  action_priority_list_t* default_list  = get_action_priority_list( "default" );
  action_priority_list_t* aoe = get_action_priority_list( "aoe" );

  default_list -> add_action( "auto_shot" );

  add_racial_actions( default_list ); 
  add_item_actions( default_list ); 

  add_potion_action( default_list, "draenic_agility", "virmens_bite",
    "if=(((cooldown.stampede.remains<1)&(cooldown.a_murder_of_crows.remains<1))&(trinket.stat.any.up|buff.archmages_greater_incandescence_agi.up))|target.time_to_die<=25" );

  default_list -> add_action( "call_action_list,name=aoe,if=spell_targets.multi_shot>1" );

  default_list -> add_talent( this, "A Murder of Crows" );
  default_list -> add_talent( this, "Stampede", "if=buff.potion.up|(cooldown.potion.remains&(buff.archmages_greater_incandescence_agi.up|trinket.stat.any.up))|target.time_to_die<=45" );
  default_list -> add_action( this, "Black Arrow", "cycle_targets=1,if=remains<gcd*1.5" );
  default_list -> add_action( this, "Arcane Shot", "if=(trinket.proc.any.react&trinket.proc.any.remains<4)|dot.serpent_sting.remains<=3" );
  default_list -> add_action( this, "Explosive Shot" );
  default_list -> add_action( this, "Cobra Shot", "if=buff.pre_steady_focus.up" );
  default_list -> add_talent( this, "Dire Beast" );
  default_list -> add_action( this, "Arcane Shot", "if=(buff.thrill_of_the_hunt.react&focus>35)|target.time_to_die<4.5" );
  default_list -> add_talent( this, "Glaive Toss" );
  default_list -> add_talent( this, "Powershot" );
  default_list -> add_talent( this, "Barrage" );
  default_list -> add_action( this, "Explosive Trap", "if=!trinket.proc.any.react&!trinket.stacking_proc.any.react" );
  default_list -> add_action( this, "Arcane Shot", "if=talent.steady_focus.enabled&!talent.focusing_shot.enabled&focus.deficit<action.cobra_shot.cast_regen*2+28" );
  default_list -> add_action( this, "Cobra Shot", "if=talent.steady_focus.enabled&buff.steady_focus.remains<5" );
  default_list -> add_talent( this, "Focusing Shot", "if=talent.steady_focus.enabled&buff.steady_focus.remains<=cast_time&focus.deficit>cast_regen" );
  default_list -> add_action( this, "Arcane Shot", "if=focus>=70|talent.focusing_shot.enabled|(talent.steady_focus.enabled&focus>=50)" );
  default_list -> add_talent( this, "Focusing Shot" );
  if ( true_level >= 81 )
    default_list -> add_action( this, "Cobra Shot" );
  else
    default_list -> add_action( this, "Steady Shot" );

  aoe -> add_talent( this, "Stampede", "if=buff.potion.up|(cooldown.potion.remains&(buff.archmages_greater_incandescence_agi.up|trinket.stat.any.up|buff.archmages_incandescence_agi.up))" );
  aoe -> add_action( this, "Explosive Shot", "if=buff.lock_and_load.react&(!talent.barrage.enabled|cooldown.barrage.remains>0)" );
  aoe -> add_talent( this, "Barrage" );
  aoe -> add_action( this, "Black Arrow", "cycle_targets=1,if=remains<gcd*1.5" );
  aoe -> add_action( this, "Explosive Shot", "if=spell_targets.multi_shot<5" );
  aoe -> add_action( this, "Explosive Trap", "if=dot.explosive_trap.remains<=5" );
  aoe -> add_talent( this, "A Murder of Crows" );
  aoe -> add_talent( this, "Dire Beast" );
  aoe -> add_action( this, "Multi-shot", "if=buff.thrill_of_the_hunt.react&focus>50&cast_regen<=focus.deficit|dot.serpent_sting.remains<=5|target.time_to_die<4.5" );
  aoe -> add_talent( this, "Glaive Toss" );
  aoe -> add_talent( this, "Powershot" );
  aoe -> add_action( this, "Cobra Shot", "if=buff.pre_steady_focus.up&buff.steady_focus.remains<5&focus+14+cast_regen<80" );
  aoe -> add_action( this, "Multi-shot", "if=focus>=70|talent.focusing_shot.enabled" );
  aoe -> add_talent( this, "Focusing Shot" );
  if ( true_level >= 81 )
    aoe -> add_action( this, "Cobra Shot" );
  else
    aoe -> add_action( this, "Steady Shot" );
}

// NO Spec Combat Action Priority List ======================================

void hunter_t::apl_default()
{
  action_priority_list_t* default_list = get_action_priority_list( "default" );

  default_list -> add_action( this, "Arcane Shot" );
}

// hunter_t::reset ==========================================================

void hunter_t::reset()
{
  player_t::reset();

  // Active
  active.pet            = nullptr;
  active.aspect         = ASPECT_NONE;
  active.ammo           = NO_AMMO;

  ppm_tier15_4pc_melee.reset();
  ppm_tier15_2pc_melee.reset();
}

// hunter_t::arise ==========================================================

void hunter_t::arise()
{
  player_t::arise();

  if ( mastery.sniper_training -> ok() )
  {
    buffs.sniper_training -> trigger();
  }
}

// hunter_t::composite_rating_multiplier =====================================

double hunter_t::composite_rating_multiplier( rating_e rating ) const
{
  double m = player_t::composite_rating_multiplier( rating );

  switch ( rating )
  {
  case RATING_MELEE_CRIT:
    return m *= 1.0 + specs.lethal_shots -> effectN( 1 ).percent();
  case RATING_SPELL_CRIT:
    return m *= 1.0 + specs.lethal_shots -> effectN( 1 ).percent();
  case RATING_MASTERY:
    return m *= 1.0 + specs.animal_handler -> effectN( 1 ).percent();
  default:
    break;
  }

  return m;
}

// hunter_t::regen ===========================================================

void hunter_t::regen( timespan_t periodicity )
{
  player_t::regen( periodicity );
  if ( buffs.steady_focus -> up() )
  {
    double base = focus_regen_per_second() * buffs.steady_focus -> current_value;
    resource_gain( RESOURCE_FOCUS, base * periodicity.total_seconds(), gains.steady_focus );
  }
}

// hunter_t::composite_attack_power_multiplier ==============================

double hunter_t::composite_attack_power_multiplier() const
{
  double mult = player_t::composite_attack_power_multiplier();
  if ( perks.improved_focus_fire -> ok() && buffs.focus_fire -> check() )
    mult += buffs.focus_fire -> current_stack * perks.improved_focus_fire -> effectN( 1 ).percent();
  return mult;
}

// hunter_t::composite_melee_crit ===========================================

double hunter_t::composite_melee_crit() const
{
  double crit = player_t::composite_melee_crit();
  crit += specs.critical_strikes -> effectN( 1 ).percent();
  return crit;
}

// hunter_t::composite_spell_crit ===========================================

double hunter_t::composite_spell_crit() const
{
  double crit = player_t::composite_spell_crit();
  crit += specs.critical_strikes -> effectN( 1 ).percent();
  return crit;
}

// Haste and speed buff computations ========================================

double hunter_t::composite_melee_haste() const
{
  double h = player_t::composite_melee_haste();
  h *= 1.0 / ( 1.0 + buffs.tier13_4pc -> up() * buffs.tier13_4pc -> data().effectN( 1 ).percent() );
  h *= 1.0 / ( 1.0 + buffs.focus_fire -> value() );
  h *= 1.0 / ( 1.0 + std::max( buffs.rapid_fire -> value(), buffs.t18_2p_rapid_fire -> value() ) );
  return h;
}

// hunter_t::composite_player_critical_damage_multiplier ====================

double hunter_t::composite_player_critical_damage_multiplier() const
{
  double cdm = player_t::composite_player_critical_damage_multiplier();

  if ( buffs.sniper_training -> up() )
  {
    cdm += cache.mastery_value();
  }

  // we use check() for rapid_fire becuase it's usage is reported from value() above
  if ( sets.has_set_bonus( HUNTER_MARKSMANSHIP, T17, B4 ) && buffs.rapid_fire -> check() )
  {
    // deadly_aim_driver
    double seconds_buffed = floor( buffs.rapid_fire -> elapsed( sim -> current_time() ).total_seconds() );
    // from Nimox
    cdm += bugs ? std::min(15.0, seconds_buffed) * 0.03
                : seconds_buffed * sets.set( HUNTER_MARKSMANSHIP, T17, B4 ) -> effectN( 1 ).percent();
  }

  return cdm;
}

// hunter_t::composite_player_multiplier ====================================

double hunter_t::composite_player_multiplier( school_e school ) const
{
  double m = player_t::composite_player_multiplier( school );

  if ( mastery.essence_of_the_viper -> ok() &&
    dbc::get_school_mask( school ) & SCHOOL_MAGIC_MASK )
  {
    m *= 1.0 + cache.mastery_value();
  }

  if ( buffs.bestial_wrath -> up() )
    m *= 1.0 + buffs.bestial_wrath -> current_value;

  if ( buffs.sniper_training -> up() )
  {
    m *= 1.0 + cache.mastery_value();
  }

  if ( longview && specialization() == HUNTER_MARKSMANSHIP )
  {
    const spell_data_t* data = find_spell( longview -> spell_id ); 
    double factor = data -> effectN( 1 ).average( longview -> item ) / 100.0 / 100.0;
    if ( sim -> distance_targeting_enabled )
      m*= 1.0 + target -> get_position_distance( x_position, y_position ) * factor;
    else
      m *= 1.0 + base.distance * factor;
  }

  if ( sets.has_set_bonus( SET_MELEE, T16, B4 ) )
    m *= 1.0 + buffs.tier16_4pc_bm_brutal_kinship -> stack() * buffs.tier16_4pc_bm_brutal_kinship -> data().effectN( 1 ).percent();

  return m;
}

// hunter_t::invalidate_cache ==============================================

void hunter_t::invalidate_cache( cache_e c )
{
  player_t::invalidate_cache( c );

  switch ( c )
  {
  case CACHE_MASTERY:
    if ( mastery.essence_of_the_viper -> ok() || mastery.sniper_training -> ok() )
    {
      player_t::invalidate_cache( CACHE_PLAYER_DAMAGE_MULTIPLIER );
      if ( sim -> distance_targeting_enabled && mastery.sniper_training -> ok() )
      {
        // Marksman is a unique butterfly, since mastery changes the max range of abilities. We need to regenerate every target cache.
        for ( size_t i = 0, end = action_list.size(); i < end; i++ )
        {
          action_list[i] -> target_cache.is_valid = false;
        }
      }
    }
    break;
  default: break;
  }
}

// hunter_t::matching_gear_multiplier =======================================

double hunter_t::matching_gear_multiplier( attribute_e attr ) const
{
  if ( attr == ATTR_AGILITY )
    return 0.05;

  return 0.0;
}

// hunter_t::create_options =================================================

void hunter_t::create_options()
{
  player_t::create_options();

  add_option( opt_string( "summon_pet", summon_pet_str ) );
}

// hunter_t::create_profile =================================================

std::string hunter_t::create_profile( save_e stype )
{
  std::string profile_str = player_t::create_profile( stype );

  profile_str += "summon_pet=" + summon_pet_str + "\n";

  return profile_str;
}

// hunter_t::copy_from ======================================================

void hunter_t::copy_from( player_t* source )
{
  player_t::copy_from( source );

  hunter_t* p = debug_cast<hunter_t*>( source );

  summon_pet_str = p -> summon_pet_str;
}

// hunter_t::armory_extensions ==============================================

void hunter_t::armory_extensions( const std::string& /* region */,
                                  const std::string& /* server */,
                                  const std::string& /* character */,
                                  cache::behavior_e  /* caching */ )
{
#if 0
  // Pet support
  static pet_e pet_types[] =
  {
    /* 0*/ PET_NONE,         PET_WOLF,         PET_CAT,          PET_SPIDER,   PET_BEAR,
    /* 5*/ PET_BOAR,         PET_CROCOLISK,    PET_CARRION_BIRD, PET_CRAB,     PET_GORILLA,
    /*10*/ PET_NONE,         PET_RAPTOR,       PET_TALLSTRIDER,  PET_NONE,     PET_NONE,
    /*15*/ PET_NONE,         PET_NONE,         PET_NONE,         PET_NONE,     PET_NONE,
    /*20*/ PET_SCORPID,      PET_TURTLE,       PET_NONE,         PET_NONE,     PET_BAT,
    /*25*/ PET_HYENA,        PET_BIRD_OF_PREY, PET_WIND_SERPENT, PET_NONE,     PET_NONE,
    /*30*/ PET_DRAGONHAWK,   PET_RAVAGER,      PET_WARP_STALKER, PET_SPOREBAT, PET_NETHER_RAY,
    /*35*/ PET_SERPENT,      PET_NONE,         PET_MOTH,         PET_CHIMERA,  PET_DEVILSAUR,
    /*40*/ PET_NONE,         PET_SILITHID,     PET_WORM,         PET_RHINO,    PET_WASP,
    /*45*/ PET_CORE_HOUND,   PET_SPIRIT_BEAST, PET_NONE,         PET_NONE,     PET_NONE,
    /*50*/ PET_FOX,          PET_MONKEY,       PET_DOG,          PET_BEETLE,   PET_NONE,
    /*55*/ PET_SHALE_SPIDER, PET_NONE,         PET_NONE,         PET_NONE,     PET_NONE,
    /*60*/ PET_NONE,         PET_NONE,         PET_NONE,         PET_NONE,     PET_NONE,
    /*65*/ PET_NONE,         PET_WASP,         PET_NONE,         PET_NONE,     PET_NONE
  };
  int num_families = sizeof( pet_types ) / sizeof( pet_e );

  std::string url = "http://" + region + ".battle.net/wow/en/character/" + server + '/' + character + "/pet";
  xml_node_t* pet_xml = xml_t::get( sim, url, caching );
  if ( sim -> debug ) xml_t::print( pet_xml, sim -> output_file );

  xml_node_t* pet_list_xml = xml_t::get_node( pet_xml, "div", "class", "pets-list" );

  xml_node_t* pet_script_xml = xml_t::get_node( pet_list_xml, "script", "type", "text/javascript" );

  if ( ! pet_script_xml )
  {
    sim -> errorf( "Hunter %s unable to download pet data from Armory\n", name() );
    sim -> cancel();
    return;
  }

  std::string cdata_str;
  if ( xml_t::get_value( cdata_str, pet_script_xml, "cdata" ) )
  {
    std::string::size_type pos = cdata_str.find( '{' );
    if ( pos != std::string::npos ) cdata_str.erase( 0, pos + 1 );
    pos = cdata_str.rfind( '}' );
    if ( pos != std::string::npos ) cdata_str.erase( pos );

    js::js_node* pet_js = js_t::create( sim, cdata_str );
    pet_js = js_t::get_node( pet_js, "Pet.data" );
    if ( sim -> debug ) js_t::print( pet_js, sim -> output_file );

    if ( ! pet_js )
    {
      sim -> errorf( "\nHunter %s unable to download pet data from Armory\n", name() );
      sim -> cancel();
      return;
    }

    std::vector<js::js_node*> pet_records;
    int num_pets = js_t::get_children( pet_records, pet_js );
    for ( int i = 0; i < num_pets; i++ )
    {
      std::string pet_name, pet_talents;
      int pet_level, pet_family;

      if ( ! js_t::get_value( pet_name,    pet_records[ i ], "name"     ) ||
           ! js_t::get_value( pet_talents, pet_records[ i ], "build"    ) ||
           ! js_t::get_value( pet_level,   pet_records[ i ], "level"    ) ||
           ! js_t::get_value( pet_family,  pet_records[ i ], "familyId" ) )
      {
        sim -> errorf( "\nHunter %s unable to decode pet name/build/level/familyId for pet %s\n", name(), pet_name.c_str() );
        continue;
      }

      // Pets can have spaces in names, replace with underscore ..
      for ( unsigned j = 0; j < pet_name.length(); j++ )
      {
        if ( pet_name[ j ] == ' ' )
          pet_name[ j ] = '_';
      }

      // Pets can have the same name, so suffix names with an unique
      // identifier from Battle.net, if one is found
      if ( js_t::get_name( pet_records[ i ] ) )
      {
        pet_name += '_';
        pet_name += js_t::get_name( pet_records[ i ] );
      }

      if ( pet_family > num_families || pet_types[ pet_family ] == PET_NONE )
      {
        sim -> errorf( "\nHunter %s unable to decode pet %s family id %d\n", name(), pet_name.c_str(), pet_family );
        continue;
      }

      hunter_main_pet_t* pet = new hunter_main_pet_t( sim, this, pet_name, pet_types[ pet_family ] );

      pet -> parse_talents_armory( pet_talents );

      pet -> talents_str.clear();
      for ( int j = 0; j < MAX_TALENT_TREES; j++ )
      {
        for ( int k = 0; k < ( int ) pet -> talent_trees[ j ].size(); k++ )
        {
          pet -> talents_str += ( char ) ( pet -> talent_trees[ j ][ k ] -> ok() + ( int ) '0' );
        }
      }
    }

    // If we have valid pets, figure out which to summon by parsing Battle.net
    if ( pet_list )
    {
      std::vector<xml_node_t*> pet_nodes;

      int n_pet_nodes = xml_t::get_nodes( pet_nodes, pet_list_xml, "a", "class", "pet" );
      for ( int i = 0; i < n_pet_nodes; i++ )
      {
        xml_node_t* summoned_node = xml_t::get_node( pet_nodes[ i ], "span", "class", "summoned" );
        std::string summoned_pet_name;
        std::string summoned_pet_id;

        if ( ! summoned_node )
          continue;

        xml_t::get_value( summoned_pet_id, pet_nodes[ i ], "data-id" );

        if ( ! xml_t::get_value( summoned_pet_name, xml_t::get_node( pet_nodes[ i ], "span", "class", "name" ), "." ) )
          continue;

        if ( ! summoned_pet_name.empty() )
        {
          summon_pet_str = util::decode_html( summoned_pet_name );
          if ( ! summoned_pet_id.empty() )
            summon_pet_str += '_' + summoned_pet_id;
        }
      }

      // Pick first pet on the list, if no pet is summoned in the battle net profile
      if ( summon_pet_str.empty() )
        summon_pet_str = pet_list -> name_str;
    }
  }
#endif
}

// hunter_::convert_hybrid_stat ==============================================

stat_e hunter_t::convert_hybrid_stat( stat_e s ) const
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

// hunter_t::schedule_ready() =======================================================

/* Set the careful_aim buff state based on rapid fire and the enemy health. */
void hunter_t::schedule_ready( timespan_t delta_time, bool waiting )
{
  if ( specs.careful_aim -> ok() )
  {
    int ca_now = buffs.careful_aim -> check();
    int threshold = specs.careful_aim -> effectN( 2 ).base_value();
    if ( buffs.rapid_fire -> check() || target -> health_percentage() > threshold || buffs.t18_2p_rapid_fire -> check() )
    {
      if ( ! ca_now )
        buffs.careful_aim -> trigger();
    }
    else
    {
      if ( ca_now )
        buffs.careful_aim -> expire();
    }
  }
  player_t::schedule_ready( delta_time, waiting );
}

// hunter_t::moving() =======================================================

/* Override moving() so that it doesn't suppress auto_shot and only interrupts the few shots that cannot be used while moving.
*/
void hunter_t::moving()
{
  if ( ( executing && ! executing -> usable_moving() ) || ( channeling && ! channeling -> usable_moving() ) )
    player_t::interrupt();
}

void hunter_t::finish_moving()
{
  player_t::finish_moving();

  movement_ended = sim -> current_time();
}

/* Report Extension Class
 * Here you can define class specific report extensions/overrides
 */
class hunter_report_t: public player_report_extension_t
{
public:
  hunter_report_t( hunter_t& player ):
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
  hunter_t& p;
};

// HUNTER MODULE INTERFACE ==================================================

struct hunter_module_t: public module_t
{
  hunter_module_t(): module_t( HUNTER ) {}

  virtual player_t* create_player( sim_t* sim, const std::string& name, race_e r = RACE_NONE ) const override
  {
    auto  p = new hunter_t( sim, name, r );
    p -> report_extension = std::unique_ptr<player_report_extension_t>( new hunter_report_t( *p ) );
    return p;
  }

  virtual bool valid() const override { return true; }
  
  virtual void static_init() const override
  {
    unique_gear::register_special_effect( 184900, beastlord);
    unique_gear::register_special_effect( 184901, longview );
    unique_gear::register_special_effect( 184902, blackness);
  }

  virtual void init( player_t* p ) const override
  {
    p -> buffs.aspect_of_the_pack = buff_creator_t( p, "aspect_of_the_pack",
                                                    p -> find_class_spell( "Aspect of the Pack" ) );
    p -> buffs.aspect_of_the_fox  = buff_creator_t( p, "aspect_of_the_fox",
                                    p -> find_spell( 172106 ) )
      .cd( timespan_t::zero() );
  }

  virtual void register_hotfixes() const override
  {
  }

  virtual void combat_begin( sim_t* ) const override {}
  virtual void combat_end( sim_t* ) const override {}
};

} // UNNAMED NAMESPACE

const module_t* module_t::hunter()
{
  static hunter_module_t m;
  return &m;
}
