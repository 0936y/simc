// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

// ==========================================================================
// Action
// ==========================================================================

// action_t::action_t =======================================================

void action_t::init_dot( const std::string& name )
{
  std::unordered_map<std::string, std::pair<player_type_e, size_t> >::iterator doti = sim->targetdata_items[ 0 ].find( name );
  if ( doti != sim -> targetdata_items[ 0 ].end() && doti -> second.first == player->type )
    targetdata_dot_offset = ( int ) doti->second.second;
}

action_t::action_t( action_type_e       ty,
                    const std::string&  token,
                    player_t*           p,
                    const spell_data_t* s,
                    school_type_e       school ) :
  s_data( s ? s : spell_data_t::nil() ),
  sim( p -> sim ),
  type( ty ),
  name_str( token ),
  player( p ),
  target( p -> target ),
  school( school ),
  id(),
  result(),
  aoe(),
  dual(),
  callbacks( true ),
  special(),
  channeled(),
  background(),
  sequence(),
  direct_tick(),
  repeating(),
  harmful( true ),
  proc(),
  item_proc(),
  proc_ignores_slot(),
  may_trigger_dtr(),
  discharge_proc(),
  auto_cast(),
  initialized(),
  may_hit( true ),
  may_miss(),
  may_dodge(),
  may_parry(),
  may_glance(),
  may_block(),
  may_crush(),
  may_crit(),
  tick_may_crit(),
  tick_zero(),
  hasted_ticks(),
  no_buffs(),
  no_debuffs(),
  stateless(),
  dot_behavior( DOT_CLIP ),
  ability_lag( timespan_t::zero() ),
  ability_lag_stddev( timespan_t::zero() ),
  rp_gain(),
  min_gcd( timespan_t() ),
  trigger_gcd( player -> base_gcd ),
  range(),
  weapon_power_mod(),
  direct_power_mod(),
  tick_power_mod(),
  base_execute_time( timespan_t::zero() ),
  base_tick_time( timespan_t::zero() )

{
  dot_behavior                   = DOT_CLIP;
  trigger_gcd                    = player -> base_gcd;
  range                          = -1.0;
  weapon_power_mod               = 1.0/14.0;


  base_dd_min                    = 0.0;
  base_dd_max                    = 0.0;
  base_td                        = 0.0;
  base_td_init                   = 0.0;
  base_td_multiplier             = 1.0;
  base_dd_multiplier             = 1.0;
  base_multiplier                = 1.0;
  base_hit                       = 0.0;
  base_crit                      = 0.0;
  player_multiplier              = 1.0;
  player_td_multiplier           = 1.0;
  player_dd_multiplier           = 1.0;
  player_hit                     = 0.0;
  player_crit                    = 0.0;
  rp_gain                        = 0.0;
  target_multiplier              = 1.0;
  target_hit                     = 0.0;
  target_crit                    = 0.0;
  base_spell_power               = 0.0;
  base_attack_power              = 0.0;
  player_spell_power             = 0.0;
  player_attack_power            = 0.0;
  target_spell_power             = 0.0;
  target_attack_power            = 0.0;
  base_spell_power_multiplier    = 0.0;
  base_attack_power_multiplier   = 0.0;
  player_spell_power_multiplier  = 1.0;
  player_attack_power_multiplier = 1.0;
  crit_multiplier                = 1.0;
  crit_bonus_multiplier          = 1.0;
  base_dd_adder                  = 0.0;
  player_dd_adder                = 0.0;
  target_dd_adder                = 0.0;
  player_haste                   = 1.0;
  direct_dmg                     = 0.0;
  tick_dmg                       = 0.0;
  snapshot_crit                  = 0.0;
  snapshot_haste                 = 1.0;
  snapshot_mastery               = 0.0;
  num_ticks                      = 0;
  weapon                         = NULL;
  weapon_multiplier              = 1.0;
  base_add_multiplier            = 1.0;
  base_aoe_multiplier            = 1.0;
  normalize_weapon_speed         = false;
  rng_result                     = NULL;
  rng_travel                     = NULL;
  stats                          = NULL;
  execute_event                  = NULL;
  travel_event                   = NULL;
  time_to_execute                = timespan_t::zero();
  time_to_travel                 = timespan_t::zero();
  travel_speed                   = 0.0;
  resource_consumed              = 0.0;
  moving                         = -1;
  wait_on_ready                  = -1;
  interrupt                      = 0;
  round_base_dmg                 = true;
  class_flag1                    = false;
  if_expr_str.clear();
  if_expr                        = NULL;
  interrupt_if_expr_str.clear();
  interrupt_if_expr              = NULL;
  sync_str.clear();
  sync_action                    = NULL;
  marker                         = 0;
  last_reaction_time             = timespan_t::zero();
  dtr_action                     = 0;
  is_dtr_action                  = false;
  cached_targetdata = NULL;
  cached_targetdata_target = NULL;
  action_dot = NULL;
  targetdata_dot_offset = -1;
  // New Stuff
  stateless = false;
  snapshot_flags = 0;
  update_flags = STATE_MUL_TGT_DA | STATE_MUL_TGT_TA;
  state_cache = 0;
  execute_state = 0;

  range::fill( base_costs, 0.0 );

  if ( name_str.empty() )
  {
    assert( data().ok() );

    name_str = dbc_t::get_token( data().id() );

    if ( name_str.empty() )
    {
      name_str = data().name_cstr();
      util_t::tokenize( name_str );
      assert( ! name_str.empty() );
      dbc_t::add_token( data().id(), name_str );
    }
  }
  else
  {
    util_t::tokenize( name_str );
  }

  init_dot( name_str );

  if ( sim -> debug ) log_t::output( sim, "Player %s creates action %s", player -> name(), name() );

  if ( unlikely( ! player -> initialized ) )
  {
    sim -> errorf( "Actions must not be created before player_t::init().  Culprit: %s %s\n", player -> name(), name() );
    sim -> cancel();
  }

  player -> action_list.push_back( this );

  cooldown = player -> get_cooldown( name_str );

  stats = player -> get_stats( name_str , this );

  if ( &data() == &spell_data_not_found_t::singleton ) {
    sim -> errorf( "Player %s could not find action %s", player -> name(), name() );
    background = true; // prevent action from being executed
  }

  if ( data().ok() )
  {
    parse_spell_data( data() );
  }

  if ( data().id() && ! data().is_level( player -> level ) && data().level() <= MAX_LEVEL )
  {
    sim -> errorf( "Player %s attempting to execute action %s without the required level (%d < %d).\n",
                   player -> name(), name(), player -> level, data().level() );

    background = true; // prevent action from being executed
  }

  std::vector<specialization_e> spec_list;
  specialization_e _s = player -> primary_tree();
  if ( data().id() && player -> dbc.ability_specialization( data().id(), spec_list ) && range::find( spec_list, _s ) == spec_list.end() )
  {
    sim -> errorf( "Player %s attempting to execute action %s without the required spec.\n",
                   player -> name(), name() );

    background = true; // prevent action from being executed
  }
  spec_list.clear();
}

