// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"
#include "sc_class_modules.hpp"

namespace {

struct death_knight_t;

#if SC_DEATH_KNIGHT == 1

struct death_knight_td_t : public actor_pair_t
{
  dot_t* dots_blood_plague;
  dot_t* dots_death_and_decay;
  dot_t* dots_frost_fever;

  int diseases()
  {
    int disease_count = 0;
    if ( dots_blood_plague -> ticking ) disease_count++;
    if ( dots_frost_fever  -> ticking ) disease_count++;
    return disease_count;
  }

  death_knight_td_t( player_t* target, player_t* death_knight ) :
    actor_pair_t( target, death_knight )
  {
    dots_blood_plague    = target -> get_dot( "blood_plague",    death_knight );
    dots_death_and_decay = target -> get_dot( "death_and_decay", death_knight );
    dots_frost_fever     = target -> get_dot( "frost_fever",     death_knight );
  }
};

struct dancing_rune_weapon_pet_t;

// ==========================================================================
// Death Knight Runes
// ==========================================================================

enum rune_type
{
  RUNE_TYPE_NONE=0, RUNE_TYPE_BLOOD, RUNE_TYPE_FROST, RUNE_TYPE_UNHOLY, RUNE_TYPE_DEATH, RUNE_TYPE_WASDEATH=8
};

const char *rune_symbols = "!bfu!!";

#define RUNE_TYPE_MASK     3
#define RUNE_SLOT_MAX      6

#define RUNIC_POWER_REFUND  0.9

// These macros simplify using the result of count_runes(), which
// returns a number of the form 0x000AABBCC where AA is the number of
// Unholy runes, BB is the number of Frost runes, and CC is the number
// of Blood runes.
#define GET_BLOOD_RUNE_COUNT(x)  ((x >>  0) & 0xff)
#define GET_FROST_RUNE_COUNT(x)  ((x >>  8) & 0xff)
#define GET_UNHOLY_RUNE_COUNT(x) ((x >> 16) & 0xff)

enum rune_state { STATE_DEPLETED, STATE_REGENERATING, STATE_FULL };

struct dk_rune_t
{
  int        type;
  rune_state state;
  double     value;   // 0.0 to 1.0, with 1.0 being full
  int        slot_number;
  bool       permanent_death_rune;
  dk_rune_t* paired_rune;

  dk_rune_t() : type( RUNE_TYPE_NONE ), state( STATE_FULL ), value( 0.0 ), permanent_death_rune( false ), paired_rune( NULL ) {}

  bool is_death()        { return ( type & RUNE_TYPE_DEATH ) != 0                ; }
  bool is_blood()        { return ( type & RUNE_TYPE_MASK  ) == RUNE_TYPE_BLOOD  ; }
  bool is_unholy()       { return ( type & RUNE_TYPE_MASK  ) == RUNE_TYPE_UNHOLY ; }
  bool is_frost()        { return ( type & RUNE_TYPE_MASK  ) == RUNE_TYPE_FROST  ; }
  bool is_ready()        { return state == STATE_FULL                            ; }
  bool is_depleted()     { return state == STATE_DEPLETED                        ; }
  bool is_regenerating() { return state == STATE_REGENERATING                    ; }
  int  get_type()        { return type & RUNE_TYPE_MASK                          ; }

  void regen_rune( death_knight_t* p, timespan_t periodicity );

  void make_permanent_death_rune()
  {
    permanent_death_rune = true;
    type |= RUNE_TYPE_DEATH;
  }

  void consume( bool convert )
  {
    assert ( value >= 1.0 );
    if ( permanent_death_rune )
    {
      type |= RUNE_TYPE_DEATH;
    }
    else
    {
      type = ( type & RUNE_TYPE_MASK ) | ( ( type << 1 ) & RUNE_TYPE_WASDEATH ) | ( convert ? RUNE_TYPE_DEATH : 0 );
    }
    value = 0.0;
    state = STATE_DEPLETED;
  }

  void fill_rune()
  {
    value = 1.0;
    state = STATE_FULL;
  }

  void reset()
  {
    value = 1.0;
    state = STATE_FULL;
    type = type & RUNE_TYPE_MASK;
    if ( permanent_death_rune )
    {
      type |= RUNE_TYPE_DEATH;
    }
  }
};

// ==========================================================================
// Death Knight
// ==========================================================================

enum death_knight_presence { PRESENCE_BLOOD=1, PRESENCE_FROST, PRESENCE_UNHOLY=4 };

struct death_knight_t : public player_t
{
  // Active
  int       active_presence;

  // Buffs
  struct buffs_t
  {
    buff_t* blood_presence;
    buff_t* bloodworms;
    buff_t* bone_shield;
    buff_t* crimson_scourge;
    buff_t* dancing_rune_weapon;
    buff_t* dark_transformation;
    buff_t* frost_presence;
    buff_t* killing_machine;
    buff_t* pillar_of_frost;
    buff_t* rime;
    buff_t* rune_of_cinderglacier;
    buff_t* rune_of_razorice;
    buff_t* rune_of_the_fallen_crusader;
    buff_t* rune_strike;
    buff_t* runic_corruption;
    buff_t* scent_of_blood;
    buff_t* shadow_infusion;
    buff_t* sudden_doom;
    buff_t* tier13_4pc_melee;
    buff_t* unholy_presence;
  } buffs;

  // Cooldowns
  struct cooldowns_t
  {
    cooldown_t* howling_blast;
  } cooldowns;

  // Diseases

  struct active_spells_t
  {
    action_t* blood_caked_blade;
    action_t* unholy_blight;
    spell_t* blood_plague;
    spell_t* frost_fever;
  } active_spells;

  // Gains
  struct gains_t
  {
    gain_t* butchery;
    gain_t* chill_of_the_grave;
    gain_t* frost_presence;
    gain_t* horn_of_winter;
    gain_t* improved_frost_presence;
    gain_t* might_of_the_frozen_wastes;
    gain_t* power_refund;
    gain_t* scent_of_blood;
    gain_t* rune;
    gain_t* rune_unholy;
    gain_t* rune_blood;
    gain_t* rune_frost;
    gain_t* rune_unknown;
    gain_t* runic_empowerment;
    gain_t* runic_empowerment_blood;
    gain_t* runic_empowerment_unholy;
    gain_t* runic_empowerment_frost;
    gain_t* empower_rune_weapon;
    gain_t* blood_tap;
    // only useful if the blood rune charts are enabled
    // charts are currently disabled so commenting out
    // gain_t* gains.blood_tap_blood;
  } gains;

  // Glyphs
  struct glyphs_t
  {

  } glyphs;

  // Options
  std::string unholy_frenzy_target_str;

  // Spells
  struct spells_t
  {
    // Passives
    const spell_data_t* brittle_bones;
    const spell_data_t* ebon_plaguebringer;


    const spell_data_t* blood_of_the_north;
    const spell_data_t* blood_rites;
    const spell_data_t* icy_talons;
    const spell_data_t* master_of_ghouls;
    const spell_data_t* plate_specialization;
    const spell_data_t* reaping;
    const spell_data_t* runic_empowerment;
    const spell_data_t* unholy_might;
    const spell_data_t* veteran_of_the_third_war;

    // Masteries
    const spell_data_t* dreadblade;
    const spell_data_t* frozen_heart;
  } spells;

  // Pets and Guardians
  struct pets_t
  {
    pet_t* army_ghoul;
    pet_t* bloodworms;
    dancing_rune_weapon_pet_t* dancing_rune_weapon;
    pet_t* ghoul;
    pet_t* gargoyle;
  } pets;

  // Procs
  struct procs_t
  {
    proc_t* runic_empowerment;
    proc_t* runic_empowerment_wasted;
    proc_t* oblit_killing_machine;
    proc_t* fs_killing_machine;
  } procs;

  // RNGs
  struct rngs_t
  {
    rng_t* blood_caked_blade;
    rng_t* might_of_the_frozen_wastes;
    rng_t* threat_of_thassarian;
  } rngs;

  // Runes
  struct runes_t
  {
    std::array<dk_rune_t,RUNE_SLOT_MAX> slot;

    runes_t()
    {
      // 6 runes, paired blood, frost and unholy
      slot[0].type = slot[1].type = RUNE_TYPE_BLOOD;
      slot[2].type = slot[3].type = RUNE_TYPE_FROST;
      slot[4].type = slot[5].type = RUNE_TYPE_UNHOLY;
      // each rune pair is paired with each other
      slot[0].paired_rune = &slot[ 1 ]; slot[ 1 ].paired_rune = &slot[ 0 ];
      slot[2].paired_rune = &slot[ 3 ]; slot[ 3 ].paired_rune = &slot[ 2 ];
      slot[4].paired_rune = &slot[ 5 ]; slot[ 5 ].paired_rune = &slot[ 4 ];
      // give each rune a slot number
      for ( size_t i = 0; i < slot.size(); ++i ) { slot[ i ].slot_number = i; }
    }
    void reset() { for ( size_t i = 0; i < slot.size(); ++i ) slot[ i ].reset(); }
  } _runes;

  // Talents
  struct talents_t
  {
    spell_data_t* unholy_blight;
  } talents;

  // Uptimes
  struct benefits_t
  {
    benefit_t* rp_cap;
  } benefits;

  target_specific_t<death_knight_td_t> target_data;

  death_knight_t( sim_t* sim, const std::string& name, race_type_e r = RACE_NIGHT_ELF ) :
    player_t( sim, DEATH_KNIGHT, name, r ),
    active_presence(),
    buffs( buffs_t() ),
    cooldowns( cooldowns_t() ),
    active_spells( active_spells_t() ),
    gains( gains_t() ),
    glyphs( glyphs_t() ),
    spells( spells_t() ),
    pets( pets_t() ),
    procs( procs_t() ),
    rngs( rngs_t() ),
    _runes( runes_t() ),
    talents( talents_t() ),
    benefits( benefits_t() ),
    target_data( "target_data", this )
  {
    cooldowns.howling_blast = get_cooldown( "howling_blast" );

    create_options();

    initial.distance = 0;
  }

  // Character Definition
  virtual void      init();
  virtual void      init_spells();
  virtual void      init_actions();
  virtual void      init_enchant();
  virtual void      init_rng();
  virtual void      init_defense();
  virtual void      init_base();
  virtual void      init_scaling();
  virtual void      init_buffs();
  virtual void      init_values();
  virtual void      init_gains();
  virtual void      init_procs();
  virtual void      init_resources( bool force );
  virtual void      init_benefits();
  virtual double    composite_armor_multiplier();
  virtual double    composite_attack_haste();
  virtual double    composite_attack_hit();
  virtual double    composite_attribute_multiplier( attribute_type_e attr );
  virtual double    matching_gear_multiplier( attribute_type_e attr );
  virtual double    composite_spell_hit();
  virtual double    composite_tank_parry();
  virtual double    composite_player_multiplier( school_type_e school, action_t* a = NULL );
  virtual double    composite_tank_crit( school_type_e school );
  virtual void      regen( timespan_t periodicity );
  virtual void      reset();
  virtual void      arise();
  virtual double    assess_damage( double amount, school_type_e school, dmg_type_e, result_type_e, action_t* a );
  virtual void      combat_begin();
  virtual void      create_options();
  virtual action_t* create_action( const std::string& name, const std::string& options );
  virtual expr_t*   create_expression( action_t*, const std::string& name );
  virtual pet_t*    create_pet( const std::string& name, const std::string& type = std::string() );
  virtual void      create_pets();
  virtual int       decode_set( item_t& );
  virtual resource_type_e primary_resource() { return RESOURCE_RUNIC_POWER; }
  virtual role_type_e primary_role();
  virtual void      trigger_runic_empowerment();
  virtual int       runes_count( rune_type rt, bool include_death, int position );
  virtual double    runes_cooldown_any( rune_type rt, bool include_death, int position );
  virtual double    runes_cooldown_all( rune_type rt, bool include_death, int position );
  virtual double    runes_cooldown_time( dk_rune_t* r );
  virtual bool      runes_depleted( rune_type rt, int position );

  death_knight_td_t* get_target_data( player_t* target )
  { 
    death_knight_td_t*& td = target_data[ target ];
    if( ! td ) td = new death_knight_td_t( target, this );
    return td;
  }

  void reset_gcd()
  {
    for ( size_t i = 0; i < action_list.size(); ++i )
    {
      action_t* a = action_list[ i ];
      if ( a -> trigger_gcd != timespan_t::zero() ) a -> trigger_gcd = base_gcd;
    }
  }

  // Temporary
  virtual std::string set_default_talents()
  {
    switch ( primary_tree() )
    {
    case SPEC_NONE: break;
    default: break;    
    }

    return player_t::set_default_talents();
  }

  virtual std::string set_default_glyphs()
  {
    switch ( primary_tree() )
    {
    case SPEC_NONE: break;
    default: break;
    }

    return player_t::set_default_glyphs();
  }
};

static void log_rune_status( death_knight_t* p )
{
  std::string rune_str;
  for ( int j = 0; j < RUNE_SLOT_MAX; ++j )
  {
    char rune_letter = rune_symbols[p -> _runes.slot[j].get_type()];
    if ( p -> _runes.slot[j].is_death() )
      rune_letter = 'd';

    if ( p -> _runes.slot[j].is_ready() )
      rune_letter = toupper( rune_letter );
    rune_str += rune_letter;
  }
  log_t::output( p -> sim, "%s runes: %s", p -> name(), rune_str.c_str() );
}

void dk_rune_t::regen_rune( death_knight_t* p, timespan_t periodicity )
{
  // If the other rune is already regening, we don't
  // but if both are full we still continue on to record resource gain overflow
  if ( state == STATE_DEPLETED &&   paired_rune -> state == STATE_REGENERATING ) return;
  if ( state == STATE_FULL     && ! ( paired_rune -> state == STATE_FULL )     ) return;

  // Base rune regen rate is 10 seconds; we want the per-second regen
  // rate, so divide by 10.0.  Haste is a multiplier (so 30% haste
  // means composite_attack_haste is 1/1.3), so we invert it.  Haste
  // linearly scales regen rate -- 100% haste means a rune regens in 5
  // seconds, etc.
  double runes_per_second = 1.0 / 10.0 / p -> composite_attack_haste();

  death_knight_t* o = p;

  double regen_amount = periodicity.total_seconds() * runes_per_second;

  // record rune gains and overflow
  gain_t* gains_rune      = o -> gains.rune         ;
  gain_t* gains_rune_type =
    is_frost()            ? o -> gains.rune_frost   :
    is_blood()            ? o -> gains.rune_blood   :
    is_unholy()           ? o -> gains.rune_unholy  :
                            o -> gains.rune_unknown ; // should never happen, so if you've seen this in a report happy bug hunting

  // full runes don't regen. if both full, record half of overflow, as the other rune will record the other half
  if ( state == STATE_FULL )
  {
    if ( paired_rune -> state == STATE_FULL )
    {
      // FIXME: Resource type?
      gains_rune_type -> add( RESOURCE_NONE, 0, regen_amount * 0.5 );
      gains_rune      -> add( RESOURCE_NONE, 0, regen_amount * 0.5 );
    }
    return;
  }

  // Chances are, we will overflow by a small amount.  Toss extra
  // overflow into our paired rune if it is regenerating or depleted.
  value += regen_amount;
  double overflow = 0.0;
  if ( value > 1.0 )
  {
    overflow = value - 1.0;
    value = 1.0;
  }

  if ( value >= 1.0 )
    state = STATE_FULL;
  else
    state = STATE_REGENERATING;

  if ( overflow > 0.0 && ( paired_rune -> state == STATE_REGENERATING || paired_rune -> state == STATE_DEPLETED ) )
  {
    // we shouldn't ever overflow the paired rune, but take care just in case
    paired_rune -> value += overflow;
    if ( paired_rune -> value > 1.0 )
    {
      overflow = paired_rune -> value - 1.0;
      paired_rune -> value = 1.0;
    }
    if ( paired_rune -> value >= 1.0 )
      paired_rune -> state = STATE_FULL;
    else
      paired_rune -> state = STATE_REGENERATING;
  }
  gains_rune_type -> add( RESOURCE_NONE, regen_amount - overflow, overflow );
  gains_rune      -> add( RESOURCE_NONE, regen_amount - overflow, overflow );

  if ( p -> sim -> debug )
    log_t::output( p -> sim, "rune %d has %.2f regen time (%.3f per second) with %.2f%% haste",
                   slot_number, 1 / runes_per_second, runes_per_second, 100 * ( 1 / p -> composite_attack_haste() - 1 ) );

  if ( state == STATE_FULL )
  {
    if ( p -> sim -> log )
      log_rune_status( o );

    if ( p -> sim -> debug )
      log_t::output( p -> sim, "rune %d regens to full", slot_number );
  }
}


// ==========================================================================
// Dancing Rune Weapon
// ==========================================================================

struct dancing_rune_weapon_pet_t : public pet_t
{
  dot_t* dots_drw_blood_plague;
  dot_t* dots_drw_frost_fever;

  int drw_diseases( player_t* /* t */ )
  {
    int drw_disease_count = 0;
    if ( dots_drw_blood_plague -> ticking ) drw_disease_count++;
    if ( dots_drw_frost_fever  -> ticking ) drw_disease_count++;
    return drw_disease_count;
  }

  struct drw_spell_t : public spell_t
  {
    drw_spell_t( const std::string& n, dancing_rune_weapon_pet_t* p, const spell_data_t* s = spell_data_t::nil() ) :
      spell_t( n, p, s )
    { }

    virtual bool ready() { return false; }
  };

  struct drw_blood_boil_t : public drw_spell_t
  {
    drw_blood_boil_t( dancing_rune_weapon_pet_t* p ) :
      drw_spell_t( "blood_boil", p, p -> find_class_spell( "Blood Boil" ) )
    {
      background       = true;
      trigger_gcd      = timespan_t::zero();
      aoe              = -1;
      may_crit         = true;
      direct_power_mod = 0.08;  // CHECK-ME
    }

    void target_debuff( player_t* t, dmg_type_e dtype )
    {
      dancing_rune_weapon_pet_t* p = ( dancing_rune_weapon_pet_t* ) player;
      drw_spell_t::target_debuff( t, dtype );

      base_dd_adder = ( p -> drw_diseases( t ) ? 95 : 0 );
      direct_power_mod  = 0.08 + ( p -> drw_diseases( t ) ? 0.035 : 0 );
    }
  };

  struct drw_blood_plague_t : public drw_spell_t
  { 
    drw_blood_plague_t( dancing_rune_weapon_pet_t* p ) :
      drw_spell_t( "blood_plague", p, p -> find_spell( 59879 ) )  // Also check spell id 55078
    {
      background         = true;
      base_tick_time     = timespan_t::from_seconds( 3.0 );
      num_ticks          = 7;
      direct_power_mod   = 0.055 * 1.15; // CHECK-ME etc
      may_miss           = false;
      hasted_ticks       = false;
    }
  };

  struct drw_death_coil_t : public drw_spell_t
  {
    drw_death_coil_t( dancing_rune_weapon_pet_t* p ) :
      drw_spell_t( "death_coil", p, p -> find_class_spell( "Death Coil" ) )
    {
      background  = true;
      trigger_gcd = timespan_t::zero();
      direct_power_mod = 0.23;
      base_dd_min      = data().effectN( 1 ).min( p ); // Values are saved in a not automatically parsed sub-effect
      base_dd_max      = data().effectN( 1 ).max( p );
    }
  };

