#include "StdAfx.h"
#include "PlayFX.h"
#include "include/TriMath.h"
#include "Eve/Renderable/Stretch/EveStretch.h"

PlayFX::PlayFX( IRoot* lockobj ) :
	PARENTLOCK( m_firingEffects ),
	m_count( 0 ),
	m_behaviorWeight( 20.f ),
	m_intensity( 10.f ),
	m_delay( 0.f ),
	m_low( 10 ),
	m_high( 20 ),
	m_stop( false ),
	m_display( false )
{
	m_firingEffect = nullptr;
}

PlayFX::~PlayFX()
{
}

int PlayFX::GetProcessPriority()
{
	return PROCESS_LATER;
}

std::string PlayFX::GetBehaviorName()
{
	return "PlayFX";
}

size_t PlayFX::GetScratchMemorySize() const
{
	return sizeof( PlayFXData );
}

void PlayFX::InitializeScratch( const DroneAgent& drone, void* scratchMemory )
{
	*static_cast<PlayFXData*>( scratchMemory ) = PlayFXData();
}

std::vector<Vector3> PlayFX::CalculateBehavior( std::vector<DroneAgent>& agents, void* scratchData, const float deltaTime,
	BehaviorGroup& group, EveChildBehaviorSystem& system, const std::vector<std::vector<DroneAgent*>>& dronesInSearchRadius )
{
	if( m_stop )
	{
		ClearEffects();
		m_behaviorWeight = 0;
		return m_todo;
	}

	if( m_behaviorWeight <= 0 )
	{
		return m_todo;
	}

	size_t diff = m_count > agents.size() ? m_count - agents.size() : agents.size() - m_count;

	if( diff > 0 )
	{
		CheckCount( agents.size() );
	}

	auto data = static_cast<PlayFXData*>( scratchData );
	int i = 0;
	// This behavior will be activated when the drone has arrived near the damage locator
	for( auto agent = agents.begin(); agent != agents.end(); ++agent, ++i, ++data )
	{
		m_firingEffects[i]->SetFiringTransform( agent->position, agent->target );

		// Drone has arrived to target so play effect
		if( agent->playFX && !data->effectPlayed )
		{
			data->seconds = TriRandInt( m_low, m_high );
			data->display = true;
			data->effectPlayed = data->display;

			m_firingEffects[i]->SetDisplay( data->display );
			m_firingEffects[i]->StartFiring( m_delay );
		}

		if( data->effectPlayed )
		{
			Be::Time diff = BeOS->GetActualTime() - agent->fxStartTime;

			auto duration = data->seconds * 10000000;

			if( diff > duration )
			{
				data->display = false;

				m_firingEffects[i]->SetDisplay( data->display );
				m_firingEffects[i]->StartFiring( m_delay );

				data->effectPlayed = data->display;
				agent->playFX = data->display;
				agent->target = Vector3( 0, 0, 0 );
			}
		}
	}

	return m_todo;
}

void PlayFX::RenderDebugInfo( ITr2DebugRenderer2& renderer, std::vector<DroneAgent>& agents, Matrix& parentWorldLocation )
{
}

void PlayFX::Update( EveUpdateContext& updateContext, BehaviorGroup& group )
{
	for( auto fx = m_firingEffects.begin(); fx != m_firingEffects.end(); ++fx )
	{
		( *fx )->Update( updateContext );
		( *fx )->UpdateVisibility( group.m_frustum, group.m_parentTransform );
	}
}

void PlayFX::GetRenderables( std::vector<ITr2Renderable*>& renderables )
{
	for( auto fx = m_firingEffects.begin(); fx != m_firingEffects.end(); ++fx )
	{
		( *fx )->GetRenderables( renderables );
	}
}

void PlayFX::ClearEffects()
{
	m_firingEffects.Clear();
	m_stop = false;
}

void PlayFX::CheckCount( size_t agentSize )
{
	if( m_count > agentSize )
	{
		size_t diff = m_count - agentSize;

		for( size_t i = 0; i < diff; ++i )
		{
			m_firingEffects.Remove( m_firingEffects.size() - 1 );
		}
		m_count = agentSize;
	}
	else if( agentSize > m_count )
	{
		size_t diff = agentSize - m_count;

		for( size_t i = 0; i < diff; ++i )
		{
			IEveFiringEffectElementPtr newFx;

			auto firingEffect = m_firingEffect;
	
			// Copies data from 'source' into '*dest'
			if( !BeClasses->CloneTo( firingEffect, (IRoot**)&newFx.p ) )
			{
				return;
			}

			m_firingEffects.Append( newFx );
		}
		m_count = agentSize;
	}
}