action_t::~action_t()
{
  if ( execute_state )
    delete execute_state;

  if ( ! is_dtr_action )
  {
    delete if_expr;
    delete interrupt_if_expr;
  }

  while ( state_cache )
  {
    action_state_t* s = state_cache;
    state_cache = s -> next;
    delete s;
  }
}

// action_t::parse_data =====================================================

void action_t::parse_spell_data( const spell_data_t& spell_data )
{
  if ( ! spell_data.id() ) // FIXME: Replace/augment with ok() check once it is in there
  {
    sim -> errorf( "%s %s: parse_spell_data: no spell to parse.\n", player -> name(), name() );
    return;
  }

  id                   = spell_data.id();
  base_execute_time    = spell_data.cast_time( player -> level );
  cooldown -> duration = spell_data.cooldown();
  range                = spell_data.max_range();
  travel_speed         = spell_data.missile_speed();
  trigger_gcd          = spell_data.gcd();
  school               = ( ( school == SCHOOL_NONE ) && spell_data.ok() ) ? spell_data.get_school_type() : school;
  stats -> school      = school;
  rp_gain              = spell_data.runic_power_gain();

  for ( size_t i = 0; spell_data._power && i < spell_data._power -> size(); i++ )
  {
    const spellpower_data_t* pd = spell_data._power -> at( i );

    if ( pd -> _cost > 0 )
      base_costs[ pd -> resource() ] = pd -> cost();
    else
      base_costs[ pd -> resource() ] = floor( pd -> cost() * player -> resources.base[ pd -> resource() ] );
  }

  for ( size_t i = 1; i <= spell_data._effects -> size(); i++ )
  {
    parse_effect_data( spell_data.effectN( i ) );
  }
}

// action_t::parse_effect_data ==============================================
void action_t::parse_effect_data( const spelleffect_data_t& spelleffect_data )
{
  if ( ! spelleffect_data.ok() )
  {
    return;
  }

  switch ( spelleffect_data.type() )
  {
    // Direct Damage
  case E_HEAL:
  case E_SCHOOL_DAMAGE:
  case E_HEALTH_LEECH:
    direct_power_mod = spelleffect_data.coeff();
    base_dd_min      = player -> dbc.effect_min( spelleffect_data.id(), player -> level );
    base_dd_max      = player -> dbc.effect_max( spelleffect_data.id(), player -> level );
    break;

  case E_NORMALIZED_WEAPON_DMG:
    normalize_weapon_speed = true;
  case E_WEAPON_DAMAGE:
    base_dd_min      = player -> dbc.effect_min( spelleffect_data.id(), player -> level );
    base_dd_max      = player -> dbc.effect_max( spelleffect_data.id(), player -> level );
    weapon = &( player -> main_hand_weapon );
    break;

  case E_WEAPON_PERCENT_DAMAGE:
    weapon = &( player -> main_hand_weapon );
    weapon_multiplier = player -> dbc.effect_min( spelleffect_data.id(), player -> level );
    break;

    // Dot
  case E_PERSISTENT_AREA_AURA:
  case E_APPLY_AURA:
    switch ( spelleffect_data.subtype() )
    {
    case A_PERIODIC_DAMAGE:
      if ( school == SCHOOL_PHYSICAL )
        school = stats -> school = SCHOOL_BLEED;
    case A_PERIODIC_LEECH:
    case A_PERIODIC_HEAL:
      tick_power_mod   = spelleffect_data.coeff();
      base_td_init     = player -> dbc.effect_average( spelleffect_data.id(), player -> level );
      base_td          = base_td_init;
    case A_PERIODIC_ENERGIZE:
    case A_PERIODIC_TRIGGER_SPELL_WITH_VALUE:
    case A_PERIODIC_HEALTH_FUNNEL:
    case A_PERIODIC_MANA_LEECH:
    case A_PERIODIC_DAMAGE_PERCENT:
    case A_PERIODIC_DUMMY:
    case A_PERIODIC_TRIGGER_SPELL:
      base_tick_time   = spelleffect_data.period();
      num_ticks        = ( int ) ( data().duration() / base_tick_time );
      break;
    case A_SCHOOL_ABSORB:
      direct_power_mod = spelleffect_data.coeff();
      base_dd_min      = player -> dbc.effect_min( spelleffect_data.id(), player -> level );
      base_dd_max      = player -> dbc.effect_max( spelleffect_data.id(), player -> level );
      break;
    case A_ADD_FLAT_MODIFIER:
      switch ( spelleffect_data.misc_value1() )
      case E_APPLY_AURA:
      switch ( spelleffect_data.subtype() )
      {
      case P_CRIT:
        base_crit += 0.01 * spelleffect_data.base_value();
        break;
      case P_COOLDOWN:
        cooldown -> duration += spelleffect_data.time_value();
        break;
      default: break;
      }
      break;
    case A_ADD_PCT_MODIFIER:
      switch ( spelleffect_data.misc_value1() )
      {
      case P_RESOURCE_COST:
        base_costs[ player -> primary_resource() ] *= 1 + 0.01 * spelleffect_data.base_value();
        break;
      }
      break;
    default: break;
    }
    break;
  default: break;
  }
}

// action_t::parse_options ==================================================

void action_t::parse_options( option_t*          options,
                              const std::string& options_str )
{
  // FIXME: remove deprecated options when all MoP class modules are finished
  option_t base_options[] =
  {
    { "bloodlust",              OPT_DEPRECATED, ( void* ) "if=buff.bloodlust.react" },
    { "haste<",                 OPT_DEPRECATED, ( void* ) "if=spell_haste>= or if=attack_haste>=" },
    { "health_percentage<",     OPT_DEPRECATED, ( void* ) "if=target.health.pct<=" },
    { "health_percentage>",     OPT_DEPRECATED, ( void* ) "if=target.health.pct>=" },
    { "if",                     OPT_STRING, &if_expr_str           },
    { "interrupt_if",           OPT_STRING, &interrupt_if_expr_str },
    { "interrupt",              OPT_BOOL,   &interrupt             },
    { "invulnerable",           OPT_DEPRECATED, ( void* ) "if=target.debuff.invulnerable.react" },
    { "not_flying",             OPT_DEPRECATED, ( void* ) "if=target.debuff.flying.down" },
    { "flying",                 OPT_DEPRECATED, ( void* ) "if=target.debuff.flying.react" },
    { "moving",                 OPT_BOOL,   &moving                },
    { "sync",                   OPT_STRING, &sync_str              },
    { "time<",                  OPT_DEPRECATED, ( void* ) "if=time<=" },
    { "time>",                  OPT_DEPRECATED, ( void* ) "if=time>=" },
    { "travel_speed",           OPT_DEPRECATED, ( void* ) "if=travel_speed" },
    { "vulnerable",             OPT_DEPRECATED, ( void* ) "if=target.debuff.vulnerable.react" },
    { "wait_on_ready",          OPT_BOOL,   &wait_on_ready         },
    { "target",                 OPT_STRING, &target_str            },
    { "label",                  OPT_STRING, &label_str             },
    { "use_off_gcd",            OPT_BOOL,   &use_off_gcd           },
    { NULL,                     OPT_NONE,   NULL                   }
  };

  std::vector<option_t> merged_options;
  option_t::merge( merged_options, options, base_options );

  std::string::size_type cut_pt = options_str.find( ':' );

  std::string options_buffer;
  if ( cut_pt != options_str.npos )
  {
    options_buffer = options_str.substr( cut_pt + 1 );
  }
  else options_buffer = options_str;

  if ( options_buffer.empty()     ) return;
  if ( options_buffer.size() == 0 ) return;

  if ( ! option_t::parse( sim, name(), merged_options, options_buffer ) )
  {
    sim -> errorf( "%s %s: Unable to parse options str '%s'.\n", player -> name(), name(), options_str.c_str() );
    sim -> cancel();
  }

  // FIXME: Move into constructor when parse_action is called from there.
  if ( ! target_str.empty() )
  {
    player_t* p = sim -> find_player( target_str );

    if ( p )
      target = p;
    else
    {
      sim -> errorf( "%s %s: Unable to locate target '%s'.\n", player -> name(), name(), options_str.c_str() );
    }
  }
}