  struct drw_melee_attack_t : public melee_attack_t
  {
    drw_melee_attack_t( const std::string& n, dancing_rune_weapon_pet_t* p, const spell_data_t* s = spell_data_t::nil() ) :
      melee_attack_t( n, p, s )
    { }

    virtual bool ready() { return false; }
  };

  struct drw_death_strike_t : public drw_melee_attack_t
  {
    drw_death_strike_t( dancing_rune_weapon_pet_t* p ) :
      drw_melee_attack_t( "death_strike", p, p -> find_class_spell( "Death Strike" ) )
    {
      background  = true;
      trigger_gcd = timespan_t::zero();

      special = true;
    }
  };

  struct drw_frost_fever_t : public drw_spell_t
  {
    drw_frost_fever_t( dancing_rune_weapon_pet_t* p ) :
      drw_spell_t( "frost_fever", p, p -> find_spell( 55095 ) )
    {
      background        = true;
      trigger_gcd       = timespan_t::zero();
      base_tick_time    = timespan_t::from_seconds( 3.0 );
      hasted_ticks      = false;
      may_miss          = false;
      num_ticks         = 7;
      direct_power_mod *= 0.055 * 1.15;
    }
  };

  struct drw_heart_strike_t : public drw_melee_attack_t
  {
    drw_heart_strike_t( dancing_rune_weapon_pet_t* p ) :
      drw_melee_attack_t( "heart_strike", p, p -> find_spell( "Heart Strike" ) )
    {
      background          = true;
      aoe                 = 2;
      base_add_multiplier = 0.75;
      trigger_gcd         = timespan_t::zero();
    }

    void target_debuff( player_t* t, dmg_type_e dtype )
    {
      dancing_rune_weapon_pet_t* p = ( dancing_rune_weapon_pet_t* ) player;
      drw_melee_attack_t::target_debuff( t, dtype );

      target_multiplier *= 1 + p -> drw_diseases( t ) * data().effectN( 3 ).percent();
    }
  };

  struct drw_icy_touch_t : public drw_spell_t
  {
    drw_icy_touch_t( dancing_rune_weapon_pet_t* p ) :
      drw_spell_t( "icy_touch", p, p -> find_class_spell( "Icy Touch" ) )
    {
      background       = true;
      trigger_gcd      = timespan_t::zero();
      direct_power_mod = 0.2;
    }

    virtual void execute()
    {
      dancing_rune_weapon_pet_t* p = ( dancing_rune_weapon_pet_t* ) player;

      drw_spell_t::execute();

      if ( result_is_hit() )
      {
        if ( ! p -> drw_frost_fever )
          p -> drw_frost_fever = new drw_frost_fever_t( p );
        p -> drw_frost_fever -> execute();
      }
    }
  };

  struct drw_pestilence_t : public drw_spell_t
  {
    drw_pestilence_t( dancing_rune_weapon_pet_t* p ) :
      drw_spell_t( "pestilence", p, p -> find_class_spell( "Pestilence" ) )
    {
      trigger_gcd = timespan_t::zero();
      background = true;
    }
  };

  struct drw_plague_strike_t : public drw_melee_attack_t
  {
    drw_plague_strike_t( dancing_rune_weapon_pet_t* p ) :
      drw_melee_attack_t( "plague_strike", p, p -> find_class_spell( "Plague Strike" ) )
    {
      background       = true;
      trigger_gcd      = timespan_t::zero();
      may_crit         = true;
      special          = true;
    }

    virtual void execute()
    {
      dancing_rune_weapon_pet_t* p = ( dancing_rune_weapon_pet_t* ) player;

      drw_melee_attack_t::execute();

      if ( result_is_hit() )
      {
        if ( ! p -> drw_blood_plague )
          p -> drw_blood_plague = new drw_blood_plague_t( p );
        p -> drw_blood_plague -> execute();
      }
    }
  };

  struct drw_melee_t : public drw_melee_attack_t
  {
    drw_melee_t( dancing_rune_weapon_pet_t* p ) :
      drw_melee_attack_t( "drw_melee", p )
    {
      weapon            = &( p -> owner -> main_hand_weapon );
      base_execute_time = weapon -> swing_time;
      base_dd_min       = 2; // FIXME: Should these be set?
      base_dd_max       = 322;
      may_crit          = true;
      background        = true;
      repeating         = true;
      weapon_power_mod *= 2.0; //Attack power scaling is unaffected by the DRW 50% penalty.
    }

    virtual bool ready()
    {
      return melee_attack_t::ready();
    }
  };

  double snapshot_spell_crit, snapshot_attack_crit, haste_snapshot, speed_snapshot;
  spell_t*  drw_blood_boil;
  spell_t*  drw_blood_plague;
  spell_t*  drw_death_coil;
  melee_attack_t* drw_death_strike;
  spell_t*  drw_frost_fever;
  melee_attack_t* drw_heart_strike;
  spell_t*  drw_icy_touch;
  spell_t*  drw_pestilence;
  melee_attack_t* drw_plague_strike;
  melee_attack_t* drw_melee;

  dancing_rune_weapon_pet_t( sim_t* sim, player_t* owner ) :
    pet_t( sim, owner, "dancing_rune_weapon", true ),
    dots_drw_blood_plague( 0 ), dots_drw_frost_fever( 0 ),
    snapshot_spell_crit( 0.0 ), snapshot_attack_crit( 0.0 ),
    haste_snapshot( 1.0 ), speed_snapshot( 1.0 ), drw_blood_boil( 0 ), drw_blood_plague( 0 ),
    drw_death_coil( 0 ), drw_death_strike( 0 ), drw_frost_fever( 0 ),
    drw_heart_strike( 0 ), drw_icy_touch( 0 ), drw_pestilence( 0 ),
    drw_plague_strike( 0 ), drw_melee( 0 )
  {
    dots_drw_blood_plague  = sim -> target -> get_dot( "blood_plague", this );
    dots_drw_frost_fever   = sim -> target -> get_dot( "frost_fever",  this );

    main_hand_weapon.type       = WEAPON_SWORD_2H;
    main_hand_weapon.min_dmg    = 685; // FIXME: Should these be hardcoded?
    main_hand_weapon.max_dmg    = 975;
    main_hand_weapon.swing_time = timespan_t::from_seconds( 3.3 );
  }

  virtual void init_base()
  {
    // Everything stays at zero.
    // DRW uses a snapshot of the DKs stats when summoned.
    drw_blood_boil    = new drw_blood_boil_t   ( this );
    drw_blood_plague  = new drw_blood_plague_t ( this );
    drw_death_coil    = new drw_death_coil_t   ( this );
    drw_death_strike  = new drw_death_strike_t ( this );
    drw_frost_fever   = new drw_frost_fever_t  ( this );
    drw_heart_strike  = new drw_heart_strike_t ( this );
    drw_icy_touch     = new drw_icy_touch_t    ( this );
    drw_pestilence    = new drw_pestilence_t   ( this );
    drw_plague_strike = new drw_plague_strike_t( this );
    drw_melee         = new drw_melee_t        ( this );
  }

  virtual double composite_attack_crit( weapon_t* ) { return snapshot_attack_crit; }
  virtual double composite_attack_haste()           { return haste_snapshot; }
  virtual double composite_attack_speed()           { return speed_snapshot; }
  virtual double composite_attack_power()           { return current.attack_power; }
  virtual double composite_spell_crit()             { return snapshot_spell_crit;  }

  virtual void summon( timespan_t duration=timespan_t::zero() )
  {
    pet_t::summon( duration );
    snapshot_spell_crit  = owner -> composite_spell_crit();
    snapshot_attack_crit = owner -> composite_attack_crit();
    haste_snapshot       = owner -> composite_attack_haste();
    speed_snapshot       = owner -> composite_attack_speed();
    current.attack_power         = owner -> composite_attack_power() * owner -> composite_attack_power_multiplier();
    drw_melee -> schedule_execute();
  }
};

namespace  // ANONYMOUS NAMESPACE ===========================================
{
struct death_knight_pet_t : public pet_t
{
  death_knight_pet_t( sim_t* sim, death_knight_t* owner, const std::string& n, bool guardian ) :
    pet_t( sim, owner, n, guardian )
  { }

  death_knight_t* o()
  { return debug_cast<death_knight_t*>( owner ); }

};

// ==========================================================================
// Guardians
// ==========================================================================

// ==========================================================================
// Army of the Dead Ghoul
// ==========================================================================

struct army_ghoul_pet_t : public death_knight_pet_t
{
  double snapshot_crit, snapshot_haste, snapshot_speed, snapshot_hit, snapshot_strength;

  army_ghoul_pet_t( sim_t* sim, death_knight_t* owner ) :
    death_knight_pet_t( sim, owner, "army_of_the_dead", true ),
    snapshot_crit( 0 ), snapshot_haste( 0 ), snapshot_speed( 0 ), snapshot_hit( 0 ), snapshot_strength( 0 )
  {
    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.min_dmg    = 228; // FIXME: Needs further testing
    main_hand_weapon.max_dmg    = 323; // FIXME: Needs further testing
    main_hand_weapon.swing_time = timespan_t::from_seconds( 2.0 );

    action_list_str = "snapshot_stats/auto_attack/claw";
  }

  struct army_ghoul_pet_melee_attack_t : public melee_attack_t
  {
    void _init_army_ghoul_pet_melee_attack_t()
    {
      weapon = &( player -> main_hand_weapon );
      may_crit = true;
      base_multiplier *= 8.0; // 8 ghouls
    }

    army_ghoul_pet_melee_attack_t( const std::string& n, army_ghoul_pet_t* p,
                                   const spell_data_t* s = spell_data_t::nil() ) :
      melee_attack_t( n, p, s )
    {
      _init_army_ghoul_pet_melee_attack_t();
    }
  };

  struct army_ghoul_pet_melee_t : public army_ghoul_pet_melee_attack_t
  {
    army_ghoul_pet_melee_t( army_ghoul_pet_t* p ) :
      army_ghoul_pet_melee_attack_t( "melee", p )
    {
      base_execute_time = weapon -> swing_time;
      background        = true;
      repeating         = true;
      weapon_power_mod  = 0.0055 / weapon -> swing_time.total_seconds(); // FIXME: Needs further testing
      special           = false;
    }
  };

  struct army_ghoul_pet_auto_melee_attack_t : public army_ghoul_pet_melee_attack_t
  {
    army_ghoul_pet_auto_melee_attack_t( army_ghoul_pet_t* p ) :
      army_ghoul_pet_melee_attack_t( "auto_attack", p )
    {
      weapon = &( p -> main_hand_weapon );
      p -> main_hand_attack = new army_ghoul_pet_melee_t( p );
      trigger_gcd = timespan_t::zero();
      special = true;
    }

    virtual void execute()
    {
      army_ghoul_pet_t* p = ( army_ghoul_pet_t* ) player -> cast_pet();
      p -> main_hand_attack -> schedule_execute();
    }

    virtual bool ready()
    {
      army_ghoul_pet_t* p = ( army_ghoul_pet_t* ) player -> cast_pet();
      return( p -> main_hand_attack -> execute_event == 0 ); // not swinging
    }
  };

  struct army_ghoul_pet_claw_t : public army_ghoul_pet_melee_attack_t
  {
    army_ghoul_pet_claw_t( army_ghoul_pet_t* p ) :
      army_ghoul_pet_melee_attack_t( "claw", p, p -> find_spell( 91776 ) )
    {
      weapon_power_mod  = 0.0055 / weapon -> swing_time.total_seconds(); // FIXME: Needs further testing
      special = true;
    }
  };

  virtual void init_base()
  {
    // FIXME: Copied from the pet ghoul
    base.attribute[ ATTR_STRENGTH  ] = 476;
    base.attribute[ ATTR_AGILITY   ] = 3343;
    base.attribute[ ATTR_STAMINA   ] = 546;
    base.attribute[ ATTR_INTELLECT ] = 69;
    base.attribute[ ATTR_SPIRIT    ] = 116;

    base.attack_power = -20;
    initial.attack_power_per_strength = 2.0;
    initial.attack_power_per_agility  = 0.0;

    // Ghouls don't appear to gain any crit from agi, they may also just have none
    // initial_attack_crit_per_agility = rating_t::interpolate( level, 0.01/25.0, 0.01/40.0, 0.01/83.3 );

    resources.base[ RESOURCE_ENERGY ] = 100;
    base_energy_regen_per_second  = 10;
  }

  virtual double energy_regen_per_second()
  {
    // Doesn't benefit from haste
    return base_energy_regen_per_second;
  }

  virtual void summon( timespan_t duration=timespan_t::zero() )
  {
    pet_t::summon( duration );
    snapshot_crit     = o() -> composite_attack_crit();
    snapshot_haste    = o() -> composite_attack_haste();
    snapshot_speed    = o() -> composite_attack_speed();
    snapshot_hit      = o() -> composite_attack_hit();
    snapshot_strength = o() -> strength();
  }

  virtual double composite_attack_expertise( weapon_t* )
  {
    return ( ( 100.0 * snapshot_hit ) * 26.0 / 8.0 ) / 100.0; // Hit gains equal to expertise
  }

  virtual double composite_attack_crit( weapon_t* ) { return snapshot_crit; }

  virtual double composite_attack_speed() { return snapshot_speed; }

  virtual double composite_attack_haste() { return snapshot_haste; }

  virtual double composite_attack_hit() { return snapshot_hit; }

  virtual resource_type_e primary_resource() { return RESOURCE_ENERGY; }

  virtual action_t* create_action( const std::string& name, const std::string& options_str )
  {
    if ( name == "auto_attack"    ) return new  army_ghoul_pet_auto_melee_attack_t( this );
    if ( name == "claw"           ) return new         army_ghoul_pet_claw_t( this );

    return pet_t::create_action( name, options_str );
  }

  timespan_t available()
  {
    double energy = resources.current[ RESOURCE_ENERGY ];

    if ( energy > 40 )
      return timespan_t::from_seconds( 0.1 );

    return std::max(
      timespan_t::from_seconds( ( 40 - energy ) / energy_regen_per_second() ),
      timespan_t::from_seconds( 0.1 )
    );
  }
};

// ==========================================================================
// Bloodworms
// ==========================================================================
struct bloodworms_pet_t : public death_knight_pet_t
{
  // FIXME: Level 80/85 values
  struct melee_t : public melee_attack_t
  {
    melee_t( player_t* player ) :
      melee_attack_t( "bloodworm_melee", player )
    {
      weapon = &( player -> main_hand_weapon );
      base_execute_time = weapon -> swing_time;
      base_dd_min = base_dd_max = 1;
      may_crit    = true;
      background  = true;
      repeating   = true;
    }
  };

  melee_t* melee;

  bloodworms_pet_t( sim_t* sim, death_knight_t* owner ) :
    death_knight_pet_t( sim, owner, "bloodworms", true /*guardian*/ )
  {
    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.min_dmg    = 20;
    main_hand_weapon.max_dmg    = 20;
    main_hand_weapon.swing_time = timespan_t::from_seconds( 2.0 );
  }

  virtual void init_base()
  {
    pet_t::init_base();

    // Stolen from Priest's Shadowfiend
    base.attribute[ ATTR_STRENGTH  ] = 145;
    base.attribute[ ATTR_AGILITY   ] =  38;
    base.attribute[ ATTR_STAMINA   ] = 190;
    base.attribute[ ATTR_INTELLECT ] = 133;

    //health_per_stamina = 7.5;
    //mana_per_intellect = 5;

    melee = new melee_t( this );
  }

  virtual void summon( timespan_t duration=timespan_t::zero() )
  {
    pet_t::summon( duration );
    melee -> schedule_execute();
  }

  virtual resource_type_e primary_resource() { return RESOURCE_MANA; }
};

// ==========================================================================
// Gargoyle
// ==========================================================================

struct gargoyle_pet_t : public death_knight_pet_t
{

  // FIXME: Did any of these stats change?
  struct gargoyle_strike_t : public spell_t
  {
    gargoyle_strike_t( pet_t* pet ) :
      spell_t( "gargoyle_strike", pet, pet -> find_pet_spell( "Gargoyle Strike" ) )
    {
      // FIX ME!
      // Resist (can be partial)? Scaling?
      trigger_gcd = timespan_t::from_seconds( 1.5 );
      may_crit    = true;
      min_gcd     = timespan_t::from_seconds( 1.5 ); // issue961

      base_spell_power_multiplier  = 0;
      base_attack_power_multiplier = 1;
    }
  };

  double snapshot_haste, snapshot_spell_crit, snapshot_power;

  gargoyle_pet_t( sim_t* sim, death_knight_t* owner ) :
    death_knight_pet_t( sim, owner, "gargoyle", true ),
    snapshot_haste( 1 ), snapshot_spell_crit( 0 ), snapshot_power( 0 )
  {
  }

  virtual void init_base()
  {
    // FIX ME!
    base.attribute[ ATTR_STRENGTH  ] = 0;
    base.attribute[ ATTR_AGILITY   ] = 0;
    base.attribute[ ATTR_STAMINA   ] = 0;
    base.attribute[ ATTR_INTELLECT ] = 0;
    base.attribute[ ATTR_SPIRIT    ] = 0;

    action_list_str = "/snapshot_stats/gargoyle_strike";
  }
  virtual double composite_spell_haste()  { return snapshot_haste; }
  virtual double composite_attack_power() { return snapshot_power; }
  virtual double composite_spell_crit()   { return snapshot_spell_crit; }

  virtual action_t* create_action( const std::string& name,
                                   const std::string& options_str )
  {
    if ( name == "gargoyle_strike" ) return new gargoyle_strike_t( this );

    return pet_t::create_action( name, options_str );
  }

  virtual void summon( timespan_t duration=timespan_t::zero() )
  {
    pet_t::summon( duration );
    // Haste etc. are taken at the time of summoning
    snapshot_haste      = o() -> composite_attack_speed();
    snapshot_power      = o() -> composite_attack_power() * o() -> composite_attack_power_multiplier();
    snapshot_spell_crit = o() -> composite_spell_crit();

  }

  virtual void arise()
  {
    if ( sim -> log )
      log_t::output( sim, "%s arises.", name() );

    if ( ! initial.sleeping )
      current.sleeping = 0;

    if ( current.sleeping )
      return;

    init_resources( true );

    readying = 0;

    arise_time = sim -> current_time;

    schedule_ready( timespan_t::from_seconds( 3.0 ), true ); // Gargoyle pet is idle for the first 3 seconds.
  }
};

// ==========================================================================
// Pet Ghoul
// ==========================================================================

struct ghoul_pet_t : public death_knight_pet_t
{
  double snapshot_crit, snapshot_haste, snapshot_speed, snapshot_hit, snapshot_strength;

  ghoul_pet_t( sim_t* sim, death_knight_t* owner ) :
    death_knight_pet_t( sim, owner, "ghoul", owner -> spells.master_of_ghouls -> ok() ? false : true ),
    snapshot_crit( 0 ), snapshot_haste( 0 ), snapshot_speed( 0 ), snapshot_hit( 0 ), snapshot_strength( 0 )
  {
    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.min_dmg    = 622.43; // should be exact as of 4.2
    main_hand_weapon.max_dmg    = 933.64; // should be exact as of 4.2
    main_hand_weapon.swing_time = timespan_t::from_seconds( 2.0 );

    action_list_str = "auto_attack/sweeping_claws/claw";
  }

  struct ghoul_pet_melee_attack_t : public melee_attack_t
  {
    ghoul_pet_melee_attack_t( const char* n, ghoul_pet_t* p, const spell_data_t* s = spell_data_t::nil() ) :
      melee_attack_t( n, p, s )
    {
      weapon = &( player -> main_hand_weapon );
      may_crit = true;
    }

