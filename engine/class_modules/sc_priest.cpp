// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

namespace { // ANONYMOUS NAMESPACE

  // The purpose of these namespaces is to allow modern IDEs to collapse sections of code.
  // Is neither intended nor desired to provide name-uniqueness, hence the global uplift.

  namespace spells {}
  namespace heals  {}
  namespace pets   {}

  using namespace spells;
  using namespace heals;
  using namespace pets;

struct priest_t;

struct remove_dots_event_t;

struct priest_td_t : public actor_pair_t
{
  struct dots_t
  {
    dot_t* devouring_plague;
    dot_t* shadow_word_pain;
    dot_t* vampiric_touch;
    dot_t* holy_fire;
    dot_t* renew;
  } dots;

  struct buffs_t
  {
    absorb_buff_t* power_word_shield;
    absorb_buff_t* divine_aegis;
    absorb_buff_t* spirit_shell;
    debuff_t* mind_spike;
  } buffs;

  remove_dots_event_t* remove_dots_event;

  priest_td_t( player_t* target, priest_t* p );

  void reset()
  {
    remove_dots_event = 0;
  }
};

struct priest_t : public player_t
{
  // Buffs

  struct buffs_t
  {
    // Talents
    buff_t* twist_of_fate;
    buff_t* surge_of_light;

    // Discipline
    buff_t* holy_evangelism;
    buff_t* dark_archangel;
    buff_t* holy_archangel;
    buff_t* inner_fire;
    buff_t* inner_focus;
    buff_t* inner_will;

    // Holy
    buff_t* chakra_pre;
    buff_t* chakra_chastise;
    buff_t* chakra_sanctuary;
    buff_t* chakra_serenity;
    buff_t* serenity;

    // Shadow
    buff_t* divine_insight_shadow;
    buff_t* shadow_word_death_reset_cooldown;
    buff_t* glyph_mind_spike;
    buff_t* shadowform;
    buff_t* vampiric_embrace;
    buff_t* surge_of_darkness;

    buff_t* consume_surge_of_darkness;
    buff_t* consume_divine_insight_shadow;
  } buffs;

  // Talents
  struct talents_t
  {
    const spell_data_t* void_tendrils;
    const spell_data_t* psyfiend;
    const spell_data_t* dominate_mind;
    const spell_data_t* body_and_soul;
    const spell_data_t* angelic_feather;
    const spell_data_t* phantasm;
    const spell_data_t* from_darkness_comes_light;
    const spell_data_t* mindbender;
    const spell_data_t* archangel;
    const spell_data_t* desperate_prayer;
    const spell_data_t* void_shift;
    const spell_data_t* angelic_bulwark;
    const spell_data_t* twist_of_fate;
    const spell_data_t* power_infusion;
    const spell_data_t* divine_insight;
    const spell_data_t* cascade;
    const spell_data_t* divine_star;
    const spell_data_t* halo;
  } talents;

  // Specialization Spells
  struct specs_t
  {
    // General

    // Discipline
    const spell_data_t* meditation_disc;
    const spell_data_t* divine_aegis;
    const spell_data_t* grace;
    const spell_data_t* evangelism;
    const spell_data_t* train_of_thought;
    const spell_data_t* divine_fury;
    const spell_data_t* rapture;
    const spell_data_t* mysticism;

    // Holy
    const spell_data_t* meditation_holy;
    const spell_data_t* revelations;
    const spell_data_t* chakra_chastise;
    const spell_data_t* chakra_sanctuary;
    const spell_data_t* chakra_serenity;

    // Shadow
    const spell_data_t* devouring_plague;
    const spell_data_t* mind_surge;
    const spell_data_t* spiritual_precision;
    const spell_data_t* shadowform;
    const spell_data_t* shadowy_apparitions;
  } specs;

  // Mastery Spells
  struct mastery_spells_t
  {
    const spell_data_t* shield_discipline;
    const spell_data_t* echo_of_light;
    const spell_data_t* shadowy_recall;
  } mastery_spells;

  // Cooldowns
  struct cooldowns_t
  {
    cooldown_t* mind_blast;
    cooldown_t* shadowfiend;
    cooldown_t* mindbender;
    cooldown_t* chakra;
    cooldown_t* inner_focus;
    cooldown_t* penance;
    cooldown_t* rapture;
  } cooldowns;

  // Gains
  struct gains_t
  {
    gain_t* dispersion;
    gain_t* shadowfiend;
    gain_t* mindbender;
    gain_t* archangel;
    gain_t* hymn_of_hope;
    gain_t* shadow_orb_mb;
    gain_t* shadow_orb_swd;
    gain_t* devouring_plague_health;
    gain_t* vampiric_touch_mana;
    gain_t* vampiric_touch_mastery_mana;
    gain_t* rapture;
  } gains;

  // Benefits
  struct benefits_t
  {
    std::array<benefit_t*, 4> mind_spike;
    benefits_t() { range::fill( mind_spike, 0 ); }
  } benefits;

  // Procs
  struct procs_t
  {
    proc_t* mastery_extra_tick;
    proc_t* shadowy_apparition;
    proc_t* surge_of_darkness;
    proc_t* divine_insight_shadow;
    proc_t* mind_spike_dot_removal;
  } procs;

  // Special

  struct spells_t
  {
    std::queue<spell_t*> apparitions_free;
    std::list<spell_t*>  apparitions_active;
    heal_t* echo_of_light;
    heal_t* spirit_shell;
    bool echo_of_light_merged;
    spells_t() : echo_of_light( NULL ), spirit_shell( NULL ), echo_of_light_merged( false ) {}
  } spells;


  // Random Number Generators
  struct rngs_t
  {
    rng_t* mastery_extra_tick;
    rng_t* shadowy_apparitions;
  } rngs;

  // Pets
  struct pets_t
  {
    pet_t* shadowfiend;
    pet_t* mindbender;
    pet_t* lightwell;
  } pets;

  // Options
  int initial_shadow_orbs;
  std::string atonement_target_str;
  std::vector<player_t *> party_list;

  // Glyphs
  struct glyphs_t
  {
    const spell_data_t* circle_of_healing;
    const spell_data_t* dispersion;
    const spell_data_t* holy_nova;
    const spell_data_t* inner_fire;
    const spell_data_t* lightwell;
    const spell_data_t* penance;
    const spell_data_t* power_word_shield;
    const spell_data_t* prayer_of_mending;
    const spell_data_t* renew;
    const spell_data_t* smite;

    // Mop
    const spell_data_t* atonement;
    const spell_data_t* holy_fire;
    const spell_data_t* mind_spike;
    const spell_data_t* strength_of_soul;
    const spell_data_t* inner_sanctum;
    const spell_data_t* dark_binding;
    const spell_data_t* mind_flay;
    const spell_data_t* mind_blast;
    const spell_data_t* devouring_plague;
    const spell_data_t* vampiric_embrace;
    const spell_data_t* borrowed_time;
  } glyphs;

  // Constants
  struct constants_t
  {
    double meditation_value;
  } constants;

  target_specific_t<priest_td_t> target_data;

  priest_t( sim_t* sim, const std::string& name, race_e r = RACE_NIGHT_ELF ) :
    player_t( sim, PRIEST, name, r ),
    // initialize containers. For POD containers this sets all elements to 0.
    buffs( buffs_t() ),
    talents( talents_t() ),
    specs( specs_t() ),
    mastery_spells( mastery_spells_t() ),
    cooldowns( cooldowns_t() ),
    gains( gains_t() ),
    benefits( benefits_t() ),
    procs( procs_t() ),
    spells( spells_t() ),
    rngs( rngs_t() ),
    pets( pets_t() ),
    initial_shadow_orbs( 0 ),
    glyphs( glyphs_t() ),
    constants( constants_t() )
  {
    target_data.init( "target_data", this );

    initial.distance                     = 40.0;

    cooldowns.mind_blast                 = get_cooldown( "mind_blast" );
    cooldowns.shadowfiend                = get_cooldown( "shadowfiend" );
    cooldowns.mindbender                 = get_cooldown( "mindbender" );
    cooldowns.chakra                     = get_cooldown( "chakra"   );
    cooldowns.inner_focus                = get_cooldown( "inner_focus" );
    cooldowns.penance                    = get_cooldown( "penance" );
    cooldowns.rapture                    = get_cooldown( "rapture" );

    create_options();
  }

  // Character Definition
  virtual void      init_base();
  virtual void      init_gains();
  virtual void      init_benefits();
  virtual void      init_rng();
  virtual void      init_spells();
  virtual void      init_buffs();
  virtual void      init_values();
  virtual void      init_actions();
  virtual void      init_procs();
  virtual void      init_scaling();
  virtual void      reset();
  virtual void      init_party();
  virtual void      create_options();
  virtual bool      create_profile( std::string& profile_str, save_e=SAVE_ALL, bool save_html=false );
  virtual action_t* create_action( const std::string& name, const std::string& options );
  virtual pet_t*    create_pet( const std::string& name, const std::string& type = std::string() );
  virtual void      create_pets();
  virtual void      copy_from( player_t* source );
  virtual int       decode_set( item_t& );
  virtual resource_e primary_resource() { return RESOURCE_MANA; }
  virtual role_e primary_role();
  virtual void      combat_begin();
  virtual double    composite_armor();
  virtual double    composite_spell_power_multiplier();
  virtual double    composite_spell_hit();
  virtual double    composite_player_multiplier( school_e school, action_t* a = NULL );
  virtual double    composite_movement_speed();
  virtual double    composite_attribute_multiplier( attribute_e attr );

  virtual double    matching_gear_multiplier( attribute_e attr );

  virtual double    target_mitigation( double amount, school_e school, dmg_e, result_e, action_t* a=0 );

  virtual double    shadowy_recall_chance();

  void fixup_atonement_stats( const std::string& trigger_spell_name, const std::string& atonement_spell_name );
  virtual void pre_analyze_hook();

  virtual priest_td_t* get_target_data( player_t* target )
  {
    priest_td_t*& td = target_data[ target ];
    if ( ! td ) td = new priest_td_t( target, this );
    return td;
  }

  void add_action( std::string action, std::string options = "", std::string alist = "default" );
  void add_action( const spell_data_t* s, std::string options = "", std::string alist = "default" );

  // Temporary
  virtual std::string set_default_talents();
  virtual std::string set_default_glyphs();
};

struct remove_dots_event_t : public event_t
{
private:
  priest_td_t* td;
  priest_t* pr;

  static void cancel_dot( dot_t* dot )
  {
    if ( dot -> ticking )
    {
      dot -> cancel();
      dot -> reset();
    }
  }

public:
  remove_dots_event_t( sim_t* sim, priest_t* pr, priest_td_t* td ) :
    event_t( sim, pr, "mind_spike_remove_dots" ), td( td ), pr( pr )
  {
    sim -> add_event( this, sim -> gauss( sim -> default_aura_delay, sim -> default_aura_delay_stddev ) );
  }

  virtual void execute()
  {
    td -> remove_dots_event = 0;
    pr -> procs.mind_spike_dot_removal -> occur();
    cancel_dot( td -> dots.shadow_word_pain );
    cancel_dot( td -> dots.vampiric_touch );
  }
};

namespace pets {
// ==========================================================================
// Priest Pet
// ==========================================================================

struct priest_pet_t : public pet_t
{
  double ap_per_owner_sp;
  double direct_power_mod;

  priest_pet_t( sim_t* sim, priest_t* owner, const std::string& pet_name, pet_e pt, bool guardian = false ) :
    pet_t( sim, owner, pet_name, pt, guardian ),
    ap_per_owner_sp( 1.0 ),
    direct_power_mod( 0.0 )
  {
    position                    = POSITION_BACK;
    initial.distance            = 3;
  }

  struct _stat_list_t
  {
    int id;
    double stats[ BASE_STAT_MAX ];
  };

  virtual void init_base()
  {
    pet_t::init_base();

    {
      // Base Stats, same for all pets. Depend on level
      static const _stat_list_t pet_base_stats[]=
      {
        //       str, agi,  sta, int, spi,   hp,  mana, scrit/int, d/agi, mcrit, scrit, mp5, spi_reg
        {  0, { 0 } },
        { 85, {  453, 883,  353, 159, 225,    0,     0,         0,     0,     0,     0,   0,       0 } }
      };

      assert( sizeof_array( pet_base_stats ) > 0 );
      _stat_list_t ps = pet_base_stats[ 0 ];
      // Loop from end to beginning to get the data for the highest available level equal or lower than the player level
      for ( int i = sizeof_array( pet_base_stats ) - 1; i >= 0 ; --i )
      {
        if ( pet_base_stats[ i ].id >= level )
        {
          ps = pet_base_stats[ i ];
          break;
        }
      }
      base.attribute[ ATTR_STRENGTH  ]  = ps.stats[ BASE_STAT_STRENGTH ];
      base.attribute[ ATTR_AGILITY   ]  = ps.stats[ BASE_STAT_AGILITY ];
      base.attribute[ ATTR_STAMINA   ]  = ps.stats[ BASE_STAT_STAMINA ];
      base.attribute[ ATTR_INTELLECT ]  = ps.stats[ BASE_STAT_INTELLECT ];
      base.attribute[ ATTR_SPIRIT    ]  = ps.stats[ BASE_STAT_SPIRIT ];
      resources.base[ RESOURCE_HEALTH ] = ps.stats[ BASE_STAT_HEALTH ];
      resources.base[ RESOURCE_MANA ]   = ps.stats[ BASE_STAT_MANA ];
      initial.attack_crit_per_agility   = ps.stats[ BASE_STAT_MELEE_CRIT_PER_AGI ];
      initial.spell_crit_per_intellect  = ps.stats[ BASE_STAT_SPELL_CRIT_PER_INT ];
      initial.dodge_per_agility         = ps.stats[ BASE_STAT_DODGE_PER_AGI ];
      base.spell_crit                   = ps.stats[ BASE_STAT_SPELL_CRIT ];
      base.attack_crit                  = ps.stats[ BASE_STAT_MELEE_CRIT ];
      base.mp5                          = ps.stats[ BASE_STAT_MP5 ];
    }

    resources.base[ RESOURCE_MANA ]   = o() -> resources.max[ RESOURCE_MANA ];
    initial.attack_power_per_strength = 2.0; // tested in-game as of 2010/12/20
    base.attack_power = -20; // technically, the first 20 str give 0 ap. - tested
    stamina_per_owner = 0.6496; // level invariant, tested
    intellect_per_owner = 0; // removed in cata, tested

    initial.attack_crit_per_agility   += 0.01 / 52.0; // untested
    initial.spell_crit_per_intellect  += owner -> initial.spell_crit_per_intellect; // untested
    //health_per_stamina = 10.0; // untested!
    //mana_per_intellect = 0; // tested - does not scale with pet int, but with owner int, at level/80 * 7.5 mana per point of owner int that exceeds owner base int
    //mp5_per_intellect  = 2.0 / 3.0; // untested!
  }

  virtual void schedule_ready( timespan_t delta_time, bool waiting )
  {
    if ( main_hand_attack && ! main_hand_attack -> execute_event )
    {
      main_hand_attack -> schedule_execute();
    }

    pet_t::schedule_ready( delta_time, waiting );
  }

  virtual double composite_spell_haste()
  { return player_t::composite_spell_haste() * owner -> spell_haste; }

  virtual double composite_attack_haste()
  { return player_t::composite_attack_haste() * owner -> spell_haste; }

  virtual double composite_spell_power( school_e school )
  { return owner -> composite_spell_power( school ) * owner -> composite_spell_power_multiplier(); }

  virtual double composite_spell_power_multiplier()
  { return 1.0; }

  virtual double composite_attack_power()
  { return owner -> composite_spell_power( SCHOOL_MAX ) * ap_per_owner_sp; }

  virtual double composite_attack_crit( weapon_t* )
  { return owner -> composite_spell_crit(); } // Seems to just use our crit directly, based on very rough numbers, needs more testing.

  virtual double composite_spell_crit()
  { return owner -> composite_spell_crit(); } // Seems to just use our crit directly, based on very rough numbers, needs more testing.

  virtual double composite_attack_expertise( weapon_t* ) { return owner -> composite_spell_hit() + owner -> composite_attack_expertise() - ( owner -> buffs.heroic_presence -> up() ? 0.01 : 0.0 ); }
  virtual double composite_attack_hit() { return owner -> composite_spell_hit(); }
  virtual resource_e primary_resource() { return RESOURCE_ENERGY; }
  priest_t* o() { return debug_cast<priest_t*>( owner ); }
};

// ==========================================================================
// Priest Guardian Pet
// ==========================================================================

struct priest_guardian_pet_t : public priest_pet_t
{
  priest_guardian_pet_t( sim_t* sim, priest_t* owner, const std::string& pet_name, pet_e pt ) :
    priest_pet_t( sim, owner, pet_name, pt, true )
  {}

  virtual void summon( timespan_t duration=timespan_t::zero() )
  {
    reset();
    priest_pet_t::summon( duration );
  }
};

// ==========================================================================
// Base Pet for Shadowfiend and Mindbender
// ==========================================================================

struct base_fiend_pet_t : public priest_guardian_pet_t
{
  struct buffs_t
  {
    buff_t* shadowcrawl;
  } buffs;
  struct gains_t
  {
    gain_t* fiend;
  } gains;
  const spell_data_t* shadowcrawl;
  const spell_data_t* mana_leech;
  action_t* shadowcrawl_action;

  base_fiend_pet_t( sim_t* sim, priest_t* owner, pet_e pt, const std::string& name = "basefiend" ) :
    priest_guardian_pet_t( sim, owner, name, pt ),
    buffs( buffs_t() ),
    shadowcrawl( spell_data_t::nil() ), mana_leech( spell_data_t::nil() ),
    shadowcrawl_action( 0 )
  {
    action_list_str += "/snapshot_stats";
    action_list_str += "/shadowcrawl";
    action_list_str += "/wait_for_shadowcrawl";


    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.min_dmg    = util::ability_rank( owner -> level, 387, 90, 360, 85, 0, 1 );
    main_hand_weapon.max_dmg    = util::ability_rank( owner -> level, 467, 90, 433, 85, 0, 1 );
    main_hand_weapon.damage     = ( main_hand_weapon.min_dmg + main_hand_weapon.max_dmg ) / 2;
    main_hand_weapon.swing_time = timespan_t::from_seconds( 1.5 );
  }

  virtual void init_spells()
  {
    priest_guardian_pet_t::init_spells();

    shadowcrawl = find_pet_spell( "Shadowcrawl" );
  }

  virtual void init_buffs()
  {
    priest_guardian_pet_t::init_buffs();

    buffs.shadowcrawl = buff_creator_t( this, "shadowcrawl", shadowcrawl );
  }

  virtual void init_gains()
  {
    priest_guardian_pet_t::init_gains();

    if      ( pet_type == PET_SHADOWFIEND )
      gains.fiend = o() -> gains.shadowfiend;
    else if ( pet_type == PET_MINDBENDER  )
      gains.fiend = o() -> gains.mindbender;
    else
      gains.fiend = get_gain( "basefiend" );
  }

  virtual void init_resources( bool force )
  {
    priest_guardian_pet_t::init_resources( force );

    resources.initial[ RESOURCE_HEALTH ] = owner -> resources.max[ RESOURCE_HEALTH ] * 0.3;
    resources.initial[ RESOURCE_MANA   ] = owner -> resources.max[ RESOURCE_MANA   ];
    resources.current = resources.max = resources.initial;
  }

  virtual void summon( timespan_t duration )
  {
    dismiss();

    priest_guardian_pet_t::summon( duration );

    if ( shadowcrawl_action )
    {
      // Ensure that it gets used after the first melee strike. In the combat logs that happen at the same time, but the melee comes first.
      shadowcrawl_action -> cooldown -> ready = sim -> current_time + timespan_t::from_seconds( 0.001 );
    }
  }

  virtual void init_base();
  virtual action_t* create_action( const std::string& name,
                                   const std::string& options_str );
};

// ==========================================================================
// Pet Shadowfiend
// ==========================================================================

struct shadowfiend_pet_t : public base_fiend_pet_t
{
  shadowfiend_pet_t( sim_t* sim, priest_t* owner, const std::string& name = "shadowfiend" ) :
    base_fiend_pet_t( sim, owner, PET_SHADOWFIEND, name )
  {
    direct_power_mod = 0.5114705;
  }