// action_t::cost ===========================================================

double action_t::cost() const
{
  if ( ! harmful && ! player -> in_combat )
    return 0;

  double c = base_costs[ current_resource() ];

  c -= player -> stats_current.resource_reduction[ school ];
  if ( c < 0 ) c = 0;

  if ( current_resource() == RESOURCE_MANA )
  {
    if ( player -> buffs.power_infusion -> check() ) c *= ( 1.0 + player -> buffs.power_infusion -> data().effectN( 2 ).percent() );
  }

  if ( is_dtr_action )
    c = 0;

  if ( sim -> debug ) log_t::output( sim, "action_t::cost: %s %.2f %.2f %s", name(), base_costs[ current_resource() ], c, util_t::resource_type_string( current_resource() ) );

  return floor( c );
}

// action_t::gcd ============================================================

timespan_t action_t::gcd() const
{
  if ( ! harmful && ! player -> in_combat )
    return timespan_t::zero();

  return trigger_gcd;
}

// action_t::travel_time ====================================================

timespan_t action_t::travel_time() const
{
  if ( travel_speed == 0 ) return timespan_t::zero();

  if ( player -> distance == 0 ) return timespan_t::zero();

  double t = player -> distance / travel_speed;

  double v = sim -> travel_variance;

  if ( v )
  {
    t = rng_travel -> gauss( t, v );
  }

  return timespan_t::from_seconds( t );
}

// action_t::player_buff ====================================================

void action_t::player_buff()
{
  player_multiplier              = 1.0;
  player_dd_multiplier           = 1.0;
  player_td_multiplier           = 1.0;
  player_hit                     = 0;
  player_crit                    = 0;
  player_dd_adder                = 0;
  player_spell_power             = 0;
  player_attack_power            = 0;
  player_spell_power_multiplier  = 1.0;
  player_attack_power_multiplier = 1.0;

  if ( ! no_buffs )
  {
    player_t* p = player;

    player_multiplier    = p -> composite_player_multiplier   ( school, this );
    player_dd_multiplier = p -> composite_player_dd_multiplier( school, this );
    player_td_multiplier = p -> composite_player_td_multiplier( school, this );

    if ( base_attack_power_multiplier > 0 )
    {
      player_attack_power            = p -> composite_attack_power();
      player_attack_power_multiplier = p -> composite_attack_power_multiplier();
    }

    if ( base_spell_power_multiplier > 0 )
    {
      player_spell_power            = p -> composite_spell_power( school );
      player_spell_power_multiplier = p -> composite_spell_power_multiplier();
    }
  }

  player_haste = total_haste();

  if ( sim -> debug )
    log_t::output( sim, "action_t::player_buff: %s hit=%.2f crit=%.2f spell_power=%.2f attack_power=%.2f ",
                   name(), player_hit, player_crit, player_spell_power, player_attack_power );
}

// action_t::target_debuff ==================================================

void action_t::target_debuff( player_t* t, dmg_type_e )
{
  target_multiplier            = 1.0;
  target_hit                   = 0;
  target_crit                  = 0;
  target_attack_power          = 0;
  target_spell_power           = 0;
  target_dd_adder              = 0;

  if ( ! no_debuffs )
  {
    target_multiplier *= t -> composite_player_vulnerability( school );
  }

  if ( sim -> debug )
    log_t::output( sim, "action_t::target_debuff: %s (target=%s) multiplier=%.2f hit=%.2f crit=%.2f attack_power=%.2f spell_power=%.2f",
                   name(), t -> name(), target_multiplier, target_hit, target_crit, target_attack_power, target_spell_power );
}

// action_t::snapshot

void action_t::snapshot()
{
  snapshot_crit    = total_crit();
  snapshot_haste   = haste();
  snapshot_mastery = player -> composite_mastery();
}

// action_t::result_is_hit ==================================================

bool action_t::result_is_hit( result_type_e r ) const
{
  if ( r == RESULT_UNKNOWN ) r = result;

  return( r == RESULT_HIT        ||
          r == RESULT_CRIT       ||
          r == RESULT_GLANCE     ||
          r == RESULT_BLOCK      ||
          r == RESULT_CRIT_BLOCK ||
          r == RESULT_NONE       );
}

// action_t::result_is_miss =================================================

bool action_t::result_is_miss( result_type_e r ) const
{
  if ( r == RESULT_UNKNOWN ) r = result;

  return( r == RESULT_MISS   ||
          r == RESULT_DODGE  ||
          r == RESULT_PARRY );
}

// action_t::armor ==========================================================

double action_t::armor() const
{
  return target -> composite_armor();
}

// action_t::resistance =====================================================

double action_t::resistance() const
{
  return 0;
}

// action_t::total_crit_bonus ===============================================

double action_t::total_crit_bonus() const
{
  double bonus = ( ( 1.0 + crit_bonus ) * crit_multiplier - 1.0 ) * crit_bonus_multiplier;

  if ( sim -> debug )
  {
    log_t::output( sim, "%s crit_bonus for %s: cb=%.3f b_cb=%.2f b_cm=%.2f b_cbm=%.2f",
                   player -> name(), name(), bonus, crit_bonus, crit_multiplier, crit_bonus_multiplier );
  }

  return bonus;
}

// action_t::total_power ====================================================

double action_t::total_power() const
{
  double power=0;

  if ( base_spell_power_multiplier  > 0 ) power += total_spell_power();
  if ( base_attack_power_multiplier > 0 ) power += total_attack_power();

  return power;
}

// action_t::calculate_weapon_damage ========================================

double action_t::calculate_weapon_damage( double attack_power )
{
  if ( ! weapon || weapon_multiplier <= 0 ) return 0;

  double dmg = sim -> range( weapon -> min_dmg, weapon -> max_dmg ) + weapon -> bonus_dmg;

  timespan_t weapon_speed  = normalize_weapon_speed  ? weapon -> normalized_weapon_speed() : weapon -> swing_time;

  double power_damage = weapon_speed.total_seconds() * weapon_power_mod * attack_power;

  double total_dmg = dmg + power_damage;

  // OH penalty
  if ( weapon -> slot == SLOT_OFF_HAND )
    total_dmg *= 0.5;

  if ( sim -> debug )
  {
    log_t::output( sim, "%s weapon damage for %s: td=%.3f wd=%.3f bd=%.3f ws=%.3f pd=%.3f ap=%.3f",
                   player -> name(), name(), total_dmg, dmg, weapon -> bonus_dmg, weapon_speed.total_seconds(), power_damage, attack_power );
  }

  return total_dmg;
}

