// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

// ==========================================================================
// Enemy
// ==========================================================================

struct enemy_t : public player_t
{
  double fixed_health, initial_health;
  double fixed_health_percentage, initial_health_percentage;
  timespan_t waiting_time;

  enemy_t( sim_t* s, const std::string& n, race_type_e r = RACE_HUMANOID ) :
    player_t( s, ENEMY, n, r ),
    fixed_health( 0 ), initial_health( 0 ),
    fixed_health_percentage( 0 ), initial_health_percentage( 100.0 ),
    waiting_time( timespan_t::from_seconds( 1.0 ) )

  {
    player_t** last = &( sim -> target_list );
    while ( *last ) last = &( ( *last ) -> next );
    *last = this;
    next = 0;

    create_options();
  }

  virtual role_type_e primary_role() const
  { return ROLE_TANK; }

  virtual resource_type_e primary_resource() const
  { return RESOURCE_NONE; }

  virtual double base_armor() const
  { return current.armor; }

  virtual action_t* create_action( const std::string& name, const std::string& options_str );
  virtual void init();
  virtual void init_base();
  virtual void init_resources( bool force=false );
  virtual void init_target();
  virtual void init_actions();
  virtual double composite_tank_block() const;
  virtual void create_options();
  virtual pet_t* create_pet( const std::string& add_name, const std::string& pet_type = std::string() );
  virtual void create_pets();
  virtual double health_percentage() const;
  virtual void combat_end();
  virtual void recalculate_health();
  virtual expr_t* create_expression( action_t* action, const std::string& type );
  virtual timespan_t available() const { return waiting_time; }
};

// ==========================================================================
// Enemy Add
// ==========================================================================

struct enemy_add_t : public pet_t
{
  enemy_add_t( sim_t* s, enemy_t* o, const std::string& n, pet_type_e pt = PET_ENEMY ) :
    pet_t( s, o, n, pt )
  {
    create_options();
  }

  virtual void init_actions()
  {
    if ( action_list_str.empty() )
    {
      action_list_str += "/snapshot_stats";
    }

    pet_t::init_actions();
  }

  virtual resource_type_e primary_resource() const
  { return RESOURCE_HEALTH; }

  virtual action_t* create_action( const std::string& name, const std::string& options_str );
};

namespace { // ANONYMOUS NAMESPACE ==========================================

// Melee ====================================================================

struct melee_t : public attack_t
{
  melee_t( const std::string& name, player_t* player ) :
    attack_t( name, player, spell_data_t::nil() )
  {
    school = SCHOOL_PHYSICAL;
    may_crit    = true;
    background  = true;
    repeating   = true;
    trigger_gcd = timespan_t::zero();
    base_dd_min = 260000;
    base_execute_time = timespan_t::from_seconds( 2.4 );
    aoe = -1;
  }

