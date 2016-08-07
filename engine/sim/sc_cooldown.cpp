// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

namespace { // UNNAMED NAMESPACE

struct recharge_event_t : event_t
{
  cooldown_t* cooldown_;
  timespan_t duration_, event_duration_;

  recharge_event_t( cooldown_t* cd, timespan_t base_duration ) :
    event_t( cd -> sim ), cooldown_( cd ),
    duration_( base_duration ),
    event_duration_( cooldown_t::cooldown_duration( cd, base_duration ) )
  {
    add_event( event_duration_ );
  }

  recharge_event_t( cooldown_t* cd, timespan_t event_duration, timespan_t base_duration ) :
    event_t( *cd -> player ), cooldown_( cd ),
    duration_( base_duration ), event_duration_( event_duration )
  {
    add_event( event_duration_ );
  }

  virtual const char* name() const override
  { return "recharge_event"; }

  virtual void execute() override
  {
    assert( cooldown_ -> current_charge < cooldown_ -> charges );
    cooldown_ -> current_charge++;
    cooldown_ -> ready = cooldown_t::ready_init();

    if ( cooldown_ -> current_charge < cooldown_ -> charges )
    {
      cooldown_ -> recharge_event = new ( sim() ) recharge_event_t( cooldown_, duration_ );
    }
    else
    {
      cooldown_ -> recharge_event = nullptr;
      cooldown_ -> last_charged = sim().current_time();
    }

    if ( sim().debug )
    {
      sim().out_debug.printf( "%s recharge cooldown %s regenerated charge, current=%d, total=%d, next=%.3f, ready=%.3f, base_duration=%.3f, mul=%f",
        cooldown_ -> player -> name(), cooldown_ -> name_str.c_str(), cooldown_ -> current_charge, cooldown_ -> charges,
        cooldown_ -> recharge_event ? cooldown_ -> recharge_event -> occurs().total_seconds() : 0,
        cooldown_ -> ready.total_seconds(),
        cooldown_t::cooldown_duration( cooldown_, duration_ ).total_seconds(),
        cooldown_ -> recharge_multiplier );
    }

    cooldown_ -> player -> trigger_ready();
  }

  static recharge_event_t* cast( event_t* e )
  { return debug_cast<recharge_event_t*>( e ); }
};

struct ready_trigger_event_t : public player_event_t
{
  cooldown_t* cooldown;

  ready_trigger_event_t( player_t& p, cooldown_t* cd ) :
    player_event_t( p ),
    cooldown( cd )
  {
    add_event( cd -> ready - sim().current_time() );
  }
  virtual const char* name() const override
  { return "ready_trigger_event"; }
  void execute() override
  {
    cooldown -> ready_trigger_event = nullptr;
    p() -> trigger_ready();
  }
};

} // UNNAMED NAMESPACE

timespan_t cooldown_t::cooldown_duration( const cooldown_t* cd,
                                          const timespan_t& override_duration )
{
  timespan_t ret;

  if ( override_duration >= timespan_t::zero() )
  {
    ret = override_duration;
  }
  else
  {
    ret = cd -> duration;
  }

  ret *= cd -> recharge_multiplier;

  return ret;
}

cooldown_t::cooldown_t( const std::string& n, player_t& p ) :
  sim( *p.sim ),
  player( &p ),
  name_str( n ),
  duration( timespan_t::zero() ),
  ready( ready_init() ),
  reset_react( timespan_t::zero() ),
  charges( 1 ),
  current_charge( 1 ),
  recharge_event( nullptr ),
  ready_trigger_event( nullptr ),
  last_start( timespan_t::zero() ),
  last_charged( timespan_t::zero() ),
  recharge_multiplier( 1.0 ),
  hasted( false ),
  action( nullptr )
{}

cooldown_t::cooldown_t( const std::string& n, sim_t& s ) :
  sim( s ),
  player( nullptr ),
  name_str( n ),
  duration( timespan_t::zero() ),
  ready( ready_init() ),
  reset_react( timespan_t::zero() ),
  charges( 1 ),
  current_charge( 1 ),
  recharge_event( nullptr ),
  ready_trigger_event( nullptr ),
  last_start( timespan_t::zero() ),
  last_charged( timespan_t::zero() ),
  recharge_multiplier( 1.0 ),
  hasted( false ),
  action( nullptr )
{}