// action_t::calculate_tick_damage ==========================================

double action_t::calculate_tick_damage( result_type_e r, double power, double multiplier )
{
  double dmg = 0;

  if ( base_td == 0 ) base_td = base_td_init;

  if ( base_td == 0 && tick_power_mod == 0 ) return 0;

  dmg  = floor( base_td + 0.5 ) + power * tick_power_mod;
  dmg *= multiplier;

  double init_tick_dmg = dmg;

  if ( r == RESULT_CRIT )
  {
    dmg *= 1.0 + total_crit_bonus();
  }

  if ( ! sim -> average_range )
    dmg = floor( dmg + sim -> real() );

  if ( sim -> debug )
  {
    log_t::output( sim, "%s dmg for %s: td=%.0f i_td=%.0f b_td=%.0f mod=%.2f power=%.0f mult=%.2f",
                   player -> name(), name(), dmg, init_tick_dmg, base_td, tick_power_mod,
                   power, multiplier );
  }

  return dmg;
}

// action_t::calculate_direct_damage ========================================

double action_t::calculate_direct_damage( result_type_e r, int chain_target, unsigned target_level, double ap, double sp, double multiplier )
{
  double dmg = sim -> range( base_dd_min, base_dd_max );

  if ( round_base_dmg ) dmg = floor( dmg + 0.5 );

  if ( dmg == 0 && weapon_multiplier == 0 && direct_power_mod == 0 ) return 0;

  double base_direct_dmg = dmg;
  double weapon_dmg = 0;

  dmg += base_dd_adder + player_dd_adder + target_dd_adder;

  if ( weapon_multiplier > 0 )
  {
    // x% weapon damage + Y
    // e.g. Obliterate, Shred, Backstab
    dmg += calculate_weapon_damage( ap );
    dmg *= weapon_multiplier;
    weapon_dmg = dmg;
  }
  dmg += direct_power_mod * ( ap + sp );
  dmg *= multiplier;

  double init_direct_dmg = dmg;

  if ( r == RESULT_GLANCE )
  {
    double delta_skill = ( target_level - player -> level ) * 5.0;

    if ( delta_skill < 0.0 )
      delta_skill = 0.0;

    double max_glance = 1.3 - 0.03 * delta_skill;

    if ( max_glance > 0.99 )
      max_glance = 0.99;
    else if ( max_glance < 0.2 )
      max_glance = 0.20;

    double min_glance = 1.4 - 0.05 * delta_skill;

    if ( min_glance > 0.91 )
      min_glance = 0.91;
    else if ( min_glance < 0.01 )
      min_glance = 0.01;

    if ( min_glance > max_glance )
    {
      double temp = min_glance;
      min_glance = max_glance;
      max_glance = temp;
    }

    dmg *= sim -> range( min_glance, max_glance ); // 0.75 against +3 targets.
  }
  else if ( r == RESULT_CRIT )
  {
    dmg *= 1.0 + total_crit_bonus();
  }

  // AoE with decay per target
  if ( chain_target > 0 && base_add_multiplier != 1.0 )
    dmg *= pow( base_add_multiplier, chain_target );

  // AoE with static reduced damage per target
  if ( chain_target > 1 && base_aoe_multiplier != 1.0 )
    dmg *= base_aoe_multiplier;

  if ( ! sim -> average_range ) dmg = floor( dmg + sim -> real() );

  if ( sim -> debug )
  {
    log_t::output( sim, "%s dmg for %s: dd=%.0f i_dd=%.0f w_dd=%.0f b_dd=%.0f mod=%.2f power=%.0f mult=%.2f w_mult=%.2f",
                   player -> name(), name(), dmg, init_direct_dmg, weapon_dmg, base_direct_dmg, direct_power_mod,
                   ( ap + sp ), multiplier, weapon_multiplier );
  }

  return dmg;
}

// action_t::consume_resource ===============================================

void action_t::consume_resource()
{
  if ( current_resource() == RESOURCE_NONE || base_costs[ current_resource() ] == 0 || proc ) return;

  resource_consumed = cost();

  player -> resource_loss( current_resource(), resource_consumed, 0, this );

  if ( sim -> log )
    log_t::output( sim, "%s consumes %.1f %s for %s (%.0f)", player -> name(),
                   resource_consumed, util_t::resource_type_string( current_resource() ),
                   name(), player -> resources.current[ current_resource() ] );

  stats -> consume_resource( current_resource(), resource_consumed );
}

// action_t::available_targets ==============================================

size_t action_t::available_targets( std::vector< player_t* >& tl ) const
{
  // TODO: This does not work for heals at all, as it presumes enemies in the
  // actor list.

  tl.push_back( target );

  for ( size_t i = 0, actors = sim -> actor_list.size(); i < actors; i++ )
  {
    if ( ! sim -> actor_list[ i ] -> sleeping &&
         ( ( type == ACTION_HEAL && !sim -> actor_list[ i ] -> is_enemy() ) || ( type != ACTION_HEAL && sim -> actor_list[ i ] -> is_enemy() ) ) &&
         sim -> actor_list[ i ] != target )
      tl.push_back( sim -> actor_list[ i ] );
  }

  return tl.size();
}

// action_t::target_list ====================================================

std::vector< player_t* > action_t::target_list() const
{
  // A very simple target list for aoe spells, pick any and all targets, up to
  // aoe amount, or if aoe == -1, pick all (enemy) targets

  std::vector< player_t* > t;

  size_t total_targets = available_targets( t );

  if ( aoe == -1 || total_targets <= static_cast< size_t >( aoe + 1 ) )
    return t;
  // Drop out targets from the end
  else
  {
    t.resize( aoe + 1 );

    return t;
  }
}

// action_t::execute ========================================================