  virtual size_t available_targets( std::vector< player_t* >& tl ) const
  {
    // TODO: This does not work for heals at all, as it presumes enemies in the
    // actor list.

    tl.push_back( target );

    for ( size_t i = 0, actors = sim -> actor_list.size(); i < actors; i++ )
    {
      if ( ! sim -> actor_list[ i ] -> current.sleeping &&
           !sim -> actor_list[ i ] -> is_enemy() && sim -> actor_list[ i ] -> primary_role() == ROLE_TANK &&
           sim -> actor_list[ i ] != target )
        tl.push_back( sim -> actor_list[ i ] );
    }

    return tl.size();
  }
};

// Auto Attack ==============================================================

struct auto_attack_t : public attack_t
{
  auto_attack_t( player_t* p, const std::string& options_str ) :
    attack_t( "auto_attack", p, spell_data_t::nil() )
  {
    school = SCHOOL_PHYSICAL;
    p -> main_hand_attack = new melee_t( "melee_main_hand", p );
    p -> main_hand_attack -> weapon = &( p -> main_hand_weapon );
    p -> main_hand_attack -> base_execute_time = timespan_t::from_seconds( 2.4 );

    int aoe_tanks = 0;
    option_t options[] =
    {
      { "damage",       OPT_FLT,      &p -> main_hand_attack -> base_dd_min       },
      { "attack_speed", OPT_TIMESPAN, &p -> main_hand_attack -> base_execute_time },
      { "aoe_tanks",    OPT_BOOL,     &aoe_tanks },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    p -> main_hand_attack -> target = target;

    if ( aoe_tanks == 1 )
      p -> main_hand_attack -> aoe = -1;

    p -> main_hand_attack -> base_dd_max = p -> main_hand_attack -> base_dd_min;
    if ( p -> main_hand_attack -> base_execute_time < timespan_t::from_seconds( 0.01 ) )
      p -> main_hand_attack -> base_execute_time = timespan_t::from_seconds( 2.4 );

    cooldown = player -> get_cooldown( name_str + "_" + target -> name() );
    stats = player -> get_stats( name_str + "_" + target -> name(), this );
    stats -> school = school;
    name_str = name_str + "_" + target -> name();

    trigger_gcd = timespan_t::zero();
  }

  virtual void execute()
  {
    player -> main_hand_attack -> schedule_execute();
    if ( player -> off_hand_attack )
    {
      player -> off_hand_attack -> schedule_execute();
    }
  }

  virtual bool ready()
  {
    if ( player -> is_moving() ) return false;
    return( player -> main_hand_attack -> execute_event == 0 ); // not swinging
  }
};

// Spell Nuke ===============================================================

struct spell_nuke_t : public spell_t
{
  spell_nuke_t( player_t* p, const std::string& options_str ) :
    spell_t( "spell_nuke", p, spell_data_t::nil() )
  {
    school = SCHOOL_FIRE;
    base_execute_time = timespan_t::from_seconds( 3.0 );
    base_dd_min = 50000;

    cooldown = player -> get_cooldown( name_str + "_" + target -> name() );

    int aoe_tanks = 0;
    option_t options[] =
    {
      { "damage",       OPT_FLT, &base_dd_min          },
      { "attack_speed", OPT_TIMESPAN, &base_execute_time    },
      { "cooldown",     OPT_TIMESPAN, &cooldown -> duration },
      { "aoe_tanks",    OPT_BOOL,     &aoe_tanks },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    base_dd_max = base_dd_min;
    if ( base_execute_time < timespan_t::zero() )
      base_execute_time = timespan_t::from_seconds( 3.0 );

    stats = player -> get_stats( name_str + "_" + target -> name(), this );
    stats -> school = school;
    name_str = name_str + "_" + target -> name();

    may_crit = false;
    if ( aoe_tanks == 1 )
      aoe = -1;
  }

  virtual size_t available_targets( std::vector< player_t* >& tl ) const
  {
    // TODO: This does not work for heals at all, as it presumes enemies in the
    // actor list.

    tl.push_back( target );

    for ( size_t i = 0, actors = sim -> actor_list.size(); i < actors; ++i )
    {
      if ( ! sim -> actor_list[ i ] -> current.sleeping &&
           !sim -> actor_list[ i ] -> is_enemy() && sim -> actor_list[ i ] -> primary_role() == ROLE_TANK &&
           sim -> actor_list[ i ] != target )
        tl.push_back( sim -> actor_list[ i ] );
    }

    return tl.size();
  }

};

// Spell AoE ================================================================

struct spell_aoe_t : public spell_t
{
  spell_aoe_t( player_t* p, const std::string& options_str ) :
    spell_t( "spell_aoe", p, spell_data_t::nil() )
  {
    school = SCHOOL_FIRE;
    base_execute_time = timespan_t::from_seconds( 3.0 );
    base_dd_min = 50000;

    cooldown = player -> get_cooldown( name_str + "_" + target -> name() );

    option_t options[] =
    {
      { "damage",       OPT_FLT, &base_dd_min          },
      { "cast_time", OPT_TIMESPAN, &base_execute_time    },
      { "cooldown",     OPT_TIMESPAN, &cooldown -> duration },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    base_dd_max = base_dd_min;
    if ( base_execute_time < timespan_t::from_seconds( 0.01 ) )
      base_execute_time = timespan_t::from_seconds( 3.0 );

    stats = player -> get_stats( name_str + "_" + target -> name(), this );
    stats -> school = school;
    name_str = name_str + "_" + target -> name();

    may_crit = false;

    aoe = -1;
  }

  virtual size_t available_targets( std::vector< player_t* >& tl ) const
  {
    // TODO: This does not work for heals at all, as it presumes enemies in the
    // actor list.

    tl.push_back( target );

    for ( size_t i = 0, actors = sim -> actor_list.size(); i < actors; ++i )
    {
      if ( ! sim -> actor_list[ i ] -> current.sleeping &&
           !sim -> actor_list[ i ] -> is_enemy() &&
           sim -> actor_list[ i ] != target )
        tl.push_back( sim -> actor_list[ i ] );
    }

    return tl.size();
  }

};

// Summon Add ===============================================================

struct summon_add_t : public spell_t
{
  std::string add_name;
  timespan_t summoning_duration;
  pet_t* pet;