  virtual void init_spells()
  {
    base_fiend_pet_t::init_spells();

    mana_leech  = find_spell( 34650, "mana_leech" );
  }
};

// ==========================================================================
// Pet Mindbender
// ==========================================================================

struct mindbender_pet_t : public base_fiend_pet_t
{
  mindbender_pet_t( sim_t* sim, priest_t* owner, const std::string& name = "mindbender" ) :
    base_fiend_pet_t( sim, owner, PET_MINDBENDER, name )
  {
    direct_power_mod = 1.16349445;
  }

  virtual void init_spells()
  {
    base_fiend_pet_t::init_spells();

    mana_leech  = find_spell( 123051, "mana_leech" );
  }
};

struct lightwell_pet_t : public priest_pet_t
{
public:
  int charges;
  lightwell_pet_t( sim_t* sim, priest_t* p ) :
    priest_pet_t( sim, p, "lightwell", PET_NONE, true ),
    charges( 0 )
  {
    role = ROLE_HEAL;

    action_list_str  = "/snapshot_stats";
    action_list_str += "/lightwell_renew";
    action_list_str += "/wait,sec=cooldown.lightwell_renew.remains";
  }

  virtual action_t* create_action( const std::string& name,
                                   const std::string& options_str );
  virtual void summon( timespan_t duration )
  {
    spell_haste = o() -> spell_haste;
    spell_power[ SCHOOL_HOLY ] = o() -> composite_spell_power( SCHOOL_HOLY ) * o() -> composite_spell_power_multiplier();

    charges = 10 + o() -> glyphs.lightwell -> effectN( 1 ).base_value();

    priest_pet_t::summon( duration );
  }
};

// ==========================================================================
// Priest Pet Melee
// ==========================================================================

struct priest_pet_melee_t : public melee_attack_t
{
  mutable bool first_swing;

  priest_pet_melee_t( priest_pet_t* p, const char* name ) :
    melee_attack_t( name, p, spell_data_t::nil() ),
    first_swing( true )
  {
    school = SCHOOL_SHADOW;
    weapon = &( p -> main_hand_weapon );
    base_execute_time = weapon -> swing_time;
    may_crit    = true;
    background  = true;
    repeating   = true;
  }

  virtual void reset()
  {
    melee_attack_t::reset();
    first_swing = true;
  }

  priest_pet_t* p()
  { return static_cast<priest_pet_t*>( player ); }

  virtual timespan_t execute_time()
  {
    if ( first_swing )
    {
      first_swing = false;
      return timespan_t::from_seconds( 0.0 );
    }
    return melee_attack_t::execute_time();
  }
};

// ==========================================================================
// Priest Pet Spell
// ==========================================================================

struct priest_pet_spell_t : public spell_t
{

  priest_pet_spell_t( priest_pet_t* p, const std::string& n ) :
    spell_t( n, p, p -> find_pet_spell( n ) )
  {
    may_crit = true;
  }

  priest_pet_spell_t( const std::string& token, priest_pet_t* p, const spell_data_t* s = spell_data_t::nil() ) :
    spell_t( token, p, s )
  {
    may_crit = true;
  }

  priest_pet_t* p()
  { return static_cast<priest_pet_t*>( player ); }
};

struct shadowcrawl_t : public priest_pet_spell_t
{
  shadowcrawl_t( base_fiend_pet_t* p ) :
    priest_pet_spell_t( p, "Shadowcrawl" )
  {
    may_miss  = false;
    harmful   = false;
    stateless = true;
  }

  base_fiend_pet_t* p()
  { return static_cast<base_fiend_pet_t*>( player ); }

  virtual void execute()
  {
    spell_t::execute();

    p() -> buffs.shadowcrawl -> trigger();
  }
};

struct fiend_melee_t : public priest_pet_melee_t
{
  fiend_melee_t( base_fiend_pet_t* p ) :
    priest_pet_melee_t( p, "melee" )
  {
    weapon = &( p -> main_hand_weapon );
    weapon_multiplier = 0.0;
    base_dd_min       = weapon -> min_dmg;
    base_dd_max       = weapon -> max_dmg;
    direct_power_mod  = p -> direct_power_mod;
    stateless = true;
  }

  base_fiend_pet_t* p()
  { return static_cast<base_fiend_pet_t*>( player ); }

  virtual void execute()
  {
    priest_pet_melee_t::execute();
  }


  virtual double action_multiplier()
  {
    double am = priest_pet_melee_t::action_multiplier();

    am *= 1.0 + p() -> buffs.shadowcrawl -> up() * p() -> shadowcrawl -> effectN( 2 ).percent();

    return am;
  }

  virtual void impact_s( action_state_t* s )
  {
    priest_pet_melee_t::impact_s( s );

    if ( result_is_hit( s -> result ) )
    {
      p() -> o() -> resource_gain( RESOURCE_MANA, p() -> o() -> resources.max[ RESOURCE_MANA ] *
                                   p() -> mana_leech -> effectN( 1 ).percent(),
                                   p() -> gains.fiend );
    }
  }
};

struct lightwell_renew_t : public heal_t
{
  lightwell_renew_t( lightwell_pet_t* player ) :
    heal_t( "lightwell_renew", player, player -> find_spell( 7001 ) )
  {
    may_crit = false;
    tick_may_crit = true;
    stateless = true;

    tick_power_mod = 0.308;
  }

  lightwell_pet_t* p()
  { return static_cast<lightwell_pet_t*>( player ); }

  virtual void execute()
  {
    p() -> charges--;

    target = find_lowest_player();

    heal_t::execute();
  }

  virtual void last_tick( dot_t* d )
  {
    heal_t::last_tick( d );

    if ( p() -> charges <= 0 )
      p() -> dismiss();
  }

  virtual bool ready()
  {
    if ( p() -> charges <= 0 )
      return false;
    return heal_t::ready();
  }
};

// ==========================================================================
// Pet Shadowfiend/Mindbender Base
// ==========================================================================

void base_fiend_pet_t::init_base()
{
  priest_guardian_pet_t::init_base();

  main_hand_attack = new fiend_melee_t( this );
}

action_t* base_fiend_pet_t::create_action( const std::string& name,
                                           const std::string& options_str )
{
  if ( name == "shadowcrawl" )
  {
    shadowcrawl_action = new shadowcrawl_t( this );
    return shadowcrawl_action;
  }

  if ( name == "wait_for_shadowcrawl" ) return new wait_for_cooldown_t( this, "shadowcrawl" );

  return priest_guardian_pet_t::create_action( name, options_str );
}

action_t* lightwell_pet_t::create_action( const std::string& name,
                                          const std::string& options_str )
{
  if ( name == "lightwell_renew" ) return new lightwell_renew_t( this );

  return priest_pet_t::create_action( name, options_str );
}

} // END pets NAMESPACE

// This is a template for common code between priest_spell_t, priest_heal_t and priest_absorb_t.
// The template is instantiated with either spell_t, heal_t or absorb_t as the 'Base' class.
// Make sure you keep the inheritance hierarchy and use base_t in the derived class,
// don't skip it and call spell_t/heal_t or absorb_t directly.
template <class Base>
struct priest_action_t : public Base
{
private:
  buff_t* sform;
public:
  bool castable_in_shadowform;
  bool can_cancel_shadowform;

  typedef Base action_base_t; // typedef for the templated action type, eg. spell_t, attack_t, heal_t
  typedef priest_action_t base_t; // typedef for priest_action_t<action_base_t>

  priest_action_t( const std::string& n, priest_t* player,
                  const spell_data_t* s = spell_data_t::nil() ) :
    action_base_t( n, player, s )
  {
    action_base_t::may_crit          = true;
    action_base_t::tick_may_crit     = true;
    action_base_t::stateless         = true;

    action_base_t::dot_behavior      = DOT_REFRESH;
    action_base_t::weapon_multiplier = 0.0;


    can_cancel_shadowform = player -> autoUnshift;
    castable_in_shadowform = true;
    sform = player -> buffs.shadowform;
  }

  bool check_shadowform()
  {
    return ( castable_in_shadowform || can_cancel_shadowform || ( sform -> current_stack == 0 ) );
  }

  void cancel_shadowform()
  {
    if ( ! castable_in_shadowform )
    {
      // FIX-ME: Needs to drop haste aura too.
      sform  -> expire();
    }
  }

  priest_t* p() { return debug_cast<priest_t*>( action_base_t::player ); }

  priest_td_t* td( player_t* t = 0 ) { return p() -> get_target_data( t ? t : action_base_t::target ); }

  virtual void schedule_execute()
  {
    cancel_shadowform();

    action_base_t::schedule_execute();
  }

  virtual bool ready()
  {
    if ( ! action_base_t::ready() )
      return false;

    return check_shadowform();
  }
};


// ==========================================================================
// Priest Absorb
// ==========================================================================

struct priest_absorb_t : public priest_action_t<absorb_t>
{
  cooldown_t* min_interval;

public:
  priest_absorb_t( const std::string& n, priest_t* player,
                   const spell_data_t* s = spell_data_t::nil() ) :
    base_t( n, player, s )
  {
    may_crit          = false;
    tick_may_crit     = false;
    may_miss          = false;
    min_interval      = player -> get_cooldown( "min_interval_" + name_str );
  }

  virtual double action_multiplier()
  {
    return base_t::action_multiplier() * ( 1.0 + ( p() -> composite_mastery() * p() -> mastery_spells.shield_discipline->effectN( 1 ).coeff() / 100.0 ) );
  }

  virtual double cost()
  {
    double c = base_t::cost();

    if ( ( base_execute_time <= timespan_t::zero() ) && ! channeled )
    {
      c *= 1.0 + p() -> buffs.inner_will -> check() * p() -> buffs.inner_will -> data().effectN( 1 ).percent();
      c  = floor( c );
    }

    return c;
  }

  virtual void consume_resource()
  {
    base_t::consume_resource();

    if ( base_execute_time <= timespan_t::zero() )
      p() -> buffs.inner_will -> up();
  }

  bool ready()
  {
    if ( min_interval -> remains() > timespan_t::zero() )
      return false;

    return base_t::ready();
  }

  void parse_options( option_t*          options,
                      const std::string& options_str )
  {
    const option_t base_options[] =
    {
      { "min_interval", OPT_TIMESPAN,     &( min_interval -> duration ) },
      { NULL,           OPT_UNKNOWN, NULL      }
    };

    std::vector<option_t> merged_options;
    base_t::parse_options( option_t::merge( merged_options, options, base_options ), options_str );
  }

  void update_ready()
  {
    base_t::update_ready();

    if ( min_interval -> duration > timespan_t::zero() && ! dual )
    {
      min_interval -> start( timespan_t::min(), timespan_t::zero() );

      if ( sim -> debug ) sim -> output( "%s starts min_interval for %s (%s). Will be ready at %.4f", player -> name(), name(), cooldown -> name(), cooldown -> ready.total_seconds() );
    }
  }
};

// ==========================================================================
// Priest Heal
// ==========================================================================

struct priest_heal_t : public priest_action_t<heal_t>
{
  struct divine_aegis_t : public priest_absorb_t
  {
    double shield_multiple;

    divine_aegis_t( const std::string& n, priest_t* p ) :
      priest_absorb_t( n, p, p -> find_spell( 47753 ) ), shield_multiple( 0 )
    {
      check_spell( p -> specs.divine_aegis );

      proc             = true;
      background       = true;
      direct_power_mod = 0;

      shield_multiple  = p -> specs.divine_aegis -> effectN( 1 ).percent();
    }

    virtual void impact_s( action_state_t* s )
    {
      double old_amount = td( s -> target ) -> buffs.divine_aegis -> value();
      double new_amount = std::min( s -> target -> resources.current[ RESOURCE_HEALTH ] * 0.4 - old_amount, s -> result_amount );
      td( s -> target ) -> buffs.divine_aegis -> trigger( 1, old_amount + new_amount );
      stats -> add_result( 0, new_amount, ABSORB, s -> result );
    }
  };

  bool can_trigger_DA;
  divine_aegis_t* da;
  cooldown_t* min_interval;

  void trigger_echo_of_light( priest_heal_t* a, player_t* t )
  {
    if ( ! p() -> mastery_spells.echo_of_light -> ok() )
      return;

    dot_t* d = p() -> spells.echo_of_light -> get_dot( t );
    if ( d -> ticking )
    {
      if ( p() -> spells.echo_of_light_merged )
      {
        // The old tick_dmg is multiplied by the remaining ticks, added to the new complete heal, and then again divided by num_ticks
        p() -> spells.echo_of_light -> base_td = ( p() -> spells.echo_of_light -> base_td *
                                                 d -> ticks() +
                                                 a -> direct_dmg * p() -> composite_mastery() *
                                                 p() -> mastery_spells.echo_of_light -> effectN( 2 ).percent() / 100.0 ) /
                                                 p() -> spells.echo_of_light -> num_ticks;
        d -> refresh_duration();
      }
      else
      {
        // The old tick_dmg is multiplied by the remaining ticks minus one!, added to the new complete heal, and then again divided by num_ticks
        p() -> spells.echo_of_light -> base_td = ( p() -> spells.echo_of_light -> base_td *
                                                 ( d -> ticks() - 1 ) +
                                                 a -> direct_dmg * p() -> composite_mastery() *
                                                 p() -> mastery_spells.echo_of_light -> effectN( 2 ).percent() / 100.0 ) /
                                                 p() -> spells.echo_of_light -> num_ticks;
        d -> refresh_duration();
        p() -> spells.echo_of_light_merged = true;
      }
    }
    else
    {
      p() -> spells.echo_of_light -> base_td = a -> direct_dmg * p() -> composite_mastery() *
                                               p() -> mastery_spells.echo_of_light -> effectN( 2 ).percent() / 100.0 /
                                               p() -> spells.echo_of_light -> num_ticks;
      p() -> spells.echo_of_light -> execute();
      p() -> spells.echo_of_light_merged = false;
    }
  }

  virtual void init()
  {
    base_t::init();

    if ( can_trigger_DA && p() -> specs.divine_aegis -> ok() )
    {
      da = new divine_aegis_t( name_str + "_divine_aegis", p() );
      add_child( da );
      da -> target = target;
    }
  }

  priest_heal_t( const std::string& n, priest_t* player,
                 const spell_data_t* s = spell_data_t::nil() ) :
    base_t( n, player, s ), can_trigger_DA( true ), da()
  {
    min_interval = player -> get_cooldown( "min_interval_" + name_str );
  }

  virtual double composite_crit()
  {
    double cc = base_t::composite_crit();

    if ( p() -> buffs.chakra_serenity -> up() )
      cc += p() -> buffs.chakra_serenity -> data().effectN( 1 ).percent();

    if ( p() -> buffs.serenity -> up() )
      cc += p() -> buffs.serenity -> data().effectN( 2 ).percent();

    return cc;
  }

  virtual double action_multiplier()
  {
    return base_t::action_multiplier() * ( 1.0 + p() -> buffs.holy_archangel -> value() );
  }

  virtual double composite_target_multiplier( player_t* t )
  {
    double ctm = base_t::composite_target_multiplier( t );

    if ( p() -> specs.grace -> ok() )
      ctm *= 1.0 + t -> buffs.grace -> check() * t -> buffs.grace -> value();

    return ctm;
  }

  virtual double cost()
  {
    double c = base_t::cost();

    if ( ( base_execute_time <= timespan_t::zero() ) && ! channeled )
    {
      c *= 1.0 + p() -> buffs.inner_will -> check() * p() -> buffs.inner_will -> data().effectN( 1 ).percent();
      c  = floor( c );
    }

    return c;
  }

  virtual void consume_resource()
  {
    base_t::consume_resource();

    if ( base_execute_time <= timespan_t::zero() )
      p() -> buffs.inner_will -> up();
  }

  void trigger_divine_aegis( player_t* t, double amount )
  {
    if ( da )
    {
      da -> base_dd_min = da -> base_dd_max = amount * da -> shield_multiple;
      da -> target = t;
      da -> execute();
    }
  }

  virtual void impact_s( action_state_t* s )
  {
    double save_health_percentage = s -> target -> health_percentage();

    base_t::impact_s( s );

    if ( s -> result_amount > 0 )
    {
      // Divine Aegis
      if ( s -> result == RESULT_CRIT )
      {
        trigger_divine_aegis( s -> target, s -> result_amount );
      }

      trigger_echo_of_light( this, s -> target );

      if ( p() -> buffs.chakra_serenity -> up() && td( s -> target ) -> dots.renew -> ticking )
        td( s -> target ) -> dots.renew -> refresh_duration();

      if ( p() -> talents.twist_of_fate -> ok() && ( save_health_percentage < p() -> talents.twist_of_fate -> effectN( 1 ).base_value() ) )
      {
        p() -> buffs.twist_of_fate -> trigger();
      }
    }
  }

  void tick( dot_t* d )
  {
    base_t::tick( d );

    // Divine Aegis
    if ( result == RESULT_CRIT )
    {
      trigger_divine_aegis( target, tick_dmg );
    }
  }

  void trigger_grace( player_t* t )
  {
    if ( p() -> specs.grace -> ok() )
      t -> buffs.grace -> trigger( 1, p() -> specs.grace -> effectN( 1 ).trigger() -> effectN( 1 ).base_value() / 100.0 );
  }

  void trigger_strength_of_soul( player_t* t )
  {
    if ( p() -> glyphs.strength_of_soul -> ok() && t -> buffs.weakened_soul -> up() )
      t -> buffs.weakened_soul -> extend_duration( p(), timespan_t::from_seconds( -1 * p() -> glyphs.strength_of_soul -> effectN( 1 ).base_value() ) );
  }

  void update_ready()
  {
    base_t::update_ready();

    if ( min_interval -> duration > timespan_t::zero() && ! dual )
    {
      min_interval -> start( timespan_t::min(), timespan_t::zero() );

      if ( sim -> debug ) sim -> output( "%s starts min_interval for %s (%s). Will be ready at %.4f", player -> name(), name(), cooldown -> name(), cooldown -> ready.total_seconds() );
    }
  }

  bool ready()
  {
    if ( ! base_t::ready() )
      return false;

    return ( min_interval -> remains() <= timespan_t::zero() );
  }

  void parse_options( option_t*          options,
                      const std::string& options_str )
  {
    const option_t base_options[] =
    {
      { "min_interval", OPT_TIMESPAN, &( min_interval -> duration ) },
      { NULL,           OPT_UNKNOWN,  NULL      }
    };

    std::vector<option_t> merged_options;
    base_t::parse_options( option_t::merge( merged_options, options, base_options ), options_str );
  }
};

// ==========================================================================
// Priest Spell
// ==========================================================================

struct priest_spell_t : public priest_action_t<spell_t>
{

  // Atonement heal ===========================================================

  struct atonement_heal_t : public priest_heal_t
  {
    atonement_heal_t( const std::string& n, priest_t* p ) :
      priest_heal_t( n, p, p -> find_spell( 81751 ) )
    {
      proc           = true;
      background     = true;
      round_base_dmg = false;

      // HACK: Setting may_crit = true will force crits.
      may_crit = false;
      base_crit = 1.0;

      if ( ! p -> atonement_target_str.empty() )
        target = sim -> find_player( p -> atonement_target_str.c_str() );
    }

    void trigger( double atonement_dmg, int dmg_e, int result )
    {
      atonement_dmg *= p() -> glyphs.atonement -> effectN( 1 ).percent();
      double cap = p() -> resources.max[ RESOURCE_HEALTH ] * 0.3;

      if ( result == RESULT_CRIT )
      {
        // FIXME: Assume capped at 200% of the non-crit cap.
        cap *= 2.0;
      }

      if ( atonement_dmg > cap )
        atonement_dmg = cap;

      if ( dmg_e == DMG_OVER_TIME )
      {
        // num_ticks = 1;
        base_td = atonement_dmg;
        tick_may_crit = ( result == RESULT_CRIT );
        tick( get_dot() );
      }
      else
      {
        assert( dmg_e == DMG_DIRECT );
        // num_ticks = 0;
        base_dd_min = base_dd_max = atonement_dmg;
        may_crit = ( result == RESULT_CRIT );

        execute();
      }
    }