    virtual void player_buff()
    {
      melee_attack_t::player_buff();

      ghoul_pet_t* p = debug_cast<ghoul_pet_t*>( player );

      player_multiplier *= 1.0 + p -> o() -> buffs.shadow_infusion -> stack() * p -> o() -> buffs.shadow_infusion -> data().effectN( 1 ).percent();

      if ( p -> o() -> buffs.dark_transformation -> up() )
      {
        player_multiplier *= 1.0 + p -> o() -> buffs.dark_transformation -> data().effectN( 1 ).percent();
      }
    }
  };

  struct ghoul_pet_melee_t : public ghoul_pet_melee_attack_t
  {
    ghoul_pet_melee_t( ghoul_pet_t* p ) :
      ghoul_pet_melee_attack_t( "melee", p )
    {
      base_execute_time = weapon -> swing_time;
      background        = true;
      repeating         = true;
      special           = false;
      weapon_power_mod  = 0.120 / weapon -> swing_time.total_seconds(); // should be exact as of 4.2
    }
  };

  struct ghoul_pet_auto_melee_attack_t : public ghoul_pet_melee_attack_t
  {
    ghoul_pet_auto_melee_attack_t( ghoul_pet_t* p ) :
      ghoul_pet_melee_attack_t( "auto_attack", p )
    {
      weapon = &( p -> main_hand_weapon );
      p -> main_hand_attack = new ghoul_pet_melee_t( p );
      trigger_gcd = timespan_t::zero();
      special = true;
    }

    virtual void execute()
    {
      ghoul_pet_t* p = ( ghoul_pet_t* ) player -> cast_pet();
      p -> main_hand_attack -> schedule_execute();
    }

    virtual bool ready()
    {
      ghoul_pet_t* p = ( ghoul_pet_t* ) player -> cast_pet();
      return( p -> main_hand_attack -> execute_event == 0 ); // not swinging
    }
  };

  struct ghoul_pet_claw_t : public ghoul_pet_melee_attack_t
  {
    ghoul_pet_claw_t( ghoul_pet_t* p ) :
      ghoul_pet_melee_attack_t( "claw", p, p -> find_spell( 91776 ) )
    {
      weapon_power_mod = 0.120 / weapon -> swing_time.total_seconds(); // should be exact as of 4.2
      special = true;
    }
  };

  struct ghoul_pet_sweeping_claws_t : public ghoul_pet_melee_attack_t
  {
    ghoul_pet_sweeping_claws_t( ghoul_pet_t* p ) :
      ghoul_pet_melee_attack_t( "sweeping_claws", p, p -> find_spell( 91778 ) )
    {
      aoe = 2;
      weapon_power_mod = 0.120 / weapon -> swing_time.total_seconds(); // Copied from claw, but seems Ok
      special = true;
    }

    virtual bool ready()
    {
      death_knight_t* dk = debug_cast<ghoul_pet_t*>( player ) -> o();

      if ( ! dk -> buffs.dark_transformation -> check() )
        return false;

      return ghoul_pet_melee_attack_t::ready();
    }
  };

  virtual void init_base()
  {
    assert( owner -> primary_tree() != SPEC_NONE );


    // Value for the ghoul of a naked worgen as of 4.2
    base.attribute[ ATTR_STRENGTH  ] = 476;
    base.attribute[ ATTR_AGILITY   ] = 3343;
    base.attribute[ ATTR_STAMINA   ] = 546;
    base.attribute[ ATTR_INTELLECT ] = 69;
    base.attribute[ ATTR_SPIRIT    ] = 116;

    base.attack_power = -20;
    initial.attack_power_per_strength = 2.0;
    initial.attack_power_per_agility  = 0.0;//no AP per agi.

    initial.attack_crit_per_agility = rating_t::interpolate( level, 0.01/25.0, 0.01/40.0, 0.01/83.3 );

    resources.base[ RESOURCE_ENERGY ] = 100;
    base_energy_regen_per_second  = 10;
  }

  virtual double energy_regen_per_second()
  {
    // Doesn't benefit from haste
    return base_energy_regen_per_second;
  }

  virtual double composite_attribute( attribute_type_e attr )
  {
    double a = current.attribute[ attr ];
    if ( attr == ATTR_STRENGTH )
    {
      double strength_scaling = 1.01;
      if ( o() -> spells.master_of_ghouls -> ok() )
      {
        // Perma Ghouls are updated constantly
        a += owner -> strength() * strength_scaling;
      }
      else
      {
        a += snapshot_strength * strength_scaling;
      }
    }
    return a;
  }

  virtual void summon( timespan_t duration=timespan_t::zero() )
  {
    pet_t::summon( duration );
    snapshot_crit     = owner -> composite_attack_crit();
    snapshot_haste    = owner -> composite_attack_haste();
    snapshot_speed    = owner -> composite_attack_speed();
    snapshot_hit      = owner -> composite_attack_hit();
    snapshot_strength = owner -> strength();
  }

  virtual double composite_attack_crit( weapon_t* )
  {
    // Perma Ghouls are updated constantly

    if ( o() -> spells.master_of_ghouls -> ok() )
    {
      return owner -> composite_attack_crit( &( owner -> main_hand_weapon ) );
    }
    else
    {
      return snapshot_crit;
    }
  }

  virtual double composite_attack_expertise( weapon_t* )
  {
    // Perma Ghouls are updated constantly
    if ( o() -> spells.master_of_ghouls -> ok() )
    {
      return ( ( 100.0 * owner -> current.attack_hit ) * 26.0 / 8.0 ) / 100.0;
    }
    else
    {
      return ( ( 100.0 * snapshot_hit ) * 26.0 / 8.0 ) / 100.0;
    }
  }

  virtual double composite_attack_haste()
  {
    // Ghouls receive 100% of their master's haste.
    // http://elitistjerks.com/f72/t42606-pet_discussion_garg_aotd_ghoul/

    // Perma Ghouls are updated constantly
    if ( o() -> spells.master_of_ghouls -> ok() )
    {
      return owner -> composite_attack_haste();
    }
    else
    {
      return snapshot_haste;
    }
  }

  virtual double composite_attack_speed()
  {
    // Ghouls receive 100% of their master's haste.
    // http://elitistjerks.com/f72/t42606-pet_discussion_garg_aotd_ghoul/

    // Perma Ghouls are updated constantly
    if ( o() -> spells.master_of_ghouls -> ok() )
    {
      return owner -> composite_attack_speed();
    }
    else
    {
      return snapshot_speed;
    }
  }

  virtual double composite_attack_hit()
  {
    // Perma Ghouls are updated constantly
    if ( o() -> spells.master_of_ghouls -> ok() )
    {
      return owner -> composite_attack_hit();
    }
    else
    {
      return snapshot_hit;
    }
  }

  //Ghoul regen doesn't benefit from haste (even bloodlust/heroism)
  virtual resource_type_e primary_resource()
  {
    return RESOURCE_ENERGY;
  }

  virtual action_t* create_action( const std::string& name, const std::string& options_str )
  {
    if ( name == "auto_attack"    ) return new    ghoul_pet_auto_melee_attack_t( this );
    if ( name == "claw"           ) return new           ghoul_pet_claw_t( this );
    if ( name == "sweeping_claws" ) return new ghoul_pet_sweeping_claws_t( this );

    return pet_t::create_action( name, options_str );
  }

  timespan_t available()
  {
    double energy = resources.current[ RESOURCE_ENERGY ];

    // Cheapest Ability need 40 Energy
    if ( energy > 40 )
      return timespan_t::from_seconds( 0.1 );

    return std::max(
      timespan_t::from_seconds( ( 40 - energy ) / energy_regen_per_second() ),
      timespan_t::from_seconds( 0.1 )
    );
  }
};
// ==========================================================================
// Death Knight Attack
// ==========================================================================

struct death_knight_melee_attack_t : public melee_attack_t
{
  bool   always_consume;
  bool   requires_weapon;
  int    cost_blood;
  int    cost_frost;
  int    cost_unholy;
  double convert_runes;
  double m_dd_additive; // Multipler for Direct Damage that are all additive with each other
  bool   use[RUNE_SLOT_MAX];
  gain_t* rp_gains;

  death_knight_melee_attack_t( const std::string& n, death_knight_t* p,
                               const spell_data_t* s = spell_data_t::nil() ) :
    melee_attack_t( n, p, s ),
    always_consume( false ), requires_weapon( true ),
    cost_blood( 0 ),cost_frost( 0 ),cost_unholy( 0 ),convert_runes( 0 ),
    m_dd_additive( 0 )
  {
    _init_dk_attack();
  }

  death_knight_t* cast() { return debug_cast<death_knight_t*>( player ); }

  death_knight_td_t* cast_td( player_t* t = 0 )
  { 
    return cast() -> get_target_data( t ? t : target );
  }

  void _init_dk_attack()
  {
    for ( int i = 0; i < RUNE_SLOT_MAX; ++i ) use[i] = false;

    may_crit   = true;
    may_glance = false;

    rp_gains = player -> get_gain( "rp_" + name_str );
  }

  virtual void   reset();
  virtual void   consume_resource();
  virtual void   execute();
  virtual void   player_buff();
  virtual bool   ready();
  virtual double swing_haste();
  virtual void   target_debuff( player_t* t, dmg_type_e );
};

// ==========================================================================
// Death Knight Spell
// ==========================================================================

struct death_knight_spell_t : public spell_t
{
  int    cost_blood;
  int    cost_frost;
  int    cost_unholy;
  double convert_runes;
  bool   use[RUNE_SLOT_MAX];
  gain_t* rp_gains;

  death_knight_spell_t( const std::string& n, death_knight_t* p,
                        const spell_data_t* s = spell_data_t::nil() ) :
    spell_t( n, p, s ),
    cost_blood( 0 ), cost_frost( 0 ), cost_unholy( 0 ), convert_runes( 0 )
  {
    _init_dk_spell();
  }

  death_knight_t* cast() { return debug_cast<death_knight_t*>( player ); }

  death_knight_td_t* cast_td( player_t* t = 0 )
  { 
    return cast() -> get_target_data( t ? t : target );
  }

  void _init_dk_spell()
  {
    for ( int i = 0; i < RUNE_SLOT_MAX; ++i ) use[i] = false;

    may_crit = true;
    // DKs have 2.09x spell crits with meta gem so they must use the "hybrid" formula of adjusting the crit-bonus multiplier
    // (As opposed to the native 1.33 crit multiplier used by Mages and Warlocks.)
    crit_bonus_multiplier = 2.0;

    base_spell_power_multiplier = 0;
    base_attack_power_multiplier = 1;

    rp_gains = player -> get_gain( "rp_" + name_str );
  }

  virtual void   reset();
  virtual void   consume_resource();
  virtual void   execute();
  virtual void   player_buff();
  virtual void   target_debuff( player_t* t, dmg_type_e );
  virtual bool   ready();
};

// ==========================================================================
// Local Utility Functions
// ==========================================================================

static void extract_rune_cost( const spell_data_t* spell, int* cost_blood, int* cost_frost, int* cost_unholy )
{
  // Rune costs appear to be in binary: 0a0b0c where 'c' is whether the ability
  // costs a blood rune, 'b' is whether it costs an unholy rune, and 'a'
  // is whether it costs a frost rune.

  if ( ! spell -> ok() ) return;

  uint32_t rune_cost = spell -> rune_cost();
  *cost_blood  =        rune_cost & 0x1;
  *cost_unholy = ( rune_cost >> 2 ) & 0x1;
  *cost_frost  = ( rune_cost >> 4 ) & 0x1;
}

// Count Runes ==============================================================

// currently not used. but useful. commenting out to get rid of compile warning
//static int count_runes( player_t* player )
//{
//  death_knight_t* p = cast();
//  int count_by_type[RUNE_SLOT_MAX / 2] = { 0, 0, 0 }; // blood, frost, unholy
//
//  for ( int i = 0; i < RUNE_SLOT_MAX / 2; ++i )
//    count_by_type[i] = ( ( int )p -> _runes.slot[2 * i].is_ready() +
//                         ( int )p -> _runes.slot[2 * i + 1].is_ready() );
//
//  return count_by_type[0] + ( count_by_type[1] << 8 ) + ( count_by_type[2] << 16 );
//}

// Count Death Runes ========================================================

static int count_death_runes( death_knight_t* p, bool inactive )
{
  // Getting death rune count is a bit more complicated as it depends
  // on talents which runetype can be converted to death runes
  int count = 0;
  for ( int i = 0; i < RUNE_SLOT_MAX; ++i )
  {
    dk_rune_t& r = p -> _runes.slot[i];
    if ( ( inactive || r.is_ready() ) && r.is_death() )
      ++count;
  }
  return count;
}

// Consume Runes ============================================================

static void consume_runes( death_knight_t* player, const bool use[RUNE_SLOT_MAX], bool convert_runes = false )
{
  death_knight_t* p = player;

  for ( int i = 0; i < RUNE_SLOT_MAX; ++i )
  {
    if ( use[i] )
    {
      // Show the consumed type of the rune
      // Not the type it is after consumption
      int consumed_type = p -> _runes.slot[i].type;
      p -> _runes.slot[i].consume( convert_runes );

      if ( p -> sim -> log )
        log_t::output( p -> sim, "%s consumes rune #%d, type %d", p -> name(), i, consumed_type );
    }
  }

  if ( p -> sim -> log )
  {
    log_rune_status( p );
  }
}

// Group Runes ==============================================================

static bool group_runes ( death_knight_t* player, int blood, int frost, int unholy, bool group[RUNE_SLOT_MAX] )
{
  death_knight_t* p = player;
  int cost[]  = { blood + frost + unholy, blood, frost, unholy };
  bool use[RUNE_SLOT_MAX] = { false };

  // Selecting available non-death runes to satisfy cost
  for ( int i = 0; i < RUNE_SLOT_MAX; ++i )
  {
    dk_rune_t& r = p -> _runes.slot[i];
    if ( r.is_ready() && ! r.is_death() && cost[r.get_type()] > 0 )
    {
      --cost[r.get_type()];
      --cost[0];
      use[i] = true;
    }
  }

  // Selecting available death runes to satisfy remaining cost
  for ( int i = RUNE_SLOT_MAX; cost[0] > 0 && i--; )
  {
    dk_rune_t& r = p -> _runes.slot[i];
    if ( r.is_ready() && r.is_death() )
    {
      --cost[0];
      use[i] = true;
    }
  }

  if ( cost[0] > 0 ) return false;

  // Storing rune slots selected
  for ( int i = 0; i < RUNE_SLOT_MAX; ++i ) group[i] = use[i];

  return true;
}

// Refund Power =============================================================

static void refund_power( death_knight_melee_attack_t* a )
{
  death_knight_t* p = a -> cast();

  if ( a -> resource_consumed > 0 )
    p -> resource_gain( RESOURCE_RUNIC_POWER, a -> resource_consumed * RUNIC_POWER_REFUND, p -> gains.power_refund );
}

// ==========================================================================
// Triggers
// ==========================================================================

// ==========================================================================
// Death Knight Attack Methods
// ==========================================================================

// death_knight_melee_attack_t::reset() ===========================================

void death_knight_melee_attack_t::reset()
{
  for ( int i = 0; i < RUNE_SLOT_MAX; ++i ) use[i] = false;

  action_t::reset();
}

// death_knight_melee_attack_t::consume_resource() ================================

void death_knight_melee_attack_t::consume_resource()
{
  death_knight_t* p = cast();

  if ( rp_gain > 0 )
  {
    if ( result_is_hit() )
    {
      if ( p -> buffs.frost_presence -> check() )
      {
        p -> resource_gain( RESOURCE_RUNIC_POWER,
                            rp_gain * player -> dbc.spell( 48266 ) -> effect2().percent(),
                            p -> gains.frost_presence );
      }
      p -> resource_gain( RESOURCE_RUNIC_POWER, rp_gain, rp_gains );
    }
  }
  else
  {
    melee_attack_t::consume_resource();
  }

  if ( result_is_hit() || always_consume )
    consume_runes( p, use, convert_runes == 0 ? false : sim -> roll( convert_runes ) == 1 );
  else
    refund_power( this );
}

// death_knight_melee_attack_t::execute() =========================================

void death_knight_melee_attack_t::execute()
{
  death_knight_t* p = cast();

  melee_attack_t::execute();

  if ( result_is_hit() )
  {
    p -> buffs.bloodworms -> trigger();
    if ( school == SCHOOL_FROST || school == SCHOOL_SHADOW )
      if ( ! proc )
        p -> buffs.rune_of_cinderglacier -> decrement();
  }
}

// death_knight_melee_attack_t::player_buff() =====================================

void death_knight_melee_attack_t::player_buff()
{
  death_knight_t* p = cast();

  melee_attack_t::player_buff();

  if ( school == SCHOOL_FROST || school == SCHOOL_SHADOW )
    if ( ! proc )
      player_multiplier *= 1.0 + p -> buffs.rune_of_cinderglacier -> value();

  // Add in all m_dd_additive
  player_multiplier *= 1.0 + m_dd_additive;
}

// death_knight_melee_attack_t::ready() ===========================================

bool death_knight_melee_attack_t::ready()
{
  death_knight_t* p = cast();

  if ( ! melee_attack_t::ready() )
    return false;

  if ( requires_weapon )
    if ( ! weapon || weapon -> group() == WEAPON_RANGED )
      return false;

  return group_runes( p, cost_blood, cost_frost, cost_unholy, use );
}

// death_knight_melee_attack_t::swing_haste() =====================================

double death_knight_melee_attack_t::swing_haste()
{
  double haste = melee_attack_t::swing_haste();
  death_knight_t* p = cast();

  haste *= 1.0 / ( 1.0 + p -> spells.icy_talons -> effectN( 1 ).percent() );

  return haste;
}

// death_knight_melee_attack_t::target_debuff =====================================

void death_knight_melee_attack_t::target_debuff( player_t* t, dmg_type_e dtype )
{
  melee_attack_t::target_debuff( t, dtype );
  death_knight_t* p = cast();

  if ( school == SCHOOL_FROST  )
  {
    target_multiplier *= 1.0 + p -> buffs.rune_of_razorice -> stack() * p -> buffs.rune_of_razorice -> data().effectN( 1 ).percent();
  }
}

// ==========================================================================
// Death Knight Spell Methods
// ==========================================================================

// death_knight_spell_t::reset() ============================================

void death_knight_spell_t::reset()
{
  for ( int i = 0; i < RUNE_SLOT_MAX; ++i ) use[i] = false;
  spell_t::reset();
}

// death_knight_spell_t::consume_resource() =================================

void death_knight_spell_t::consume_resource()
{
  death_knight_t* p = cast();
  if ( rp_gain > 0 )
  {
    if ( result_is_hit() )
    {
      if ( p -> buffs.frost_presence -> check() )
      {
        p -> resource_gain( RESOURCE_RUNIC_POWER,
                            rp_gain * player -> dbc.spell( 48266 ) -> effect2().percent(),
                            p -> gains.frost_presence );
      }
      p -> resource_gain( RESOURCE_RUNIC_POWER, rp_gain, rp_gains );
    }
  }
  else
  {
    spell_t::consume_resource();
  }

  if ( result_is_hit() )
    consume_runes( p, use, convert_runes == 0 ? false : sim -> roll( convert_runes ) == 1 );
}

// death_knight_spell_t::execute() ==========================================

void death_knight_spell_t::execute()
{
  spell_t::execute();

  if ( result_is_hit() )
  {
    if ( school == SCHOOL_FROST || school == SCHOOL_SHADOW )
    {
      death_knight_t* p = cast();
      p -> buffs.rune_of_cinderglacier -> decrement();
    }
  }
}

// death_knight_spell_t::player_buff() ======================================

void death_knight_spell_t::player_buff()
{
  death_knight_t* p = cast();

  spell_t::player_buff();

  if ( ( school == SCHOOL_FROST || school == SCHOOL_SHADOW ) )
    player_multiplier *= 1.0 + p -> buffs.rune_of_cinderglacier -> value();

  if ( sim -> debug )
    log_t::output( sim, "death_knight_spell_t::player_buff: %s hit=%.2f crit=%.2f power=%.2f p_mult=%.0f",
                   name(), player_hit, player_crit, player_spell_power, player_multiplier );
}

// death_knight_spell_t::ready() ============================================

bool death_knight_spell_t::ready()
{
  if ( ! spell_t::ready() )
    return false;

  death_knight_t* p = cast();

  if ( ! player -> in_combat && ! harmful )
    return group_runes( p, 0, 0, 0, use );
  else
    return group_runes( p, cost_blood, cost_frost, cost_unholy, use );
}

// death_knight_spell_t::target_debuff ======================================

void death_knight_spell_t::target_debuff( player_t* t, dmg_type_e dtype )
{
  spell_t::target_debuff( t, dtype );
  death_knight_t* p = cast();

  if ( school == SCHOOL_FROST  )
  {
    target_multiplier *= 1.0 + p -> buffs.rune_of_razorice -> stack() * p -> buffs.rune_of_razorice -> data().effectN( 1 ).percent();
  }
}

// ==========================================================================
// Death Knight Attacks
// ==========================================================================

// Melee Attack =============================================================

struct melee_t : public death_knight_melee_attack_t
{
  int sync_weapons;
  melee_t( const char* name, death_knight_t* p, int sw ) :
    death_knight_melee_attack_t( name, p ), sync_weapons( sw )
  {
    may_glance      = true;
    background      = true;
    repeating       = true;
    trigger_gcd     = timespan_t::zero();

    if ( p -> dual_wield() )
      base_hit -= 0.19;
  }

