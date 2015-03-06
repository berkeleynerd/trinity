#include "StdAfx.h"
#include "EveAnimationState.h"
#include "EveAnimationSequencer.h"
#include "Eve/SpaceObject/EveSpaceObject2.h"
#include "Eve/SpaceObject/EveMobile.h"
#include "Eve/SpaceObject/EveShip2.h"
#include "Curves/TriCurveSet.h"
#include "Tr2GrannyAnimation.h"
#include "Tr2Renderer.h"

static BlueStructureDefinition EveAnimationStateTransitionStructureDef[] =
{ 
	{ "name", Be::SHAREDSTRING_1, 0 },
	{ "transitionName", Be::SHAREDSTRING_1, 8 }, 
	{0} 
};

	
// --------------------------------------------------------------------------------
// Description:
//   Initialize data members
// --------------------------------------------------------------------------------
EveAnimationState::EveAnimationState( IRoot* lockobj ) :
	PARENTLOCK( m_curves ),
	PARENTLOCK( m_commands ),
	PARENTLOCK( m_initCommands ),
	PARENTLOCK( m_initCurves ),
	PARENTLOCK( m_transitions ),
	m_progress( EVE_ANIM_INACTIVE ),
	m_animationDuration( 0.f ),
	m_startTime( 0.f ),
	m_secondsRemaining( 0.f ),
	m_doInitialization( false )
{
	m_transitions.SetStructureDefinition( EveAnimationStateTransitionStructureDef );
}

EveAnimationState::~EveAnimationState() 
{
}

// --------------------------------------------------------------------------------
// Description:
//   Update the time when animations/curves are completed for this state.
// --------------------------------------------------------------------------------
void EveAnimationState::UpdateDuration( EveAnimationStateMachine* sm, EveSpaceObject2* so )
{
	float currentAnimationTime = Tr2Renderer::GetAnimationTime();
	m_startTime = currentAnimationTime;
	m_animationDuration = 0;
	auto ac = so->GetAnimationController();
	if( ac && ac->IsInitialized() )
	{
		if( sm->HasTrackMask() )
		{
			m_animationDuration = ac->GetAnimationChainCompleteTimeForLayer( sm->GetTrackMask() ) - currentAnimationTime;
		}
		else
		{
			m_animationDuration = ac->GetAnimationChainCompleteTime() - currentAnimationTime;
		}
	}

	float maxCurveDuration = 0.f;
	for( auto it = m_curves.cbegin(); it != m_curves.cend(); it++ )
	{
		maxCurveDuration = max( maxCurveDuration, so->GetCurveSetDuration( (*it)->m_name ) );
	}

	m_animationDuration = max( m_animationDuration, maxCurveDuration );
}

// --------------------------------------------------------------------------------
// Description:
//   Deactivate this state. May take a while to wind down.
// Side Effect:
//   Empties out m_parameters
// --------------------------------------------------------------------------------
void EveAnimationState::Stop( EveAnimationStateMachine* sm, EveSpaceObject2* owner )
{
	m_parameters.clear();

	m_progress = EVE_ANIM_FINALIZING;
	EndAnimation( sm, owner );
}

// --------------------------------------------------------------------------------
// Description:
//   Start this state.
// Arguments:
//   owner - the owning animation sequencer
//   mode - is it a transition, does it need to run init curves etc.
// --------------------------------------------------------------------------------
void EveAnimationState::Start( EveAnimationStateMachine* sm, EveSpaceObject2* so, EveAnimationStateStartCommand mode )
{
	m_doInitialization = mode == EVE_ANIM_START_INIT;

	m_startTime = Tr2Renderer::GetAnimationTime();
	m_animationDuration = 0.f;

	if( mode == EVE_ANIM_START_TRANSITION )
	{
		m_progress = EVE_ANIM_FINALIZING;
	}
	else
	{
		m_progress = EVE_ANIM_RUNNING;
	}

	PlayAnimation( sm, so );
	UpdateDuration( sm, so );
}

// --------------------------------------------------------------------------------
// Description: 
//	 Sets a parameter value to a parameter name
// Arguments: 
//   parameterName - the name of the parameter
//   parameterValue - the value of the parameter
// Side Effect:
//   sets m_parameters[parameterName] to parameterValue
// --------------------------------------------------------------------------------
void EveAnimationState::SetParameter( const std::string& parameterName, float parameterValue )
{
	m_parameters[parameterName] = parameterValue;
}

// --------------------------------------------------------------------------------
// Description:
//   Check if this state has a transition sequence for states identified by stateName.
// Arguments:
//   stateName - The name of the state we'd transition into
// Returns:
//   name of transition state or nullptr
// --------------------------------------------------------------------------------
const char* EveAnimationState::GetTransition( const std::string& stateName ) const
{
	for( auto it = m_transitions.begin(); it != m_transitions.end(); it++ )
	{
		if( stateName == it->name.c_str() )
		{
			return it->transitionName.c_str();
		}
	}
	return nullptr;
}