// Adjust a dynamic cooldown (reduction) multiplier based on the current action associated with the
// cooldown. Actions are associated by start() calls.
void cooldown_t::adjust_recharge_multiplier()
{
  if ( up() )
  {
    return;
  }

  double old_multiplier = recharge_multiplier;
  assert( action && "Only cooldowns with associated action can have their recharge multiplier adjusted.");
  recharge_multiplier = action -> recharge_multiplier();
  if ( old_multiplier == recharge_multiplier )
  {
    return;
  }

  double delta = recharge_multiplier / old_multiplier;
  timespan_t new_remains, remains;
  if ( charges == 1 )
  {
    remains = ready - sim.current_time();
    new_remains = remains * delta;
  }
  else
  {
    remains = recharge_event -> remains();
    new_remains = remains * delta;
    // Shortened, reschedule the event
    if ( delta < 1 )
    {
      timespan_t duration_ = recharge_event_t::cast( recharge_event ) -> duration_;
      event_t::cancel( recharge_event );
      recharge_event = new ( sim ) recharge_event_t( this, new_remains, duration_ );
    }
    else
    {
      recharge_event -> reschedule( new_remains );
    }
  }

  if ( sim.debug )
  {
    sim.out_debug.printf("%s dynamic cooldown %s adjusted: new_ready=%.3f old_ready=%.3f remains=%.3f old_mul=%f new_mul=%f",
        player -> name(), name(), (sim.current_time() + new_remains).total_seconds(), ready.total_seconds(),
        remains.total_seconds(), old_multiplier, recharge_multiplier );
  }

  ready = sim.current_time() + new_remains;
  if ( charges == 1 )
  {
    last_charged = ready;
  }
}

void cooldown_t::adjust( timespan_t amount, bool require_reaction )
{
  // Normal cooldown, just adjust as we see fit
  if ( charges == 1 )
  {
    // No cooldown ongoing, don't do anything
    if ( up() )
      return;

    // Cooldown resets
    if ( ready + amount <= sim.current_time() )
      reset( require_reaction );
    // Still some time left, adjust ready
    else
      ready += amount;
  }
  // Charge-based cooldown
  else if ( current_charge < charges )
  {
    // Remaining time on the recharge event
    timespan_t remains = recharge_event -> remains() + amount;

    // Didnt recharge a charge, just recreate the recharge event to occur
    // sooner
    if ( remains > timespan_t::zero() )
    {
      timespan_t duration_ = recharge_event_t::cast( recharge_event ) -> duration_;
      event_t::cancel( recharge_event );
      recharge_event = new ( sim ) recharge_event_t( this, remains, duration_ );

      // If we have no charges, adjust ready time to the new occurrence time
      // of the recharge event, plus a millisecond
      if ( current_charge == 0 )
        ready += amount;

      if ( sim.debug )
        sim.out_debug.printf( "%s recharge cooldown %s adjustment=%.3f, remains=%.3f, occurs=%.3f, ready=%.3f",
          player -> name(), name_str.c_str(), amount.total_seconds(), remains.total_seconds(),
          recharge_event -> occurs().total_seconds(), ready.total_seconds() );
    }
    // Recharged a charge
    else
    {
      reset( require_reaction );
      // Excess time adjustment goes to the next recharge event, if we didnt
      // max out on charges (recharge_event is still present after reset()
      // call)
      if ( remains < timespan_t::zero() && recharge_event )
      {
        timespan_t duration_ = recharge_event_t::cast( recharge_event ) -> duration_;
        event_t::cancel( recharge_event );

        // Note, the next recharge cycle uses the previous recharge cycle's
        // base duration, if overridden
        timespan_t new_duration = cooldown_duration( this, duration_ );
        new_duration += remains;

        recharge_event = new ( sim ) recharge_event_t( this, new_duration, duration_ );
      }

      if ( sim.debug )
      {
        sim.out_debug.printf( "%s recharge cooldown %s regenerated charge, current=%d, total=%d, reminder=%.3f, next=%.3f, ready=%.3f",
          player -> name(), name_str.c_str(), current_charge, charges,
          remains.total_seconds(),
          recharge_event ? recharge_event -> occurs().total_seconds() : 0,
          ready.total_seconds() );
      }
    }
  }
}

void cooldown_t::reset_init()
{
  ready = ready_init();
  last_start = timespan_t::zero();
  last_charged = timespan_t::zero();
  reset_react = timespan_t::zero();

  current_charge = charges;

  recharge_event = nullptr;
  ready_trigger_event = nullptr;
}