  virtual timespan_t execute_time()
  {
    timespan_t t = death_knight_melee_attack_t::execute_time();
    if ( ! player -> in_combat )
    {
      return ( weapon -> slot == SLOT_OFF_HAND ) ? ( sync_weapons ? std::min( t/2, timespan_t::from_seconds( 0.2 ) ) : t/2 ) : timespan_t::from_seconds( 0.01 );
    }
    return t;
  }

  virtual void execute()
  {
    death_knight_t* p = cast();

    death_knight_melee_attack_t::execute();

    if ( result_is_hit() )
    {
      death_knight_td_t* td = cast_td( target );

      if ( weapon -> slot == SLOT_MAIN_HAND )
      {
        // T13 2pc gives 2 stacks of SD, otherwise we can only ever have one
        // Ensure that if we have 1 that we only refresh, not add another stack
        int new_stacks = ( p -> set_bonus.tier13_2pc_melee() && sim -> roll( p -> sets -> set( SET_T13_2PC_MELEE ) -> effect1().percent() ) ) ? 2 : 1;
        ( void )new_stacks;
      }

      // TODO: Confirm PPM for ranks 1 and 2 http://elitistjerks.com/f72/t110296-frost_dps_|_cataclysm_4_0_3_nothing_lose/p9/#post1869431

      if ( td -> dots_blood_plague && td -> dots_blood_plague -> ticking )
        p -> buffs.crimson_scourge -> trigger();

      if ( p -> buffs.scent_of_blood -> up() )
      {
        p -> resource_gain( RESOURCE_RUNIC_POWER, 10, p -> gains.scent_of_blood );
        p -> buffs.scent_of_blood -> decrement();
      }
    }
  }
};

// Auto Attack ==============================================================

struct auto_attack_t : public death_knight_melee_attack_t
{
  int sync_weapons;

  auto_attack_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_melee_attack_t( "auto_attack", p ), sync_weapons( 0 )
  {
    option_t options[] =
    {
      { "sync_weapons", OPT_BOOL, &sync_weapons },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    assert( p -> main_hand_weapon.type != WEAPON_NONE );

    p -> main_hand_attack = new melee_t( "melee_main_hand", p, sync_weapons );
    p -> main_hand_attack -> weapon = &( p -> main_hand_weapon );
    p -> main_hand_attack -> base_execute_time = p -> main_hand_weapon.swing_time;

    if ( p -> off_hand_weapon.type != WEAPON_NONE )
    {
      p -> off_hand_attack = new melee_t( "melee_off_hand", p, sync_weapons );
      p -> off_hand_attack -> weapon = &( p -> off_hand_weapon );
      p -> off_hand_attack -> base_execute_time = p -> off_hand_weapon.swing_time;
    }

    trigger_gcd = timespan_t::zero();
  }

  virtual void execute()
  {
    death_knight_t* p = cast();
    p -> main_hand_attack -> schedule_execute();
    if ( p -> off_hand_attack )
    {
      p -> off_hand_attack -> schedule_execute();
    }
  }

  virtual bool ready()
  {
    death_knight_t* p = cast();
    if ( p -> is_moving() )
      return false;
    return( p -> main_hand_attack -> execute_event == 0 ); // not swinging
  }
};

// ==========================================================================
// Death Knight Abilities
// ==========================================================================

// Army of the Dead =========================================================

struct army_of_the_dead_t : public death_knight_spell_t
{
  army_of_the_dead_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "army_of_the_dead", p, p -> find_class_spell( "Army of the Dead" ) )
  {
    parse_options( NULL, options_str );

    harmful     = false;
    channeled   = true;
    extract_rune_cost( &data(), &cost_blood, &cost_frost, &cost_unholy );
  }

  virtual void execute()
  {
    death_knight_t* p = cast();
    if ( ! p -> in_combat )
    {
      // Pre-casting it before the fight
      int saved_ticks = num_ticks;
      num_ticks = 0;
      channeled = false;
      death_knight_spell_t::execute();
      channeled = true;
      num_ticks = saved_ticks;
      // Because of the new rune regen system in 4.0, it only makes
      // sense to cast ghouls 7-10s before a fight begins so you don't
      // waste rune regen and enter the fight depleted.  So, the time
      // you get for ghouls is 4-6 seconds less.
      p -> pets.army_ghoul -> summon( timespan_t::from_seconds( 35.0 ) );
    }
    else
    {
      death_knight_spell_t::execute();

      p -> pets.army_ghoul -> summon( timespan_t::from_seconds( 40.0 ) );
    }
  }

  virtual bool ready()
  {
    death_knight_t* p = cast();

    if ( p -> pets.army_ghoul && ! p -> pets.army_ghoul -> current.sleeping )
      return false;

    return death_knight_spell_t::ready();
  }
};

// Blood Boil ===============================================================

struct blood_boil_t : public death_knight_spell_t
{
  blood_boil_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "blood_boil", p, p -> find_class_spell( "Blood Boil" ) )
  {
    parse_options( NULL, options_str );

    aoe                = -1;
    extract_rune_cost( &data(), &cost_blood, &cost_frost, &cost_unholy );
    direct_power_mod   = 0.08; // hardcoded into tooltip, 31/10/2011
  }

  virtual void execute()
  {
    death_knight_spell_t::execute();

    death_knight_t* p = cast();

    if ( p -> buffs.dancing_rune_weapon -> check() )
      p -> pets.dancing_rune_weapon -> drw_blood_boil -> execute();

    if ( p -> buffs.crimson_scourge -> up() )
      p -> buffs.crimson_scourge -> expire();
  }

  virtual void target_debuff( player_t* t, dmg_type_e dtype )
  {
    death_knight_spell_t::target_debuff( t, dtype );

    death_knight_td_t* td = cast_td( t );

    base_dd_adder = td -> diseases() ? 95 : 0;
    direct_power_mod = 0.08 + ( td -> diseases() ? 0.035 : 0 );
  }

  virtual bool ready()
  {
    death_knight_t* p = cast();

    if ( ! spell_t::ready() )
      return false;

    if ( ( ! p -> in_combat && ! harmful ) || p -> buffs.crimson_scourge -> check() )
      return group_runes( p, 0, 0, 0, use );
    else
      return group_runes( p, cost_blood, cost_frost, cost_unholy, use );
  }
};

// Blood Plague =============================================================

struct blood_plague_t : public death_knight_spell_t
{
  blood_plague_t( death_knight_t* p ) :
    death_knight_spell_t( "blood_plague", p, p -> find_spell( 59879 ) )
  {
    crit_bonus            = 1.0;
    crit_bonus_multiplier = 1.0;

    base_td          = data().effectN( 1 ).average( p ) * 1.15;
    base_tick_time   = timespan_t::from_seconds( 3.0 );
    tick_may_crit    = true;
    background       = true;
    num_ticks        = 7;
    tick_power_mod   = 0.055 * 1.15;
    dot_behavior     = DOT_REFRESH;
    may_miss         = false;
    may_crit         = false;
    hasted_ticks     = false;
  }

  virtual void impact( player_t* t, result_type_e impact_result, double impact_dmg=0 )
  {
    death_knight_spell_t::impact( t, impact_result, impact_dmg );

    if ( ! sim -> overrides.weakened_blows && player -> primary_tree() == DEATH_KNIGHT_BLOOD &&
         result_is_hit( impact_result ) )
      t -> debuffs.weakened_blows -> trigger();

    if ( ! sim -> overrides.physical_vulnerability && player -> primary_tree() == DEATH_KNIGHT_UNHOLY &&
         result_is_hit( impact_result ) )
      t -> debuffs.physical_vulnerability -> trigger();
  }
};

// Blood Strike =============================================================

struct blood_strike_offhand_t : public death_knight_melee_attack_t
{
  blood_strike_offhand_t( death_knight_t* p ) :
    death_knight_melee_attack_t( "blood_strike_offhand", p, p -> find_spell( 66215 ) )
  {
    special          = true;
    background       = true;
    weapon           = &( p -> off_hand_weapon );
    rp_gain          = 0;
    cost_blood       = 0;
  }

  virtual void target_debuff( player_t* t, dmg_type_e dtype )
  {
    death_knight_melee_attack_t::target_debuff( t, dtype );

    target_multiplier *= 1 + cast_td() -> diseases() * 0.1875; // Currently giving a 18.75% increase per disease instead of expected 12.5
  }
};

struct blood_strike_t : public death_knight_melee_attack_t
{
  melee_attack_t* oh_attack;

  blood_strike_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_melee_attack_t( "blood_strike", p, p -> find_spell( "Blood Strike" ) ), oh_attack( 0 )
  {
    parse_options( NULL, options_str );

    special = true;

    weapon = &( p -> main_hand_weapon );

    extract_rune_cost( &data(), &cost_blood, &cost_frost, &cost_unholy );

    if ( p -> primary_tree() == DEATH_KNIGHT_FROST ||
         p -> primary_tree() == DEATH_KNIGHT_UNHOLY )
      convert_runes = 1.0;

    if ( p -> off_hand_weapon.type != WEAPON_NONE )
      oh_attack = new blood_strike_offhand_t( p );
  }

  virtual void target_debuff( player_t* t, dmg_type_e dtype )
  {
    death_knight_melee_attack_t::target_debuff( t, dtype );

    target_multiplier *= 1 + cast_td() -> diseases() * 0.1875; // Currently giving a 18.75% increase per disease instead of expected 12.5
  }
};

// Blood Tap ================================================================

struct blood_tap_t : public death_knight_spell_t
{
  blood_tap_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "blood_tap", p, p -> find_class_spell( "Blood Tap" ) )
  {
    parse_options( NULL, options_str );

    harmful   = false;
  }

  void execute()
  {
    death_knight_t* p = cast();

    // Blood tap has some odd behavior.  One of the oddest is that, if
    // you have a death rune on cooldown and a full blood rune, using
    // it will take the death rune off cooldown and turn the blood
    // rune into a death rune!  This allows for a nice sequence for
    // Unholy Death Knights: "IT PS BS SS tap SS" which replaces a
    // blood strike with a scourge strike.

    // Find a non-death blood rune and refresh it.
    bool rune_was_refreshed = false;
    for ( int i = 0; i < RUNE_SLOT_MAX; ++i )
    {
      dk_rune_t& r = p -> _runes.slot[i];
      if ( r.get_type() == RUNE_TYPE_BLOOD && ! r.is_death() && ! r.is_ready() )
      {
        p -> gains.blood_tap       -> add( RESOURCE_RUNE, 1 - r.value, r.value );
        r.fill_rune();
        rune_was_refreshed = true;
        break;
      }
    }
    // Couldn't find a non-death blood rune needing refreshed?
    // Instead, refresh a death rune that is a blood rune.
    if ( ! rune_was_refreshed )
    {
      for ( int i = 0; i < RUNE_SLOT_MAX; ++i )
      {
        dk_rune_t& r = p -> _runes.slot[i];
        if ( r.get_type() == RUNE_TYPE_BLOOD && r.is_death() && ! r.is_ready() )
        {
          p -> gains.blood_tap       -> add( RESOURCE_RUNE, 1 - r.value, r.value );
          // p -> gains.blood_tap_blood -> add(1 - r.value, r.value);
          r.fill_rune();
          rune_was_refreshed = true;
          break;
        }
      }
    }

    // Now find a ready blood rune and turn it into a death rune.
    for ( int i = 0; i < RUNE_SLOT_MAX; ++i )
    {
      dk_rune_t& r = p -> _runes.slot[i];
      if ( r.get_type() == RUNE_TYPE_BLOOD && ! r.is_death() && r.is_ready() )
      {
        r.type = ( r.type & RUNE_TYPE_MASK ) | RUNE_TYPE_DEATH;
        break;
      }
    }

    // Called last so we print the correct runes
    death_knight_spell_t::execute();
  }
};

// Bone Shield ==============================================================

struct bone_shield_t : public death_knight_spell_t
{
  bone_shield_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "bone_shield", p, p -> find_class_spell( "Bone Shield" ) )
  {
    parse_options( NULL, options_str );

    extract_rune_cost( &data(), &cost_blood, &cost_frost, &cost_unholy );
    harmful = false;
  }

  virtual void execute()
  {
    death_knight_t* p = cast();
    if ( ! p -> in_combat )
    {
      // Pre-casting it before the fight, perfect timing would be so
      // that the used rune is ready when it is needed in the
      // rotation.  Assume we casted Bone Shield somewhere between
      // 8-16s before we start fighting.  The cost in this case is
      // zero and we don't cause any cooldown.
      timespan_t pre_cast = timespan_t::from_seconds( p -> sim -> range( 8.0, 16.0 ) );

      cooldown -> duration -= pre_cast;
      p -> buffs.bone_shield -> buff_duration -= pre_cast;

      p -> buffs.bone_shield -> trigger( 1, 0.0 ); // FIXME
      death_knight_spell_t::execute();

      cooldown -> duration += pre_cast;
      p -> buffs.bone_shield -> buff_duration += pre_cast;
    }
    else
    {
      p -> buffs.bone_shield -> trigger( 1, 0.0 ); // FIXME
      death_knight_spell_t::execute();
    }
  }
};

// Dancing Rune Weapon ======================================================

struct dancing_rune_weapon_t : public death_knight_spell_t
{
  dancing_rune_weapon_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "dancing_rune_weapon", p, p -> find_class_spell( "Dancing Rune Weapon" ) )
  {
    parse_options( NULL, options_str );
  }

  virtual void execute()
  {
    death_knight_spell_t::execute();

    death_knight_t* p = cast();

    p -> buffs.dancing_rune_weapon -> trigger();
    p -> pets.dancing_rune_weapon -> summon( data().duration() );
  }
};

// Dark Transformation ======================================================

struct dark_transformation_t : public death_knight_spell_t
{
  dark_transformation_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "dark_transformation", p, p -> find_class_spell( "Dark Transformation" ) )
  {
    parse_options( NULL, options_str );

    harmful = false;
    extract_rune_cost( &data(), &cost_blood, &cost_frost, &cost_unholy );
  }

  virtual void execute()
  {
    death_knight_t* p = cast();
    death_knight_spell_t::execute();

    p -> buffs.dark_transformation -> trigger();
    p -> buffs.shadow_infusion -> expire();
  }

  virtual bool ready()
  {
    death_knight_t* p = cast();

    if ( p -> buffs.shadow_infusion -> check() != p -> buffs.shadow_infusion -> max_stack() )
      return false;

    return death_knight_spell_t::ready();
  }
};

// Death and Decay ==========================================================

struct death_and_decay_t : public death_knight_spell_t
{
  death_and_decay_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "death_and_decay", p, p -> find_class_spell( "Death and Decay" ) )
  {
    parse_options( NULL, options_str );

    aoe              = -1;
    extract_rune_cost( &data(), &cost_blood, &cost_frost, &cost_unholy );
    tick_power_mod   = 0.064;
    base_td          = data().effectN( 1 ).average( p );
    base_tick_time   = timespan_t::from_seconds( 1.0 );
    num_ticks        = 10; // 11 with tick_zero
    tick_may_crit    = true;
    tick_zero        = true;
    hasted_ticks     = false;
  }

  virtual void impact( player_t* t, result_type_e impact_result, double impact_dmg=0 )
  {
    if ( t -> debuffs.flying -> check() )
    {
      if ( sim -> debug ) log_t::output( sim, "Ground effect %s can not hit flying target %s", name(), t -> name_str.c_str() );
    }
    else
    {
      death_knight_spell_t::impact( t, impact_result, impact_dmg );
    }
  }
};

// Death Coil ===============================================================

struct death_coil_t : public death_knight_spell_t
{
  death_coil_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "death_coil", p, p -> find_class_spell( "Death Coil" ) )
  {
    parse_options( NULL, options_str );

    direct_power_mod = 0.23;
    base_dd_min      = data().effectN( 1 ).min( p );
    base_dd_max      = data().effectN( 1 ).max( p );
  }

  virtual double cost()
  {
    death_knight_t* p = cast();

    if ( p -> buffs.sudden_doom -> check() ) return 0;

    return death_knight_spell_t::cost();
  }

  void execute()
  {
    death_knight_spell_t::execute();

    death_knight_t* p = cast();

    p -> buffs.sudden_doom -> decrement();

    if ( p -> buffs.dancing_rune_weapon -> check() )
      p -> pets.dancing_rune_weapon -> drw_death_coil -> execute();

    if ( result_is_hit() )
    {
      p -> trigger_runic_empowerment();
    }

    if ( ! p -> buffs.dark_transformation -> check() )
      p -> buffs.shadow_infusion -> trigger(); // Doesn't stack while your ghoul is empowered
  }
};

// Death Strike =============================================================

struct death_strike_offhand_t : public death_knight_melee_attack_t
{
  death_strike_offhand_t( death_knight_t* p ) :
    death_knight_melee_attack_t( "death_strike_offhand", p, p -> find_spell( 66188 ) )
  {
    background       = true;
    weapon           = &( p -> off_hand_weapon );
  }
};

struct death_strike_t : public death_knight_melee_attack_t
{
  melee_attack_t* oh_attack;