// --------------------------------------------------------------------------------
// Description:
//   Play curves in the animation sequence
// --------------------------------------------------------------------------------
void EveAnimationState::PlayCurves( EveSpaceObject2* owner )
{
	if( m_doInitialization )
	{	
		for( auto it = m_initCurves.cbegin(); it != m_initCurves.cend(); it++ )
		{
			owner->PlayCurveSet( (*it)->m_name );
		}
	}
	for( auto it = m_curves.cbegin(); it != m_curves.cend(); it++ )
	{
		owner->PlayCurveSet( (*it)->m_name );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Play commands in the animation sequence
// --------------------------------------------------------------------------------
void EveAnimationState::ExecuteCommands( EveSpaceObject2* owner )
{
	if( m_doInitialization )
	{
		for( auto it = m_initCommands.cbegin(); it != m_initCommands.cend(); it++ )
		{
			owner->ExecuteAnimationStateCommand( (*it)->m_command, (*it)->m_data, m_parameters );
		}
	}
	
	for( auto it = m_commands.cbegin(); it != m_commands.cend(); it++ )
	{
		owner->ExecuteAnimationStateCommand( (*it)->m_command, (*it)->m_data, m_parameters );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Play animations, commands and curves in the animation sequence
// --------------------------------------------------------------------------------
void EveAnimationState::PlayAnimation( EveAnimationStateMachine* sm, EveSpaceObject2* so )
{
	if( m_animation )
	{
		auto ac = so->GetAnimationController();
		bool success = false;
		if( sm->HasTrackMask() )
		{
			success = ac->PlayLayerAnimationByName( sm->GetTrackMask(), m_animation->m_name.c_str(), false, m_animation->m_loops, m_animation->m_delay, m_animation->m_speed, false );
		}
		else
		{
			success = ac->PlayAnimation( m_animation->m_name.c_str(), false, m_animation->m_loops, m_animation->m_delay, m_animation->m_speed, false );
		}
		if( m_progress == EVE_ANIM_RUNNING && !success )
		{
			if( sm->HasTrackMask() )
			{
				ac->PlayLayerAnimationByName( sm->GetTrackMask(), sm->GetDefaultAnimation(), false, m_animation->m_loops, m_animation->m_delay, m_animation->m_speed, false );
			}
			else
			{
				ac->PlayAnimation( sm->GetDefaultAnimation(), false, m_animation->m_loops, m_animation->m_delay, m_animation->m_speed, false );
			}
		}
	}

	PlayCurves( so );
	ExecuteCommands( so );
}


// --------------------------------------------------------------------------------
// Description:
//   End current animation.
// --------------------------------------------------------------------------------
void EveAnimationState::EndAnimation( EveAnimationStateMachine* sm, EveSpaceObject2* owner )
{
	auto ac = owner->GetAnimationController();
	ac->EndAnimation();

	float currentAnimationTime = Tr2Renderer::GetAnimationTime();
	float animationDuration = 0;
	if( ac && ac->IsInitialized() )
	{
		if( sm->HasTrackMask() )
		{
			m_animationDuration = ac->GetAnimationChainCompleteTimeForLayer( sm->GetTrackMask() ) - currentAnimationTime;
		}
		else
		{
			m_animationDuration = ac->GetAnimationChainCompleteTime() - currentAnimationTime;
		}
	}

	m_animationDuration = max( m_animationDuration, animationDuration );
}

// --------------------------------------------------------------------------------
// Description:
//   Update the state
// --------------------------------------------------------------------------------
void EveAnimationState::Update( Be::Time time, EveSpaceObject2* owner )
{
	float animationDelta = Tr2Renderer::GetAnimationTimeElapsed( m_startTime );
	m_secondsRemaining = m_animationDuration - animationDelta;

	if( animationDelta < m_animationDuration )
	{
		return;
	}

	if( m_progress == EVE_ANIM_FINALIZING )
	{
		m_progress = EVE_ANIM_DONE;
		Cleanup( owner, time );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Stop animations/curves associated with this state.
// --------------------------------------------------------------------------------
void EveAnimationState::Cleanup( EveSpaceObject2* owner, Be::Time time )
{
	for( auto it = m_curves.cbegin(); it != m_curves.cend(); it++ )
	{
		owner->UpdateCurveSet( (*it)->m_name, time );
		owner->StopCurveSet( (*it)->m_name );
	}
	if( m_doInitialization )
	{
		for( auto it = m_initCurves.cbegin(); it != m_initCurves.cend(); it++ )
		{
			owner->UpdateCurveSet( (*it)->m_name, time );
			owner->StopCurveSet( (*it)->m_name );
		}
	}

	m_doInitialization = false;
	m_startTime = 0.f;
	m_animationDuration = 0.f;
}