void cooldown_t::reset( bool require_reaction, bool all_charges )
{
  bool was_down = down();
  ready = ready_init();
  if ( last_start > sim.current_time() )
    last_start = timespan_t::zero();
  if ( charges == 1 || all_charges )
    current_charge = charges;
  else if ( current_charge < charges )
    current_charge ++;
  if ( require_reaction && player )
  {
    if ( was_down )
      reset_react = sim.current_time() + player -> total_reaction_time();
  }
  else
  {
    reset_react = timespan_t::zero();
  }
  if ( current_charge == charges )
  {
    event_t::cancel( recharge_event );
    last_charged = sim.current_time();
  }
  event_t::cancel( ready_trigger_event );
  if ( player )
  {
    player -> trigger_ready();
  }
}

void cooldown_t::start( action_t* a, timespan_t _override, timespan_t delay )
{
  // Zero duration cooldowns are nonsense
  if ( _override < timespan_t::zero() && duration <= timespan_t::zero() )
  {
    return;
  }

  reset_react = timespan_t::zero();

  action = a;

  if ( a )
  {
    recharge_multiplier = a -> recharge_multiplier();
  }

  timespan_t event_duration = cooldown_duration( this, _override );
  if ( event_duration == timespan_t::zero() )
  {
    return;
  }

  if ( delay > timespan_t::zero() )
  {
    event_duration += delay;
  }

  // Normal cooldowns have charges = 0 or 1, and do not use the event system to
  // trigger ready status (for efficiency reasons). Charged cooldowns recharge
  // through the event system.
  if ( charges > 1 )
  {
    last_charged = timespan_t::zero();

    assert( current_charge > 0 );
    current_charge--;

    // Begin a recharge event
    if ( ! recharge_event )
    {
      recharge_event = new ( sim ) recharge_event_t( this, event_duration, _override );
    }
    // No charges left, the cooldown won't be ready until a recharge event
    // occurs. Note, ready still needs to be properly set as it ultimately
    // controls whether a cooldown is "up".
    else if ( current_charge == 0 )
    {
      assert( recharge_event );
      ready = recharge_event -> occurs() + timespan_t::from_millis( 1 );
    }
  }
  else
  {
    ready = sim.current_time() + event_duration;
    last_start = sim.current_time();
    last_charged = ready;
  }

  assert( player );
  if ( player -> ready_type == READY_TRIGGER )
    ready_trigger_event = new ( sim ) ready_trigger_event_t( *player, this );
}

void cooldown_t::start( timespan_t _override, timespan_t delay )
{
  start( nullptr, _override, delay );
}

expr_t* cooldown_t::create_expression( action_t*, const std::string& name_str )
{
  if ( name_str == "remains" )
    return make_mem_fn_expr( name_str, *this, &cooldown_t::remains );
  else if ( name_str == "duration" )
    return make_mem_fn_expr( name_str, *this, &cooldown_t::duration );
  else if ( name_str == "up" || name_str == "ready" )
    return make_mem_fn_expr( name_str, *this, &cooldown_t::up );
  else if ( name_str == "charges" )
    return make_ref_expr( name_str, current_charge );
  else if ( name_str == "charges_fractional" )
  {
    struct charges_fractional_expr_t : public expr_t
    {
      const cooldown_t* cd;
      charges_fractional_expr_t( const cooldown_t* c ) :
        expr_t( "charges_fractional" ), cd( c )
      { }

      virtual double evaluate() override
      {
        double charges = cd -> current_charge;
        if ( cd -> recharge_event )
        {
          recharge_event_t* re = debug_cast<recharge_event_t*>( cd -> recharge_event );
          charges += 1 - ( re -> remains() / re -> event_duration_ );
        }
        return charges;
      }
    };
    return new charges_fractional_expr_t( this );
  }
  else if ( name_str == "recharge_time" )
  {
    struct recharge_time_expr_t : public expr_t
    {
      const cooldown_t* cd;
      recharge_time_expr_t( const cooldown_t* c ) :
        expr_t( "recharge_time" ), cd( c )
      { }

      virtual double evaluate() override
      {
        if ( cd -> recharge_event )
          return cd -> recharge_event -> remains().total_seconds();
        else
          return cd -> duration.total_seconds();
      }
    };
    return new recharge_time_expr_t( this );
  }
  else if ( name_str == "max_charges" )
    return make_ref_expr( name_str, charges );

  return nullptr;
}