  death_strike_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_melee_attack_t( "death_strike", p, p -> find_class_spell( "Death Strike" ) ),
    oh_attack( 0 )
  {
    parse_options( NULL, options_str );
    special = true;

    always_consume = true; // Death Strike always consumes runes, even if doesn't hit

    extract_rune_cost( &data(), &cost_blood, &cost_frost, &cost_unholy );
    if ( p -> primary_tree() == DEATH_KNIGHT_BLOOD )
      convert_runes = 1.0;

    weapon = &( p -> main_hand_weapon );

    if ( p -> off_hand_weapon.type != WEAPON_NONE )
      oh_attack = new death_strike_offhand_t( p );
  }

  virtual void execute()
  {
    death_knight_melee_attack_t::execute();
    death_knight_t* p = cast();

    if ( p -> buffs.dancing_rune_weapon -> check() )
      p -> pets.dancing_rune_weapon -> drw_death_strike -> execute();
  }
};

// Empower Rune Weapon ======================================================

struct empower_rune_weapon_t : public death_knight_spell_t
{
  empower_rune_weapon_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "empower_rune_weapon", p, p -> find_spell( 47568 ) )
  {
    parse_options( NULL, options_str );

    harmful = false;
  }

  virtual void execute()
  {
    death_knight_t* p = cast();
    death_knight_spell_t::execute();

    double erw_gain = 0.0;
    double erw_over = 0.0;
    for ( int i = 0; i < RUNE_SLOT_MAX; ++i )
    {
      dk_rune_t& r = p -> _runes.slot[i];
      erw_gain += 1-r.value;
      erw_over += r.value;
      r.fill_rune();
    }
    p -> gains.empower_rune_weapon -> add( RESOURCE_RUNE, erw_gain, erw_over );
  }
};

// Festering Strike =========================================================

struct festering_strike_t : public death_knight_melee_attack_t
{
  festering_strike_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_melee_attack_t( "festering_strike", p, p -> find_class_spell( "Festering Strike" ) )
  {
    parse_options( NULL, options_str );

    extract_rune_cost( &data(), &cost_blood, &cost_frost, &cost_unholy );
    if ( p -> primary_tree() == DEATH_KNIGHT_UNHOLY )
    {
      convert_runes = 1.0;
    }
  }

  virtual void execute()
  {
    death_knight_melee_attack_t::execute();

    if ( result_is_hit() )
    {
      death_knight_td_t* td = cast_td();
      td -> dots_blood_plague -> extend_duration_seconds( timespan_t::from_seconds( 8 ) );
      td -> dots_frost_fever  -> extend_duration_seconds( timespan_t::from_seconds( 8 ) );
    }
  }
};

// Frost Fever ==============================================================

struct frost_fever_t : public death_knight_spell_t
{
  frost_fever_t( death_knight_t* p ) :
    death_knight_spell_t( "frost_fever", p, p -> find_spell( 59921 ) )
  {
    base_td          = data().effectN( 1 ).average( p ) * 1.15;
    base_tick_time   = timespan_t::from_seconds( 3.0 );
    hasted_ticks     = false;
    may_miss         = false;
    may_crit         = false;
    background       = true;
    tick_may_crit    = true;
    dot_behavior     = DOT_REFRESH;
    num_ticks        = 7;
    tick_power_mod   = 0.055 * 1.15;
  }

  virtual void impact( player_t* t, result_type_e impact_result, double impact_dmg=0 )
  {
    death_knight_spell_t::impact( t, impact_result, impact_dmg );

    if ( ! sim -> overrides.physical_vulnerability && player -> primary_tree() == DEATH_KNIGHT_FROST &&
         result_is_hit( impact_result ) )
      t -> debuffs.physical_vulnerability -> trigger();
  }

};

// Frost Strike =============================================================

struct frost_strike_offhand_t : public death_knight_melee_attack_t
{
  frost_strike_offhand_t( death_knight_t* p ) :
    death_knight_melee_attack_t( "frost_strike_offhand", p, p -> find_spell( 66196 ) )
  {
    background       = true;
    weapon           = &( p -> off_hand_weapon );
    special          = true;

    rp_gain = 0; // Incorrectly set to 10 in the DBC
  }

  virtual void player_buff()
  {
    death_knight_melee_attack_t::player_buff();
    death_knight_t* p = cast();

    player_crit += p -> buffs.killing_machine -> value();
  }
};

struct frost_strike_t : public death_knight_melee_attack_t
{
  melee_attack_t* oh_attack;

  frost_strike_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_melee_attack_t( "frost_strike", p, p -> find_class_spell( "Frost Strike" ) ), oh_attack( 0 )
  {
    special = true;

    parse_options( NULL, options_str );

    weapon     = &( p -> main_hand_weapon );

    if ( p -> off_hand_weapon.type != WEAPON_NONE )
      oh_attack = new frost_strike_offhand_t( p );
  }

  virtual void execute()
  {
    death_knight_t* p = cast();
    death_knight_melee_attack_t::execute();

    if ( result_is_hit() )
      p -> trigger_runic_empowerment();

    if ( p -> buffs.killing_machine -> check() )
      p -> procs.fs_killing_machine -> occur();

    p -> buffs.killing_machine -> expire();
  }

  virtual void player_buff()
  {
    death_knight_melee_attack_t::player_buff();
    death_knight_t* p = cast();

    player_crit += p -> buffs.killing_machine -> value();
  }

};

// Heart Strike =============================================================

struct heart_strike_t : public death_knight_melee_attack_t
{
  heart_strike_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_melee_attack_t( "heart_strike", p, p -> find_class_spell( "Heart Strike" ) )
  {
    parse_options( NULL, options_str );

    special = true;

    extract_rune_cost( &data(), &cost_blood, &cost_frost, &cost_unholy );

    aoe = 2;
    base_add_multiplier *= 0.75;
  }

  void execute()
  {
    death_knight_melee_attack_t::execute();
    death_knight_t* p = cast();

    if ( result_is_hit() )
    {
      if ( p -> buffs.dancing_rune_weapon -> check() )
      {
        p -> pets.dancing_rune_weapon -> drw_heart_strike -> execute();
      }
    }
  }

  void target_debuff( player_t* t, dmg_type_e dtype )
  {
    death_knight_melee_attack_t::target_debuff( t, dtype );

    target_multiplier *= 1 + cast_td() -> diseases() * data().effectN( 3 ).percent();
  }
};

// Horn of Winter============================================================

struct horn_of_winter_t : public death_knight_spell_t
{
  horn_of_winter_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "horn_of_winter", p, p -> find_spell( 57330 ) )
  {
    parse_options( NULL, options_str );

    harmful = false;
  }

  virtual void execute()
  {
    if ( sim -> log )
      log_t::output( sim, "%s performs %s", player -> name(), name() );

    update_ready();

    death_knight_t* p = cast();
    if ( ! sim -> overrides.attack_power_multiplier )
      sim -> auras.attack_power_multiplier -> trigger( 1, -1.0, -1.0, data().duration() );

    player -> resource_gain( RESOURCE_RUNIC_POWER, 10, p -> gains.horn_of_winter );
  }
};

// Howling Blast ============================================================

// FIXME: -3% spell crit suppression? Seems to have the same crit chance as FS in the absence of KM
struct howling_blast_t : public death_knight_spell_t
{
  howling_blast_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "howling_blast", p, p -> find_class_spell( "Howling Blast" ) )
  {
    parse_options( NULL, options_str );

    extract_rune_cost( &data() , &cost_blood, &cost_frost, &cost_unholy );
    aoe                 = -1;
    base_aoe_multiplier = data().effectN( 3 ).percent(); // Only 50% of the direct damage is done for AoE
    direct_power_mod    = 0.4;

    assert( p -> active_spells.frost_fever );
  }

  virtual void consume_resource() {}

  virtual double cost()
  {
    // Rime also prevents getting RP because there are no runes used!
    death_knight_t* p = cast();
    if ( p -> buffs.rime -> check() )
      return 0;
    return death_knight_spell_t::cost();
  }

  virtual void execute()
  {
    death_knight_t* p = cast();

    if ( ! p -> buffs.rime -> up() )
    {
      // We only consume resources when rime is not up
      // Rime procs generate no RP from rune abilites, which is handled in consume_resource as well
      death_knight_spell_t::consume_resource();
    }

    death_knight_spell_t::execute();

    if ( result_is_hit() )
    {
    }

    p -> buffs.rime -> decrement();
  }

  virtual void impact( player_t* t, result_type_e impact_result, double impact_dmg=0 )
  {
    death_knight_spell_t::impact( t, impact_result, impact_dmg );

    if ( result_is_hit( result ) )
    {
    }
  }

  virtual bool ready()
  {
    death_knight_t* p = cast();

    if ( p -> buffs.rime -> check() )
    {
      // If Rime is up, runes are no restriction.
      cost_frost  = 0;
      bool rime_ready = death_knight_spell_t::ready();
      cost_frost  = 1;
      return rime_ready;
    }
    return death_knight_spell_t::ready();
  }
};

// Icy Touch ================================================================

struct icy_touch_t : public death_knight_spell_t
{
  icy_touch_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "icy_touch", p, p -> find_class_spell( "Icy Touch" ) )
  {
    parse_options( NULL, options_str );

    extract_rune_cost( &data(), &cost_blood, &cost_frost, &cost_unholy );
    direct_power_mod = 0.2;

    assert( p -> active_spells.frost_fever );
  }

  virtual void consume_resource() {}

  virtual double cost()
  {
    // Rime also prevents getting RP because there are no runes used!
    death_knight_t* p = cast();
    if ( p -> buffs.rime -> check() )
      return 0;
    return death_knight_spell_t::cost();
  }

  virtual void execute()
  {
    death_knight_t* p = cast();
    if ( ! p -> buffs.rime -> up() )
    {
      // We only consume resources when rime is not up
      // Rime procs generate no RP from rune abilites, which is handled in consume_resource as well
      death_knight_spell_t::consume_resource();
    }

    death_knight_spell_t::execute();

    if ( p -> buffs.dancing_rune_weapon -> check() )
      p -> pets.dancing_rune_weapon -> drw_icy_touch -> execute();

    if ( result_is_hit() )
    {
      p -> active_spells.frost_fever -> execute();
    }

    p -> buffs.rime -> decrement();
  }

  virtual bool ready()
  {
    death_knight_t* p = cast();

    if ( p -> buffs.rime -> check() )
    {
      // If Rime is up, runes are no restriction.
      cost_frost  = 0;
      bool rime_ready = death_knight_spell_t::ready();
      cost_frost  = 1;
      return rime_ready;
    }
    return death_knight_spell_t::ready();
  }
};

// Mind Freeze ==============================================================

struct mind_freeze_t : public death_knight_spell_t
{
  mind_freeze_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "mind_freeze", p, p -> find_class_spell( "Mind Freeze" ) )
  {
    parse_options( NULL, options_str );

    may_miss = may_glance = may_block = may_dodge = may_parry = may_crit = false;
  }

  virtual bool ready()
  {
    if ( ! target -> debuffs.casting -> check() )
      return false;

    return death_knight_spell_t::ready();
  }
};

// Necrotic Strike ==========================================================

struct necrotic_strike_t : public death_knight_melee_attack_t
{
  necrotic_strike_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_melee_attack_t( "necrotic_strike", p, p -> find_class_spell( "Necrotic Strike" ) )
  {
    parse_options( NULL, options_str );

    special = true;

    extract_rune_cost( &data(), &cost_blood, &cost_frost, &cost_unholy );
  }

  virtual void impact( player_t* t, result_type_e impact_result, double impact_dmg=0 )
  {
    death_knight_melee_attack_t::impact( t, impact_result, impact_dmg );

    if ( ! sim -> overrides.slowed_casting && result_is_hit( result ) )
      t -> debuffs.slowed_casting -> trigger();
  }
};

// Obliterate ===============================================================

struct obliterate_offhand_t : public death_knight_melee_attack_t
{
  obliterate_offhand_t( death_knight_t* p ) :
    death_knight_melee_attack_t( "obliterate_offhand", p, p -> find_spell( 66198 ) )
  {
    background       = true;
    weapon           = &( p -> off_hand_weapon );
    special          = true;

    // These both stack additive with MOTFW
    // http://elitistjerks.com/f72/t110296-frost_dps_cataclysm_4_0_6_my_life/p14/#post1886388
  }

  virtual void player_buff()
  {
    death_knight_t* p = cast();

    death_knight_melee_attack_t::player_buff();

    player_crit += p -> buffs.killing_machine -> value();
  }

  virtual void target_debuff( player_t* t, dmg_type_e dtype )
  {
    death_knight_melee_attack_t::target_debuff( t, dtype );

    target_multiplier *= 1 + cast_td() -> diseases() * data().effectN( 3 ).percent() / 2.0;
  }
};

struct obliterate_t : public death_knight_melee_attack_t
{
  melee_attack_t* oh_attack;

  obliterate_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_melee_attack_t( "obliterate", p, p -> find_class_spell( "Obliterate" ) ), oh_attack( 0 )
  {
    parse_options( NULL, options_str );

    special = true;

    weapon = &( p -> main_hand_weapon );

    extract_rune_cost( &data(), &cost_blood, &cost_frost, &cost_unholy );
    if ( p -> primary_tree() == DEATH_KNIGHT_BLOOD )
      convert_runes = 1.0;

    // These both stack additive with MOTFW
    // http://elitistjerks.com/f72/t110296-frost_dps_cataclysm_4_0_6_my_life/p14/#post1886388

    if ( p -> off_hand_weapon.type != WEAPON_NONE )
      oh_attack = new obliterate_offhand_t( p );
  }

  virtual void execute()
  {
    death_knight_t* p = cast();
    death_knight_melee_attack_t::execute();

    if ( result_is_hit() )
    {

      // T13 2pc gives 2 stacks of Rime, otherwise we can only ever have one
      // Ensure that if we have 1 that we only refresh, not add another stack
      int new_stacks = ( p -> set_bonus.tier13_2pc_melee() && sim -> roll( p -> sets -> set( SET_T13_2PC_MELEE ) -> effect2().percent() ) ) ? 2 : 1;
      ( void )new_stacks;
    }

    if ( p -> buffs.killing_machine -> check() )
      p -> procs.oblit_killing_machine -> occur();

    p -> buffs.killing_machine -> expire();
  }

  virtual void player_buff()
  {
    death_knight_t* p = cast();

    death_knight_melee_attack_t::player_buff();

    player_crit += p -> buffs.killing_machine -> value();
  }

  virtual void target_debuff( player_t* t, dmg_type_e dtype )
  {
    death_knight_melee_attack_t::target_debuff( t, dtype );

    target_multiplier *= 1 + cast_td() -> diseases() * data().effectN( 3 ).percent() / 2.0;
  }
};

// Outbreak =================================================================

struct outbreak_t : public death_knight_spell_t
{
  outbreak_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "outbreak", p, p -> find_class_spell( "Outbreak" ) )
  {
    parse_options( NULL, options_str );

    may_crit = false;

    cooldown -> duration += p -> spells.veteran_of_the_third_war -> effect3().time_value();

    assert( p -> active_spells.blood_plague );
    assert( p -> active_spells.frost_fever );
  }

  virtual void execute()
  {
    death_knight_spell_t::execute();
    death_knight_t* p = cast();

    if ( result_is_hit() )
    {
      p -> active_spells.blood_plague -> execute();
      p -> active_spells.frost_fever -> execute();
    }
  }
};

// Pestilence ===============================================================

struct pestilence_t : public death_knight_spell_t
{
  double multiplier;
  int spread_bp, spread_ff;
  player_t* source;

  pestilence_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "pestilence", p, p -> find_class_spell( "Pestilence" ) ),
    multiplier( 0 ), spread_bp( 0 ), spread_ff( 0 ), source( 0 )
  {
    parse_options( NULL, options_str );

    extract_rune_cost( &data(), &cost_blood, &cost_frost, &cost_unholy );

    if ( p -> primary_tree() == DEATH_KNIGHT_FROST ||
         p -> primary_tree() == DEATH_KNIGHT_UNHOLY )
      convert_runes = 1.0;

    aoe = -1;

    multiplier = data().effectN( 2 ).base_value();
    source = target;
  }

  virtual void execute()
  {
    // See which diseases we can spread
    death_knight_td_t* td = cast_td();

    spread_bp = td -> dots_blood_plague -> ticking;
    spread_ff = td -> dots_frost_fever -> ticking;

    death_knight_spell_t::execute();
    death_knight_t* p = cast();

    if ( p -> buffs.dancing_rune_weapon -> check() )
      p -> pets.dancing_rune_weapon -> drw_pestilence -> execute();
  }

  virtual void impact( player_t* t, result_type_e impact_result, double impact_dmg=0 )
  {
    death_knight_spell_t::impact( t, impact_result, impact_dmg );

    // Doesn't affect the original target
    if ( t == source )
      return;

    if ( result_is_hit( result ) )
    {
      // FIXME: if the source of the dot was pestilence, the multiplier doesn't change
      // Not sure how to support that
      death_knight_t* p = cast();
      if ( spread_bp )
      {
        p -> active_spells.blood_plague -> target = t;
        p -> active_spells.blood_plague -> player_multiplier *= multiplier;
        p -> active_spells.blood_plague -> execute();
      }
      if ( spread_ff )
      {
        p -> active_spells.frost_fever -> target = t;
        p -> active_spells.frost_fever -> player_multiplier *= multiplier;
        p -> active_spells.frost_fever -> execute();
      }
    }
  }

  virtual bool ready()
  {
    death_knight_td_t* td = cast_td();

    // BP or FF must be ticking to use
    if ( ( td -> dots_blood_plague && td -> dots_blood_plague -> ticking ) ||
         ( td -> dots_frost_fever && td -> dots_frost_fever -> ticking ) )
      return death_knight_spell_t::ready();

    return false;
  }
};

// Pillar of Frost ==========================================================

struct pillar_of_frost_t : public death_knight_spell_t
{
  pillar_of_frost_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "pillar_of_frost", p, p -> find_class_spell( "Pillar of Frost" ) )
  {
    parse_options( NULL, options_str );

    extract_rune_cost( &data(), &cost_blood, &cost_frost, &cost_unholy );

    harmful = false;
  }

  bool ready()
  {
    if ( ! spell_t::ready() )
      return false;

    // Always activate runes, even pre-combat.
    return group_runes( cast(), cost_blood, cost_frost, cost_unholy, use );
  }
};

// Plague Strike ============================================================

struct plague_strike_offhand_t : public death_knight_melee_attack_t
{
  plague_strike_offhand_t( death_knight_t* p ) :
    death_knight_melee_attack_t( "plague_strike_offhand", p, p -> find_spell( 66216 ) )
  {
    background       = true;
    weapon           = &( p -> off_hand_weapon );
    special          = true;

    assert( p -> active_spells.blood_plague );
  }

  virtual void execute()
  {
    death_knight_t* p = cast();
    death_knight_melee_attack_t::execute();

    if ( result_is_hit() )
    {
      p -> active_spells.blood_plague -> execute();
    }
  }
};

struct plague_strike_t : public death_knight_melee_attack_t
{
  melee_attack_t* oh_attack;

  plague_strike_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_melee_attack_t( "plague_strike", p, p -> find_class_spell( "Plague Strike" ) ), oh_attack( 0 )
  {
    parse_options( NULL, options_str );

    special = true;

    extract_rune_cost( &data(), &cost_blood, &cost_frost, &cost_unholy );
    weapon = &( p -> main_hand_weapon );

    if ( p -> off_hand_weapon.type != WEAPON_NONE )
      oh_attack = new plague_strike_offhand_t( p );

    assert( p -> active_spells.blood_plague );
  }