void action_t::execute()
{
  if ( unlikely( ! initialized ) )
  {
    sim -> errorf( "action_t::execute: action %s from player %s is not initialized.\n", name(), player -> name() );
    assert( 0 );
  }

  if ( sim -> log && ! dual )
  {
    log_t::output( sim, "%s performs %s (%.0f)", player -> name(), name(),
                   player -> resources.current[ player -> primary_resource() ] );
  }

  if ( harmful )
  {
    if ( player -> in_combat == false && sim -> debug )
      log_t::output( sim, "%s enters combat.", player -> name() );

    player -> in_combat = true;
  }

  if ( ! stateless )
  {
    player_buff();

    if ( aoe == -1 || aoe > 0 )
    {
      std::vector< player_t* > tl = target_list();

      for ( size_t t = 0, targets = tl.size(); t < targets; t++ )
      {
        target_debuff( tl[ t ], DMG_DIRECT );

        result = calculate_result( total_crit(), tl[ t ] -> level );

        if ( result_is_hit() )
          direct_dmg = calculate_direct_damage( result, t + 1, tl[ t ] -> level, total_attack_power(), total_spell_power(), total_dd_multiplier() );

        schedule_travel( tl[ t ] );
      }
    }
    else
    {
      target_debuff( target, DMG_DIRECT );

      result = calculate_result( total_crit(), target -> level );

      if ( result_is_hit() )
        direct_dmg = calculate_direct_damage( result, 0, target -> level, total_attack_power(), total_spell_power(), total_dd_multiplier() );

      schedule_travel( target );
    }
  }
  else
  {
    if ( aoe == -1 || aoe > 0 ) // stateless aoe
    {
      std::vector< player_t* > tl = target_list();

      for ( size_t t = 0, targets = tl.size(); t < targets; t++ )
      {
        action_state_t* s = get_state();
        s -> target = tl[ t ];
        snapshot_state( s, snapshot_flags );
        s -> result = calculate_result( s -> crit, s -> target -> level );

        if ( result_is_hit( s -> result ) )
          s -> result_amount = calculate_direct_damage( s -> result, t + 1, s -> target -> level, s -> attack_power, s -> spell_power, s -> composite_da_multiplier() );

        if ( sim -> debug )
          s -> debug();

        schedule_travel_s( s );
      }
    }
    else // stateless single target
    {
      action_state_t* s = get_state();
      s -> target = target;
      snapshot_state( s, snapshot_flags );
      s -> result = calculate_result( s -> crit, s -> target -> level );

      if ( result_is_hit( s -> result ) )
        s -> result_amount = calculate_direct_damage( s -> result, 0, s -> target -> level, s -> attack_power, s -> spell_power, s -> composite_da_multiplier() );

      if ( sim -> debug )
        s -> debug();

      schedule_travel_s( s );
    }
  }

  consume_resource();

  update_ready();

  if ( ! dual ) stats -> add_execute( time_to_execute );

  if ( repeating && ! proc ) schedule_execute();
}

// action_t::tick ===========================================================

void action_t::tick( dot_t* d )
{
  if ( sim -> debug ) log_t::output( sim, "%s ticks (%d of %d)", name(), d -> current_tick, d -> num_ticks );

  if ( ! stateless )
  {
    result = RESULT_HIT;

    player_tick();

    target_debuff( target, type == ACTION_HEAL ? HEAL_OVER_TIME : DMG_OVER_TIME );

    if ( tick_may_crit )
    {
      int delta_level = target -> level - player -> level;

      if ( rng_result-> roll( crit_chance( total_crit(), delta_level ) ) )
      {
        result = RESULT_CRIT;
      }
    }

    tick_dmg = calculate_tick_damage( result, total_power(), total_td_multiplier() );

    d -> prev_tick_amount = tick_dmg;

    assess_damage( target, tick_dmg, type == ACTION_HEAL ? HEAL_OVER_TIME : DMG_OVER_TIME, result );

    if ( harmful && callbacks ) action_callback_t::trigger( player -> callbacks.tick[ result ], this );
  }
  else
  {
    d -> state -> result = RESULT_HIT;
    snapshot_state( d -> state, update_flags );

    if ( tick_may_crit )
    {
      if ( rng_result -> roll( crit_chance( d -> state -> crit, d -> state -> target -> level - player -> level ) ) )
        d -> state -> result = RESULT_CRIT;
    }

    d -> state -> result_amount = calculate_tick_damage( d -> state -> result, d -> state -> composite_power(), d -> state -> composite_ta_multiplier() );

    assess_damage( d -> state -> target, d -> state -> result_amount, type == ACTION_HEAL ? HEAL_OVER_TIME : DMG_OVER_TIME, d -> state -> result );

    if ( harmful && callbacks )
      action_callback_t::trigger( player -> callbacks.tick[ d -> state -> result ], this );

    if ( sim -> debug )
      d -> state -> debug();
  }

  stats -> add_tick( d -> time_to_tick );

  player -> trigger_ready();
}

// action_t::last_tick ======================================================

void action_t::last_tick( dot_t* d )
{
  if ( sim -> debug ) log_t::output( sim, "%s fades from %s", d -> name(), target -> name() );

  d -> ticking = false;
  if ( d -> state )
  {
    release_state( d -> state );
    d -> state = 0;
  }

  if ( school == SCHOOL_BLEED ) target -> debuffs.bleeding -> decrement();
}

// action_t::impact =========================================================

void action_t::impact( player_t* t, result_type_e impact_result, double impact_dmg )
{
  assess_damage( t, impact_dmg, type == ACTION_HEAL ? HEAL_DIRECT : DMG_DIRECT, impact_result );

  // Set target so aoe dots work
  player_t* orig_target = target;
  target = t;

  if ( result_is_hit( impact_result ) )
  {
    if ( num_ticks > 0 )
    {
      dot_t* dot = this -> dot( t );
      if ( dot_behavior == DOT_CLIP ) dot -> cancel();
      dot -> action = this;
      dot -> num_ticks = hasted_num_ticks( player_haste );
      dot -> current_tick = 0;
      dot -> added_ticks = 0;
      dot -> added_seconds = timespan_t::zero();
      if ( dot -> ticking )
      {
        assert( dot -> tick_event );
        if ( ! channeled )
        {
          // Recasting a dot while it's still ticking gives it an extra tick in total
          dot -> num_ticks++;

          // tick_zero dots tick again when reapplied
          if ( tick_zero )
          {
            tick( dot );
          }
        }
      }
      else
      {
        if ( school == SCHOOL_BLEED ) target -> debuffs.bleeding -> increment();

        dot -> schedule_tick();
      }
      dot -> recalculate_ready();

      if ( sim -> debug )
        log_t::output( sim, "%s extends dot-ready to %.2f for %s (%s)",
                       player -> name(), dot -> ready.total_seconds(), name(), dot -> name() );
    }
  }
  else
  {
    if ( sim -> log )
    {
      log_t::output( sim, "Target %s avoids %s %s (%s)", target -> name(), player -> name(), name(), util_t::result_type_string( impact_result ) );
    }
  }

  // Reset target
  target = orig_target;
}

// action_t::assess_damage ==================================================

void action_t::assess_damage( player_t*     t,
                              double        amount,
                              dmg_type_e    type,
                              result_type_e result )
{
  double dmg_adjusted = t -> assess_damage( amount, school, type, result, this );
  double actual_amount = t -> resources.is_infinite( RESOURCE_HEALTH ) ? dmg_adjusted : std::min( dmg_adjusted, t -> resources.current[ RESOURCE_HEALTH ] );

  if ( type == DMG_DIRECT )
  {
    if ( sim -> log )
    {
      log_t::output( sim, "%s %s hits %s for %.0f %s damage (%s)",
                     player -> name(), name(),
                     t -> name(), dmg_adjusted,
                     util_t::school_type_string( school ),
                     util_t::result_type_string( result ) );
    }

    direct_dmg = dmg_adjusted;

    if ( callbacks ) action_callback_t::trigger( player -> callbacks.direct_damage[ school ], this );
  }
  else // DMG_OVER_TIME
  {
    if ( sim -> log )
    {
      dot_t* dot = this -> dot( t );
      log_t::output( sim, "%s %s ticks (%d of %d) %s for %.0f %s damage (%s)",
                     player -> name(), name(),
                     dot -> current_tick, dot -> num_ticks,
                     t -> name(), dmg_adjusted,
                     util_t::school_type_string( school ),
                     util_t::result_type_string( result ) );
    }

    tick_dmg = dmg_adjusted;

    if ( callbacks ) action_callback_t::trigger( player -> callbacks.tick_damage[ school ], this );
  }

  stats -> add_result( actual_amount, dmg_adjusted, ( direct_tick ? DMG_OVER_TIME : type ), result );
}