    virtual double total_crit_bonus()
    { return 0; }

    virtual void execute()
    {
      target = find_lowest_player();

      priest_heal_t::execute();
    }

    virtual void tick( dot_t* d )
    {
      target = find_lowest_player();

      priest_heal_t::tick( d );
    }
  };

  atonement_heal_t* atonement;
  bool can_trigger_atonement;

  priest_spell_t( const std::string& n, priest_t* player,
                  const spell_data_t* s = spell_data_t::nil() ) :
    base_t( n, player, s ),
    atonement( 0 ), can_trigger_atonement( 0 )
  {
    dot_behavior      = DOT_REFRESH;
    weapon_multiplier = 0.0;

    can_trigger_atonement = false;
  }

  virtual void init()
  {
    base_t::init();

    if ( can_trigger_atonement && p() -> glyphs.atonement -> ok() )
    {
      std::string n = "atonement_" + name_str;
      atonement = new atonement_heal_t( n, p() );
    }
  }

  virtual double cost()
  {
    double c = base_t::cost();

    if ( ( base_execute_time <= timespan_t::zero() ) && ! channeled )
    {
      c *= 1.0 + p() -> buffs.inner_will -> check() * p() -> buffs.inner_will -> data().effectN( 1 ).percent();
      c  = floor( c );
    }

    return c;
  }

  virtual void impact_s( action_state_t* s )
  {
    double save_health_percentage = s -> target -> health_percentage();

    base_t::impact_s( s );

    if ( result_is_hit( s -> result ) )
    {
      if ( p() -> talents.twist_of_fate -> ok() && ( save_health_percentage < p() -> talents.twist_of_fate -> effectN( 1 ).base_value() ) )
      {
        p() -> buffs.twist_of_fate -> trigger();
      }
    }
  }


  virtual void consume_resource()
  {
    base_t::consume_resource();

    if ( base_execute_time <= timespan_t::zero() )
      p() -> buffs.inner_will -> up();
  }

  virtual void assess_damage( player_t* t,
                              double amount,
                              dmg_e type,
                              result_e impact_result )
  {
    base_t::assess_damage( t, amount, type, impact_result );

    if ( aoe == 0 && p() -> buffs.vampiric_embrace -> up() && result_is_hit( impact_result ) )
    {
      double a = amount * ( p() -> buffs.vampiric_embrace -> data().effectN( 1 ).percent() + p() -> glyphs.vampiric_embrace -> effectN( 2 ).percent() ) ;

      // Split amongst number of people in raid.
      a /= 1.0 + p() -> party_list.size();

      // Priest Heal
      p() -> resource_gain( RESOURCE_HEALTH, a, player -> gains.vampiric_embrace );

      // Pet Heal
      // Pet's get a full share without counting against the number in the raid.
      for ( size_t i = 0; i < player -> pet_list.size(); ++i )
      {
        pet_t* r = player -> pet_list[ i ];
        r -> resource_gain( RESOURCE_HEALTH, a, r -> gains.vampiric_embrace );
      }

      // Party Heal
      for ( size_t i = 0; i < p() -> party_list.size(); i++ )
      {
        player_t* q = p() -> party_list[ i ];

        q -> resource_gain( RESOURCE_HEALTH, a, q -> gains.vampiric_embrace );

        for ( size_t i = 0; i < player -> pet_list.size(); ++i )
        {
          pet_t* r = player -> pet_list[ i ];
          r -> resource_gain( RESOURCE_HEALTH, a, r -> gains.vampiric_embrace );
        }
      }
    }

    if ( atonement )
      atonement -> trigger( amount, type, impact_result );
  }

  static void trigger_shadowy_apparition( dot_t* d )
  {
    priest_t* pr = debug_cast<priest_t*>( d -> action -> player );

    if ( ! pr -> specs.shadowy_apparitions -> ok() )
      return;

    if ( ! pr -> rngs.shadowy_apparitions -> roll( pr -> specs.shadowy_apparitions -> effectN( 1 ).percent() ) )
      return;

    if ( ! pr -> spells.apparitions_free.empty() )
    {
      spell_t* s = pr -> spells.apparitions_free.front();

      s -> target = d -> state -> target;

      pr -> spells.apparitions_free.pop();

      pr -> spells.apparitions_active.push_back( s );

      pr -> procs.shadowy_apparition -> occur();

      s -> execute();
    }
  }

private:
  friend void priest_t::init_spells();
public:
  static void generate_shadow_orb( action_t* s, gain_t* g, unsigned number = 1 )
  {
    if ( s -> player -> primary_tree() != PRIEST_SHADOW )
      return;

    s -> player -> resource_gain( RESOURCE_SHADOW_ORB, number, g, s );
  }
};

void trigger_chakra( priest_t* p, buff_t* chakra_buff )
{
  if ( p -> buffs.chakra_pre -> up() )
  {
    chakra_buff -> trigger();

    p -> buffs.chakra_pre -> expire();

    p -> cooldowns.chakra -> reset();
    p -> cooldowns.chakra -> duration = p -> buffs.chakra_pre -> data().cooldown();
    p -> cooldowns.chakra -> start();
  }
}

namespace spells {

// ==========================================================================
// Priest Abilities
// ==========================================================================

// ==========================================================================
// Priest Non-Harmful Spells
// ==========================================================================

// Archangel Spell ==========================================================

struct archangel_t : public priest_spell_t
{
  archangel_t( priest_t* player, const std::string& options_str ) :
    priest_spell_t( "archangel", player, player -> find_talent_spell( "Archangel" ) )
  {
    parse_options( NULL, options_str );

    harmful           = false;

    if ( p() -> primary_tree() == PRIEST_SHADOW )
    {
      cooldown -> duration = p() -> buffs.dark_archangel -> buff_cooldown;
    }
    else
    {
      cooldown -> duration = p() -> buffs.holy_archangel -> buff_cooldown;
    }
  }

  virtual void execute()
  {
    priest_spell_t::execute();

    if ( p() -> primary_tree() == PRIEST_SHADOW )
    {
      p() -> buffs.dark_archangel -> trigger( 1, p() -> buffs.dark_archangel -> default_value );
    }
    else
    {
      p() -> buffs.holy_archangel -> trigger( 1, p() -> buffs.holy_archangel -> default_value * p() -> buffs.holy_evangelism -> stack() );
      p() -> buffs.holy_evangelism -> expire();
    }
  }
};

// Chakra_Pre Spell =========================================================

struct chakra_t : public priest_spell_t
{
  chakra_t( priest_t* p, const std::string& options_str ) :
    priest_spell_t( "chakra", p, p -> find_class_spell( "Chakra" ) )
  {
    parse_options( NULL, options_str );

    harmful = false;
  }

  virtual void execute()
  {
    // FIXME: Doesn't the cooldown work like Inner Focus?
    cooldown -> duration = timespan_t::from_seconds( sim -> wheel_seconds );

    priest_spell_t::execute();

    p() -> buffs.chakra_pre -> start();
  }

  virtual bool ready()
  {
    if ( p() -> buffs.chakra_pre -> check() )
      return false;

    return priest_spell_t::ready();
  }
};

// Dispersion Spell =========================================================

struct dispersion_t : public priest_spell_t
{
  dispersion_t( priest_t* player, const std::string& options_str ) :
    priest_spell_t( "dispersion", player, player -> find_class_spell( "Dispersion" ) )
  {
    parse_options( NULL, options_str );

    base_tick_time    = timespan_t::from_seconds( 1.0 );
    num_ticks         = 6;

    channeled         = true;
    harmful           = false;
    tick_may_crit     = false;

    cooldown -> duration += p() -> glyphs.dispersion -> effectN( 1 ).time_value();
  }

  virtual void tick( dot_t* d )
  {
    double regen_amount = p() -> find_spell( 49766 ) -> effectN( 1 ).percent() * p() -> resources.max[ RESOURCE_MANA ];

    p() -> resource_gain( RESOURCE_MANA, regen_amount, p() -> gains.dispersion );

    priest_spell_t::tick( d );
  }
};

// Fortitude Spell ==========================================================

struct fortitude_t : public priest_spell_t
{
  fortitude_t( priest_t* player, const std::string& options_str ) :
    priest_spell_t( "fortitude", player, player -> find_class_spell( "Power Word: Fortitude" ) )
  {
    parse_options( NULL, options_str );

    harmful = false;

    background = ( sim -> overrides.stamina != 0 );
  }

  virtual void execute()
  {
    priest_spell_t::execute();

    if ( ! sim -> overrides.stamina )
      sim -> auras.stamina -> trigger();
  }
};

// Hymn of Hope Spell =======================================================

struct hymn_of_hope_tick_t : public priest_spell_t
{
  hymn_of_hope_tick_t( priest_t* player ) :
    priest_spell_t( "hymn_of_hope_tick", player, player -> find_spell( 64904 ) )
  {
    dual        = true;
    background  = true;
    may_crit    = true;
    direct_tick = true;

    harmful     = false;
  }

  virtual void execute()
  {
    priest_spell_t::execute();

    p() -> resource_gain( RESOURCE_MANA, data().effectN( 1 ).percent() * p() -> resources.max[ RESOURCE_MANA ], p() -> gains.hymn_of_hope );

    // Hymn of Hope only adds +x% of the current_max mana, it doesn't change if afterwards max_mana changes.
    player -> buffs.hymn_of_hope -> trigger();
  }
};

struct hymn_of_hope_t : public priest_spell_t
{
  hymn_of_hope_tick_t* hymn_of_hope_tick;

  hymn_of_hope_t( priest_t* p, const std::string& options_str ) :
    priest_spell_t( "hymn_of_hope", p, p -> find_class_spell( "Hymn of Hope" ) ),
    hymn_of_hope_tick( 0 )
  {
    parse_options( NULL, options_str );

    harmful = false;

    channeled = true;

    hymn_of_hope_tick = new hymn_of_hope_tick_t( p );
  }

  virtual void init()
  {
    priest_spell_t::init();

    hymn_of_hope_tick -> stats = stats;
  }

  virtual void tick( dot_t* d )
  {
    hymn_of_hope_tick -> execute();

    stats -> add_tick( d -> time_to_tick );
  }
};

// Inner Focus Spell ========================================================

struct inner_focus_t : public priest_spell_t
{
  inner_focus_t( priest_t* p, const std::string& options_str ) :
    priest_spell_t( "inner_focus", p, p -> find_class_spell( "Inner Focus" ) )
  {
    parse_options( NULL, options_str );

    harmful = false;
  }

  virtual void execute()
  {
    cooldown -> duration = timespan_t::from_seconds( sim -> wheel_seconds );

    priest_spell_t::execute();

    p() -> buffs.inner_focus -> trigger();
  }
};

// Inner Fire Spell =========================================================

struct inner_fire_t : public priest_spell_t
{
  inner_fire_t( priest_t* player, const std::string& options_str ) :
    priest_spell_t( "inner_fire", player, player -> find_class_spell( "Inner Fire" ) )
  {
    parse_options( NULL, options_str );

    harmful = false;
  }

  virtual void execute()
  {
    priest_spell_t::execute();

    p() -> buffs.inner_will -> expire ();
    p() -> buffs.inner_fire -> trigger();
  }

  virtual bool ready()
  {
    if ( p() -> buffs.inner_fire -> check() )
      return false;

    return priest_spell_t::ready();
  }
};

// Inner Will Spell =========================================================

struct inner_will_t : public priest_spell_t
{
  inner_will_t( priest_t* player, const std::string& options_str ) :
    priest_spell_t( "inner_will", player, player -> find_class_spell( "Inner Will" ) )
  {
    parse_options( NULL, options_str );

    harmful = false;
  }

  virtual void execute()
  {
    priest_spell_t::execute();

    p() -> buffs.inner_fire -> expire();

    p() -> buffs.inner_will -> trigger();
  }

  virtual bool ready()
  {
    if ( p() -> buffs.inner_will -> check() )
      return false;

    return priest_spell_t::ready();
  }
};

// Pain Supression ==========================================================

struct pain_suppression_t : public priest_spell_t
{
  pain_suppression_t( priest_t* p, const std::string& options_str ) :
    priest_spell_t( "pain_suppression", p, p -> find_class_spell( "Pain Suppression" ) )
  {
    parse_options( NULL, options_str );

    // If we don't specify a target, it's defaulted to the mob, so default to the player instead
    if ( target -> is_enemy() || target -> is_add() )
    {
      target = p;
    }

    harmful = false;
  }

  virtual void execute()
  {
    priest_spell_t::execute();

    target -> buffs.pain_supression -> trigger();
  }
};

// Power Infusion Spell =====================================================

struct power_infusion_t : public priest_spell_t
{
  power_infusion_t( priest_t* p, const std::string& options_str ) :
    priest_spell_t( "power_infusion", p, p -> find_talent_spell( "Power Infusion" ) )
  {
    parse_options( NULL, options_str );

    // If we don't specify a target, it's defaulted to the mob, so default to the player instead
    if ( target -> is_enemy() || target -> is_add() )
    {
      target = p;
    }

    harmful = false;
  }

  virtual void execute()
  {
    priest_spell_t::execute();

    target -> buffs.power_infusion -> trigger();
  }

  virtual bool ready()
  {
    if ( target -> buffs.bloodlust -> check() )
      return false;

    if ( target -> buffs.power_infusion -> check() )
      return false;

    return priest_spell_t::ready();
  }
};

// Shadow Form Spell ========================================================

struct shadowform_t : public priest_spell_t
{
  shadowform_t( priest_t* p, const std::string& options_str ) :
    priest_spell_t( "shadowform", p, p -> find_class_spell( "Shadowform" ) )
  {
    parse_options( NULL, options_str );

    harmful = false;
  }

  virtual void execute()
  {
    priest_spell_t::execute();

    p() -> buffs.shadowform -> trigger();

    if ( ! sim -> overrides.spell_haste )
      sim -> auras.spell_haste -> trigger();
  }

  virtual bool ready()
  {
    if (  p() -> buffs.shadowform -> check() )
      return false;

    return priest_spell_t::ready();
  }
};

// PET SPELLS

struct summon_pet_t : public priest_spell_t
{
  timespan_t summoning_duration;
  pet_t* pet;

public:
  summon_pet_t( const std::string& n, priest_t* p, const spell_data_t* sd = spell_data_t::nil() ) :
    priest_spell_t( n, p, sd ), summoning_duration ( timespan_t::zero() ), pet( 0 )
  {
    harmful = false;

    pet = player -> find_pet( n );
    if ( ! pet )
    {
      sim -> errorf( "Player %s unable to find pet %s for summons.\n", player -> name(), n.c_str() );
    }
  }

  virtual void execute()
  {
    pet -> summon( summoning_duration );

    priest_spell_t::execute();
  }
};


struct summon_shadowfiend_t : public summon_pet_t
{
  summon_shadowfiend_t( priest_t* p, const std::string& options_str ) :
    summon_pet_t( "shadowfiend", p, p -> find_class_spell( "Shadowfiend" ) )
  {
    parse_options( NULL, options_str );
    harmful    = false;
    summoning_duration = data().duration();
    cooldown = p -> cooldowns.shadowfiend;
    cooldown -> duration = data().cooldown();
  }
};


struct summon_mindbender_t : public summon_pet_t
{
  summon_mindbender_t( priest_t* p, const std::string& options_str ) :
    summon_pet_t( "mindbender", p, p -> find_talent_spell( "Mindbender" ) )
  {
    parse_options( NULL, options_str );
    harmful    = false;
    summoning_duration = data().duration();
    cooldown = p -> cooldowns.mindbender;
    cooldown -> duration = data().cooldown();
  }
};

// ==========================================================================
// Priest Damage Spells
// ==========================================================================

struct priest_procced_mastery_spell_t : public priest_spell_t
{
  priest_procced_mastery_spell_t( const std::string& n, priest_t* p,
                                  const spell_data_t* s = spell_data_t::nil() ) :
    priest_spell_t( n, p, p -> mastery_spells.shadowy_recall -> ok() ? s : spell_data_t::not_found() )
  {
    background       = true;
    may_crit         = true;
    callbacks        = false;
    tick_zero        = true;
    num_ticks        = 0;
    hasted_ticks     = false;
    base_td          = base_dd_min;
    tick_power_mod   = direct_power_mod;
    base_dd_min = base_dd_max = 0.0;
    direct_power_mod = 0.0;
  }

  virtual timespan_t execute_time()
  {
    return sim -> gauss( sim -> default_aura_delay, sim -> default_aura_delay_stddev );
  }

  virtual void execute()
  {
    priest_spell_t::execute();

    p() -> procs.mastery_extra_tick -> occur();
  }
};

// Shadowy Apparition Spell =================================================

struct shadowy_apparition_spell_t : public priest_spell_t
{
  shadowy_apparition_spell_t( priest_t* player ) :
    priest_spell_t( "shadowy_apparition", player, player -> specs.shadowy_apparitions -> ok() ? player -> find_spell( 87532 ) : spell_data_t::not_found() )
  {
    background        = true;
    proc              = true;
    callbacks         = false;

    trigger_gcd       = timespan_t::zero();
    travel_speed      = 3.5;
  }