  virtual void execute()
  {
    death_knight_t* p = cast();
    death_knight_melee_attack_t::execute();

    if ( p -> buffs.dancing_rune_weapon -> check() )
      p -> pets.dancing_rune_weapon -> drw_plague_strike -> execute();

    if ( result_is_hit() )
    {
      p -> active_spells.blood_plague -> execute();
    }
  }
};

// Presence =================================================================

struct presence_t : public death_knight_spell_t
{
  int switch_to_presence;
  presence_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "presence", p ), switch_to_presence( 0 )
  {
    std::string presence_str;
    option_t options[] =
    {
      { "choose",  OPT_STRING, &presence_str },
      { NULL,     OPT_UNKNOWN, NULL          }
    };
    parse_options( options, options_str );

    if ( ! presence_str.empty() )
    {
      if ( presence_str == "blood" || presence_str == "bp" )
      {
        switch_to_presence = PRESENCE_BLOOD;
      }
      else if ( presence_str == "frost" || presence_str == "fp" )
      {
        switch_to_presence = PRESENCE_FROST;
      }
      else if ( presence_str == "unholy" || presence_str == "up" )
      {
        switch_to_presence = PRESENCE_UNHOLY;
      }
    }
    else
    {
      // Default to Frost Presence
      switch_to_presence = PRESENCE_FROST;
    }

    trigger_gcd = timespan_t::zero();
    cooldown -> duration = timespan_t::from_seconds( 1.0 );
    harmful     = false;
  }

  virtual resource_type_e current_resource()
  {
    return RESOURCE_RUNIC_POWER;
  }

  virtual double cost()
  {
    death_knight_t* p = cast();

    // Presence changes consume all runic power
    return p -> resources.current [ RESOURCE_RUNIC_POWER ];
  }

  virtual void execute()
  {
    death_knight_t* p = cast();

    p -> base_gcd = timespan_t::from_seconds( 1.50 );

    switch ( p -> active_presence )
    {
    case PRESENCE_BLOOD:  p -> buffs.blood_presence  -> expire(); break;
    case PRESENCE_FROST:  p -> buffs.frost_presence  -> expire(); break;
    case PRESENCE_UNHOLY: p -> buffs.unholy_presence -> expire(); break;
    }
    p -> active_presence = switch_to_presence;

    switch ( p -> active_presence )
    {
    case PRESENCE_BLOOD:
      p -> buffs.blood_presence  -> trigger();
      break;
    case PRESENCE_FROST:
    {
      double fp_value = p -> dbc.spell( 48266 ) -> effect1().percent();
      p -> buffs.frost_presence -> trigger( 1, fp_value );
    }
    break;
    case PRESENCE_UNHOLY:
      p -> base_gcd = timespan_t::from_seconds( 1.0 );
      break;
    }

    p -> reset_gcd();

    consume_resource();
    update_ready();
    if ( sim -> log ) log_t::output( sim, "%s performs %s", player -> name(),name() );
  }

  virtual bool ready()
  {
    death_knight_t* p = cast();

    if ( p -> active_presence == switch_to_presence )
      return false;

    return death_knight_spell_t::ready();
  }
};

// Raise Dead ===============================================================

struct raise_dead_t : public death_knight_spell_t
{
  raise_dead_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "raise_dead", p, p -> find_class_spell( "Raise Dead" ) )
  {
    parse_options( NULL, options_str );

    cooldown -> duration += p -> spells.master_of_ghouls -> effect1().time_value();

    harmful = false;
  }

  virtual void execute()
  {
    death_knight_spell_t::execute();
    death_knight_t* p = cast();

    p -> pets.ghoul -> summon( ( p -> primary_tree() == DEATH_KNIGHT_UNHOLY ) ? timespan_t::zero() : p -> dbc.spell( data().effect1().base_value() ) -> duration() );
  }

  virtual bool ready()
  {
    death_knight_t* p = cast();

    if ( p -> pets.ghoul && ! p -> pets.ghoul -> current.sleeping )
      return false;

    return death_knight_spell_t::ready();
  }
};

// Rune Strike ==============================================================

struct rune_strike_offhand_t : public death_knight_melee_attack_t
{
  rune_strike_offhand_t( death_knight_t* p ) :
    death_knight_melee_attack_t( "rune_strike_offhand", p, p -> find_spell( 66217 ) )
  {
    background       = true;
    weapon           = &( p -> off_hand_weapon );
    special          = true;

    direct_power_mod = 0.15;
    may_dodge = may_block = may_parry = false;
  }
};

struct rune_strike_t : public death_knight_melee_attack_t
{
  melee_attack_t* oh_attack;

  rune_strike_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_melee_attack_t( "rune_strike", p, p -> find_class_spell( "Rune Strike" ) ),
    oh_attack( 0 )
  {
    parse_options( NULL, options_str );

    special          = true;

    direct_power_mod = 0.15;
    may_dodge = may_block = may_parry = false;

    weapon = &( p -> main_hand_weapon );

    if ( p -> off_hand_weapon.type != WEAPON_NONE )
      oh_attack = new rune_strike_offhand_t( p );
  }

  virtual void execute()
  {
    death_knight_melee_attack_t::execute();
    death_knight_t* p = cast();

    p -> buffs.rune_strike -> expire();

    if ( result_is_hit() )
      p -> trigger_runic_empowerment();
  }

  virtual bool ready()
  {
    death_knight_t* p = cast();

    if ( ! p -> buffs.blood_presence -> check() || p -> buffs.rune_strike -> check() )
      return false;

    return death_knight_melee_attack_t::ready();
  }
};

// Scourge Strike ===========================================================

struct scourge_strike_t : public death_knight_melee_attack_t
{
  spell_t* scourge_strike_shadow;

  struct scourge_strike_shadow_t : public death_knight_spell_t
  {
    scourge_strike_shadow_t( death_knight_t* p ) :
      death_knight_spell_t( "scourge_strike_shadow", p, p -> find_spell( 70890 ) )
    {
      check_spec( DEATH_KNIGHT_UNHOLY );

      special           = true;
      weapon = &( player -> main_hand_weapon );
      may_miss = may_parry = may_dodge = false;
      may_crit          = false;
      proc              = true;
      background        = true;
      weapon_multiplier = 0;
    }

    virtual void target_debuff( player_t* t, dmg_type_e )
    {
      // Shadow portion doesn't double dips in debuffs, other than EP/E&M/CoE below
      // death_knight_spell_t::target_debuff( t, dmg_type_e );

      target_multiplier = cast_td() -> diseases() * 0.18;

      if ( t -> debuffs.magic_vulnerability -> check() )
        target_multiplier *= 1.0 + t -> debuffs.magic_vulnerability -> value();
    }
  };

  scourge_strike_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_melee_attack_t( "scourge_strike", p, p -> find_class_spell( "Scourge Strike" ) ),
    scourge_strike_shadow( 0 )
  {
    parse_options( NULL, options_str );

    special = true;

    scourge_strike_shadow = new scourge_strike_shadow_t( p );
    extract_rune_cost( &data(), &cost_blood, &cost_frost, &cost_unholy );
  }

  void execute()
  {
    death_knight_melee_attack_t::execute();
    if ( result_is_hit() )
    {
      // We divide out our composite_player_multiplier here because we
      // don't want to double dip; in particular, 3% damage from ret
      // paladins, arcane mages, and beastmaster hunters do not affect
      // scourge_strike_shadow.
      double modified_dd = direct_dmg / player -> player_t::composite_player_multiplier( SCHOOL_SHADOW, this );
      scourge_strike_shadow -> base_dd_max = scourge_strike_shadow -> base_dd_min = modified_dd;
      scourge_strike_shadow -> execute();
    }
  }
};

// Summon Gargoyle ==========================================================

struct summon_gargoyle_t : public death_knight_spell_t
{
  summon_gargoyle_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "summon_gargoyle", p, p -> find_class_spell( "Summon Gargoyle" ) )
  {
    rp_gain = 0.0;  // For some reason, the inc file thinks we gain RP for this spell
    parse_options( NULL, options_str );

    harmful = false;
    num_ticks = 0;
    base_tick_time = timespan_t::zero();
  }

  virtual void execute()
  {
    death_knight_spell_t::execute();
    death_knight_t* p = cast();

    p -> pets.gargoyle -> summon( p -> dbc.spell( data().effectN( 3 ).trigger_spell_id() ) -> duration() );
  }
};

// Unholy Blight ====================================================

struct unholy_blight_t : public death_knight_spell_t
{
  unholy_blight_t( death_knight_t* p ) :
    death_knight_spell_t( "unholy_blight", p, p -> find_talent_spell( "Unholy Blight" ) )
  {
    base_tick_time = timespan_t::from_seconds( 1.0 );
    num_ticks      = 10;
    background     = true;
    proc           = true;
    may_crit       = false;
    may_miss       = false;
    hasted_ticks   = false;
  }

  void target_debuff( player_t*, dmg_type_e )
  {
    // no debuff effect
  }

  void player_buff()
  {
    // no buffs
  }
};

// Unholy Frenzy ============================================================

struct unholy_frenzy_t : public spell_t
{
  unholy_frenzy_t( death_knight_t* p, const std::string& options_str ) :
    spell_t( "unholy_frenzy", p, p -> find_class_spell( "Unholy Frenzy" ) )
  {
    std::string target_str = p -> unholy_frenzy_target_str;
    parse_options( NULL, options_str );

    // If we don't specify a target, it's defaulted to the mob, so default to the player instead
    if ( target -> is_enemy() || target -> is_add() )
    {
      target = p;
    }

    harmful = false;
    trigger_gcd = timespan_t::zero();
  }

  virtual void execute()
  {
    spell_t::execute();
    target -> buffs.unholy_frenzy -> trigger( 1 );
  }
};

} // ANONYMOUS NAMESPACE ====================================================

// ==========================================================================
// Death Knight Character Definition
// ==========================================================================

// death_knight_t::create_action  ===========================================

action_t* death_knight_t::create_action( const std::string& name, const std::string& options_str )
{
  // General Actions
  if ( name == "auto_attack"              ) return new auto_attack_t              ( this, options_str );
  if ( name == "presence"                 ) return new presence_t                 ( this, options_str );

  // Blood Actions
  if ( name == "blood_boil"               ) return new blood_boil_t               ( this, options_str );
  if ( name == "blood_strike"             ) return new blood_strike_t             ( this, options_str );
  if ( name == "blood_tap"                ) return new blood_tap_t                ( this, options_str );
  if ( name == "dancing_rune_weapon"      ) return new dancing_rune_weapon_t      ( this, options_str );
  if ( name == "heart_strike"             ) return new heart_strike_t             ( this, options_str );
  if ( name == "pestilence"               ) return new pestilence_t               ( this, options_str );

  // Frost Actions
  if ( name == "empower_rune_weapon"      ) return new empower_rune_weapon_t      ( this, options_str );
  if ( name == "frost_strike"             ) return new frost_strike_t             ( this, options_str );
  if ( name == "horn_of_winter"           ) return new horn_of_winter_t           ( this, options_str );
  if ( name == "howling_blast"            ) return new howling_blast_t            ( this, options_str );
  if ( name == "icy_touch"                ) return new icy_touch_t                ( this, options_str );
  if ( name == "mind_freeze"              ) return new mind_freeze_t              ( this, options_str );
  if ( name == "obliterate"               ) return new obliterate_t               ( this, options_str );
  if ( name == "pillar_of_frost"          ) return new pillar_of_frost_t          ( this, options_str );
  if ( name == "rune_strike"              ) return new rune_strike_t              ( this, options_str );

  // Unholy Actions
  if ( name == "army_of_the_dead"         ) return new army_of_the_dead_t         ( this, options_str );
  if ( name == "bone_shield"              ) return new bone_shield_t              ( this, options_str );
  if ( name == "dark_transformation"      ) return new dark_transformation_t      ( this, options_str );
  if ( name == "death_and_decay"          ) return new death_and_decay_t          ( this, options_str );
  if ( name == "death_coil"               ) return new death_coil_t               ( this, options_str );
  if ( name == "death_strike"             ) return new death_strike_t             ( this, options_str );
  if ( name == "festering_strike"         ) return new festering_strike_t         ( this, options_str );
  if ( name == "outbreak"                 ) return new outbreak_t                 ( this, options_str );
  if ( name == "necrotic_strike"          ) return new necrotic_strike_t          ( this, options_str );
  if ( name == "plague_strike"            ) return new plague_strike_t            ( this, options_str );
  if ( name == "raise_dead"               ) return new raise_dead_t               ( this, options_str );
  if ( name == "scourge_strike"           ) return new scourge_strike_t           ( this, options_str );
  if ( name == "summon_gargoyle"          ) return new summon_gargoyle_t          ( this, options_str );
  if ( name == "unholy_frenzy"            ) return new unholy_frenzy_t            ( this, options_str );

  return player_t::create_action( name, options_str );
}

// death_knight_t::create_expression ========================================

expr_t* death_knight_t::create_expression( action_t* a, const std::string& name_str )
{
  std::vector<std::string> splits;
  int num_splits = util::string_split( splits, name_str, "." );

  if ( util::str_compare_ci( splits[ 0 ], "rune" ) )
  {
    rune_type rt = RUNE_TYPE_NONE;
    bool include_death = true; // whether to include death runes
    switch ( splits[ 1 ][0] )
    {
    case 'B': include_death = false;
    case 'b': rt = RUNE_TYPE_BLOOD; break;
    case 'U': include_death = false;
    case 'u': rt = RUNE_TYPE_UNHOLY; break;
    case 'F': include_death = false;
    case 'f': rt = RUNE_TYPE_FROST; break;
    case 'D': include_death = false;
    case 'd': rt = RUNE_TYPE_DEATH; break;
    }
    int position = 0; // any
    switch ( splits[1][splits[1].size()-1] )
    {
    case '1': position = 1; break;
    case '2': position = 2; break;
    }

    int act = 0;
    if ( num_splits == 3 && util::str_compare_ci( splits[ 2 ], "cooldown_remains" ) )
      act = 1;
    else if ( num_splits == 3 && util::str_compare_ci( splits[ 2 ], "cooldown_remains_all" ) )
      act = 2;
    else if ( num_splits == 3 && util::str_compare_ci( splits[ 2 ], "depleted" ) )
      act = 3;

    struct rune_inspection_expr_t : public expr_t
    {
      death_knight_t* dk;
      rune_type r;
      bool include_death;
      int position;
      int myaction; // -1 count, 0 cooldown remains, 1 cooldown_remains_all

      rune_inspection_expr_t( death_knight_t* p, rune_type r, bool include_death, int position, int myaction )
        : expr_t( "rune_evaluation" ), dk( p ), r( r ),
          include_death( include_death ), position( position ), myaction( myaction )
      { }

      virtual double evaluate()
      {
        switch ( myaction )
        {
        case 0: return dk -> runes_count( r, include_death, position );
        case 1: return dk -> runes_cooldown_any( r, include_death, position );
        case 2: return dk -> runes_cooldown_all( r, include_death, position );
        case 3: return dk -> runes_depleted( r, position );
        }
        return 0.0;
      }
    };
    return new rune_inspection_expr_t( this, rt, include_death, position, act );
  }
  else if ( num_splits == 2 )
  {
    rune_type rt = RUNE_TYPE_NONE;
    if ( util::str_compare_ci( splits[ 0 ], "blood" ) || util::str_compare_ci( splits[ 0 ], "b" ) )
      rt = RUNE_TYPE_BLOOD;
    else if ( util::str_compare_ci( splits[ 0 ], "frost" ) || util::str_compare_ci( splits[ 0 ], "f" ) )
      rt = RUNE_TYPE_FROST;
    else if ( util::str_compare_ci( splits[ 0 ], "unholy" ) || util::str_compare_ci( splits[ 0 ], "u" ) )
      rt = RUNE_TYPE_UNHOLY;
    else if ( util::str_compare_ci( splits[ 0 ], "death" ) || util::str_compare_ci( splits[ 0 ], "d" ) )
      rt = RUNE_TYPE_DEATH;

    if ( rt != RUNE_TYPE_NONE && util::str_compare_ci( splits[ 1 ], "cooldown_remains" ) )
    {
      struct rune_cooldown_expr_t : public expr_t
      {
        death_knight_t* dk;
        rune_type r;

        rune_cooldown_expr_t( death_knight_t* p, rune_type r ) :
          expr_t( "rune_cooldown_remains" ),
          dk( p ), r( r ) {}
        virtual double evaluate()
        {
          return dk -> runes_cooldown_any( r, true, 0 );
        }
      };

      return new rune_cooldown_expr_t( this, rt );
    }
  }
  else
  {
    rune_type rt = RUNE_TYPE_NONE;
    if ( util::str_compare_ci( splits[ 0 ], "blood" ) || util::str_compare_ci( splits[ 0 ], "b" ) )
      rt = RUNE_TYPE_BLOOD;
    else if ( util::str_compare_ci( splits[ 0 ], "frost" ) || util::str_compare_ci( splits[ 0 ], "f" ) )
      rt = RUNE_TYPE_FROST;
    else if ( util::str_compare_ci( splits[ 0 ], "unholy" ) || util::str_compare_ci( splits[ 0 ], "u" ) )
      rt = RUNE_TYPE_UNHOLY;
    else if ( util::str_compare_ci( splits[ 0 ], "death" ) || util::str_compare_ci( splits[ 0 ], "d" ) )
      rt = RUNE_TYPE_DEATH;

    struct rune_expr_t : public expr_t
    {
      death_knight_t* dk;
      rune_type r;
      rune_expr_t( death_knight_t* p, rune_type r ) :
        expr_t( "rune" ), dk( p ), r( r ) { }
      virtual double evaluate()
      {
        return dk -> runes_count( r, false, 0 );
      }
    };
    if ( rt ) return new rune_expr_t( this, rt );

    if ( name_str == "inactive_death" )
    {
      struct death_expr_t : public expr_t
      {
        death_knight_t* dk;
        std::string name;
        death_expr_t( death_knight_t* p, const std::string name_in ) :
          expr_t( name_in ), dk( p ), name( name_in ) { }
        virtual double evaluate()
        {
          return count_death_runes( dk, name == "inactive_death" );
        }
      };
      return new death_expr_t( this, name_str );
    }
  }

  return player_t::create_expression( a, name_str );
}

// death_knight_t::create_pets ==============================================

void death_knight_t::create_pets()
{
  pets.army_ghoul           = create_pet( "army_of_the_dead" );
  pets.bloodworms           = create_pet( "bloodworms" );
  pets.dancing_rune_weapon  = new dancing_rune_weapon_pet_t ( sim, this );
  pets.gargoyle             = create_pet( "gargoyle" );
  pets.ghoul                = create_pet( "ghoul" );
}

// death_knight_t::create_pet ===============================================

pet_t* death_knight_t::create_pet( const std::string& pet_name,
                                   const std::string& /* pet_type */ )
{
  pet_t* p = find_pet( pet_name );

  if ( p ) return p;

  if ( pet_name == "army_of_the_dead"         ) return new army_ghoul_pet_t          ( sim, this );
  if ( pet_name == "bloodworms"               ) return new bloodworms_pet_t          ( sim, this );
  if ( pet_name == "gargoyle"                 ) return new gargoyle_pet_t            ( sim, this );
  if ( pet_name == "ghoul"                    ) return new ghoul_pet_t               ( sim, this );

  return 0;
}

// death_knight_t::composite_attack_haste() =================================

double death_knight_t::composite_attack_haste()
{
  double haste = player_t::composite_attack_haste();

  haste *= 1.0 / ( 1.0 + buffs.unholy_presence -> value() );

  return haste;
}