// action_t::additional_damage ==============================================

void action_t::additional_damage( player_t*     t,
                                  double        amount,
                                  dmg_type_e    type,
                                  result_type_e result )
{
  amount /= target_multiplier; // FIXME! Weak lip-service to the fact that the adds probably will not be properly debuffed.
  double dmg_adjusted = t -> assess_damage( amount, school, type, result, this );
  double actual_amount = std::min( dmg_adjusted, t -> resources.current[ current_resource() ] );
  stats -> add_result( actual_amount, amount, type, result );
}

// action_t::schedule_execute ===============================================

void action_t::schedule_execute()
{
  if ( sim -> log )
  {
    log_t::output( sim, "%s schedules execute for %s", player -> name(), name() );
  }

  time_to_execute = execute_time();

  execute_event = new ( sim ) action_execute_event_t( sim, this, time_to_execute );

  if ( ! background )
  {
    player -> executing = this;
    player -> gcd_ready = sim -> current_time + gcd();
    if ( player -> action_queued && sim -> strict_gcd_queue )
    {
      player -> gcd_ready -= sim -> queue_gcd_reduction;
    }

    if ( special && time_to_execute > timespan_t::zero() && ! proc )
    {
      // While an ability is casting, the auto_attack is paused
      // So we simply reschedule the auto_attack by the ability's casttime
      timespan_t time_to_next_hit;
      // Mainhand
      if ( player -> main_hand_attack && player -> main_hand_attack -> execute_event )
      {
        time_to_next_hit  = player -> main_hand_attack -> execute_event -> occurs();
        time_to_next_hit -= sim -> current_time;
        time_to_next_hit += time_to_execute;
        player -> main_hand_attack -> execute_event -> reschedule( time_to_next_hit );
      }
      // Offhand
      if ( player -> off_hand_attack && player -> off_hand_attack -> execute_event )
      {
        time_to_next_hit  = player -> off_hand_attack -> execute_event -> occurs();
        time_to_next_hit -= sim -> current_time;
        time_to_next_hit += time_to_execute;
        player -> off_hand_attack -> execute_event -> reschedule( time_to_next_hit );
      }
    }
  }
}

// action_t::schedule_travel ================================================

void action_t::schedule_travel( player_t* t )
{
  time_to_travel = travel_time();

  snapshot();

  if ( time_to_travel == timespan_t::zero() )
  {
    impact( t, result, direct_dmg );
  }
  else
  {
    if ( sim -> log )
    {
      log_t::output( sim, "%s schedules travel (%.2f) for %s", player -> name(), time_to_travel.total_seconds(), name() );
    }

    travel_event = new ( sim ) action_travel_event_t( sim, t, this, time_to_travel );
  }
}

// action_t::reschedule_execute =============================================

void action_t::reschedule_execute( timespan_t time )
{
  if ( sim -> log )
  {
    log_t::output( sim, "%s reschedules execute for %s", player -> name(), name() );
  }

  timespan_t delta_time = sim -> current_time + time - execute_event -> occurs();

  time_to_execute += delta_time;

  if ( delta_time > timespan_t::zero() )
  {
    execute_event -> reschedule( time );
  }
  else // Impossible to reschedule events "early".  Need to be canceled and re-created.
  {
    event_t::cancel( execute_event );
    execute_event = new ( sim ) action_execute_event_t( sim, this, time );
  }
}

// action_t::update_ready ===================================================

void action_t::update_ready()
{
  timespan_t delay = timespan_t::zero();
  if ( cooldown -> duration > timespan_t::zero() && ! dual )
  {

    if ( ! background && ! proc )
    {
      timespan_t lag, dev;

      lag = player -> world_lag_override ? player -> world_lag : sim -> world_lag;
      dev = player -> world_lag_stddev_override ? player -> world_lag_stddev : sim -> world_lag_stddev;
      delay = player -> rngs.lag_world -> gauss( lag, dev );
      if ( sim -> debug ) log_t::output( sim, "%s delaying the cooldown finish of %s by %f", player -> name(), name(), delay.total_seconds() );
    }

    cooldown -> start( timespan_t::min(), delay );

    if ( sim -> debug ) log_t::output( sim, "%s starts cooldown for %s (%s). Will be ready at %.4f", player -> name(), name(), cooldown -> name(), cooldown -> ready.total_seconds() );
  }
  if ( num_ticks )
  {
    if ( result_is_miss() )
    {
      dot_t* dot = this -> dot();
      last_reaction_time = player -> total_reaction_time();
      if ( sim -> debug )
        log_t::output( sim, "%s pushes out re-cast (%.2f) on miss for %s (%s)",
                       player -> name(), last_reaction_time.total_seconds(), name(), dot -> name() );

      dot -> miss_time = sim -> current_time;
    }
  }
}

// action_t::usable_moving ==================================================

bool action_t::usable_moving()
{
  bool usable = true;

  if ( execute_time() > timespan_t::zero() )
    return false;

  if ( channeled )
    return false;

  if ( range > 0 && range <= 5 )
    return false;

  return usable;
}

// action_t::ready ==========================================================

bool action_t::ready()
{
  player_t* t = target;

  if ( unlikely( is_dtr_action ) )
    assert( false );

  if ( player -> skill < 1.0 && ! sim -> roll( player -> skill ) )
    return false;

  if ( cooldown -> remains() > timespan_t::zero() )
    return false;

  if ( ! player -> resource_available( current_resource(), cost() ) )
    return false;

  if ( if_expr && ! if_expr -> success() )
    return false;

  if ( sync_action && ! sync_action -> ready() )
    return false;

  if ( unlikely( t -> sleeping ) )
    return false;

  if ( target -> debuffs.invulnerable -> check() && harmful )
    return false;

  if ( player -> is_moving() && ! usable_moving() )
    return false;

  if ( moving != -1 && moving != ( player -> is_moving() ? 1 : 0 ) )
    return false;

  return true;
}

// action_t::init ===========================================================