  virtual void impact_s( action_state_t* s )
  {
    priest_spell_t::impact_s( s );

    // Cleanup. Re-add to free list.
    p() -> spells.apparitions_active.remove( this );
    p() -> spells.apparitions_free.push( this );
  }
};

void add_more_shadowy_apparitions( priest_t* p, size_t n )
{
  spell_t* s = NULL;

  if ( ! p -> spells.apparitions_free.size() )
  {
    for ( size_t i = 0; i < n; i++ )
    {
      s = new shadowy_apparition_spell_t( p );
      p -> spells.apparitions_free.push( s );
    }
  }

  if ( p -> sim -> debug )
    p -> sim -> output( "%s created %d shadowy apparitions", p -> name(), static_cast<unsigned>( p -> spells.apparitions_free.size() ) );
}

// Mind Blast Spell =========================================================

struct mind_blast_t : public priest_spell_t
{
  mind_blast_t( priest_t* player, const std::string& options_str, bool dtr=false ) :
    priest_spell_t( "mind_blast", player, player -> find_class_spell( "Mind Blast" ) )
  {
    parse_options( NULL, options_str );

    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new mind_blast_t( player, options_str, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual double action_multiplier()
  {
    double m = priest_spell_t::action_multiplier();

    if ( p() -> buffs.dark_archangel -> check() )
    {
      m *= 1.0 + p() -> buffs.dark_archangel -> value();
    }

    return m;
  }

  virtual void execute()
  {
    priest_spell_t::execute();

    p() -> buffs.consume_divine_insight_shadow -> expire();
  }

  virtual void impact_s( action_state_t* s )
  {
    priest_spell_t::impact_s( s );

    if ( result_is_hit( s -> result ) )
    {
      generate_shadow_orb( this, p() -> gains.shadow_orb_mb );
      for ( int i=0; i < 4; i++ )
      {
        p() -> benefits.mind_spike[ i ] -> update( i == td( s -> target ) -> buffs.mind_spike -> check() );
      }
      td( s -> target ) -> buffs.mind_spike -> expire();
      p() -> buffs.glyph_mind_spike -> expire();
    }
  }

  virtual double composite_target_crit( player_t* target )
  {
    double c = priest_spell_t::composite_target_crit( target );

    c += td( target ) -> buffs.mind_spike -> value() * td( target ) -> buffs.mind_spike -> check();

    return c;
  }

  void consume_resource()
  {
/* BUG: Still costs mana as of 15726
    if ( p() -> buffs.consume_divine_insight_shadow -> check() )
      resource_consumed = 0.0;
    else
*/
      resource_consumed = cost();

    player -> resource_loss( current_resource(), resource_consumed, 0, this );

    if ( sim -> log )
      sim -> output( "%s consumes %.1f %s for %s (%.0f)", player -> name(),
                     resource_consumed, util::resource_type_string( current_resource() ),
                     name(), player -> resources.current[ current_resource() ] );

    stats -> consume_resource( current_resource(), resource_consumed );
  }

  virtual double cost()
  {
/* BUG: Still costs mana as of 15726
    if ( p() -> buffs.divine_insight_shadow -> check() )
      return 0.0;
*/
    return priest_spell_t::cost();
  }

  virtual void schedule_execute()
  {
    if ( p() -> buffs.divine_insight_shadow -> up() )
    {
      p() -> buffs.consume_divine_insight_shadow -> trigger();
    }

    priest_spell_t::schedule_execute();

    p() -> buffs.divine_insight_shadow -> expire();
  }

  virtual void update_ready()
  {
    if ( p() -> specs.mind_surge -> ok() && ! p() -> buffs.divine_insight_shadow -> check() )
    {
      cooldown -> duration = data().cooldown() * composite_haste();
    }
    priest_spell_t::update_ready();
  }

  virtual timespan_t execute_time()
  {
    timespan_t a = priest_spell_t::execute_time();

    if ( p() -> buffs.divine_insight_shadow -> check() )
    {
      return timespan_t::zero();
    }

    a *= 1 + ( p() -> buffs.glyph_mind_spike -> stack() * p() -> glyphs.mind_spike -> effect1().percent() );

    return a;
  }
};

// Mind Flay Mastery Proc ===========================================

struct mind_flay_mastery_t : public priest_procced_mastery_spell_t
{
  mind_flay_mastery_t( priest_t* p ) :
    priest_procced_mastery_spell_t( "mind_flay_mastery", p,
                                    p -> find_class_spell( "Mind Flay" ) -> ok() ? p -> find_spell( 124468 ) : spell_data_t::not_found() )
  {
  }
};

// Mind Flay Spell ==========================================================

struct mind_flay_t : public priest_spell_t
{
  mind_flay_mastery_t* proc_spell;

  mind_flay_t( priest_t* p, const std::string& options_str,
               const std::string& name = "mind_flay" ) :
    priest_spell_t( name, p, p -> find_class_spell( "Mind Flay" ) ),
    proc_spell( 0 )
  {
    parse_options( NULL, options_str );

    may_crit     = false;
    channeled    = true;
    hasted_ticks = false;

    if ( p -> mastery_spells.shadowy_recall -> ok() )
    {
      proc_spell = new mind_flay_mastery_t( p );
    }
  }

  virtual double action_multiplier()
  {
    double m = priest_spell_t::action_multiplier();

    if ( p() -> buffs.dark_archangel -> up() )
    {
      m *= 1.0 + p() -> buffs.dark_archangel -> value();
    }

    return m;
  }

  virtual void tick( dot_t* d )
  {
    priest_spell_t::tick( d );

    if ( proc_spell && p() -> rngs.mastery_extra_tick -> roll( p() -> shadowy_recall_chance() ) )
    {
      proc_spell -> schedule_execute();
    }
  }
};

// Mind Spike Spell =========================================================

struct mind_spike_t : public priest_spell_t
{
  struct mind_spike_state_t : public action_state_t
  {
    bool surge_of_darkness;

    mind_spike_state_t( action_t* a, player_t* t ) : action_state_t( a, t ),
      surge_of_darkness ( false )
    {

    }

    virtual void copy_state( const action_state_t* o )
    {
      if ( this == o || o == 0 ) return;

      action_state_t::copy_state( o );

      const mind_spike_state_t* dps_t = static_cast< const mind_spike_state_t* >( o );
      surge_of_darkness = dps_t -> surge_of_darkness;
    }
  };


  mind_spike_t( priest_t* player, const std::string& options_str, bool dtr=false ) :
    priest_spell_t( "mind_spike", player, player -> find_class_spell( "Mind Spike" ) )
  {
    parse_options( NULL, options_str );

    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new mind_spike_t( player, options_str, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual void reset()
  {
    priest_spell_t::reset();

    td( target ) -> remove_dots_event = 0;
  }

  virtual action_state_t* new_state()
  {
    return new mind_spike_state_t( this, target );
  }

  virtual action_state_t* get_state( const action_state_t* s )
  {
    action_state_t* s_ = priest_spell_t::get_state( s );

    if ( s == 0 )
    {
      mind_spike_state_t* ds_ = static_cast< mind_spike_state_t* >( s_ );
      ds_ -> surge_of_darkness = false;
    }

    return s_;
  }

  virtual void snapshot_state( action_state_t* state, uint32_t flags )
  {
    mind_spike_state_t* dps_t = static_cast< mind_spike_state_t* >( state );

    dps_t -> surge_of_darkness = p() -> buffs.consume_surge_of_darkness -> check() != 0;

    priest_spell_t::snapshot_state( state, flags );
  }


  virtual double action_multiplier()
  {
    double m = priest_spell_t::action_multiplier();

    if ( p() -> buffs.dark_archangel -> up() )
    {
      m *= 1.0 + p() -> buffs.dark_archangel -> value();
    }

    return m;
  }

  virtual void execute()
  {
    priest_spell_t::execute();

    p() -> buffs.consume_surge_of_darkness -> expire();
  }

  virtual void impact_s( action_state_t* s )
  {
    priest_spell_t::impact_s( s );

    if ( result_is_hit( s -> result ) )
    {
      td( s -> target ) -> buffs.mind_spike -> trigger();
      if ( p() -> glyphs.mind_spike -> ok() )
        p() -> buffs.glyph_mind_spike -> trigger();

      mind_spike_state_t* dps_t = static_cast< mind_spike_state_t* >( s );
      if ( ! dps_t -> surge_of_darkness && ! td( s -> target ) -> remove_dots_event )
      {
        td( s -> target ) -> remove_dots_event = new ( sim ) remove_dots_event_t( sim, p(), td( s -> target ) );
      }
    }
  }

  virtual void schedule_execute()
  {
    if ( p() -> buffs.surge_of_darkness -> up() )
    {
      p() -> buffs.consume_surge_of_darkness -> trigger();
    }

    priest_spell_t::schedule_execute();

    p() -> buffs.surge_of_darkness -> expire();
  }

  void consume_resource()
  {
/* BUG: Still costs mana as of 15726
    if ( p() -> buffs.consume_surge_of_darkness -> check() )
      resource_consumed = 0.0;
    else
*/
      resource_consumed = cost();

    player -> resource_loss( current_resource(), resource_consumed, 0, this );

    if ( sim -> log )
      sim -> output( "%s consumes %.1f %s for %s (%.0f)", player -> name(),
                     resource_consumed, util::resource_type_string( current_resource() ),
                     name(), player -> resources.current[ current_resource() ] );

    stats -> consume_resource( current_resource(), resource_consumed );
  }

  virtual double cost()
  {
/* BUG: Still costs mana as of 15726
    if ( p() -> buffs.surge_of_darkness -> check() )
      return 0.0;
*/

    return priest_spell_t::cost();
  }


  virtual timespan_t execute_time()
  {
    if ( p() -> buffs.surge_of_darkness -> check() )
    {
      return timespan_t::zero();
    }

    return priest_spell_t::execute_time();
  }
};

// Mind Sear Mastery Proc ===========================================

struct mind_sear_mastery_t : public priest_procced_mastery_spell_t
{
  mind_sear_mastery_t( priest_t* p ) :
    priest_procced_mastery_spell_t( "mind_sear_mastery", p,
                                    p -> find_class_spell( "Mind Sear" ) -> ok() ? p -> find_spell( 124469 ) : spell_data_t::not_found() )
  {
  }
};

// Mind Sear Spell ==========================================================

struct mind_sear_tick_t : public priest_spell_t
{
  mind_sear_tick_t( priest_t* player ) :
    priest_spell_t( "mind_sear_tick", player, player -> find_class_spell( "Mind Sear" ) -> effectN( 1 ).trigger() )
  {
    background  = true;
    dual        = true;
    aoe         = -1;
    callbacks   = false;
  }

  virtual void execute()
  {
    priest_spell_t::execute();
  }
};

struct mind_sear_t : public priest_spell_t
{
  mind_sear_tick_t* mind_sear_tick;
  mind_sear_mastery_t* proc_spell;

  mind_sear_t( priest_t* p, const std::string& options_str ) :
    priest_spell_t( "mind_sear", p, p -> find_class_spell( "Mind Sear" ) ),
    mind_sear_tick( 0 ), proc_spell( 0 )
  {
    parse_options( NULL, options_str );

    channeled    = true;
    may_crit     = false;
    hasted_ticks = false;

    mind_sear_tick = new mind_sear_tick_t( p );

    if ( p -> mastery_spells.shadowy_recall -> ok() )
    {
      proc_spell = new mind_sear_mastery_t( p );
    }
  }

  virtual void execute()
  {
    priest_spell_t::execute();
  }

  virtual void init()
  {
    priest_spell_t::init();

    mind_sear_tick -> stats = stats;
  }

  virtual void tick( dot_t* d )
  {
    if ( mind_sear_tick )
      mind_sear_tick -> execute();

    if ( proc_spell && p() -> rngs.mastery_extra_tick -> roll( p() -> shadowy_recall_chance() ) )
    {
      proc_spell -> schedule_execute();
    }

    stats -> add_tick( d -> time_to_tick );
  }
};

// Shadow Word Death Spell ==================================================

struct shadow_word_death_t : public priest_spell_t
{
  struct shadow_word_death_backlash_t : public priest_spell_t
  {
    double spellpower;
    double multiplier;

    shadow_word_death_backlash_t( priest_t* p ) :
      priest_spell_t( "shadow_word_death_backlash", p, p -> find_class_spell( "Shadow Word: Death" ) ),
      spellpower( 0.0 ), multiplier( 1.0 )
    {
      background = true;
      harmful    = false;
      proc       = true;
      may_crit   = false;
      callbacks  = false;

      // Hard-coded values as nothing in DBC
      base_dd_min = base_dd_max = 0.533 * p -> dbc.spell_scaling( data().scaling_class(), p -> level );
      direct_power_mod = 0.599;

      target = p;
    }

    virtual void init()
    {
      priest_spell_t::init();

      stats -> type = STATS_NEUTRAL;
    }

    virtual timespan_t execute_time()
    {
      return sim -> gauss( sim -> default_aura_delay, sim -> default_aura_delay_stddev );
    }

    virtual double composite_spell_power()
    {
      return spellpower;
    }

    virtual double composite_spell_power_multiplier()
    {
      return 1.0;
    }

    virtual double composite_da_multiplier()
    {
      double d = multiplier;
      if ( p() -> set_bonus.tier13_2pc_caster() )
        d *= 0.663587;

      return d;
    }
  };

  shadow_word_death_backlash_t* backlash;

  shadow_word_death_t( priest_t* p, const std::string& options_str, bool dtr=false ) :
    priest_spell_t( "shadow_word_death", p, p -> find_class_spell( "Shadow Word: Death" ) ),
    backlash( 0 )
  {
    parse_options( NULL, options_str );

    base_multiplier *= 1.0 + p -> set_bonus.tier13_2pc_caster() * p -> sets -> set( SET_T13_2PC_CASTER ) -> effectN( 1 ).percent();

    backlash = new shadow_word_death_backlash_t( p );

    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new shadow_word_death_t( p, options_str, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual void execute()
  {
    bool below_20 = ( target -> health_percentage() < 20.0 );

    priest_spell_t::execute();

    if ( below_20 && ! is_dtr_action && ! p() -> buffs.shadow_word_death_reset_cooldown -> up() )
    {
      cooldown -> reset();
      p() -> buffs.shadow_word_death_reset_cooldown -> trigger();
    }
  }

  virtual void impact_s( action_state_t* s )
  {
    s -> result_amount = floor( s -> result_amount );

    if ( backlash )
    {
      backlash -> spellpower = s -> spell_power;
      backlash -> multiplier = s -> da_multiplier;
      backlash -> schedule_execute();
    }

    if ( result_is_hit() )
    {
      if ( target -> health_percentage() < 20.0 )
      {
        s -> result_amount *= 4.0;

        generate_shadow_orb( this, p() -> gains.shadow_orb_swd );
      }
    }

    priest_spell_t::impact_s( s );
  }

  virtual double action_multiplier()
  {
    double m = priest_spell_t::action_multiplier();

    if ( p() -> buffs.dark_archangel -> up() )
    {
      m *= 1.0 + p() -> buffs.dark_archangel -> value();
    }

    return m;
  }
};

// Devouring Plague Mastery Proc ===========================================

struct devouring_plague_mastery_t : public priest_procced_mastery_spell_t
{
  int orbs_used;

  devouring_plague_mastery_t( priest_t* p ) :
    priest_procced_mastery_spell_t( "devouring_plague_mastery", p,
                                    p -> find_class_spell( "Devouring Plague" ) -> ok() ? p -> find_spell( 124467 ) : spell_data_t::not_found() ),
    orbs_used( 0 )
  {
  }

  virtual double action_ta_multiplier()
  {
    double m = priest_spell_t::action_ta_multiplier();

    m *= orbs_used;

    return m;
  }

  // BUG: Doesn't heal from ticks as of 15762
};


// Devouring Plague Spell ===================================================

struct devouring_plague_t : public priest_spell_t
{
  struct devouring_plague_state_t : public action_state_t
  {
    int orbs_used;

    devouring_plague_state_t( action_t* a, player_t* t ) : action_state_t( a, t ),
      orbs_used ( 0 )
    {

    }

    virtual void copy_state( const action_state_t* o )
    {
      if ( this == o || o == 0 ) return;

      action_state_t::copy_state( o );

      const devouring_plague_state_t* dps_t = static_cast< const devouring_plague_state_t* >( o );
      orbs_used = dps_t -> orbs_used;
    }
  };

  devouring_plague_mastery_t* proc_spell;

  devouring_plague_t( priest_t* p, const std::string& options_str, bool dtr=false ) :
    priest_spell_t( "devouring_plague", p, p -> find_class_spell( "Devouring Plague" ) ),
    proc_spell( 0 )
  {
    parse_options( NULL, options_str );

    tick_power_mod = 0.166; // hardcoded into tooltip in MoP build 15752

    tick_may_crit = true;

    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new devouring_plague_t( p, options_str, true );
      dtr_action -> is_dtr_action = true;
    }

    if ( p -> mastery_spells.shadowy_recall -> ok() )
    {
      proc_spell = new devouring_plague_mastery_t( p );
    }
  }

  virtual void init()
  {
    priest_spell_t::init();

    // Override snapshot flags
    snapshot_flags |= STATE_USER_1;
  }

  virtual action_state_t* new_state()
  {
    return new devouring_plague_state_t( this, target );
  }

  virtual action_state_t* get_state( const action_state_t* s )
  {
    action_state_t* s_ = priest_spell_t::get_state( s );

    if ( s == 0 )
    {
      devouring_plague_state_t* ds_ = static_cast< devouring_plague_state_t* >( s_ );
      ds_ -> orbs_used = 0;
    }

    return s_;
  }

  virtual void snapshot_state( action_state_t* state, uint32_t flags )
  {
    devouring_plague_state_t* dps_t = static_cast< devouring_plague_state_t* >( state );

    if ( flags & STATE_USER_1 )
      dps_t -> orbs_used = ( int ) p() -> resources.current[ current_resource() ];

    priest_spell_t::snapshot_state( state, flags );
  }

  virtual void consume_resource()
  {
    resource_consumed = cost();

    if ( execute_state -> result != RESULT_MISS )
    {
      resource_consumed = p() -> resources.current[ current_resource() ];
    }

    player -> resource_loss( current_resource(), resource_consumed, 0, this );

    if ( sim -> log )
      sim -> output( "%s consumes %.1f %s for %s (%.0f)", player -> name(),
                     resource_consumed, util::resource_type_string( current_resource() ),
                     name(), player -> resources.current[ current_resource() ] );

    stats -> consume_resource( current_resource(), resource_consumed );
  }

  virtual double action_da_multiplier()
  {
    double m = priest_spell_t::action_da_multiplier();

    m *= p() -> resources.current[ current_resource() ];

    return m;
  }

  virtual double action_ta_multiplier()
  {
    double m = priest_spell_t::action_ta_multiplier();

    m *= p() -> resources.current[ current_resource() ];

    return m;
  }

  virtual void impact_s( action_state_t* s )
  {
    priest_spell_t::impact_s( s );

    devouring_plague_state_t* dps_t = static_cast< devouring_plague_state_t* >( s );
    double a = data().effectN( 3 ).percent() * dps_t -> orbs_used * p() -> resources.max[ RESOURCE_HEALTH ];
    p() -> resource_gain( RESOURCE_HEALTH, a, p() -> gains.devouring_plague_health );
  }

  virtual void tick( dot_t* d )
  {
    priest_spell_t::tick( d );

    devouring_plague_state_t* dps_t = static_cast< devouring_plague_state_t* >( d -> state );
    double a = data().effectN( 4 ).percent() / 100.0 * dps_t -> orbs_used * p() -> resources.max[ RESOURCE_HEALTH ];

    p() -> resource_gain( RESOURCE_HEALTH, a, p() -> gains.devouring_plague_health );

    if ( proc_spell && dps_t -> orbs_used && p() -> rngs.mastery_extra_tick -> roll( p() -> shadowy_recall_chance() ) )
    {
      devouring_plague_state_t* dps_t = static_cast< devouring_plague_state_t* >( d -> state );
      proc_spell -> orbs_used = dps_t -> orbs_used;
      proc_spell -> schedule_execute();
    }
  }
};

// Shadow Word: Pain Mastery Proc ===========================================

struct shadow_word_pain_mastery_t : public priest_procced_mastery_spell_t
{
  shadow_word_pain_mastery_t( priest_t* p ) :
    priest_procced_mastery_spell_t( "shadow_word_pain_mastery", p,
                                    p -> find_class_spell( "Shadow Word: Pain" ) -> ok() ? p -> find_spell( 124464 ) : spell_data_t::not_found() )
  {
  }
};

// Shadow Word Pain Spell ===================================================

struct shadow_word_pain_t : public priest_spell_t
{
  shadow_word_pain_mastery_t* proc_spell;

  shadow_word_pain_t( priest_t* p, const std::string& options_str ) :
    priest_spell_t( "shadow_word_pain", p, p -> find_class_spell( "Shadow Word: Pain" ) ),
    proc_spell( 0 )
  {
    parse_options( NULL, options_str );

    may_crit   = false;
    tick_zero  = true;

    base_multiplier *= 1.0 + p -> sets -> set( SET_T13_4PC_CASTER ) -> effectN( 1 ).percent();

    if ( p -> mastery_spells.shadowy_recall -> ok() )
    {
      proc_spell = new shadow_word_pain_mastery_t( p );
    }
  }

  virtual void tick( dot_t* d )
  {
    priest_spell_t::tick( d );

    if ( ( tick_dmg > 0 ) && ( p() -> specs.shadowy_apparitions -> ok() ) )
    {
      trigger_shadowy_apparition( d );
    }
    if ( ( tick_dmg > 0 ) && ( p() -> talents.divine_insight -> ok() ) )
    {
      if ( p() -> buffs.divine_insight_shadow -> trigger() )
      {
        p() -> cooldowns.mind_blast -> reset( true );
        p() -> procs.divine_insight_shadow -> occur();
      }
    }
    if ( proc_spell && p() -> rngs.mastery_extra_tick -> roll( p() -> shadowy_recall_chance() ) )
    {
      proc_spell -> schedule_execute();
    }
  }
};

// Vampiric Embrace Spell ===================================================

struct vampiric_embrace_t : public priest_spell_t
{
  vampiric_embrace_t( priest_t* p, const std::string& options_str ) :
    priest_spell_t( "vampiric_embrace", p, p -> find_class_spell( "Vampiric Embrace" ) )
  {
    parse_options( NULL, options_str );

    harmful = false;
  }

  virtual void execute()
  {
    priest_spell_t::execute();

    p() -> buffs.vampiric_embrace -> trigger();
  }

  virtual bool ready()
  {
    if ( p() -> buffs.vampiric_embrace -> check() )
      return false;

    return priest_spell_t::ready();
  }
};

// Vampiric Touch Mastery Proc ===========================================

struct vampiric_touch_mastery_t : public priest_procced_mastery_spell_t
{
  vampiric_touch_mastery_t( priest_t* p ) :
    priest_procced_mastery_spell_t( "vampiric_touch_mastery", p,
                                    p -> find_class_spell( "Vampiric Touch" ) -> ok() ? p -> find_spell( 124465 ) : spell_data_t::not_found() )
  {
  }

  virtual void execute()
  {
    priest_procced_mastery_spell_t::execute();

    double m = player -> resources.max[ RESOURCE_MANA ] * data().effectN( 1 ).percent();
    player -> resource_gain( RESOURCE_MANA, m, p() -> gains.vampiric_touch_mastery_mana, this );
  }
};

// Vampiric Touch Spell =====================================================

struct vampiric_touch_t : public priest_spell_t
{
  vampiric_touch_mastery_t* proc_spell;

  vampiric_touch_t( priest_t* p, const std::string& options_str ) :
    priest_spell_t( "vampiric_touch", p, p -> find_class_spell( "Vampiric Touch" ) ),
    proc_spell( 0 )
  {
    parse_options( NULL, options_str );
    may_crit   = false;
    if ( p -> mastery_spells.shadowy_recall -> ok() )
    {
      proc_spell = new vampiric_touch_mastery_t( p );
    }
  }

  virtual void tick( dot_t* d )
  {
    priest_spell_t::tick( d );

    double m = player->resources.max[ RESOURCE_MANA ] * data().effectN( 1 ).percent();
    player -> resource_gain( RESOURCE_MANA, m, p() -> gains.vampiric_touch_mana, this );

    if ( p() -> buffs.surge_of_darkness -> trigger() )
    {
      p() -> procs.surge_of_darkness -> occur();
    }

    if ( proc_spell && p() -> rngs.mastery_extra_tick -> roll( p() -> shadowy_recall_chance() ) )
    {
      proc_spell -> schedule_execute();
    }
  }
};

// Holy Fire Spell ==========================================================

struct holy_fire_t : public priest_spell_t
{
  holy_fire_t( priest_t* player, const std::string& options_str, bool dtr=false ) :
    priest_spell_t( "holy_fire", player, player -> find_class_spell( "Holy Fire" ) )
  {
    parse_options( NULL, options_str );

    base_hit += p() -> specs.divine_fury -> effectN( 1 ).percent();

    can_trigger_atonement = true;

    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new holy_fire_t( player, options_str, true );
      dtr_action -> is_dtr_action = true;
    }

    castable_in_shadowform = false;
  }

  virtual void execute()
  {
    priest_spell_t::execute();

    p() -> buffs.holy_evangelism  -> trigger();
  }

  virtual double action_multiplier()
  {
    double m = priest_spell_t::action_multiplier();

    m *= 1.0 + ( p() -> buffs.holy_evangelism -> stack() * p() -> buffs.holy_evangelism -> data().effectN( 1 ).percent() );

    return m;
  }

  virtual double cost()
  {
    double c = priest_spell_t::cost();

    c *= 1.0 + ( p() -> buffs.holy_evangelism -> check() * p() -> buffs.holy_evangelism -> data().effectN( 2 ).percent() );

    return c;
  }

  virtual timespan_t execute_time()
  {
    timespan_t a = priest_spell_t::execute_time();

    if ( p() -> glyphs.holy_fire -> ok() )
      a *= 0.0;

    return a;
  }

};

// Penance Spell ============================================================

struct penance_t : public priest_spell_t
{
  struct penance_tick_t : public priest_spell_t
  {
    penance_tick_t( priest_t* p ) :
      priest_spell_t( "penance_tick", p, p -> find_spell( 47666 ) )
    {
      background  = true;
      dual        = true;
      direct_tick = true;
      base_hit += p -> specs.divine_fury -> effectN( 1 ).percent();
    }

    virtual double action_multiplier()
    {
      double m = priest_spell_t::action_multiplier();

      m *= 1.0 + ( p() -> buffs.holy_evangelism -> stack() * p() -> buffs.holy_evangelism -> data().effectN( 1 ).percent() );

      return m;
    }
  };

  penance_tick_t* tick_spell;

  penance_t( priest_t* p, const std::string& options_str ) :
    priest_spell_t( "penance", p, p -> find_class_spell( "Penance" ) ),
    tick_spell( 0 )
  {
    parse_options( NULL, options_str );

    may_crit       = false;
    may_miss       = false;
    channeled      = true;
    tick_zero      = true;
    num_ticks      = 2;
    base_tick_time = timespan_t::from_seconds( 1.0 );
    hasted_ticks   = false;

    cooldown -> duration = data().cooldown() + p -> glyphs.penance -> effectN( 2 ).time_value();

    tick_spell = new penance_tick_t( p );

    castable_in_shadowform = false;
  }

  virtual void init()
  {
    priest_spell_t::init();

    tick_spell -> stats = stats;
  }

  virtual void tick( dot_t* d )
  {
    if ( sim -> debug ) sim -> output( "%s ticks (%d of %d)", name(), d -> current_tick, d -> num_ticks );
    tick_spell -> execute();
    stats -> add_tick( d -> time_to_tick );
  }

  virtual double cost()
  {
    double c = priest_spell_t::cost();

    c *= 1.0 + ( p() -> buffs.holy_evangelism -> check() * p() -> buffs.holy_evangelism -> data().effectN( 2 ).percent() );

    c *= 1.0 + p() -> glyphs.penance -> effectN( 1 ).percent();

    return c;
  }
};

// Smite Spell ==============================================================

struct smite_t : public priest_spell_t
{
  smite_t( priest_t* p, const std::string& options_str, bool dtr=false ) :
    priest_spell_t( "smite", p, p -> find_class_spell( "Smite" ) )
  {
    parse_options( NULL, options_str );

    base_hit += p -> specs.divine_fury -> effectN( 1 ).percent();

    can_trigger_atonement = true;

    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new smite_t( p, options_str, true );
      dtr_action -> is_dtr_action = true;
    }

    castable_in_shadowform = false;
  }

  virtual void execute()
  {
    priest_spell_t::execute();

    p() -> buffs.holy_evangelism -> trigger();

    trigger_chakra( p(), p() -> buffs.chakra_chastise );

    // Train of Thought
    if ( p() -> specs.train_of_thought -> ok() )
    {
      if ( p() -> cooldowns.penance -> remains() > p() -> specs.train_of_thought -> effectN( 2 ).time_value() )
        p() -> cooldowns.penance -> adjust ( - p() -> specs.train_of_thought -> effectN( 2 ).time_value() );
      else
        p() -> cooldowns.penance -> reset();
    }
  }

  virtual double composite_target_multiplier( player_t* target )
  {
    double m = priest_spell_t::composite_target_multiplier( target );

    if ( td( target ) -> dots.holy_fire -> ticking && p() -> glyphs.smite -> ok() )
      m *= 1.0 + p() -> glyphs.smite -> effectN( 1 ).percent();

    return m;
  }

  virtual double action_multiplier()
  {
    double am = priest_spell_t::action_multiplier();

    am *= 1.0 + ( p() -> buffs.holy_evangelism -> stack() * p() -> buffs.holy_evangelism -> data().effectN( 1 ).percent() );

    return am;
  }

  virtual double cost()
  {
    double c = priest_spell_t::cost();

    c *= 1.0 + ( p() -> buffs.holy_evangelism -> check() * p() -> buffs.holy_evangelism -> data().effectN( 2 ).percent() );

    return c;
  }
};

// Cascade Spell

// Assumptions: Target list is created on execute() and will not change dynamically during the jump process

template <class Base>
struct cascade_base_t : public Base
{
  typedef Base action_base_t; // typedef for the templated action type, priest_spell_t, or priest_heal_t
  typedef cascade_base_t base_t; // typedef for cascade_base_t<action_base_t>

  struct cascade_state_t : action_state_t
  {
    int jump_counter;
    cascade_state_t( action_t* a, player_t* t ) : action_state_t( a, t ),
        jump_counter( 0 )
    { }
  };

  std::vector<player_t*> targets;
  const spell_data_t* talent_data;

  cascade_base_t( const std::string& n, priest_t* p, const std::string& options_str, const spell_data_t* s ) :
    action_base_t( n, p, s )
  {
    talent_data = p -> find_talent_spell( "Cascade" );

    action_base_t::parse_options( NULL, options_str );

    action_base_t::cooldown -> duration = timespan_t::from_seconds( 50.0 );
  }

  virtual action_state_t* new_state()
  {
    return new cascade_state_t( this, action_base_t::target );
  }

  player_t* get_next_player()
  {
    player_t* t = NULL;

    // Get target at first position
    if ( targets.size() >= 1 )
    {
      t = *targets.begin();
      targets.erase( targets.begin() );
    }

    return t;
  }

  virtual void execute()
  {
    // Clear and create targets list
    targets.clear();

    //for ( size_t i = 0, actors = sim -> actor_list.size(); i < actors; ++i )
    //{ player_t* t = sim -> actor_list[ i ]; if ( t != target ) targets.push_back( t ); }


    // For now only use enemy targets
    for ( player_t* t = action_base_t::sim -> target_list; t; t = t -> next )
    { if ( t != action_base_t::target ) targets.push_back( t ); }

    action_base_t::execute();
  }

  virtual void impact_s( action_state_t* q )
  {
    action_base_t::impact_s( q );

    cascade_state_t* cs = debug_cast<cascade_state_t*>( q );

    if ( cs -> jump_counter < 3 /* talent_data->effectN( 1 ).base_value() */ )
    {
      for ( unsigned i = 0; i < 2; ++i )
      {
        player_t* t = get_next_player();

        if ( t )
        {
          if ( action_base_t::sim -> debug )
            action_base_t::sim -> output( "%s action %s jumps to player %s",
                action_base_t::player -> name(), action_base_t::name(), t -> name() );


          // Copy-Pasted action_t::execute() code. Additionally increasing jump counter by one.
          cascade_state_t* s = debug_cast<cascade_state_t*>( action_base_t::get_state() );
          s -> target = t;
          s -> jump_counter = cs -> jump_counter + 1;
          snapshot_state( s, action_base_t::snapshot_flags );
          s -> result = calculate_result( s -> composite_crit(), s -> target -> level );

          if ( result_is_hit( s -> result ) )
            s -> result_amount = calculate_direct_damage( s -> result, 0, s -> attack_power, s -> spell_power, s -> composite_da_multiplier(), s -> target );

          if ( action_base_t::sim -> debug )
            s -> debug();

          schedule_travel_s( s );
        }
      }
    }
    else
    {
      cs -> jump_counter = 0;
    }
  }
};

struct cascade_damage_t : public cascade_base_t<priest_spell_t>
{
  cascade_damage_t( priest_t* p, const std::string& options_str ) :
    base_t( "cascade_damage", p, options_str, p -> find_spell( 120785 ) )
  {

  }
};

struct cascade_heal_t : public cascade_base_t<priest_heal_t>
{
  cascade_heal_t( priest_t* p, const std::string& options_str ) :
    base_t( "cascade_heal", p, options_str, p -> find_spell( 121148 ) )
  {

  }
};
} // NAMESPACE spells

// ==========================================================================
// Priest Heal & Absorb Spells
// ==========================================================================



namespace heals {

void consume_inner_focus( priest_t* p )
{
  if ( p -> buffs.inner_focus -> up() )
  {
    // Inner Focus cooldown starts when consumed.
    p -> cooldowns.inner_focus -> reset();
    p -> cooldowns.inner_focus -> duration = p -> buffs.inner_focus -> data().cooldown();
    p -> cooldowns.inner_focus -> start();
    p -> buffs.inner_focus -> expire();
  }
}

struct echo_of_light_t : public priest_heal_t
{
  echo_of_light_t( priest_t* p ) :
    priest_heal_t( "echo_of_light", p, p -> find_spell( 77489 ) )
  {
    base_tick_time = timespan_t::from_seconds( 1.0 );
    num_ticks      = static_cast<int>( data().duration() / base_tick_time );

    background     = true;
    tick_may_crit  = false;
    hasted_ticks   = false;
    may_crit       = false;

  }
};

// Binding Heal Spell =======================================================

struct binding_heal_t : public priest_heal_t
{
  binding_heal_t( priest_t* p, const std::string& options_str ) :
    priest_heal_t( "binding_heal", p, p -> find_class_spell( "Binding Heal" ) )
  {
    parse_options( NULL, options_str );

    aoe = 1;
  }

  virtual void execute()
  {
    priest_heal_t::execute();

    consume_inner_focus( p() );

    trigger_chakra( p(), p() -> buffs.chakra_serenity );
  }

  virtual double composite_crit()
  {
    double cc = priest_heal_t::composite_crit();

    if ( p() -> buffs.inner_focus -> check() )
      cc += p() -> buffs.inner_focus -> data().effectN( 2 ).percent();

    return cc;
  }

  virtual double cost()
  {
    double c = priest_heal_t::cost();

    if ( p() -> buffs.inner_focus -> check() )
      c = 0;

    return c;
  }
};

// Circle of Healing ========================================================

struct circle_of_healing_t : public priest_heal_t
{
  circle_of_healing_t( priest_t* p, const std::string& options_str ) :
    priest_heal_t( "circle_of_healing", p, p -> find_class_spell( "Circle of Healing" ) )
  {
    parse_options( NULL, options_str );

    base_costs[ current_resource() ] *= 1.0 + p -> glyphs.circle_of_healing -> effectN( 2 ).percent();
    base_costs[ current_resource() ]  = floor( base_costs[ current_resource() ] );
    aoe = p -> glyphs.circle_of_healing -> ok() ? 5 : 4;
  }

  virtual void execute()
  {
    cooldown -> duration = data().cooldown();

    if ( p() -> buffs.chakra_sanctuary -> up() )
      cooldown -> duration +=  p() -> buffs.chakra_sanctuary -> data().effectN( 2 ).time_value();

    // Choose Heal Target
    target = find_lowest_player();

    priest_heal_t::execute();
  }

  virtual double action_multiplier()
  {
    double am = priest_heal_t::action_multiplier();

    if ( p() -> buffs.chakra_sanctuary -> up() )
      am *= 1.0 + p() -> buffs.chakra_sanctuary -> data().effectN( 1 ).percent();

    return am;
  }
};

// Divine Hymn Spell ========================================================

struct divine_hymn_tick_t : public priest_heal_t
{
  divine_hymn_tick_t( priest_t* player, int nr_targets ) :
    priest_heal_t( "divine_hymn_tick", player, player -> find_spell( 64844 ) )
  {
    background  = true;

    aoe = nr_targets - 1;
  }

  virtual double action_multiplier()
  {
    double am = priest_heal_t::action_multiplier();

    if ( p() -> buffs.chakra_sanctuary -> up() )
      am *= 1.0 + p() -> buffs.chakra_sanctuary -> data().effectN( 1 ).percent();

    return am;
  }
};

struct divine_hymn_t : public priest_heal_t
{
  divine_hymn_tick_t* divine_hymn_tick;

  divine_hymn_t( priest_t* p, const std::string& options_str ) :
    priest_heal_t( "divine_hymn", p, p -> find_class_spell( "Divine Hymn" ) ),
    divine_hymn_tick( 0 )
  {
    parse_options( NULL, options_str );

    harmful = false;
    channeled = true;

    divine_hymn_tick = new divine_hymn_tick_t( p, data().effectN( 2 ).base_value() );
    add_child( divine_hymn_tick );
  }

  virtual void tick( dot_t* d )
  {
    if ( sim -> debug ) sim -> output( "%s ticks (%d of %d)", name(), d -> current_tick, d -> num_ticks );
    divine_hymn_tick -> execute();
    stats -> add_tick( d -> time_to_tick );
  }
};

// Flash Heal Spell =========================================================

struct flash_heal_t : public priest_heal_t
{
  flash_heal_t( priest_t* p, const std::string& options_str ) :
    priest_heal_t( "flash_heal", p, p -> find_class_spell( "Flash Heal" ) )
  {
    parse_options( NULL, options_str );
  }

  virtual void execute()
  {
    priest_heal_t::execute();

    consume_inner_focus( p() );

    trigger_chakra( p(), p() -> buffs.chakra_serenity );
  }

  virtual void impact_s( action_state_t* s )
  {
    priest_heal_t::impact_s( s );

    trigger_grace( s -> target );
    trigger_strength_of_soul( s -> target );
  }

  virtual double composite_crit()
  {
    double cc = priest_heal_t::composite_crit();

    if ( p() -> buffs.inner_focus -> check() )
      cc += p() -> buffs.inner_focus -> data().effectN( 2 ).percent();

    return cc;
  }

  virtual timespan_t execute_time()
  {
    if ( p () -> buffs.surge_of_light -> up() )
      return timespan_t::zero();

    return priest_heal_t::execute_time();
  }

  virtual double cost()
  {
    double c = priest_heal_t::cost();

    if ( p() -> buffs.inner_focus -> check() )
      c = 0;

    if ( p() -> buffs.surge_of_light -> check() )
      c = 0;

    return c;
  }
};

// Guardian Spirit ==========================================================

struct guardian_spirit_t : public priest_heal_t
{
  guardian_spirit_t( priest_t* p, const std::string& options_str ) :
    priest_heal_t( "guardian_spirit", p, p -> find_class_spell( "Guardian Spirit" ) )
  {
    parse_options( NULL, options_str );

    // The absorb listed isn't a real absorb
    base_dd_min = base_dd_max = 0;

    harmful = false;
  }

  virtual void execute()
  {
    priest_heal_t::execute();

    target -> buffs.guardian_spirit -> trigger();
    target -> buffs.guardian_spirit -> source = player;
  }
};

// Greater Heal Spell =======================================================

struct greater_heal_t : public priest_heal_t
{
  greater_heal_t( priest_t* p, const std::string& options_str ) :
    priest_heal_t( "greater_heal", p, p -> find_class_spell( "Greater Heal" ) )
  {
    parse_options( NULL, options_str );
  }

  virtual void execute()
  {
    priest_heal_t::execute();

    // Train of Thought
    // NOTE: Process Train of Thought _before_ Inner Focus: the GH that consumes Inner Focus does not
    //       reduce the cooldown, since Inner Focus doesn't go on cooldown until after it is consumed.
    if ( p() -> specs.train_of_thought -> ok() )
    {
      if ( p() -> cooldowns.inner_focus -> remains() > timespan_t::from_seconds( p() -> specs.train_of_thought -> effectN( 1 ).base_value() ) )
        p() -> cooldowns.inner_focus -> adjust( timespan_t::from_seconds( - p() -> specs.train_of_thought -> effectN( 1 ).base_value() ) );
      else
        p() -> cooldowns.inner_focus -> reset();
    }

    consume_inner_focus( p() );

    trigger_chakra( p(), p() -> buffs.chakra_serenity );
  }

  virtual void impact_s( action_state_t* s )
  {
    priest_heal_t::impact_s( s );

    trigger_grace( s -> target );
    trigger_strength_of_soul( s -> target );
  }

  virtual double composite_crit()
  {
    double cc = priest_heal_t::composite_crit();

    if ( p() -> buffs.inner_focus -> check() )
      cc += p() -> buffs.inner_focus -> data().effectN( 2 ).percent();

    return cc;
  }

  virtual double cost()
  {
    double c = priest_heal_t::cost();

    if ( p() -> buffs.inner_focus -> check() )
      c = 0;

    return c;
  }
};

// Heal Spell ===============================================================

struct _heal_t : public priest_heal_t
{
  _heal_t( priest_t* p, const std::string& options_str ) :
    priest_heal_t( "heal", p, p -> find_class_spell( "Heal" ) )
  {
    parse_options( NULL, options_str );
  }

  virtual void execute()
  {
    priest_heal_t::execute();

    trigger_chakra( p(), p() -> buffs.chakra_serenity );
  }

  virtual void impact_s( action_state_t* s )
  {
    priest_heal_t::impact_s( s );

    trigger_grace( s -> target );
    trigger_strength_of_soul( s -> target );
  }
};

// Holy Word Sanctuary ======================================================

struct holy_word_sanctuary_t : public priest_heal_t
{
  struct holy_word_sanctuary_tick_t : public priest_heal_t
  {
    holy_word_sanctuary_tick_t( priest_t* player ) :
      priest_heal_t( "holy_word_sanctuary_tick", player, player -> find_spell( 88686 ) )
    {
      dual        = true;
      background  = true;
      direct_tick = true;
    }

    virtual double action_multiplier()
    {
      double am = priest_heal_t::action_multiplier();

      if ( p() -> buffs.chakra_sanctuary -> up() )
        am *= 1.0 + p() -> buffs.chakra_sanctuary -> data().effectN( 1 ).percent();

      return am;
    }
  };

  holy_word_sanctuary_tick_t* tick_spell;

  holy_word_sanctuary_t( priest_t* p, const std::string& options_str ) :
    priest_heal_t( "holy_word_sanctuary", p, p -> find_spell( 88685 ) ),
    tick_spell( 0 )
  {
    check_spell( p -> specs.revelations );

    parse_options( NULL, options_str );

    hasted_ticks = false;
    may_crit     = false;

    base_tick_time = timespan_t::from_seconds( 2.0 );
    num_ticks = 9;

    tick_spell = new holy_word_sanctuary_tick_t( p );

    // Needs testing
    cooldown -> duration *= 1.0 + p -> set_bonus.tier13_4pc_heal() * -0.2;
  }

  virtual void init()
  {
    priest_heal_t::init();

    tick_spell -> stats = stats;
  }

  virtual void tick( dot_t* d )
  {
    tick_spell -> execute();
    stats -> add_tick( d -> time_to_tick );
  }

  virtual bool ready()
  {
    if ( ! p() -> buffs.chakra_sanctuary -> check() )
      return false;

    return priest_heal_t::ready();
  }

  // HW: Sanctuary is treated as a instant cast spell, both affected by Inner Will and Mental Agility

  virtual double cost()
  {
    double c = priest_heal_t::cost();

    // HW: Sanctuary is treated as a instant cast spell, both affected by Inner Will and Mental Agility
    // Implemented 06/12/2011 ( Patch 4.3 ),
    // see Issue1023 and http://elitistjerks.com/f77/t110245-cataclysm_holy_priest_compendium/p25/#post2054467

    c *= 1.0 + p() -> buffs.inner_will -> check() * p() -> buffs.inner_will -> data().effectN( 1 ).percent();
    c  = floor( c );

    return c;
  }

  virtual void consume_resource()
  {
    priest_heal_t::consume_resource();

    p() -> buffs.inner_will -> up();
  }
};

// Holy Word Chastise =======================================================

struct holy_word_chastise_t : public priest_spell_t
{
  holy_word_chastise_t( priest_t* p, const std::string& options_str ) :
    priest_spell_t( "holy_word_chastise", p, p -> find_class_spell( "Holy Word: Chastise" ) )
  {
    parse_options( NULL, options_str );

    base_costs[ current_resource() ]  = floor( base_costs[ current_resource() ] );

    // Needs testing
    cooldown -> duration *= 1.0 + p -> set_bonus.tier13_4pc_heal() * -0.2;

    castable_in_shadowform = false;
  }

  virtual bool ready()
  {
    if ( p() -> specs.revelations -> ok() )
    {
      if ( p() -> buffs.chakra_sanctuary -> check() )
        return false;

      if ( p() -> buffs.chakra_serenity -> check() )
        return false;
    }

    return priest_spell_t::ready();
  }
};

// Holy Word Serenity =======================================================

struct holy_word_serenity_t : public priest_heal_t
{
  holy_word_serenity_t( priest_t* p, const std::string& options_str ) :
    priest_heal_t( "holy_word_serenity", p, p -> find_spell( 88684 ) )
  {
    check_spell( p -> specs.revelations );

    parse_options( NULL, options_str );

    base_costs[ current_resource() ]  = floor( base_costs[ current_resource() ] );

    // Needs testing
    cooldown -> duration *= 1.0 + p -> set_bonus.tier13_4pc_heal() * -0.2;
  }

  virtual void execute()
  {
    priest_heal_t::execute();

    p() -> buffs.serenity -> trigger();
  }

  virtual bool ready()
  {
    if ( ! p() -> buffs.chakra_serenity -> check() )
      return false;

    return priest_heal_t::ready();
  }
};

// Holy Word ================================================================

struct holy_word_t : public priest_spell_t
{
  holy_word_sanctuary_t* hw_sanctuary;
  holy_word_chastise_t*  hw_chastise;
  holy_word_serenity_t*  hw_serenity;

  holy_word_t( priest_t* p, const std::string& options_str ) :
    priest_spell_t( "holy_word", p, spell_data_t::nil() ),
    hw_sanctuary( 0 ), hw_chastise( 0 ), hw_serenity( 0 )
  {
    school = SCHOOL_HOLY;
    hw_sanctuary = new holy_word_sanctuary_t( p, options_str );
    hw_chastise  = new holy_word_chastise_t ( p, options_str );
    hw_serenity  = new holy_word_serenity_t ( p, options_str );

    castable_in_shadowform = false;
  }

  virtual void schedule_execute()
  {
    if ( p() -> specs.revelations -> ok() && p() -> buffs.chakra_serenity -> up() )
    {
      player -> last_foreground_action = hw_serenity;
      hw_serenity -> schedule_execute();
    }
    else if ( p() -> specs.revelations -> ok() && p() -> buffs.chakra_sanctuary -> up() )
    {
      player -> last_foreground_action = hw_sanctuary;
      hw_sanctuary -> schedule_execute();
    }
    else
    {
      player -> last_foreground_action = hw_chastise;
      hw_chastise -> schedule_execute();
    }
  }

  virtual void execute()
  {
    assert( 0 );
  }

  virtual bool ready()
  {
    if ( p() -> specs.revelations -> ok() && p() -> buffs.chakra_serenity -> check() )
      return hw_serenity -> ready();

    else if ( p() -> specs.revelations -> ok() && p() -> buffs.chakra_sanctuary -> check() )
      return hw_sanctuary -> ready();

    else
      return hw_chastise -> ready();
  }
};

// Lightwell Spell ==========================================================

struct lightwell_t : public priest_spell_t
{
  timespan_t consume_interval;

  lightwell_t( priest_t* p, const std::string& options_str ) :
    priest_spell_t( "lightwell", p, p -> find_class_spell( "Lightwell" ) ), consume_interval( timespan_t::from_seconds( 10 ) )
  {
    option_t options[] =
    {
      { "consume_interval", OPT_TIMESPAN,     &consume_interval },
      { NULL,               OPT_UNKNOWN, NULL              }
    };
    parse_options( options, options_str );

    harmful = false;

    castable_in_shadowform = false;

    assert( consume_interval > timespan_t::zero() && consume_interval < cooldown -> duration );
  }

  virtual void execute()
  {
    priest_spell_t::execute();

    p() -> pets.lightwell -> get_cooldown( "lightwell_renew" ) -> duration = consume_interval;
    p() -> pets.lightwell -> summon( data().duration() );
  }
};

// Penance Heal Spell =======================================================

struct penance_heal_tick_t : public priest_heal_t
{
  penance_heal_tick_t( priest_t* player ) :
    priest_heal_t( "penance_heal_tick", player, player -> find_spell( 47666 ) )
  {
    background  = true;
    may_crit    = true;
    dual        = true;
    direct_tick = true;

    school = SCHOOL_HOLY;
    stats = player -> get_stats( "penance_heal", this );
  }

  virtual void impact_s( action_state_t* s )
  {
    priest_heal_t::impact_s( s );

    trigger_grace( s -> target );
  }
};

struct penance_heal_t : public priest_heal_t
{
  penance_heal_tick_t* penance_tick;

  penance_heal_t( priest_t* p, const std::string& options_str ) :
    priest_heal_t( "penance_heal", p, p -> find_class_spell( "Penance" ) ), penance_tick( 0 )
  {
    parse_options( NULL, options_str );

    may_crit       = false;
    channeled      = true;
    tick_zero      = true;
    num_ticks      = 2;
    base_tick_time = timespan_t::from_seconds( 1.0 );
    hasted_ticks   = false;

    cooldown = p -> cooldowns.penance;
    cooldown -> duration = data().cooldown() + p -> glyphs.penance -> effectN( 2 ).time_value();

    penance_tick = new penance_heal_tick_t( p );
    penance_tick -> target = target;
  }

  virtual void tick( dot_t* d )
  {
    if ( sim -> debug ) sim -> output( "%s ticks (%d of %d)", name(), d -> current_tick, d -> num_ticks );
    penance_tick -> target = target;
    penance_tick -> execute();
    stats -> add_tick( d -> time_to_tick );
  }

  virtual double cost()
  {
    double c = priest_heal_t::cost();

    c *= 1.0 + ( p() -> buffs.holy_evangelism -> check() * p() -> buffs.holy_evangelism -> data().effectN( 2 ).percent() );

    c *= 1.0 + p() -> glyphs.penance -> effectN( 1 ).percent();

    return c;
  }
};

// Power Word: Shield Spell =================================================

struct glyph_power_word_shield_t : public priest_heal_t
{
  glyph_power_word_shield_t( priest_t* player ) :
    priest_heal_t( "power_word_shield_glyph", player, player -> find_spell( 55672 ) )
  {
    school          = SCHOOL_HOLY;
    stats -> school = school;

    background = true;
    proc       = true;

    castable_in_shadowform = true;
  }
};

struct power_word_shield_t : public priest_absorb_t
{
  glyph_power_word_shield_t* glyph_pws;
  int ignore_debuff;

  power_word_shield_t( priest_t* p, const std::string& options_str ) :
    priest_absorb_t( "power_word_shield", p, p -> find_class_spell( "Power Word: Shield" ) ), glyph_pws( 0 ), ignore_debuff( 0 )
  {
    option_t options[] =
    {
      { "ignore_debuff", OPT_BOOL,    &ignore_debuff },
      { NULL,            OPT_UNKNOWN, NULL           }
    };
    parse_options( options, options_str );

    if ( p -> specs.rapture -> ok() )
      cooldown -> duration = timespan_t::zero();

    // Tooltip is wrong.
    // direct_power_mod = 0.87; // hardcoded into tooltip
    direct_power_mod = 1.8709; // matches in-game actual value

    if ( p -> glyphs.power_word_shield -> ok() )
    {
      glyph_pws = new glyph_power_word_shield_t( p );
      glyph_pws -> target = target;
      add_child( glyph_pws );
    }

    castable_in_shadowform = true;
  }

  virtual double cost()
  {
    double c = priest_absorb_t::cost();

    return c;
  }

  virtual void impact_s( action_state_t* s )
  {

    s -> target -> buffs.weakened_soul -> trigger();

    // Rapture
    if ( p() -> cooldowns.rapture -> remains() == timespan_t::zero() && p() -> specs.rapture -> ok() )
    {
      player -> resource_gain( RESOURCE_MANA, player -> spirit() * p() -> specs.rapture -> effectN( 1 ).percent(), p() -> gains.rapture );
      p() -> cooldowns.rapture -> start();
    }

    // Glyph
    if ( glyph_pws )
    {
      glyph_pws -> base_dd_min  = glyph_pws -> base_dd_max  = p() -> glyphs.power_word_shield -> effectN( 1 ).percent() * s -> result_amount;
      glyph_pws -> target = s -> target;
      glyph_pws -> execute();
    }

    td( s -> target ) -> buffs.power_word_shield -> trigger( 1, s -> result_amount );
    stats -> add_result( 0, s -> result_amount, ABSORB, s -> result );
  }

  virtual bool ready()
  {
    if ( ! ignore_debuff && target -> buffs.weakened_soul -> check() )
      return false;

    return priest_absorb_t::ready();
  }
};

// Prayer of Healing Spell ==================================================

struct prayer_of_healing_t : public priest_heal_t
{
  prayer_of_healing_t( priest_t* p, const std::string& options_str ) :
    priest_heal_t( "prayer_of_healing", p, p -> find_class_spell( "Prayer of Healing" ) )
  {
    parse_options( NULL, options_str );

    aoe = 4;
    group_only = true;
  }

  virtual void init()
  {
    priest_heal_t::init();

    // PoH crits double the DA percentage.
    if ( da ) da -> shield_multiple *= 2;
  }

  virtual void execute()
  {
    priest_heal_t::execute();

    consume_inner_focus( p() );

    trigger_chakra( p(), p() -> buffs.chakra_sanctuary );
  }

  virtual void impact_s( action_state_t* s )
  {
    priest_heal_t::impact_s( s );

    // Divine Aegis
    if ( s -> result != RESULT_CRIT )
    {
      trigger_divine_aegis( s -> target, s -> result_amount * 0.5 );
    }
  }

  virtual double action_multiplier()
  {
    double am = priest_heal_t::action_multiplier();

    if ( p() -> buffs.chakra_sanctuary -> up() )
      am *= 1.0 + p() -> buffs.chakra_sanctuary -> data().effectN( 1 ).percent();

    return am;
  }

  virtual double composite_crit()
  {
    double cc = priest_heal_t::composite_crit();

    if ( p() -> buffs.inner_focus -> check() )
      cc += p() -> buffs.inner_focus -> data().effectN( 2 ).percent();

    return cc;
  }

  virtual double cost()
  {
    double c = priest_heal_t::cost();

    if ( p() -> buffs.inner_focus -> check() )
      c = 0;

    return c;
  }
};

// Prayer of Mending Spell ==================================================

struct prayer_of_mending_t : public priest_heal_t
{
  int single;
  prayer_of_mending_t( priest_t* p, const std::string& options_str ) :
    priest_heal_t( "prayer_of_mending", p, p -> find_class_spell( "Prayer of Mending" ) ), single( false )
  {
    option_t options[] =
    {
      { "single", OPT_BOOL,       &single },
      { NULL,     OPT_UNKNOWN, NULL       }
    };
    parse_options( options, options_str );

    direct_power_mod = data().effectN( 1 ).coeff();
    base_dd_min = base_dd_max = data().effectN( 1 ).min( p );

    can_trigger_DA = false;

    aoe = 4;

    castable_in_shadowform = p -> glyphs.dark_binding -> ok();
  }

  virtual double action_multiplier()
  {
    double am = priest_heal_t::action_multiplier();

    if ( p() -> buffs.chakra_sanctuary -> up() )
      am *= 1.0 + p() -> buffs.chakra_sanctuary -> data().effectN( 1 ).percent();

    return am;
  }

  virtual void execute()
  {

    priest_heal_t::execute();

    trigger_chakra( p(), p() -> buffs.chakra_sanctuary );
  }

  virtual double composite_target_multiplier( player_t* t )
  {
    double ctm = priest_heal_t::composite_target_multiplier( t );

    if ( p() -> glyphs.prayer_of_mending -> ok() && t == target )
      ctm *= 1.0 + p() -> glyphs.prayer_of_mending -> effectN( 1 ).percent();

    return ctm;
  }
};

// Renew Spell ==============================================================

struct divine_touch_t : public priest_heal_t
{
  divine_touch_t( priest_t* player ) :
    priest_heal_t( "divine_touch", player, player -> find_spell( 63544 ) )
  {
    school          = SCHOOL_HOLY;
    stats -> school = school;

    background = true;
    proc       = true;
  }
};

struct renew_t : public priest_heal_t
{
  renew_t( priest_t* p, const std::string& options_str ) :
    priest_heal_t( "renew", p, p -> find_class_spell( "Renew" ) )
  {
    parse_options( NULL, options_str );

    may_crit = false;

    base_multiplier *= 1.0 + p -> glyphs.renew -> effectN( 1 ).percent();

    num_ticks       += ( int ) ( p -> glyphs.renew -> effectN( 2 ).time_value() / base_tick_time );

    castable_in_shadowform = p -> glyphs.dark_binding -> ok();
  }

  virtual double action_multiplier()
  {
    double am = priest_heal_t::action_multiplier();

    if ( p() -> buffs.chakra_sanctuary -> up() )
      am *= 1.0 + p() -> buffs.chakra_sanctuary -> data().effectN( 1 ).percent();

    return am;
  }
};

struct spirit_shell_heal_t : priest_heal_t
{
  spirit_shell_heal_t( priest_t* p ) :
    priest_heal_t( "spirit_shell_heal", p, spell_data_t::nil() )
  {
    background = true;
    school = SCHOOL_HOLY;
  }
};

struct spirit_shell_buff_t : public absorb_buff_t
{
public:
  heal_t* spirit_shell_heal;
  spirit_shell_buff_t( actor_pair_t p ) :
    absorb_buff_t( absorb_buff_creator_t( p, "spirit_shell" )
                   .spell( p.source -> find_spell( 114908 ) )
                   .source( p.source -> get_stats( "spirit_shell" ) )
                 ),
    spirit_shell_heal( NULL )
  { }

  virtual void expire()
  {
    if ( current_value > 0 )
    {
      if ( spirit_shell_heal )
      {
        spirit_shell_heal -> base_dd_min = spirit_shell_heal -> base_dd_max = current_value * data().effectN( 2 ).percent();
        spirit_shell_heal->execute();
      }
    }

    absorb_buff_t::expire();
  }
};

struct spirit_shell_absorb_t : priest_absorb_t
{
  spirit_shell_absorb_t( priest_t* p, const std::string& options_str ) :
    priest_absorb_t( "spirit_shell", p, p -> find_class_spell( "Spirit Shell" ) )
  {
    parse_options( NULL, options_str );

    stats->add_child( p->get_stats( "spirit_shell_heal" ) );

    dynamic_cast<spirit_shell_buff_t*>( td( target )->buffs.spirit_shell ) ->spirit_shell_heal = p -> spells.spirit_shell;

    // Parse values from buff spell effect
    action_t::parse_effect_data( p -> find_spell( 114908 ) -> effectN( 1 ) );
  }

  virtual void impact_s( action_state_t* s )
  {
    td( s -> target ) -> buffs.spirit_shell -> trigger( 1, s -> result_amount );
    stats -> add_result( 0, s -> result_amount, ABSORB, s -> result );
  }
};

} // NAMESPACE heals

// ==========================================================================
// Priest
// ==========================================================================

priest_td_t::priest_td_t( player_t* target, priest_t* p ) :
  actor_pair_t( target, p ),
  dots( dots_t() ),
  buffs( buffs_t() ),
  remove_dots_event( NULL )
{
  if ( target -> is_enemy() )
  {
    dots.holy_fire        = target -> get_dot( "holy_fire",        p );
    dots.devouring_plague = target -> get_dot( "devouring_plague", p );
    dots.shadow_word_pain = target -> get_dot( "shadow_word_pain", p );
    dots.vampiric_touch   = target -> get_dot( "vampiric_touch",   p );

    const spell_data_t* sd = p -> find_class_spell( "Mind Spike" );

    buffs.mind_spike = buff_creator_t( *this, "mind_spike", sd )
                         .max_stack( sd -> effectN( 3 ).base_value() )
                         .duration( sd -> effectN( 2 ).trigger() -> duration() )
                         .default_value( sd -> effectN( 2 ).percent() );
  }
  else
  {
    dots.renew = target -> get_dot( "renew", p );

    buffs.power_word_shield = absorb_buff_creator_t( *this, "power_word_shield", source -> find_spell( 17 ) )
                              .source( source -> get_stats( "power_word_shield" ) );

    buffs.divine_aegis = absorb_buff_creator_t( *this, "divine_aegis", source -> find_spell( 47753 ) )
                         .source( source -> get_stats( "divine_aegis" ) );

    buffs.spirit_shell = new spirit_shell_buff_t( *this );

    target -> absorb_buffs.push_back( buffs.power_word_shield );
    target -> absorb_buffs.push_back( buffs.divine_aegis );
    target -> absorb_buffs.push_back( buffs.spirit_shell );
  }
}


// priest_t::shadowy_recall_chance ==============================================

double priest_t::shadowy_recall_chance()
{
  return mastery_spells.shadowy_recall -> effectN( 1 ).mastery_value() * composite_mastery();
}

// priest_t::primary_role ===================================================

role_e priest_t::primary_role()
{
  switch ( player_t::primary_role() )
  {
  case ROLE_HEAL:
    return ROLE_HEAL;
  case ROLE_DPS:
  case ROLE_SPELL:
    return ROLE_SPELL;
  default:
    if ( primary_tree() == PRIEST_DISCIPLINE || primary_tree() == PRIEST_HOLY )
      return ROLE_HEAL;
    break;
  }

  return ROLE_SPELL;
}

// priest_t::combat_begin ===================================================

void priest_t::combat_begin()
{
  player_t::combat_begin();

  resources.current[ RESOURCE_SHADOW_ORB ] = ( ( initial_shadow_orbs >= 0 ) && ( initial_shadow_orbs <= resources.base[ RESOURCE_SHADOW_ORB ] ) ) ? initial_shadow_orbs : 0;
}
// priest_t::composite_armor ================================================

double priest_t::composite_armor()
{
  double a = player_t::composite_armor();

  if ( buffs.inner_fire -> up() )
    a *= 1.0 + buffs.inner_fire -> data().effectN( 1 ).base_value() / 100.0 * ( 1.0 + glyphs.inner_fire -> effectN( 1 ).percent() );

  return floor( a );
}

// priest_t::composite_spell_power_multiplier ===============================

double priest_t::composite_spell_power_multiplier()
{
  double m = player_t::composite_spell_power_multiplier();

  m *= 1.0 + buffs.inner_fire -> data().effectN( 2 ).percent();

  return m;
}

// priest_t::composite_spell_hit ============================================

double priest_t::composite_spell_hit()
{
  double hit = player_t::composite_spell_hit();

  hit += ( ( spirit() - base.attribute[ ATTR_SPIRIT ] ) * specs.spiritual_precision -> effectN( 1 ).percent() ) / rating.spell_hit;

  return hit;
}

// priest_t::composite_player_multiplier ====================================

double priest_t::composite_player_multiplier( school_e school, action_t* a )
{
  double m = player_t::composite_player_multiplier( school, a );

  if ( spell_data_t::is_school( school, SCHOOL_SHADOW ) )
  {
    m *= 1.0 + buffs.shadowform -> check() * specs.shadowform -> effectN( 2 ).percent();
  }
  if ( spell_data_t::is_school( school, SCHOOL_SHADOWLIGHT ) )
  {
    if ( buffs.chakra_chastise -> up() )
    {
      m *= 1.0 + 0.15;
    }
  }

  if ( buffs.twist_of_fate -> check() )
  {
    m *= 1.0 + buffs.twist_of_fate -> value();
  }

  return m;
}

// priest_t::composite_movement_speed =======================================

double priest_t::composite_movement_speed()
{
  double speed = player_t::composite_movement_speed();

  if ( buffs.inner_will -> up() )
    speed *= 1.0 + buffs.inner_will -> data().effectN( 2 ).percent() + glyphs.inner_sanctum -> effectN( 2 ).percent();

  return speed;
}

// priest_t::composite_attribute_multiplier =================================

double priest_t::composite_attribute_multiplier( attribute_e attr )
{
  double m = player_t::composite_attribute_multiplier( attr );

  if ( attr == ATTR_INTELLECT )
    m *= 1.0 + specs.mysticism -> effectN( 1 ).percent();

  return m;
}

// priest_t::matching_gear_multiplier =======================================

double priest_t::matching_gear_multiplier( attribute_e attr )
{
  if ( attr == ATTR_INTELLECT )
    return 0.05;

  return 0.0;
}

// priest_t::create_action ==================================================

action_t* priest_t::create_action( const std::string& name,
                                   const std::string& options_str )
{
  // Misc
  if ( name == "archangel"              ) return new archangel_t             ( this, options_str );
  if ( name == "chakra"                 ) return new chakra_t                ( this, options_str );
  if ( name == "dispersion"             ) return new dispersion_t            ( this, options_str );
  if ( name == "power_word_fortitude"   ) return new fortitude_t             ( this, options_str );
  if ( name == "hymn_of_hope"           ) return new hymn_of_hope_t          ( this, options_str );
  if ( name == "inner_fire"             ) return new inner_fire_t            ( this, options_str );
  if ( name == "inner_focus"            ) return new inner_focus_t           ( this, options_str );
  if ( name == "inner_will"             ) return new inner_will_t            ( this, options_str );
  if ( name == "pain_suppression"       ) return new pain_suppression_t      ( this, options_str );
  if ( name == "power_infusion"         ) return new power_infusion_t        ( this, options_str );
  if ( name == "shadowform"             ) return new shadowform_t            ( this, options_str );
  if ( name == "vampiric_embrace"       ) return new vampiric_embrace_t      ( this, options_str );

  // Damage
  if ( name == "devouring_plague"       ) return new devouring_plague_t      ( this, options_str );
  if ( name == "holy_fire"              ) return new holy_fire_t             ( this, options_str );
  if ( name == "mind_blast"             ) return new mind_blast_t            ( this, options_str );
  if ( name == "mind_flay"              ) return new mind_flay_t             ( this, options_str );
  if ( name == "mind_spike"             ) return new mind_spike_t            ( this, options_str );
  if ( name == "mind_sear"              ) return new mind_sear_t             ( this, options_str );
  if ( name == "penance"                ) return new penance_t               ( this, options_str );
  if ( name == "shadow_word_death"      ) return new shadow_word_death_t     ( this, options_str );
  if ( name == "shadow_word_pain"       ) return new shadow_word_pain_t      ( this, options_str );
  if ( name == "smite"                  ) return new smite_t                 ( this, options_str );
  if ( ( name == "shadowfiend"          ) || ( name == "mindbender" ) )
  {
    if ( talents.mindbender -> ok() )
      return new summon_mindbender_t    ( this, options_str );
    else
      return new summon_shadowfiend_t   ( this, options_str );
  }
  if ( name == "vampiric_touch"         ) return new vampiric_touch_t        ( this, options_str );
  if ( name == "cascade_damage"         ) return new cascade_damage_t        ( this, options_str );

  // Heals
  if ( name == "binding_heal"           ) return new binding_heal_t          ( this, options_str );
  if ( name == "circle_of_healing"      ) return new circle_of_healing_t     ( this, options_str );
  if ( name == "divine_hymn"            ) return new divine_hymn_t           ( this, options_str );
  if ( name == "flash_heal"             ) return new flash_heal_t            ( this, options_str );
  if ( name == "greater_heal"           ) return new greater_heal_t          ( this, options_str );
  if ( name == "guardian_spirit"        ) return new guardian_spirit_t       ( this, options_str );
  if ( name == "heal"                   ) return new _heal_t                 ( this, options_str );
  if ( name == "holy_word"              ) return new holy_word_t             ( this, options_str );
  if ( name == "lightwell"              ) return new lightwell_t             ( this, options_str );
  if ( name == "penance_heal"           ) return new penance_heal_t          ( this, options_str );
  if ( name == "power_word_shield"      ) return new power_word_shield_t     ( this, options_str );
  if ( name == "prayer_of_healing"      ) return new prayer_of_healing_t     ( this, options_str );
  if ( name == "prayer_of_mending"      ) return new prayer_of_mending_t     ( this, options_str );
  if ( name == "renew"                  ) return new renew_t                 ( this, options_str );
  if ( name == "spirit_shell"           ) return new spirit_shell_absorb_t   ( this, options_str );
  if ( name == "cascade_heal"           ) return new cascade_heal_t        ( this, options_str );

  return player_t::create_action( name, options_str );
}

// priest_t::create_pet =====================================================

pet_t* priest_t::create_pet( const std::string& pet_name,
                             const std::string& /* pet_type */ )
{
  pet_t* p = find_pet( pet_name );

  if ( p ) return p;

  if ( pet_name == "shadowfiend" ) return new pets::shadowfiend_pet_t( sim, this );
  if ( pet_name == "mindbender"  ) return new pets::mindbender_pet_t ( sim, this );
  if ( pet_name == "lightwell"   ) return new pets::lightwell_pet_t  ( sim, this );

  return 0;
}

// priest_t::create_pets ====================================================

void priest_t::create_pets()
{
  pets.shadowfiend      = create_pet( "shadowfiend" );
  pets.mindbender       = create_pet( "mindbender"  );
  pets.lightwell        = create_pet( "lightwell"   );
}

// priest_t::init_base ======================================================

void priest_t::init_base()
{
  player_t::init_base();

  base.attack_power = 0;

  resources.base[ RESOURCE_SHADOW_ORB ] = 3;

  initial.attack_power_per_strength = 1.0;
  initial.spell_power_per_intellect = 1.0;

  diminished_kfactor    = 0.009830;
  diminished_dodge_capi = 0.006650;
  diminished_parry_capi = 0.006650;
}

// priest_t::init_gains =====================================================

void priest_t::init_gains()
{
  player_t::init_gains();

  gains.dispersion                    = get_gain( "dispersion" );
  gains.shadowfiend                   = get_gain( "shadowfiend" );
  gains.mindbender                    = get_gain( "mindbender" );
  gains.archangel                     = get_gain( "archangel" );
  gains.hymn_of_hope                  = get_gain( "hymn_of_hope" );
  gains.shadow_orb_mb                 = get_gain( "Shadow Orbs from Mind Blast" );
  gains.shadow_orb_swd                = get_gain( "Shadow Orbs from Shadow Word: Death" );
  gains.devouring_plague_health       = get_gain( "Devouring Plague Health" );
  gains.vampiric_touch_mana           = get_gain( "Vampiric Touch Mana" );
  gains.vampiric_touch_mastery_mana   = get_gain( "Vampiric Touch Mastery Mana" );
  gains.rapture                       = get_gain( "Rapture" );
}

// priest_t::init_procs. =====================================================

void priest_t::init_procs()
{
  player_t::init_procs();

  procs.mastery_extra_tick             = get_proc( "Shadowy Recall Extra Tick"          );
  procs.shadowy_apparition             = get_proc( "Shadowy Apparition Procced"         );
  procs.divine_insight_shadow          = get_proc( "Divine Insight Mind Blast CD Reset" );
  procs.surge_of_darkness              = get_proc( "FDCL Mind Spike proc"               );
  procs.mind_spike_dot_removal         = get_proc( "Mind Spike removed DoTs"            );
}

// priest_t::init_scaling ===================================================

void priest_t::init_scaling()
{
  player_t::init_scaling();

  // An Atonement Priest might be Health-capped
  scales_with[ STAT_STAMINA ] = glyphs.atonement -> ok();

  // For a Shadow Priest Spirit is the same as Hit Rating so invert it.
  if ( ( specs.spiritual_precision -> ok() ) && ( sim -> scaling -> scale_stat == STAT_SPIRIT ) )
  {
    double v = sim -> scaling -> scale_value;

    if ( ! sim -> scaling -> positive_scale_delta )
    {
      invert_scaling = 1;
      initial.attribute[ ATTR_SPIRIT ] -= v * 2;
    }
  }
}

// priest_t::init_benefits ===================================================

void priest_t::init_benefits()
{
  player_t::init_benefits();

  for ( size_t i = 0; i < 4; ++i )
    benefits.mind_spike[ i ]     = get_benefit( "Mind Spike " + util::to_string( i ) );
}

// priest_t::init_rng =======================================================

void priest_t::init_rng()
{
  player_t::init_rng();

  rngs.mastery_extra_tick  = get_rng( "shadowy_recall_extra_tick" );
  rngs.shadowy_apparitions = get_rng( "shadowy_apparitions" );
}

// priest_t::init_spells
void priest_t::init_spells()
{
  player_t::init_spells();

  // Talents
  talents.void_tendrils               = find_talent_spell( "Void Tendrils" );
  talents.psyfiend                    = find_talent_spell( "Psyfiend" );
  talents.dominate_mind               = find_talent_spell( "Dominate Mind" );
  talents.body_and_soul               = find_talent_spell( "Body and Soul" );
  talents.angelic_feather             = find_talent_spell( "Angelic Feather" );
  talents.phantasm                    = find_talent_spell( "Phantasm" );
  talents.from_darkness_comes_light   = find_talent_spell( "From Darkness, Comes Light" );
  talents.mindbender                  = find_talent_spell( "Mindbender" );
  talents.archangel                   = find_talent_spell( "Archangel" );
  talents.desperate_prayer            = find_talent_spell( "Desperate Prayer" );
  talents.void_shift                  = find_talent_spell( "Void Shift" );
  talents.angelic_bulwark             = find_talent_spell( "Angelic Bulwark" );
  talents.twist_of_fate               = find_talent_spell( "Twist of Fate" );
  talents.power_infusion              = find_talent_spell( "Power Infusion" );
  talents.divine_insight              = find_talent_spell( "Divine Insight" );
  talents.cascade                     = find_talent_spell( "Cascade" );
  talents.divine_star                 = find_talent_spell( "Divine Star" );
  talents.halo                        = find_talent_spell( "Halo" );

  // Passive Spells

  // General Spells

  // Discipline
  specs.meditation_disc                = find_specialization_spell( "Meditation", "meditation_disc", PRIEST_DISCIPLINE );
  specs.divine_aegis                   = find_specialization_spell( "Divine Aegis" );
  specs.grace                          = find_specialization_spell( "Grace" );
  specs.evangelism                     = find_specialization_spell( "Evangelism" );
  specs.train_of_thought               = find_specialization_spell( "Train of Thought" );
  specs.divine_fury                    = find_specialization_spell( "Divine Fury" );
  specs.rapture                        = find_specialization_spell( "Rapture" );
  specs.mysticism                      = find_specialization_spell( "Mysticism" );

  // Holy
  specs.meditation_holy                = find_specialization_spell( "Meditation", "meditation_holy", PRIEST_HOLY );
  specs.revelations                    = find_specialization_spell( "Revelations" );
  specs.chakra_chastise                = find_class_spell( "Chakra: Chastise" );
  specs.chakra_sanctuary               = find_class_spell( "Chakra: Sanctuary" );
  specs.chakra_serenity                = find_class_spell( "Chakra: Serenity" );

  // Shadow
  specs.mind_surge                     = find_specialization_spell( "Mind Surge" );
  specs.spiritual_precision            = find_specialization_spell( "Spiritual Precision" );
  specs.shadowform                     = find_class_spell( "Shadowform" );
  specs.shadowy_apparitions            = find_specialization_spell( "Shadowy Apparitions" );

  // Mastery Spells
  mastery_spells.shield_discipline    = find_mastery_spell( PRIEST_DISCIPLINE );
  mastery_spells.echo_of_light        = find_mastery_spell( PRIEST_HOLY );
  mastery_spells.shadowy_recall       = find_mastery_spell( PRIEST_SHADOW );

  // Glyphs
  glyphs.circle_of_healing            = find_glyph_spell( "Glyph of Circle of Healing" );
  glyphs.dispersion                   = find_glyph_spell( "Glyph of Dispersion" );
  glyphs.holy_nova                    = find_glyph_spell( "Glyph of Holy Nova" );
  glyphs.inner_fire                   = find_glyph_spell( "Glyph of Inner Fire" );
  glyphs.lightwell                    = find_glyph_spell( "Glyph of Lightwell" );
  glyphs.penance                      = find_glyph_spell( "Glyph of Penance" );
  glyphs.power_word_shield            = find_glyph_spell( "Glyph of Power Word: Shield" );
  glyphs.prayer_of_mending            = find_glyph_spell( "Glyph of Prayer of Mending" );
  glyphs.renew                        = find_glyph_spell( "Glyph of Renew" );
  glyphs.smite                        = find_glyph_spell( "Glyph of Smite" );
  glyphs.holy_fire                    = find_glyph_spell( "Glyph of Holy Fire" );
  glyphs.atonement                    = find_glyph_spell( "Atonement" );
  glyphs.dark_binding                 = find_glyph_spell( "Glyph of Dark Binding" );
  glyphs.mind_spike                   = find_glyph_spell( "Glyph of Mind Spike" );
  glyphs.strength_of_soul             = find_glyph_spell( "Glyph of Strength of Soul" );
  glyphs.inner_sanctum                = find_glyph_spell( "Glyph of Inner Sanctum" );
  glyphs.mind_flay                    = find_glyph_spell( "Glyph of Mind Flay" );
  glyphs.mind_blast                   = find_glyph_spell( "Glyph of Mind Blast" );
  glyphs.devouring_plague             = find_glyph_spell( "Glyph of Devouring Plague" );
  glyphs.vampiric_embrace             = find_glyph_spell( "Glyph of Vampiric Embrace" );
  glyphs.borrowed_time                = find_glyph_spell( "Glyph of Borrowed Time" );

  if ( mastery_spells.echo_of_light -> ok() )
    spells.echo_of_light = new echo_of_light_t( this );
  else
    spells.echo_of_light = NULL;

  spells.spirit_shell = new spirit_shell_heal_t( this );

  if ( specs.shadowy_apparitions -> ok() )
  {
    spells::add_more_shadowy_apparitions( this, specs.shadowy_apparitions -> effectN( 2 ).base_value() );
  }

  // Set Bonuses
  static const uint32_t set_bonuses[N_TIER][N_TIER_BONUS] =
  {
    //   C2P     C4P    M2P    M4P    T2P    T4P     H2P     H4P
    { 105843, 105844,     0,     0,     0,     0, 105827, 105832 }, // Tier13
    {      0,      0,     0,     0,     0,     0,      0,      0 },
  };

  sets = new set_bonus_array_t( this, set_bonuses );
}

// priest_t::init_buffs =====================================================

void priest_t::init_buffs()
{
  player_t::init_buffs();

  // buff_t( player, name, max_stack, duration, chance=-1, cd=-1, quiet=false, reverse=false, rngs.type=rngs.CYCLIC, activated=true )
  // buff_t( player, id, name, chance=-1, cd=-1, quiet=false, reverse=false, rngs.type=rngs.CYCLIC, activated=true )
  // buff_t( player, name, spellname, chance=-1, cd=-1, quiet=false, reverse=false, rngs.type=rngs.CYCLIC, activated=true )

  // Talents
  buffs.twist_of_fate                    = buff_creator_t( this, "twist_of_fate", find_talent_spell( "Twist of Fate" ) )
                                           .duration( find_talent_spell( "Twist of Fate" ) -> effectN( 1 ).trigger() -> duration() )
                                           .default_value( find_talent_spell( "Twist of Fate" ) -> effectN( 1 ).trigger() -> effectN( 2 ).percent() );

  buffs.surge_of_light = buff_creator_t( this, "surge_of_light", find_spell( 114255 ) ).chance( find_talent_spell( "From Darkness, Comes Light" )->effectN( 1 ).percent() );
  // Discipline
  buffs.holy_evangelism                  = buff_creator_t( this, 81661, "holy_evangelism" )
                                           .chance( specs.evangelism -> ok() )
                                           .activated( false );
  buffs.dark_archangel                   = buff_creator_t( this, "dark_archangel", find_spell( 87153 ) )
                                           .chance( talents.archangel -> ok() )
                                           .default_value( find_spell( 87153 ) -> effect1().percent() );
  buffs.holy_archangel                   = buff_creator_t( this, "holy_archangel", find_spell( 81700 ) )
                                           .chance( talents.archangel -> ok() )
                                           .default_value( find_spell( 81700 ) -> effect1().percent() );
  buffs.inner_fire                       = buff_creator_t( this, "inner_fire", find_class_spell( "Inner Fire" ) );
  buffs.inner_focus                      = buff_creator_t( this, "inner_focus", find_class_spell( "Inner Focus" ) )
                                           .cd( timespan_t::zero() );
  buffs.inner_will                       = buff_creator_t( this, "inner_will", find_class_spell( "Inner Will" ) );
  // Holy
  buffs.chakra_pre                       = buff_creator_t( this, "chakra_pre", find_spell( 14751 ) );
  buffs.chakra_chastise                  = buff_creator_t( this, "chakra_chastise", find_spell( 81209 ) );
  buffs.chakra_sanctuary                 = buff_creator_t( this, "chakra_sanctuary", find_spell( 81206 ) );
  buffs.chakra_serenity                  = buff_creator_t( this, "chakra_serenity", find_spell( 81208 ) );
  buffs.serenity                         = buff_creator_t( this, "serenity", find_spell( 88684 ) )
                                           .cd( timespan_t::zero() )
                                           .activated( false );

  // Shadow
  const spell_data_t* divine_insight_shadow = ( talents.divine_insight -> ok() && ( primary_tree() == PRIEST_SHADOW ) ) ? talents.divine_insight -> effectN( 2 ).trigger() : spell_data_t::not_found();
  buffs.divine_insight_shadow            = buff_creator_t( this, "divine_insight_shadow", divine_insight_shadow )
                                           .chance( talents.divine_insight -> proc_chance() );
  buffs.shadowform                       = buff_creator_t( this, "shadowform", find_class_spell( "Shadowform" ) );
  buffs.vampiric_embrace                 = buff_creator_t( this, "vampiric_embrace", find_class_spell( "Vampiric Embrace" ) )
                                           .duration( find_class_spell( "Vampiric Embrace" ) -> duration() + glyphs.vampiric_embrace -> effectN( 1 ).time_value() );
  buffs.glyph_mind_spike                 = buff_creator_t( this, "glyph_mind_spike", glyphs.mind_spike ).max_stack( 2 ).duration( timespan_t::from_seconds( 6.0 ) );

  buffs.shadow_word_death_reset_cooldown = buff_creator_t( this, "shadow_word_death_reset_cooldown" )
                                           .max_stack( 1 ).duration( timespan_t::from_seconds( 6.0 ) );

  const spell_data_t* surge_of_darkness  = talents.from_darkness_comes_light -> ok() ? find_spell( 87160 ) : spell_data_t::not_found();
  buffs.surge_of_darkness                = buff_creator_t( this, "surge_of_darkness", surge_of_darkness )
                                           .chance( 0.15 );

  buffs.consume_surge_of_darkness        = buff_creator_t( this, "consume_surge_of_darkness" ).quiet( true );
  buffs.consume_divine_insight_shadow    = buff_creator_t( this, "consume_divine_insight_shadow" ).quiet( true );

  // Set Bonus
}

void priest_t::add_action( std::string action, std::string options, std::string alist )
{
  add_action( find_talent_spell( action ) -> ok() ? find_talent_spell( action ) : find_class_spell( action ), options, alist );
}

void priest_t::add_action( const spell_data_t* s, std::string options, std::string alist )
{
  std::string *str = ( alist == "default" ) ? &action_list_str : &( get_action_priority_list( alist ) -> action_list_str );
  if ( s -> ok() )
  {
    *str += "/" + dbc_t::get_token( s -> id() );
    if ( ! options.empty() ) *str += "," + options;
  }
}

// priest_t::init_actions ===================================================

void priest_t::init_actions()
{
  if ( primary_tree() != PRIEST_DISCIPLINE && primary_tree() != PRIEST_SHADOW )
  {
    if ( ! quiet )
      sim -> errorf( "Player %s's role or spec isn't supported yet.", name() );
    quiet = true;
    return;
  }

  if ( action_list_str.empty() )
  {
    clear_action_priority_lists();

    std::string& precombat_list = get_action_priority_list( "precombat" ) -> action_list_str;

    // Flask
    if ( level > 85 )
      precombat_list += "/flask,type=warm_sun";
    else if ( level >= 80 )
      precombat_list += "/flask,type=draconic_mind";

    if ( level > 85 )
    {
      precombat_list += "/food,type=great_pandaren_banquet";
    }
    else if ( level >= 80 )
    {
      precombat_list += "/food,type=seafood_magnifique_feast";
    }

    add_action( "Power Word: Fortitude", "if=!aura.stamina.up", "precombat" );
    add_action( "Inner Fire", "", "precombat" );
    add_action( "Shadowform", "", "precombat" );

    precombat_list += "/snapshot_stats";

    if ( level > 85 )
    {
      precombat_list += "/jinyu_potion";
    }
    else if ( level >= 80 )
    {
      precombat_list += "/volcanic_potion";
    }

    // End precombat list

    add_action( "Shadowform" );
//    add_action( "Vampiric Embrace" );

    action_list_str += init_use_item_actions();

    action_list_str += init_use_profession_actions();

    // ======================================================================

    switch ( primary_tree() )
    {
      // SHADOW =============================================================
    case PRIEST_SHADOW:
      if ( level > 85 )
      {
        action_list_str += "/jinyu_potion,if=buff.bloodlust.react|target.time_to_die<=40";
      }
      else if ( level > 80 )
      {
        action_list_str += "/volcanic_potion,if=buff.bloodlust.react|target.time_to_die<=40";
      }


      add_action( "Devouring Plague", "if=shadow_orb=3&(!ticking|remains<tick_time)" );

      add_action( "Archangel" );

      if ( set_bonus.tier13_2pc_caster() )
      {
        add_action( "Shadow Word: Death", "if=target.health.pct<20" );
      }

      action_list_str += init_use_racial_actions();

      add_action( "Mind Blast", "if=cooldown_react" );

      if ( find_talent_spell( "From Darkness Comes Light" ) -> ok() )
        add_action( "Mind Spike", "if=buff.surge_of_darkness.react" );

      add_action( "Power Infusion" );

      add_action( "Shadow Word: Pain", "cycle_targets=1,max_cycle_targets=5,if=(!ticking|remains<tick_time)&miss_react" );

      if ( ! set_bonus.tier13_2pc_caster() )
      {
        add_action( "Shadow Word: Death", "if=target.health.pct<20" );
      }

      add_action( "Vampiric Touch", "cycle_targets=1,max_cycle_targets=5,if=(!ticking|remains<cast_time+tick_time)&miss_react" );

      if ( find_talent_spell( "Mindbender" ) -> ok() )
        action_list_str += "/mindbender,if=cooldown_react";
      else if ( find_class_spell( "Shadowfiend" ) -> ok() )
        action_list_str += "/shadowfiend,if=cooldown_react";

      add_action( "Mind Sear", "chain=1,interrupt=1,if=num_targets>=5" );

      add_action( "Mind Flay", "chain=1,interrupt=1" );

      add_action( "Shadow Word: Death", "moving=1" );

      add_action( "Mind Blast", "moving=1,if=buff.divine_insight_shadow.react&cooldown_react" );

      add_action( "Shadow Word: Pain", "moving=1" );

      add_action( "Dispersion" );

      break;
      // SHADOW END =========================================================


      // DISCIPLINE =========================================================
    case PRIEST_DISCIPLINE:

      // DAMAGE DISCIPLINE ==================================================
      if ( primary_role() != ROLE_HEAL )
      {
        if ( level > 85 )
        {
          action_list_str += "/jinyu_potion,if=buff.bloodlust.react|target.time_to_die<=40";
        }
        else if ( level > 80 )
        {
          action_list_str += "/volcanic_potion,if=buff.bloodlust.react|target.time_to_die<=40";
        }
        if ( race == RACE_BLOOD_ELF )
          action_list_str += "/arcane_torrent,if=mana.pct<=90";
        if ( find_talent_spell( "Mindbender" ) -> ok() )
          action_list_str += "/mindbender,if=cooldown_react";
        else if ( find_class_spell( "Shadowfiend" ) -> ok() )
          action_list_str += "/shadowfiend,if=cooldown_react";
        if ( level >= 64 )
          action_list_str += "/hymn_of_hope";
        if ( level >= 66 )
          action_list_str += ",if=(pet.shadowfiend.active|pet.shadowfiend.active)&mana.pct<=20";
        if ( race == RACE_TROLL )
          action_list_str += "/berserking";
        action_list_str += "/power_infusion";
        action_list_str += "/power_word_shield,if=buff.weakened_soul.down";

        action_list_str += "/shadow_word_pain,if=miss_react&(remains<tick_time|!ticking)";

        action_list_str += "/holy_fire";
        action_list_str += "/penance";
        action_list_str += "/mind_blast";

        action_list_str += "/smite";
      }
      // DAMAGE DISCIPLINE END ==============================================

      // HEALER DISCIPLINE ==================================================
      else
      {
        // DEFAULT
        action_list_str += "/mana_potion,if=mana.pct<=75";
        if ( race == RACE_BLOOD_ELF )
          action_list_str  += "/arcane_torrent,if=mana.pct<=90";
        if ( level >= 66 )
          action_list_str += "/shadowfiend,if=mana.pct<=20";
        if ( level >= 64 )
          action_list_str += "/hymn_of_hope";
        if ( level >= 66 )
          action_list_str += ",if=pet.shadowfiend.active";
        if ( race == RACE_TROLL )
          action_list_str += "/berserking";
        action_list_str += "/inner_focus";
        action_list_str += "/power_infusion";
        action_list_str += "/power_word_shield";
        action_list_str += "/greater_heal,if=buff.inner_focus.up";

        action_list_str += "/holy_fire";
        if ( glyphs.atonement -> ok() )
        {
          action_list_str += "/smite,if=";
          if ( glyphs.smite -> ok() )
            action_list_str += "dot.holy_fire.remains>cast_time&";
          action_list_str += "buff.holy_evangelism.stack<5&buff.holy_archangel.down";
        }
        action_list_str += "/penance_heal";
        //action_list_str += "/greater_heal";
        action_list_str += "/heal";
        // DEFAULT END

/*
        // PWS
        list_pws += "/mana_potion,if=mana_pct<=75";
        if ( race == RACE_BLOOD_ELF )
          list_pws  += "/arcane_torrent,if=mana_pct<=90";
        if ( level >= 66 )
          list_pws += "/shadowfiend,if=mana_pct<=20";
        if ( level >= 64 )
          list_pws += "/hymn_of_hope";
        if ( level >= 66 )
          list_pws += ",if=pet.shadowfiend.active";
        if ( race == RACE_TROLL )
          list_pws += "/berserking";
        list_pws += "/inner_focus";
        list_pws += "/power_infusion";

        list_pws += "/power_word_shield,ignore_debuff=1";
*/
        // PWS END
      }
      break;
      // HEALER DISCIPLINE END ==============================================

      // HOLY
    case PRIEST_HOLY:
      // DAMAGE DEALER
      if ( primary_role() != ROLE_HEAL )
      {
                                                         action_list_str += "/mana_potion,if=mana_pct<=75";
        if ( race == RACE_BLOOD_ELF )                    action_list_str += "/arcane_torrent,if=mana_pct<=90";
        if ( level >= 66 )                               action_list_str += "/shadowfiend,if=mana_pct<=50";
        if ( level >= 64 )                               action_list_str += "/hymn_of_hope";
        if ( level >= 66 )                               action_list_str += ",if=pet.shadowfiend.active&time>200";
        if ( race == RACE_TROLL )                        action_list_str += "/berserking";
                                                         action_list_str += "/chakra";
                                                         action_list_str += "/holy_fire";
                                                         action_list_str += "/shadow_word_pain,if=remains<tick_time|!ticking";
                                                         action_list_str += "/mind_blast";
                                                         action_list_str += "/smite";
      }
      // HEALER
      else
      {
                                                         action_list_str += "/mana_potion,if=mana_pct<=75";
        if ( race == RACE_BLOOD_ELF )                    action_list_str += "/arcane_torrent,if=mana_pct<80";
        if ( level >= 66 )                               action_list_str += "/shadowfiend,if=mana_pct<=20";
        if ( level >= 64 )                               action_list_str += "/hymn_of_hope";
        if ( level >= 66 )                               action_list_str += ",if=pet.shadowfiend.active";
        if ( race == RACE_TROLL )                        action_list_str += "/berserking";
      }
      break;
    default:
                                                         action_list_str += "/mana_potion,if=mana_pct<=75";
      if ( level >= 66 )                                 action_list_str += "/shadowfiend,if=mana_pct<=50";
      if ( level >= 64 )                                 action_list_str += "/hymn_of_hope";
      if ( level >= 66 )                                 action_list_str += ",if=pet.shadowfiend.active&time>200";
      if ( race == RACE_TROLL )                          action_list_str += "/berserking";
      if ( race == RACE_BLOOD_ELF )                      action_list_str += "/arcane_torrent,if=mana_pct<=90";
                                                         action_list_str += "/holy_fire";
                                                         action_list_str += "/shadow_word_pain,if=remains<tick_time|!ticking";
                                                         action_list_str += "/smite";
      break;
    }
  }

  player_t::init_actions();
}

// priest_t::init_party =====================================================

void priest_t::init_party()
{
  party_list.clear();

  if ( party == 0 )
    return;

  for ( player_t* p = sim -> player_list; p; p = p -> next )
  {
    if ( ( p != this ) && ( ! p -> quiet ) && ( ! p -> is_pet() ) )
    {
      party_list.push_back( p );
    }
  }
}

// priest_t::init_values ====================================================

void priest_t::init_values()
{
  player_t::init_values();

  // Discipline/Holy
  constants.meditation_value                = specs.meditation_disc -> ok() ?
                                              specs.meditation_disc -> effectN( 1 ).percent() :
                                              specs.meditation_holy -> effectN( 1 ).percent();

  initial.mp5_from_spirit_multiplier = constants.meditation_value;

  // Shadow
  if ( set_bonus.pvp_2pc_caster() )
    initial.attribute[ ATTR_INTELLECT ] += 70;

  if ( set_bonus.pvp_4pc_caster() )
    initial.attribute[ ATTR_INTELLECT ] += 90;

  if ( set_bonus.pvp_2pc_heal() )
    initial.attribute[ ATTR_INTELLECT ] += 70;

  if ( set_bonus.pvp_4pc_heal() )
    initial.attribute[ ATTR_INTELLECT ] += 90;
}

// priest_t::reset ==========================================================

void priest_t::reset()
{
  player_t::reset();

  for ( size_t i=0; i < sim -> actor_list.size(); i++ )
  {
    priest_td_t* td = target_data[ sim -> actor_list[ i ] ];
    if ( td ) td -> reset();
  }

  if ( specs.shadowy_apparitions -> ok() )
  {
    while ( spells.apparitions_active.size() )
    {
      spell_t* s = spells.apparitions_active.front();

      spells.apparitions_active.pop_front();

      spells.apparitions_free.push( s );
    }
  }

  spells.echo_of_light_merged = false;

  init_party();
}

// priest_t::fixup_atonement_stats  =========================================

void priest_t::fixup_atonement_stats( const std::string& trigger_spell_name,
                                      const std::string& atonement_spell_name )
{
  if ( stats_t* trigger = find_stats( trigger_spell_name ) )
  {
    if ( stats_t* atonement = get_stats( atonement_spell_name ) )
    {
      // Copy stats from the trigger spell to the atonement spell
      // to get proper HPR and HPET reports.
      atonement -> resource_gain.merge( trigger->resource_gain );
      atonement -> total_execute_time = trigger -> total_execute_time;
      atonement -> total_tick_time = trigger -> total_tick_time;
    }
  }
}

// priest_t::pre_analyze_hook  ==============================================

void priest_t::pre_analyze_hook()
{
  if ( glyphs.atonement -> ok() )
  {
    fixup_atonement_stats( "smite", "atonement_smite" );
    fixup_atonement_stats( "holy_fire", "atonement_holy_fire" );
  }
}

// priest_t::target_mitigation ==============================================

double priest_t::target_mitigation( double        amount,
                                    school_e school,
                                    dmg_e    dt,
                                    result_e result,
                                    action_t*     action )
{
  amount = player_t::target_mitigation( amount, school, dt, result, action );

  if ( buffs.shadowform -> check() )
  { amount *= 1.0 + buffs.shadowform -> data().effectN( 3 ).percent(); }

  if ( buffs.inner_fire -> check() && glyphs.inner_sanctum -> ok() && ( spell_data_t::get_school_mask( school ) & SCHOOL_MAGIC_MASK ) )
  { amount *= 1.0 - glyphs.inner_sanctum -> effectN( 1 ).percent(); }

  return amount;
}

// priest_t::create_options =================================================

void priest_t::create_options()
{
  player_t::create_options();

  option_t priest_options[] =
  {
    { "atonement_target",        OPT_STRING,            &( atonement_target_str     ) },
    { "double_dot",              OPT_DEPRECATED, ( void* ) "action_list=double_dot"   },
    { "initial_shadow_orbs",     OPT_INT,               &( initial_shadow_orbs      ) },
    { NULL, OPT_UNKNOWN, NULL }
  };

  option_t::copy( options, priest_options );
}

// priest_t::create_profile =================================================

bool priest_t::create_profile( std::string& profile_str, save_e type, bool save_html )
{
  player_t::create_profile( profile_str, type, save_html );

  if ( type == SAVE_ALL )
  {
    if ( ! atonement_target_str.empty() )
      profile_str += "atonement_target=" + atonement_target_str + "\n";

    if ( initial_shadow_orbs != 0 )
    {
      profile_str += "initial_shadow_orbs=";
      profile_str += util::to_string( initial_shadow_orbs );
      profile_str += "\n";
    }
  }

  return true;
}

// priest_t::copy_from ======================================================

void priest_t::copy_from( player_t* source )
{
  player_t::copy_from( source );

  priest_t* source_p = debug_cast<priest_t*>( source );

  atonement_target_str = source_p -> atonement_target_str;
}

// priest_t::decode_set =====================================================

int priest_t::decode_set( item_t& item )
{
  if ( item.slot != SLOT_HEAD      &&
       item.slot != SLOT_SHOULDERS &&
       item.slot != SLOT_CHEST     &&
       item.slot != SLOT_HANDS     &&
       item.slot != SLOT_LEGS      )
  {
    return SET_NONE;
  }

  const char* s = item.name();

  bool is_caster = false;
  bool is_healer = false;

  if ( strstr( s, "_of_dying_light" ) )
  {
    is_caster = ( strstr( s, "hood"          ) ||
                  strstr( s, "shoulderwraps" ) ||
                  strstr( s, "vestment"      ) ||
                  strstr( s, "gloves"        ) ||
                  strstr( s, "leggings"      ) );

    /* FIX-ME: Kludge due to caster shoulders and chests have the wrong name */
    const char* t = item.encoded_stats_str.c_str();

    if ( ( ( item.slot == SLOT_SHOULDERS ) && strstr( t, "crit"  ) ) ||
         ( ( item.slot == SLOT_CHEST     ) && strstr( t, "haste" ) ) )
    {
      is_caster = 1;
    }

    if ( is_caster ) return SET_T13_CASTER;

    is_healer = ( strstr( s, "cowl"          ) ||
                  strstr( s, "mantle"        ) ||
                  strstr( s, "robes"         ) ||
                  strstr( s, "handwraps"     ) ||
                  strstr( s, "legwraps"      ) );
    if ( is_healer ) return SET_T13_HEAL;
  }

  if ( strstr( s, "_gladiators_mooncloth_" ) ) return SET_PVP_HEAL;
  if ( strstr( s, "_gladiators_satin_"     ) ) return SET_PVP_CASTER;

  return SET_NONE;
}

std::string priest_t::set_default_talents()
{
  switch ( primary_tree() )
  {
  case PRIEST_SHADOW: return "002030";
  default: break;
  }

  return player_t::set_default_talents();
}

std::string priest_t::set_default_glyphs()
{
  switch ( primary_tree() )
  {
  case PRIEST_SHADOW: if ( talent_list[ 2 * MAX_TALENT_COLS + 0 ] ) return "mind_spike"; break;
  case SPEC_NONE: break;
  default: break;
  }

  return player_t::set_default_glyphs();
}

// PRIEST MODULE INTERFACE ================================================

struct priest_module_t : public module_t
{
  priest_module_t() : module_t( PRIEST ) {}

  virtual player_t* create_player( sim_t* sim, const std::string& name, race_e r = RACE_NONE )
  {
    return new priest_t( sim, name, r );
  }
  virtual bool valid() { return true; }
  virtual void init( sim_t* sim )
  {
    for ( size_t i = 0; i < sim -> actor_list.size(); i++ )
    {
      player_t* p = sim -> actor_list[ i ];
      p -> buffs.guardian_spirit  = buff_creator_t( p, "guardian_spirit", p -> find_spell( 47788 ) ); // Let the ability handle the CD
      p -> buffs.pain_supression  = buff_creator_t( p, "pain_supression", p -> find_spell( 33206 ) ); // Let the ability handle the CD
      p -> buffs.power_infusion   = buff_creator_t( p, "power_infusion",  p -> find_spell( 10060 ) ).max_stack( 1 ).duration( timespan_t::from_seconds( 15.0 ) );
      p -> buffs.weakened_soul    = buff_creator_t( p, "weakened_soul",   p -> find_spell(  6788 ) );
    }
  }
  virtual void combat_begin( sim_t* ) {}
  virtual void combat_end( sim_t* ) {}
};

} // ANONYMOUS NAMESPACE

module_t* module_t::priest()
{
  static module_t* m = 0;
  if ( ! m ) m = new priest_module_t();
  return m;
}