// death_knight_t::composite_attack_hit() ===================================

double death_knight_t::composite_attack_hit()
{
  double hit = player_t::composite_attack_hit();

  // Factor in the hit from NoCS here so it shows up in the report, to match the character sheet
  if ( main_hand_weapon.group() == WEAPON_1H || off_hand_weapon.group() == WEAPON_1H )
  {
  }

  return hit;
}
// death_knight_t::init =====================================================

void death_knight_t::init()
{
  player_t::init();

  if ( ( primary_tree() == DEATH_KNIGHT_FROST ) )
  {
    for ( int i = 0; i < RUNE_SLOT_MAX; ++i )
    {
      if ( _runes.slot[i].type == RUNE_TYPE_BLOOD )
      {
        _runes.slot[i].make_permanent_death_rune();
      }
    }
  }
}

// death_knight_t::init_rng =================================================

void death_knight_t::init_rng()
{
  player_t::init_rng();

  rngs.blood_caked_blade          = get_rng( "blood_caked_blade"          );
  rngs.might_of_the_frozen_wastes = get_rng( "might_of_the_frozen_wastes" );
  rngs.threat_of_thassarian       = get_rng( "threat_of_thassarian"       );
}

// death_knight_t::init_defense =============================================

void death_knight_t::init_defense()
{
  player_t::init_defense();

  initial.parry_rating_per_strength = 0.27;
}

// death_knight_t::init_base ================================================

void death_knight_t::init_base()
{
  player_t::init_base();

  double str_mult = 0.0;

  str_mult += spells.unholy_might -> effect1().percent();

  initial.attribute_multiplier[ ATTR_STRENGTH ] *= 1.0 + str_mult;

  initial.attribute_multiplier[ ATTR_STAMINA ]  *= 1.0 + spells.veteran_of_the_third_war -> effect1().percent();
  base.attack_expertise = spells.veteran_of_the_third_war -> effect2().percent();

  base.attack_power = level * ( level > 80 ? 3.0 : 2.0 );

  initial.attack_power_per_strength = 2.0;

  if ( primary_tree() == DEATH_KNIGHT_BLOOD )
    vengeance.enabled = true;

  resources.base[ RESOURCE_RUNIC_POWER ] = 100;

  base_gcd = timespan_t::from_seconds( 1.5 );

  diminished_kfactor    = 0.009560;
  diminished_dodge_capi = 0.01523660;
  diminished_parry_capi = 0.01523660;
}

// death_knight_t::init_spells ==============================================

void death_knight_t::init_spells()
{
  player_t::init_spells();

  // Blood
  spells.blood_rites              = find_spell( 50034, "blood_rites" );
  spells.veteran_of_the_third_war = find_spell( 50029, "veteran_of_the_third_war" );

  // Frost
  spells.blood_of_the_north = find_spell( 54637, "blood_of_the_north" );
  spells.icy_talons         = find_spell( 50887, "icy_talons" );
  spells.frozen_heart       = find_spell( 77514, "frozen_heart" );

  // Unholy
  spells.dreadblade       = find_spell( 77515, "dreadblade" );
  spells.master_of_ghouls = find_spell( 52143, "master_of_ghouls" );
  spells.reaping          = find_spell( 56835, "reaping" );
  spells.unholy_might     = find_spell( 91107, "unholy_might" );

  // General
  spells.plate_specialization = find_spell( 86524, "plate_specialization" );
  spells.runic_empowerment    = find_spell( 81229, "runic_empowerment" );

  // Glyphs

  // Active Spells
  active_spells.blood_plague = new blood_plague_t( this );
  active_spells.frost_fever = new frost_fever_t( this );

  // Tier Bonuses
  static const uint32_t set_bonuses[N_TIER][N_TIER_BONUS] =
  {
    //  C2P    C4P     M2P     M4P     T2P     T4P    H2P    H4P
    {     0,     0, 105609, 105646, 105552, 105587,     0,     0 }, // Tier13
    {     0,     0,      0,      0,      0,      0,     0,     0 },
  };

  sets = new set_bonus_array_t( this, set_bonuses );
}

// death_knight_t::init_actions =============================================

void death_knight_t::init_actions()
{
  if ( true )
  {
    if ( ! quiet )
      sim -> errorf( "Player %s's class isn't supported yet.", name() );
    quiet = true;
    return;
  }

  if ( main_hand_weapon.type == WEAPON_NONE )
  {
    if ( ! quiet )
      sim -> errorf( "Player %s has no weapon equipped at the Main-Hand slot.", name() );
    quiet = true;
    return;
  }

  if ( action_list_str.empty() )
  {
    clear_action_priority_lists();

    int tree = primary_tree();

    if ( tree == DEATH_KNIGHT_FROST || tree == DEATH_KNIGHT_UNHOLY || ( tree == DEATH_KNIGHT_BLOOD && primary_role() != ROLE_TANK ) )
    {
      if ( level >= 80 )
      {
        // Flask
        if ( level > 85 )
          action_list_str += "/flask,type=winters_bite,precombat=1";
        else
          action_list_str += "/flask,type=titanic_strength,precombat=1";

        // Food
        if ( level > 85 )
        {
          action_list_str += "/food,type=great_pandaren_banquet,precombat=1";
        }
        else
        {
          action_list_str += "/food,type=beer_basted_crocolisk,precombat=1";
        }
      }

      // Stance
      action_list_str += "/presence,choose=unholy";
    }
    else if ( tree == DEATH_KNIGHT_BLOOD && primary_role() == ROLE_TANK )
    {
      if ( level >= 80 )
      {
        // Flask
        if ( level >  85 )
          action_list_str += "/flask,type=earth,precombat=1";
        else
          action_list_str += "/flask,type=steelskin,precombat=1";

        // Food
        if ( level > 85 )
          action_list_str += "/food,type=great_pandaren_banquet,precombat=1";
        else
          action_list_str += "/food,type=beer_basted_crocolisk,precombat=1";
      }

      // Stance
      action_list_str += "/presence,choose=blood,precombat=1";
    }

    action_list_str += "/army_of_the_dead";

    action_list_str += "/snapshot_stats,precombat=1";

    switch ( tree )
    {
    case DEATH_KNIGHT_BLOOD:
      action_list_str += init_use_item_actions( ",time>=10" );
      action_list_str += init_use_profession_actions();
      action_list_str += init_use_racial_actions( ",time>=10" );
      if ( level > 85 )
        action_list_str += "/mogu_power_potion,precombat=1/mogu_power_potion,if=buff.bloodlust.react|target.time_to_die<=60";
      else if ( level >= 80 )
        action_list_str += "/golemblood_potion,precombat=1/golemblood_potion,if=buff.bloodlust.react|target.time_to_die<=60";
      action_list_str += "/auto_attack";
      action_list_str += "/raise_dead,time>=10";
      action_list_str += "/outbreak,if=(dot.frost_fever.remains<=2|dot.blood_plague.remains<=2)|(!dot.blood_plague.ticking&!dot.frost_fever.ticking)";
      action_list_str += "/plague_strike,if=!dot.blood_plague.ticking";
      action_list_str += "/icy_touch,if=!dot.frost_fever.ticking";
      action_list_str += "/blood_tap,if=(unholy=0&frost>=1)|(unholy>=1&frost=0)|(death=1)";
      action_list_str += "/death_strike";
      action_list_str += "/blood_boil,if=buff.crimson_scourge.up";
      action_list_str += "/heart_strike,if=(blood=1&blood.cooldown_remains<1)|blood=2";
      action_list_str += "/rune_strike,if=runic_power>=40";
      action_list_str += "/horn_of_winter";
      action_list_str += "/empower_rune_weapon,if=blood=0&unholy=0&frost=0";
      break;
    case DEATH_KNIGHT_FROST:
    {
      action_list_str += init_use_item_actions( ",time>=10" );
      action_list_str += init_use_profession_actions();
      action_list_str += init_use_racial_actions( ",time>=10" );
      if ( level > 85 )
        action_list_str += "/mogu_power_potion,precombat=1/mogu_power_potion,if=buff.bloodlust.react|target.time_to_die<=60";
      else if ( level >= 80 )
        action_list_str += "/golemblood_potion,precombat=1/golemblood_potion,if=buff.bloodlust.react|target.time_to_die<=60";
      action_list_str += "/auto_attack";
      action_list_str += "/blood_tap,if=death.cooldown_remains>2.0";
      // this results in a dps loss. which is odd, it probalby shouldn't. although it only ever affects the very first ghoul summon
      // leaving it here until further testing.
      if ( false )
      {
        // Try and time a better ghoul
        bool has_heart_of_rage = false;
        for ( int i=0, n=items.size(); i < n; i++ )
        {
          // check for Heart of Rage
          if ( strstr( items[ i ].name(), "heart_of_rage" ) )
          {
            has_heart_of_rage = true;
            break;
          }
        }
        action_list_str += "/raise_dead,if=buff.rune_of_the_fallen_crusader.react";
        if ( has_heart_of_rage )
          action_list_str += "&buff.heart_of_rage.react";
      }

      action_list_str += "/raise_dead,time>=15";
      // priority:
      // Diseases
      // Obliterate if 2 rune pair are capped, or there is no candidate for RE
      // FS if RP > 110 - avoid RP capping (value varies. going with 110)
      // Rime
      // OBL if any pair are capped
      // FS to avoid RP capping (maxRP - OBL rp generation + GCD generation. Lets say 100)
      // OBL
      // FS
      // HB (it turns out when resource starved using a lonely death/frost rune to generate RP/FS/RE is better than waiting for OBL

      // optimal timing for diseases depends on points in epidemic, and if using improved blood tap
      // players with only 2 points in epidemic want a 0 second refresh to avoid two PS in one minute instead of 1
      // IBT players use 2 PS every minute
      std::string drefresh = "0";
      if ( level > 81 )
        action_list_str += "/outbreak,if=dot.frost_fever.remains<=" + drefresh + "|dot.blood_plague.remains<=" + drefresh;
      action_list_str += "/howling_blast,if=dot.frost_fever.remains<=" + drefresh;
      action_list_str += "/plague_strike,if=dot.blood_plague.remains<=" + drefresh;
      action_list_str += "/obliterate,if=death>=1&frost>=1&unholy>=1";
      action_list_str += "/obliterate,if=(death=2&frost=2)|(death=2&unholy=2)|(frost=2&unholy=2)";
      // XXX TODO 110 is based on MAXRP - FSCost + a little, as a break point. should be varialble based on RPM GoFS etc
      action_list_str += "/frost_strike,if=runic_power>=110";
      action_list_str += "/obliterate,if=(death=2|unholy=2|frost=2)";
      action_list_str += "/frost_strike,if=runic_power>=100";
      action_list_str += "/obliterate";
      action_list_str += "/empower_rune_weapon,if=target.time_to_die<=45";
      action_list_str +="/frost_strike";
      // avoid using ERW if runes are almost ready
      action_list_str += "/empower_rune_weapon,if=(blood.cooldown_remains+frost.cooldown_remains+unholy.cooldown_remains)>8";
      action_list_str += "/horn_of_winter";
      // add in goblin rocket barrage when nothing better to do. 40dps or so.
      if ( race == RACE_GOBLIN )
        action_list_str += "/rocket_barrage";
      break;
    }
    case DEATH_KNIGHT_UNHOLY:
      action_list_str += init_use_item_actions( ",time>=2" );
      action_list_str += init_use_profession_actions();
      action_list_str += init_use_racial_actions( ",time>=2" );
      action_list_str += "/raise_dead";
      if ( level > 85 )
        action_list_str += "/mogu_power_potion,precombat=1/mogu_power_potion,if=buff.bloodlust.react|target.time_to_die<=60";
      else if ( level >= 80 )
        action_list_str += "/golemblood_potion,precombat=1/golemblood_potion,if=buff.bloodlust.react|target.time_to_die<=60";
      action_list_str += "/auto_attack";
      if ( level > 81 )
        action_list_str += "/outbreak,if=dot.frost_fever.remains<=2|dot.blood_plague.remains<=2";
      action_list_str += "/icy_touch,if=dot.frost_fever.remains<2&cooldown.outbreak.remains>2";
      action_list_str += "/plague_strike,if=dot.blood_plague.remains<2&cooldown.outbreak.remains>2";
      action_list_str += "/death_and_decay,not_flying=1,if=unholy=2&runic_power<110";
      action_list_str += "/scourge_strike,if=unholy=2&runic_power<110";
      action_list_str += "/festering_strike,if=blood=2&frost=2&runic_power<110";
      action_list_str += "/death_coil,if=runic_power>90";
      action_list_str += "/death_and_decay,not_flying=1";
      action_list_str += "/scourge_strike";
      action_list_str += "/festering_strike";
      action_list_str += "/death_coil";
      action_list_str += "/blood_tap";
      action_list_str += "/empower_rune_weapon";
      action_list_str += "/horn_of_winter";
      break;
    default: break;
    }

    action_list_default = 1;
  }

  player_t::init_actions();
}

// death_knight_t::init_enchant =============================================

void death_knight_t::init_enchant()
{
  player_t::init_enchant();

  std::string& mh_enchant = items[ SLOT_MAIN_HAND ].encoded_enchant_str;
  std::string& oh_enchant = items[ SLOT_OFF_HAND  ].encoded_enchant_str;

  // Rune of Cinderglacier ==================================================
  struct cinderglacier_callback_t : public action_callback_t
  {
    int slot;
    buff_t* buff;

    cinderglacier_callback_t( player_t* p, int s, buff_t* b ) : action_callback_t( p ), slot( s ), buff( b ) {}

    virtual void trigger( action_t* a, void* /* call_data */ )
    {
      weapon_t* w = a -> weapon;
      if ( ! w || w -> slot != slot ) return;

      // FIX ME: What is the proc rate? For now assuming the same as FC
      buff -> trigger( 2, 0.2, w -> proc_chance_on_swing( 2.0 ) );

      // FIX ME: This should roll the benefit when casting DND, it does not
    }
  };

  // Rune of the Fallen Crusader ============================================
  struct fallen_crusader_callback_t : public action_callback_t
  {
    int slot;
    buff_t* buff;

    fallen_crusader_callback_t( player_t* p, int s, buff_t* b ) : action_callback_t( p ), slot( s ), buff( b ) {}

    virtual void trigger( action_t* a, void* /* call_data */ )
    {
      weapon_t* w = a -> weapon;
      if ( ! w ) return;
      if ( w -> slot != slot ) return;

      // RotFC is 2 PPM.
      buff -> trigger( 1, 0.15, w -> proc_chance_on_swing( 2.0 ) );
    }
  };

  // Rune of the Razorice ===================================================

  // Damage Proc
  struct razorice_spell_t : public death_knight_spell_t
  {
    razorice_spell_t( death_knight_t* player ) : death_knight_spell_t( "razorice", player, player -> find_spell( 50401 ) )
    {
      may_miss    = false;
      may_crit    = false;
      background  = true;
      proc        = true;
    }

    void target_debuff( player_t* t, dmg_type_e dtype )
    {
      death_knight_spell_t::target_debuff( t, dtype );
      death_knight_t* p = cast();

      target_multiplier /= 1.0 + p -> buffs.rune_of_razorice -> check() * p -> buffs.rune_of_razorice -> data().effect1().percent();
    }
  };

  struct razorice_callback_t : public action_callback_t
  {
    int slot;
    buff_t* buff;
    spell_t* razorice_damage_proc;

    razorice_callback_t( death_knight_t* p, int s, buff_t* b ) :
      action_callback_t( p ), slot( s ), buff( b ), razorice_damage_proc( 0 )
    {
      razorice_damage_proc = new razorice_spell_t( p );
    }

    virtual void trigger( action_t* a, void* /* call_data */ )
    {
      weapon_t* w = a -> weapon;
      if ( ! w ) return;
      if ( w -> slot != slot ) return;

      // http://elitistjerks.com/f72/t64830-dw_builds_3_2_revenge_offhand/p28/#post1332820
      // double PPM        = 2.0;
      // double swing_time = a -> time_to_execute;
      // double chance     = w -> proc_chance_on_swing( PPM, swing_time );
      buff -> trigger();

      razorice_damage_proc -> execute();
    }
  };

  buffs.rune_of_cinderglacier       = buff_creator_t( this, "rune_of_cinderglacier" ).max_stack( 2 ).duration( timespan_t::from_seconds( 30.0 ) );
  buffs.rune_of_razorice            = buff_creator_t( this, 51714, "rune_of_razorice" );
  buffs.rune_of_the_fallen_crusader = buff_creator_t( this, "rune_of_the_fallen_crusader" ).max_stack( 1 ).duration( timespan_t::from_seconds( 15.0 ) );

  if ( mh_enchant == "rune_of_the_fallen_crusader" )
  {
    callbacks.register_attack_callback( RESULT_HIT_MASK, new fallen_crusader_callback_t( this, SLOT_MAIN_HAND, buffs.rune_of_the_fallen_crusader ) );
  }
  else if ( mh_enchant == "rune_of_razorice" )
  {
    callbacks.register_attack_callback( RESULT_HIT_MASK, new razorice_callback_t( this, SLOT_MAIN_HAND, buffs.rune_of_razorice ) );
  }
  else if ( mh_enchant == "rune_of_cinderglacier" )
  {
    callbacks.register_attack_callback( RESULT_HIT_MASK, new cinderglacier_callback_t( this, SLOT_MAIN_HAND, buffs.rune_of_cinderglacier ) );
  }

  if ( oh_enchant == "rune_of_the_fallen_crusader" )
  {
    callbacks.register_attack_callback( RESULT_HIT_MASK, new fallen_crusader_callback_t( this, SLOT_OFF_HAND, buffs.rune_of_the_fallen_crusader ) );
  }
  else if ( oh_enchant == "rune_of_razorice" )
  {
    callbacks.register_attack_callback( RESULT_HIT_MASK, new razorice_callback_t( this, SLOT_OFF_HAND, buffs.rune_of_razorice ) );
  }
  else if ( oh_enchant == "rune_of_cinderglacier" )
  {
    callbacks.register_attack_callback( RESULT_HIT_MASK, new cinderglacier_callback_t( this, SLOT_OFF_HAND, buffs.rune_of_cinderglacier ) );
  }
}

// death_knight_t::init_scaling =============================================

void death_knight_t::init_scaling()
{
  player_t::init_scaling();

  if ( off_hand_weapon.type != WEAPON_NONE )
  {
    scales_with[ STAT_WEAPON_OFFHAND_DPS   ] = true;
    scales_with[ STAT_WEAPON_OFFHAND_SPEED ] = sim -> weapon_speed_scale_factors != 0;
    scales_with[ STAT_HIT_RATING2          ] = true;
  }

  if ( primary_role() == ROLE_TANK )
    scales_with[ STAT_PARRY_RATING ] = true;
}

// death_knight_t::init_buffs ===============================================