void action_t::init()
{
  if ( initialized ) return;

  rng_result = player -> get_rng( name_str + "_result" );

  if ( ! sync_str.empty() )
  {
    sync_action = player -> find_action( sync_str );

    if ( ! sync_action )
    {
      sim -> errorf( "Unable to find sync action '%s' for primary action '%s'\n", sync_str.c_str(), name() );
      sim -> cancel();
    }
  }

  if ( ! if_expr_str.empty() && ! is_dtr_action )
  {
    if_expr = expr_t::parse( this, if_expr_str );
  }

  if ( ! interrupt_if_expr_str.empty() )
  {
    interrupt_if_expr = expr_t::parse( this, interrupt_if_expr_str );
  }

  if ( sim -> travel_variance && travel_speed && player -> distance )
    rng_travel = player -> get_rng( name_str + "_travel" );

  if ( is_dtr_action )
  {
    cooldown = player -> get_cooldown( name_str + "_DTR" );
    cooldown -> duration = timespan_t::zero();

    if ( sim -> separate_stats_by_actions <= 0 )
      stats = player -> get_stats( stats -> name_str + "_DTR", this );

    background = true;
  }

  if ( may_crit || tick_may_crit )
    snapshot_flags |= STATE_CRIT;

  if ( base_td > 0 || num_ticks > 0 )
    snapshot_flags |= STATE_MUL_TA | STATE_MUL_TGT_TA;

  if ( ( base_dd_min > 0 && base_dd_max > 0 ) || weapon_multiplier > 0 )
    snapshot_flags |= STATE_MUL_DA | STATE_MUL_TGT_DA;

  if ( ! ( background || sequence ) )
    player->foreground_action_list.push_back( this );

  initialized = true;
}

// action_t::reset ==========================================================

void action_t::reset()
{
  cooldown -> reset();
  if ( action_dot )
    action_dot -> reset();
  result = RESULT_NONE;
  execute_event = 0;
  travel_event = 0;
}

// action_t::cancel =========================================================

void action_t::cancel()
{
  if ( sim -> debug ) log_t::output( sim, "action %s of %s is canceled", name(), player -> name() );

  if ( channeled )
  {
    dot_t* dot = this -> dot();
    if ( dot -> ticking )
    {
      last_tick( dot );
      event_t::cancel( dot -> tick_event );
      dot -> reset();
    }
  }

  if ( player -> executing  == this ) player -> executing  = 0;
  if ( player -> channeling == this ) player -> channeling = 0;

  event_t::cancel( execute_event );

  player -> debuffs.casting -> expire();
}

// action_t::interrupt ======================================================

void action_t::interrupt_action()
{
  if ( sim -> debug ) log_t::output( sim, "action %s of %s is interrupted", name(), player -> name() );

  if ( cooldown -> duration > timespan_t::zero() && ! dual )
  {
    if ( sim -> debug ) log_t::output( sim, "%s starts cooldown for %s (%s)", player -> name(), name(), cooldown -> name() );

    cooldown -> start();
  }

  if ( player -> executing  == this ) player -> executing  = 0;
  if ( player -> channeling == this )
  {
    dot_t* dot = this->dot();
    if ( dot -> ticking ) last_tick( dot );
    player -> channeling = 0;
    event_t::cancel( dot -> tick_event );
    dot -> reset();
  }

  event_t::cancel( execute_event );

  player -> debuffs.casting -> expire();
}

// action_t::check_talent ===================================================

void action_t::check_talent( int talent_rank )
{
  if ( talent_rank != 0 ) return;

  if ( player -> is_pet() )
  {
    pet_t* p = player -> cast_pet();
    sim -> errorf( "Player %s has pet %s attempting to execute action %s without the required talent.\n",
                   p -> owner -> name(), p -> name(), name() );
  }
  else
  {
    sim -> errorf( "Player %s attempting to execute action %s without the required talent.\n", player -> name(), name() );
  }

  background = true; // prevent action from being executed
}

// action_t::check_race =====================================================

void action_t::check_race( race_type_e race )
{
  if ( player -> race != race )
  {
    sim -> errorf( "Player %s attempting to execute action %s while not being a %s.\n", player -> name(), name(), util_t::race_type_string( race ) );

    background = true; // prevent action from being executed
  }
}

// action_t::check_spec =====================================================

void action_t::check_spec( specialization_e necessary_spec )
{
  if ( player -> primary_tree() != necessary_spec )
  {
    sim -> errorf( "Player %s attempting to execute action %s without %s spec.\n",
                   player -> name(), name(), util_t::specialization_string( necessary_spec ).c_str() );

    background = true; // prevent action from being executed
  }
}

// action_t::check_spec =====================================================

void action_t::check_spell( const spell_data_t* sp )
{
  if ( ! sp -> ok() )
  {
    sim -> errorf( "Player %s attempting to execute action %s without spell ok().\n",
                   player -> name(), name() );

    background = true; // prevent action from being executed
  }
}

// action_t::create_expression ==============================================