  summon_add_t( player_t* p, const std::string& options_str ) :
    spell_t( "summon_add", player, spell_data_t::nil() ),
    add_name( "" ), summoning_duration( timespan_t::zero() ), pet( 0 )
  {
    option_t options[] =
    {
      { "name",     OPT_STRING, &add_name             },
      { "duration", OPT_TIMESPAN,    &summoning_duration   },
      { "cooldown", OPT_TIMESPAN,    &cooldown -> duration },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    school = SCHOOL_PHYSICAL;
    pet = p -> find_pet( add_name );
    if ( ! pet )
    {
      sim -> errorf( "Player %s unable to find pet %s for summons.\n", p -> name(), add_name.c_str() );
      sim -> cancel();
    }

    harmful = false;

    trigger_gcd = timespan_t::from_seconds( 1.5 );
  }

  virtual void execute()
  {
    spell_t::execute();

    player -> summon_pet( add_name, summoning_duration );
  }

  virtual bool ready()
  {
    if ( ! pet -> current.sleeping )
      return false;

    return spell_t::ready();
  }
};
}

// ==========================================================================
// Enemy Extensions
// ==========================================================================

// enemy_t::create_action ===================================================

action_t* enemy_t::create_action( const std::string& name,
                                  const std::string& options_str )
{
  if ( name == "auto_attack" ) return new auto_attack_t( this, options_str );
  if ( name == "spell_nuke"  ) return new  spell_nuke_t( this, options_str );
  if ( name == "spell_aoe"   ) return new   spell_aoe_t( this, options_str );
  if ( name == "summon_add"  ) return new  summon_add_t( this, options_str );

  return player_t::create_action( name, options_str );
}

// enemy_t::init ============================================================

void enemy_t::init()
{


  player_t::init();
}

// enemy_t::init_base =======================================================

void enemy_t::init_base()
{
  level = sim -> max_player_level + 3;

  if ( sim -> target_level >= 0 )
    level = sim -> target_level;

  waiting_time = timespan_t::from_seconds( std::min( ( int ) floor( sim -> max_time.total_seconds() ), sim -> wheel_seconds - 1 ) );
  if ( waiting_time < timespan_t::from_seconds( 1.0 ) )
    waiting_time = timespan_t::from_seconds( 1.0 );

  base.attack_crit = 0.05;

  if ( initial.armor <= 0 )
  {
    double& a = initial.armor;
    // TO-DO: Fill in the blanks.
    // For level 80+ at least it seems to pretty much follow a trend line of: armor = 280.26168*level - 12661.51713
    switch ( level )
    {
    case 80: a = 9729; break;
    case 81: a = 10034; break;
    case 82: a = 10338; break;
    case 83: a = 10643; break;
    case 84: a = 10880; break; // Need real value
    case 85: a = 11092; break;
    case 86: a = 11387; break;
    case 87: a = 11682; break;
    case 88: a = 11977; break;
    default: if ( level < 80 )
        a = ( int ) floor ( ( level / 80.0 ) * 9729 ); // Need a better value here.
      break;
    }
  }
  base.armor = initial.armor;

  initial_health = fixed_health;

  if ( ( initial_health_percentage < 1   ) ||
       ( initial_health_percentage > 100 ) )
  {
    initial_health_percentage = 100.0;
  }
}

// enemy_t::init_resources ==================================================

void enemy_t::init_resources( bool /* force */ )
{
  double health_adjust = 1.0 + sim -> vary_combat_length * sim -> iteration_adjust();

  resources.base[ RESOURCE_HEALTH ] = initial_health * health_adjust;

  player_t::init_resources( true );

  if ( initial_health_percentage > 0 )
  {
    resources.max[ RESOURCE_HEALTH ] *= 100.0 / initial_health_percentage;
  }
}

// enemy_t::init_target ====================================================

void enemy_t::init_target()
{
  if ( ! target_str.empty() )
  {
    target = sim -> find_player( target_str );
  }

  if ( target )
    return;

  for ( player_t* q = sim -> player_list; q; q = q -> next )
  {
    if ( q -> primary_role() != ROLE_TANK )
      continue;

    target = q;
    break;
  }

  if ( !target )
    target = sim -> target;
}

// enemy_t::init_actions ====================================================

void enemy_t::init_actions()
{
  if ( !is_add() )
  {
    if ( action_list_str.empty() )
    {
      action_list_str += "/snapshot_stats";

      if ( target != this )
      {
        action_list_str += "/auto_attack,damage=260000,attack_speed=2.4,aoe_tanks=1";
        action_list_str += "/spell_nuke,damage=6000,cooldown=4,attack_speed=0.1,aoe_tanks=1";
      }
    }
  }
  player_t::init_actions();

  // Small hack to increase waiting time for target without any actions
  for ( size_t i = 0; i < action_list.size(); ++i )
  {
    action_t* action = action_list[ i ];
    if ( action -> background ) continue;
    if ( action -> name_str == "snapshot_stats" ) continue;
    if ( action -> name_str.find( "auto_attack" ) != std::string::npos )
      continue;
    waiting_time = timespan_t::from_seconds( 1.0 );
    break;
  }
}

// enemy_t::composite_tank_block ============================================

double enemy_t::composite_tank_block() const
{
  double b = player_t::composite_tank_block();

  b += 0.05;

  return b;
}

// enemy_t::create_options ==================================================

void enemy_t::create_options()
{
  option_t target_options[] =
  {
    { "target_health",                    OPT_FLT,    &( fixed_health                      ) },
    { "target_initial_health_percentage", OPT_FLT,    &( initial_health_percentage         ) },
    { "target_fixed_health_percentage",   OPT_FLT,    &( fixed_health_percentage           ) },
    { "target_tank",                      OPT_STRING, &( target_str                        ) },
    { NULL, OPT_UNKNOWN, NULL }
  };

  option_t::copy( sim -> options, target_options );

  player_t::create_options();
}

// enemy_t::create_add ======================================================

pet_t* enemy_t::create_pet( const std::string& add_name, const std::string& /* pet_type */ )
{
  pet_t* p = find_pet( add_name );
  if ( p ) return p;

  return new enemy_add_t( sim, this, add_name, PET_ENEMY );

  return 0;
}

// enemy_t::create_pets =====================================================

void enemy_t::create_pets()
{
  for ( int i=0; i < sim -> target_adds; i++ )
  {
    std::string s = "add" + i;
    create_pet( s );
  }
}

// enemy_t::health_percentage() =============================================

double enemy_t::health_percentage() const
{
  if ( fixed_health_percentage > 0 ) return fixed_health_percentage;

  if ( resources.base[ RESOURCE_HEALTH ] == 0 ) // first iteration
  {
    timespan_t remainder = std::max( timespan_t::zero(), ( sim -> expected_time - sim -> current_time ) );

    return ( remainder / sim -> expected_time ) * ( initial_health_percentage - sim -> target_death_pct ) + sim ->  target_death_pct;
  }

  return resources.pct( RESOURCE_HEALTH ) * 100 ;
}

// enemy_t::recalculate_health ==============================================

void enemy_t::recalculate_health()
{
  if ( sim -> expected_time <= timespan_t::zero() || fixed_health > 0 ) return;

  if ( initial_health == 0 ) // first iteration
  {
    initial_health = iteration_dmg_taken * ( sim -> expected_time / sim -> current_time );
  }
  else
  {
    timespan_t delta_time = sim -> current_time - sim -> expected_time;
    delta_time /= ( sim -> current_iteration + 1 ); // dampening factor
    double factor = 1.0 - ( delta_time / sim -> expected_time );

    if ( factor > 1.5 ) factor = 1.5;
    if ( factor < 0.5 ) factor = 0.5;

    initial_health *= factor;
  }

  if ( sim -> debug ) log_t::output( sim, "Target %s initial health calculated to be %.0f. Damage was %.0f", name(), initial_health, iteration_dmg_taken );
}

// enemy_t::create_expression ===============================================

expr_t* enemy_t::create_expression( action_t* action,
                                    const std::string& name_str )
{
  if ( name_str == "adds" )
    return make_ref_expr( name_str, active_pets );

  // override enemy health.pct expression
  if ( name_str == "health.pct" )
    return make_mem_fn_expr( name_str, *this, &enemy_t::health_percentage );

  return player_t::create_expression( action, name_str );
}

// enemy_t::combat_end ======================================================

void enemy_t::combat_end()
{
  player_t::combat_end();

  recalculate_health();
}

// ==========================================================================
// Enemy Add Extensions
// ==========================================================================

action_t* enemy_add_t::create_action( const std::string& name,
                                      const std::string& options_str )
{
  if ( name == "auto_attack"             ) return new auto_attack_t( this, options_str );
  if ( name == "spell_nuke"              ) return new spell_nuke_t( this, options_str );

  return pet_t::create_action( name, options_str );
}

// ==========================================================================
// PLAYER_T EXTENSIONS
// ==========================================================================

// player_t::create_enemy ===================================================

player_t* player_t::create_enemy( sim_t* sim, const std::string& name, race_type_e /* r */ )
{
  return new enemy_t( sim, name );
}

// player_t::enemy_init =====================================================

void player_t::enemy_init( sim_t* /* sim */ )
{

}

// player_t::enemy_combat_begin =============================================

void player_t::enemy_combat_begin( sim_t* /* sim */ )
{

}