void death_knight_t::init_buffs()
{
  player_t::init_buffs();

  // buff_t( player, name, max_stack, duration, chance=-1, cd=-1, quiet=false, reverse=false, activated=true )
  // buff_t( player, id, name, chance=-1, cd=-1, quiet=false, reverse=false, activated=true )
  // buff_t( player, name, spellname, chance=-1, cd=-1, quiet=false, reverse=false, activated=true )

  buffs.blood_presence      = buff_creator_t( this, "blood_presence", find_class_spell( "Blood Presence" ) );
  buffs.bone_shield         = buff_creator_t( this, "bone_shield", find_class_spell( "Bone Shield" ) );
  buffs.crimson_scourge     = buff_creator_t( this, 81141, "crimson_scourge" );
  buffs.dancing_rune_weapon = buff_creator_t( this, "dancing_rune_weapon", find_class_spell( "Dancing Rune Weapon" ) );
  buffs.dark_transformation = buff_creator_t( this, "dark_transformation", find_class_spell( "Dark Transformation" ) );
  buffs.frost_presence      = buff_creator_t( this, "frost_presence", find_class_spell( "Frost Presence" ) );
  buffs.killing_machine     = buff_creator_t( this, 51124, "killing_machine" ); // PPM based!
  buffs.pillar_of_frost     = buff_creator_t( this, "pillar_of_frost", find_class_spell( "Pillar of Frost" ) );
  buffs.rime                = buff_creator_t( this, "rime", find_specialization_spell( "Rime" ) )
                              .max_stack( ( set_bonus.tier13_2pc_melee() ) ? 2 : 1 )
                              .duration( timespan_t::from_seconds( 30.0 ) )
                              .cd( timespan_t::zero() )
                              .chance( 1.0 ); // Trigger controls proc chance
  buffs.rune_strike         = buff_creator_t( this, "runestrike", find_class_spell( "Rune Strike" ) )
                              .max_stack( 1 )
                              .duration( timespan_t::from_seconds( 10.0 ) )
                              .cd( timespan_t::zero() )
                              .chance( 1.0 )
                              .quiet( true );
  buffs.runic_corruption    = buff_creator_t( this, 51460, "runic_corruption" );
  buffs.sudden_doom         = buff_creator_t( this, "sudden_doom", find_specialization_spell( "Sudden Doom" ) )
                              .max_stack( ( set_bonus.tier13_2pc_melee() ) ? 2 : 1 )
                              .duration( timespan_t::from_seconds( 10.0 ) )
                              .cd( timespan_t::zero() )
                              .chance( 1.0 );
  buffs.tier13_4pc_melee    = stat_buff_creator_t( buff_creator_t( this, 105647, "tier13_4pc_melee" ) ).stat( STAT_MASTERY_RATING ).amount( dbc.spell( 105647 ) -> effect1().base_value() );
  buffs.unholy_presence     = buff_creator_t( this, "unholy_presence", find_class_spell( "Unholy Presence" ) );

  struct bloodworms_buff_t : public buff_t
  {
    death_knight_t* dk;
    bloodworms_buff_t( death_knight_t* p ) :
      buff_t( buff_creator_t( p, "bloodworms" ).max_stack( 1 ).duration( timespan_t::from_seconds( 19.99 ) ).cd( timespan_t::zero() ).chance( 0.0 ) ),
    dk( p ) {}
    virtual void start( int stacks, double value, timespan_t duration = timespan_t::min() )
    {
      buff_t::start( stacks, value, duration );
      dk -> pets.bloodworms -> summon();
    }
    virtual void expire()
    {
      buff_t::expire();
      if (dk -> pets.bloodworms ) dk -> pets.bloodworms -> dismiss();
    }
  };
  buffs.bloodworms = new bloodworms_buff_t( this );
}

// death_knight_t::init_values ====================================================

void death_knight_t::init_values()
{
  player_t::init_values();

  if ( set_bonus.pvp_2pc_melee() )
    initial.attribute[ ATTR_STRENGTH ]   += 70;

  if ( set_bonus.pvp_4pc_melee() )
    initial.attribute[ ATTR_STRENGTH ]   += 90;
}

// death_knight_t::init_gains ===============================================

void death_knight_t::init_gains()
{
  player_t::init_gains();

  gains.butchery                         = get_gain( "butchery"                   );
  gains.chill_of_the_grave               = get_gain( "chill_of_the_grave"         );
  gains.frost_presence                   = get_gain( "frost_presence"             );
  gains.horn_of_winter                   = get_gain( "horn_of_winter"             );
  gains.improved_frost_presence          = get_gain( "improved_frost_presence"    );
  gains.might_of_the_frozen_wastes       = get_gain( "might_of_the_frozen_wastes" );
  gains.power_refund                     = get_gain( "power_refund"               );
  gains.scent_of_blood                   = get_gain( "scent_of_blood"             );
  gains.rune                             = get_gain( "rune_regen_all"             );
  gains.rune_unholy                      = get_gain( "rune_regen_unholy"          );
  gains.rune_blood                       = get_gain( "rune_regen_blood"           );
  gains.rune_frost                       = get_gain( "rune_regen_frost"           );
  gains.rune_unknown                     = get_gain( "rune_regen_unknown"         );
  gains.runic_empowerment                = get_gain( "runic_empowerment"          );
  gains.runic_empowerment_blood          = get_gain( "runic_empowerment_blood"    );
  gains.runic_empowerment_frost          = get_gain( "runic_empowerment_frost"    );
  gains.runic_empowerment_unholy         = get_gain( "runic_empowerment_unholy"   );
  gains.empower_rune_weapon              = get_gain( "empower_rune_weapon"        );
  gains.blood_tap                        = get_gain( "blood_tap"                  );
  // gains.blood_tap_blood                  = get_gain( "blood_tap_blood"            );
  //gains.blood_tap_blood          -> type = ( resource_type_e ) RESOURCE_RUNE_BLOOD   ;
}

// death_knight_t::init_procs ===============================================

void death_knight_t::init_procs()
{
  player_t::init_procs();

  procs.runic_empowerment        = get_proc( "runic_empowerment"            );
  procs.runic_empowerment_wasted = get_proc( "runic_empowerment_wasted"     );
  procs.oblit_killing_machine    = get_proc( "oblit_killing_machine"        );
  procs.fs_killing_machine       = get_proc( "frost_strike_killing_machine" );
}

// death_knight_t::init_resources ===========================================

void death_knight_t::init_resources( bool force )
{
  player_t::init_resources( force );

  resources.current[ RESOURCE_RUNIC_POWER ] = 0;
}

// death_knight_t::init_uptimes =============================================

void death_knight_t::init_benefits()
{
  player_t::init_benefits();

  benefits.rp_cap = get_benefit( "rp_cap" );
}

// death_knight_t::reset ====================================================

void death_knight_t::reset()
{
  player_t::reset();

  // Active
  active_presence = 0;

  _runes.reset();
}

// death_knight_t::combat_begin =============================================

void death_knight_t::combat_begin()
{
  player_t::combat_begin();
}

// death_knight_t::assess_damage ============================================

double death_knight_t::assess_damage( double            amount,
                                      school_type_e     school,
                                      dmg_type_e        dtype,
                                      result_type_e     result,
                                      action_t*         action )
{
  if ( buffs.blood_presence -> check() )
    amount *= 1.0 - dbc.spell( 61261 ) -> effect1().percent();

  if ( result != RESULT_MISS )
    buffs.scent_of_blood -> trigger();

  if ( result == RESULT_DODGE || result == RESULT_PARRY )
    buffs.rune_strike -> trigger();

  return player_t::assess_damage( amount, school, dtype, result, action );
}

// death_knight_t::composite_armor_multiplier ===============================

double death_knight_t::composite_armor_multiplier()
{
  double a = player_t::composite_armor_multiplier();

  if ( buffs.blood_presence -> check() )
    a += buffs.blood_presence -> data().effect1().percent();

  return a;
}

// death_knight_t::composite_attribute_multiplier ===========================

double death_knight_t::composite_attribute_multiplier( attribute_type_e attr )
{
  double m = player_t::composite_attribute_multiplier( attr );

  if ( attr == ATTR_STRENGTH )
  {
    m *= 1.0 + buffs.rune_of_the_fallen_crusader -> value();
    m *= 1.0 + buffs.pillar_of_frost -> value();
  }

  if ( attr == ATTR_STAMINA )
    if ( buffs.blood_presence -> check() )
      m *= 1.0 + buffs.blood_presence -> data().effectN( 3 ).percent();

  return m;
}

// death_knight_t::matching_gear_multiplier =================================

double death_knight_t::matching_gear_multiplier( attribute_type_e attr )
{
  int tree = primary_tree();

  if ( tree == DEATH_KNIGHT_UNHOLY || tree == DEATH_KNIGHT_FROST )
    if ( attr == ATTR_STRENGTH )
      return spells.plate_specialization -> effect1().percent();

  if ( tree == DEATH_KNIGHT_BLOOD )
    if ( attr == ATTR_STAMINA )
      return spells.plate_specialization -> effect1().percent();

  return 0.0;
}

// death_knight_t::composite_spell_hit ======================================

double death_knight_t::composite_spell_hit()
{
  double hit = player_t::composite_spell_hit();

  hit += .09; // Not in Runic Empowerment's data yet

  return hit;
}

// death_knight_t::composite_tank_parry =====================================

double death_knight_t::composite_tank_parry()
{
  double parry = player_t::composite_tank_parry();

  if ( buffs.dancing_rune_weapon -> up() )
    parry += 0.20;

  return parry;
}

// death_knight_t::composite_player_multiplier ==============================

double death_knight_t::composite_player_multiplier( school_type_e school, action_t* a )
{
  double m = player_t::composite_player_multiplier( school, a );

  // Factor flat multipliers here so they effect procs, grenades, etc.
  m *= 1.0 + buffs.frost_presence -> value();
  m *= 1.0 + buffs.bone_shield -> value();

  if ( school == SCHOOL_SHADOW )
    m *= 1.0 + spells.dreadblade -> effect1().coeff() * 0.01 * composite_mastery();

  if ( school == SCHOOL_FROST )
    m *= 1.0 + spells.frozen_heart -> effect1().coeff() * 0.01 * composite_mastery();

  return m;
}

// death_knight_t::composite_tank_crit ======================================

double death_knight_t::composite_tank_crit( school_type_e school )
{
  double c = player_t::composite_tank_crit( school );

  return c;
}

// death_knight_t::primary_role =============================================

role_type_e death_knight_t::primary_role()
{
  if ( player_t::primary_role() == ROLE_TANK )
    return ROLE_TANK;

  if ( player_t::primary_role() == ROLE_DPS || player_t::primary_role() == ROLE_ATTACK )
    return ROLE_ATTACK;

  if ( primary_tree() == DEATH_KNIGHT_BLOOD )
    return ROLE_TANK;

  return ROLE_ATTACK;
}

// death_knight_t::regen ====================================================

void death_knight_t::regen( timespan_t periodicity )
{
  player_t::regen( periodicity );

  for ( int i = 0; i < RUNE_SLOT_MAX; ++i )
    _runes.slot[i].regen_rune( this, periodicity );

  benefits.rp_cap -> update( resources.current[ RESOURCE_RUNIC_POWER ] ==
                             resources.max    [ RESOURCE_RUNIC_POWER] );
}

// death_knight_t::create_options ===========================================

void death_knight_t::create_options()
{
  player_t::create_options();

  option_t death_knight_options[] =
  {
    { "unholy_frenzy_target", OPT_STRING, &( unholy_frenzy_target_str ) },
    { NULL, OPT_UNKNOWN, NULL }
  };

  option_t::copy( options, death_knight_options );
}

// death_knight_t::decode_set ===============================================

int death_knight_t::decode_set( item_t& item )
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

  if ( strstr( s, "necrotic_boneplate" ) )
  {
    bool is_melee = ( strstr( s, "helmet"        ) ||
                      strstr( s, "pauldrons"     ) ||
                      strstr( s, "breastplate"   ) ||
                      strstr( s, "greaves"       ) ||
                      strstr( s, "gauntlets"     ) );

    bool is_tank = ( strstr( s, "faceguard"      ) ||
                     strstr( s, "shoulderguards" ) ||
                     strstr( s, "chestguard"     ) ||
                     strstr( s, "legguards"      ) ||
                     strstr( s, "handguards"     ) );

    if ( is_melee ) return SET_T13_MELEE;
    if ( is_tank  ) return SET_T13_TANK;
  }

  if ( strstr( s, "_gladiators_dreadplate_" ) ) return SET_PVP_MELEE;

  return SET_NONE;
}

// death_knight_t::trigger_runic_empowerment ================================

void death_knight_t::trigger_runic_empowerment()
{
  if ( ! sim -> roll( spells.runic_empowerment -> proc_chance() ) )
    return;

  int depleted_runes[RUNE_SLOT_MAX];
  int num_depleted=0;

  for ( int i = 0; i < RUNE_SLOT_MAX; ++i )
    if ( _runes.slot[i].is_depleted() )
      depleted_runes[ num_depleted++ ] = i;

  if ( num_depleted > 0 )
  {
    int rune_to_regen = depleted_runes[ ( int ) ( sim -> rng -> real() * num_depleted * 0.9999 ) ];
    dk_rune_t* regen_rune = &_runes.slot[rune_to_regen];
    regen_rune -> fill_rune();
    if      ( regen_rune -> is_blood()  ) gains.runic_empowerment_blood  -> add ( RESOURCE_RUNE_BLOOD, 1,0 );
    else if ( regen_rune -> is_unholy() ) gains.runic_empowerment_unholy -> add ( RESOURCE_RUNE_UNHOLY, 1,0 );
    else if ( regen_rune -> is_frost()  ) gains.runic_empowerment_frost  -> add ( RESOURCE_RUNE_FROST, 1,0 );

    gains.runic_empowerment -> add ( RESOURCE_RUNE, 1,0 );
    if ( sim -> log ) log_t::output( sim, "runic empowerment regen'd rune %d", rune_to_regen );
    procs.runic_empowerment -> occur();

    if ( set_bonus.tier13_4pc_melee() )
      buffs.tier13_4pc_melee -> trigger( 1, 0, 0.25 );
  }
  else
  {
    // If there were no available runes to refresh
    procs.runic_empowerment_wasted -> occur();
    gains.runic_empowerment -> add ( RESOURCE_RUNE, 0,1 );
  }
}

// death_knight_t rune inspections ==========================================

// death_knight_t::runes_count ==============================================
// how many runes of type rt are available
int death_knight_t::runes_count( rune_type rt, bool include_death, int position )
{
  int result = 0;
  // positional checks first
  if ( position > 0 && ( rt == RUNE_TYPE_BLOOD || rt == RUNE_TYPE_FROST || rt == RUNE_TYPE_UNHOLY ) )
  {
    dk_rune_t* r = &_runes.slot[( ( rt-1 )*2 ) + ( position - 1 ) ];
    if ( r -> is_ready() )
      result = 1;
  }
  else
  {
    int rpc = 0;
    for ( int i = 0; i < RUNE_SLOT_MAX; i++ )
    {
      dk_rune_t* r = &_runes.slot[ i ];
      // query a specific position death rune.
      if ( position != 0 && rt == RUNE_TYPE_DEATH && r -> is_death() )
      {
        if ( ++rpc == position )
        {
          if ( r -> is_ready() )
            result = 1;
          break;
        }
      }
      // just count the runes
      else if ( ( ( ( include_death || rt == RUNE_TYPE_DEATH ) && r -> is_death() ) || ( r -> get_type() == rt ) )
                && r -> is_ready() )
      {
        result++;
      }
    }
  }
  return result;
}

// death_knight_t::runes_cooldown_any =======================================

double death_knight_t::runes_cooldown_any( rune_type rt, bool include_death, int position )
{
  dk_rune_t* rune = 0;
  int rpc = 0;
  for ( int i = 0; i < RUNE_SLOT_MAX; i++ )
  {
    dk_rune_t* r = &_runes.slot[i];
    if ( position == 0 && include_death && r -> is_death() && r -> is_ready() )
      return 0;
    if ( position == 0 && r -> get_type() == rt && r -> is_ready() )
      return 0;
    if ( ( ( include_death && r -> is_death() ) || ( r -> get_type() == rt ) ) )
    {
      if ( position != 0 && ++rpc == position )
      {
        rune = r;
        break;
      }
      if ( !rune || ( r -> value > rune -> value ) )
        rune = r;
    }
  }

  assert( rune );

  double time = this -> runes_cooldown_time( rune );
  // if it was a  specified rune and is depleted, we have to add its paired rune to the time
  if ( rune -> is_depleted() )
    time += this -> runes_cooldown_time( rune -> paired_rune );

  return time;
}

// death_knight_t::runes_cooldown_all =======================================

double death_knight_t::runes_cooldown_all( rune_type rt, bool include_death, int position )
{
  // if they specified position then they only get 1 answer. duh. handled in the other function
  if ( position > 0 )
    return this -> runes_cooldown_any( rt, include_death, position );

  // find all matching runes. total the pairs. Return the highest number.

  double max = 0;
  for ( int i = 0; i < RUNE_SLOT_MAX; i+=2 )
  {
    double total = 0;
    dk_rune_t* r = &_runes.slot[i];
    if ( ( ( rt == RUNE_TYPE_DEATH && r -> is_death() ) || r -> get_type() == rt ) && !r -> is_ready() )
    {
      total += this->runes_cooldown_time( r );
    }
    r = r -> paired_rune;
    if ( ( ( rt == RUNE_TYPE_DEATH && r -> is_death() ) || r -> get_type() == rt ) && !r -> is_ready() )
    {
      total += this->runes_cooldown_time( r );
    }
    if ( max < total )
      max = total;
  }
  return max;
}

// death_knight_t::runes_cooldown_time ======================================

double death_knight_t::runes_cooldown_time( dk_rune_t* rune )
{
  double result_num;

  double runes_per_second = 1.0 / 10.0 / composite_attack_haste();
  // Unholy Presence's 10% (or, talented, 15%) increase is factored in elsewhere as melee haste.
  result_num = ( 1.0 - rune -> value ) / runes_per_second;

  return result_num;
}

// death_knight_t::runes_depleted ===========================================

bool death_knight_t::runes_depleted( rune_type rt, int position )
{
  dk_rune_t* rune = 0;
  int rpc = 0;
  // iterate, to allow finding death rune slots as well
  for ( int i = 0; i < RUNE_SLOT_MAX; i++ )
  {
    dk_rune_t* r = &_runes.slot[i];
    if ( r -> get_type() == rt && ++rpc == position )
    {
      rune = r;
      break;
    }
  }
  if ( ! rune ) return false;
  return rune -> is_depleted();
}

void death_knight_t::arise()
{
  player_t::arise();

  if ( primary_tree() == DEATH_KNIGHT_FROST  && ! sim -> overrides.attack_haste ) sim -> auras.attack_haste -> trigger();
  if ( primary_tree() == DEATH_KNIGHT_UNHOLY && ! sim -> overrides.attack_haste ) sim -> auras.attack_haste -> trigger();
}

#endif // SC_DEATH_KNIGHT

} // END ANONYMOUS NAMESPACE

// ==========================================================================
// player_t implementations
// ==========================================================================

// class_modules::create::death_knight ============================================

player_t* class_modules::create::death_knight( sim_t* sim, const std::string& name, race_type_e r )
{
  return sc_create_class<death_knight_t,SC_DEATH_KNIGHT>()( "Death Knight", sim, name, r );
}

// class_modules::init::death_knight ======================================

void class_modules::init::death_knight( sim_t* sim )
{
  for ( size_t i = 0; i < sim -> actor_list.size(); i++ )
  {
    player_t* p = sim -> actor_list[i];
    p -> buffs.unholy_frenzy = buff_creator_t( p, "unholy_frenzy", p -> find_spell( 49016 ) );
  }
}
// class_modules::combat_begin::death_knight ==============================================

void class_modules::combat_begin::death_knight( sim_t* )
{
}

// class_modules::combat_end::death_knight ======================================

void class_modules::combat_end::death_knight( sim_t* )
{
}