expr_t* action_t::create_expression( const std::string& name_str )
{
  class action_expr_t : public expr_t
  {
  public:
    const action_t& action;

    action_expr_t( const std::string& name, const action_t& a ) :
      expr_t( name ), action( a ) {}
  };

  if ( name_str == "n_ticks" )
  {
    struct n_ticks_expr_t : public action_expr_t
    {
      n_ticks_expr_t( const action_t& a ) : action_expr_t( "n_ticks", a ) {}
      virtual double evaluate() {
        int n_ticks = action.hasted_num_ticks( action.player -> composite_spell_haste() );
        if ( action.dot_behavior == DOT_EXTEND && action.dot() -> ticking )
          n_ticks += std::min( (int) ( n_ticks / 2 ), action.dot() -> num_ticks - action.dot() -> current_tick );
        return n_ticks; 
      }
    };
    return new n_ticks_expr_t( *this );
  }
  else if ( name_str == "add_ticks" )
  {
    struct add_ticks_expr_t : public action_expr_t
    {
      add_ticks_expr_t( const action_t& a ) : action_expr_t( "add_ticks", a ) {}
      virtual double evaluate() { return action.hasted_num_ticks( action.player -> composite_spell_haste() ); }
    };
    return new add_ticks_expr_t( *this );
  }
  else if ( name_str == "cast_time" )
    return make_mem_fn_expr( name_str, *this, &action_t::execute_time );
  else if ( name_str == "cooldown" )
    return make_ref_expr( name_str, cooldown -> duration );
  else if ( name_str == "tick_time" )
  {
    struct tick_time_expr_t : public action_expr_t
    {
      tick_time_expr_t( const action_t& a ) : action_expr_t( "tick_time", a ) {}
      virtual double evaluate()
      {
        dot_t* dot = action.dot();
        if ( dot -> ticking )
          return action.tick_time( action.player_haste ).total_seconds();
        else
          return 0;
      }
    };
    return new tick_time_expr_t( *this );
  }
  else if ( name_str == "new_tick_time" )
  {
    struct new_tick_time_expr_t : public action_expr_t
    {
      new_tick_time_expr_t( const action_t& a ) : action_expr_t( "new_tick_time", a ) {}
      virtual double evaluate()
      {
        return action.tick_time( action.player -> composite_spell_haste() ).total_seconds();
      }
    };
    return new new_tick_time_expr_t( *this );
  }
  else if ( name_str == "gcd" )
    return make_mem_fn_expr( name_str, *this, &action_t::gcd );
  else if ( name_str == "travel_time" )
    return make_mem_fn_expr( name_str, *this, &action_t::travel_time );
  else if ( name_str == "in_flight" )
  {
    struct in_flight_expr_t : public action_expr_t
    {
      in_flight_expr_t( const action_t& a ) : action_expr_t( "in_flight", a ) {}
      virtual double evaluate() { return action.travel_event != NULL; }
    };
    return new in_flight_expr_t( *this );
  }

  else if ( expr_t* q = dot() -> create_expression( name_str ) )
    return q;

  else if ( name_str == "miss_react" )
  {
    struct miss_react_expr_t : public action_expr_t
    {
      miss_react_expr_t( const action_t& a ) : action_expr_t( "miss_react", a ) {}
      virtual double evaluate()
      {
        dot_t* dot = action.dot();
        if ( dot -> miss_time < timespan_t::zero() ||
             action.sim -> current_time >= ( dot -> miss_time + action.last_reaction_time ) )
          return true;
        else
          return false;
      }
    };
    return new miss_react_expr_t( *this );
  }
  else if ( name_str == "cast_delay" )
  {
    struct cast_delay_expr_t : public action_expr_t
    {
      cast_delay_expr_t( const action_t& a ) : action_expr_t( "cast_delay", a ) {}
      virtual double evaluate()
      {
        if ( action.sim -> debug )
        {
          log_t::output( action.sim, "%s %s cast_delay(): can_react_at=%f cur_time=%f",
                         action.player -> name_str.c_str(),
                         action.name_str.c_str(),
                         ( action.player -> cast_delay_occurred + action.player -> cast_delay_reaction ).total_seconds(),
                         action.sim -> current_time.total_seconds() );
        }

        if ( action.player -> cast_delay_occurred == timespan_t::zero() ||
             action.player -> cast_delay_occurred + action.player -> cast_delay_reaction < action.sim -> current_time )
          return true;
        else
          return false;
      }
    };
    return new cast_delay_expr_t( *this );
  }

  std::vector<std::string> splits;
  int num_splits = util_t::string_split( splits, name_str, "." );

  if ( num_splits == 2 )
  {
    if ( splits[ 0 ] == "prev" )
    {
      struct prev_expr_t : public action_expr_t
      {
        std::string prev_action;
        prev_expr_t( const action_t& a, const std::string& prev_action ) : action_expr_t( "prev", a ), prev_action( prev_action ) {}
        virtual double evaluate()
        {
          if ( action.player -> last_foreground_action )
            return action.player -> last_foreground_action -> name_str == prev_action;
          return false;
        }
      };

      return new prev_expr_t( *this, splits[ 1 ] );
    }
  }

  if ( num_splits == 3 && ( splits[0] == "buff" || splits[0] == "debuff" || splits[0] == "aura" ) )
  {
    buff_t* buff = sim -> get_targetdata_aura( player, target, splits[1] );
    if ( buff )
      return buff -> create_expression( splits[ 2 ] );
  }

  if ( num_splits >= 2 && ( splits[ 0 ] == "debuff" || splits[ 0 ] == "dot" ) )
  {
    return target -> create_expression( this, name_str );
  }

  if ( num_splits >= 2 && splits[ 0 ] == "aura" )
  {
    return sim -> create_expression( this, name_str );
  }

  if ( num_splits > 2 && splits[ 0 ] == "target" )
  {
    // Find target
    player_t* expr_target = sim -> find_player( splits[ 1 ] );
    size_t start_rest = 2;
    if ( ! expr_target )
    {
      expr_target = target;
      start_rest = 1;
    }

    assert( expr_target );

    std::string rest = splits[ start_rest ];
    for ( int i = start_rest + 1; i < num_splits; ++i )
      rest += '.' + splits[ i ];

    return expr_target -> create_expression( this, rest );
  }

  // necessary for self.target.*, self.dot.*
  if ( num_splits >= 2 && splits[ 0 ] == "self" )
  {
    std::string rest = splits[1];
    for ( int i = 2; i < num_splits; ++i )
      rest += '.' + splits[i];
    return player -> create_expression( this, rest );
  }

  // necessary for sim.target.*
  if ( num_splits >= 2 && splits[ 0 ] == "sim" )
  {
    std::string rest = splits[1];
    for ( int i = 2; i < num_splits; ++i )
      rest += '.' + splits[i];
    return sim -> create_expression( this, rest );
  }

  if ( name_str == "enabled" )
  {
    struct ok_expr_t : public action_expr_t
    {
      ok_expr_t( const action_t& a ) : action_expr_t( "enabled", a ) {}
      virtual double evaluate() { return action.data().found(); }
    };
    return new ok_expr_t( *this );
  }

  return player -> create_expression( this, name_str );
}

// action_t::ppm_proc_chance ================================================

double action_t::ppm_proc_chance( double PPM ) const
{
  if ( weapon )
  {
    return weapon -> proc_chance_on_swing( PPM );
  }
  else
  {
    timespan_t time = channeled ? dot() -> time_to_tick : time_to_execute;

    if ( time == timespan_t::zero() ) time = player -> base_gcd;

    return ( PPM * time.total_minutes() );
  }
}

// action_t::tick_time ======================================================

timespan_t action_t::tick_time( double haste ) const
{
  timespan_t t = base_tick_time;
  if ( channeled || hasted_ticks )
  {
    t *= haste;
  }
  return t;
}

// action_t::hasted_num_ticks ===============================================

int action_t::hasted_num_ticks( double haste, timespan_t d ) const
{
  if ( ! hasted_ticks ) return num_ticks;

  assert( player_haste > 0.0 );

  // For the purposes of calculating the number of ticks, the tick time is rounded to the 3rd decimal place.
  // It's important that we're accurate here so that we model haste breakpoints correctly.

  if ( d < timespan_t::zero() )
    d = num_ticks * base_tick_time;

  timespan_t t = timespan_t::from_millis( ( int ) ( ( base_tick_time.total_millis() * haste ) + 0.5 ) );

  double n = d / t;

  // banker's rounding
  if ( n - 0.5 == ( double ) ( int ) n && ( ( int ) n ) % 2 == 0 )
    return ( int ) ceil ( n - 0.5 );

  return ( int ) floor( n + 0.5 );
}

// action_t::snapshot_state ===================================================

void action_t::snapshot_state( action_state_t* state, uint32_t flags )
{
  assert( state );

  if ( flags & STATE_CRIT )
    state -> crit = composite_crit( state );

  if ( flags & STATE_HASTE )
    state -> haste = composite_haste();

  if ( flags & STATE_AP )
    state -> attack_power = util_t::round( composite_attack_power() * composite_attack_power_multiplier() );

  if ( flags & STATE_SP )
    state -> spell_power = util_t::round( composite_spell_power() * composite_spell_power_multiplier() );

  if ( flags & STATE_MUL_DA )
    state -> da_multiplier = composite_da_multiplier( state );

  if ( flags & STATE_MUL_TA )
    state -> ta_multiplier = composite_ta_multiplier( state );

  if ( flags & STATE_MUL_TGT_DA )
    state -> target_da_multiplier = composite_target_da_multiplier( state -> target );

  if ( flags & STATE_MUL_TGT_TA )
    state -> target_ta_multiplier = composite_target_ta_multiplier( state -> target );
}

