#include "StdAfx.h"
#include "include/TriMath.h"
#include "SeekTarget.h"

SeekTarget::SeekTarget( IRoot* lockobj ) :
	m_behaviorWeight( 20.f ),
	m_arrivedRadius( 0.f ),
	m_slowDownRadius( 0.f ),
	m_seconds( 0.25f ),
	m_exit( false ),
	m_target( nullptr ),
	m_tunnelBehavior( nullptr ),
	m_fxBehavior( nullptr )
{
	m_takenLocatorIndices.clear();
}

SeekTarget::~SeekTarget()
{
}

int SeekTarget::GetProcessPriority()
{
	return PROCESS_FIRST;
}

size_t SeekTarget::GetScratchMemorySize() const
{
	return sizeof( LocatorData );
}

void SeekTarget::InitializeScratch( const DroneAgent& drone, void* scratchMemory )
{
	*static_cast<LocatorData*>( scratchMemory ) = LocatorData();
}

std::vector<Vector3> SeekTarget::CalculateBehavior( std::vector<DroneAgent>& agents, void* scratchData, const float deltaTime,
	BehaviorGroup& group, EveChildBehaviorSystem& system, const std::vector<std::vector<DroneAgent*>>& dronesInSearchRadius )
{
	// Make sure python gets called first
	if( m_target == nullptr || m_behaviorWeight <= 0 )
	{
		return m_todo;
	}

	// **** Can this be better? **** 
	if( m_tunnelBehavior == nullptr && m_fxBehavior == nullptr )
	{
		m_tunnelBehavior = group.GetBehaviorByName( "ProcessLifetime" );
		m_fxBehavior = group.GetBehaviorByName( "PlayFX" );
	}

	auto data = static_cast<LocatorData*>( scratchData );
	for( auto agent = agents.begin(); agent != agents.end(); ++agent, ++data )
	{
		if( m_arrivedRadius == 0.f )
		{
			SetArrivedRadius();
		}

		if( m_slowDownRadius == 0.f )
		{
			SetSlowDownRadius();
		}

		// If drone does not have a picked locator, then pick one
		if( agent->target == Vector3( 0, 0, 0 ) )
		{
			unsigned int count = m_target->GetDamageLocatorCount();
			int rand = TriRandInt( count );
			// If we've already picked that locator then pick a new one
			while( std::find( m_takenLocatorIndices.begin(), m_takenLocatorIndices.end(), rand ) != m_takenLocatorIndices.end() )
			{
				rand = TriRandInt( count );
			}

			// Add that index to the vector
			m_takenLocatorIndices.push_back( rand );

			data->index = rand;
			data->position = GetRandomPosition( rand );
			data->direction = GetLocatorDirection( rand );
			agent->target = data->position;

			// For debugging
			m_debugLocator = data->position;
		}

		// Set the target point on the radius sphere
		Vector3 fakePoint = data->direction;
		fakePoint = Normalize( fakePoint );
		fakePoint *= m_arrivedRadius;
		fakePoint += data->position;

		m_arrivalPoint = fakePoint;

		Vector3 desiredVelocity = fakePoint - agent->position;
		float distance = Length( desiredVelocity );
		desiredVelocity = Normalize( desiredVelocity );
		static const Vector3 zAxis( 0.f, 0.f, 1.f );

		// If the agent is approaching, slow him down
		if( distance < m_slowDownRadius )
		{
			desiredVelocity = desiredVelocity * m_behaviorWeight * ( distance / m_slowDownRadius );
			
			// Set the rotation of the drone
			Quaternion newRotation;
			auto invDir = Normalize(data->position - agent->position);
			TriQuaternionRotationArc( &newRotation, &zAxis, &invDir );
			agent->rotation = newRotation;
			data->timePassed = 0.f;

			// If the target has arrived then start playing effect
			if( distance < m_arrivedRadius )
			{
				if( !agent->playFX )
				{
					agent->fxStartTime = BeOS->GetActualTime();
					agent->playFX = true;

					// Remove data position from the locatorList as it's no longer taken
					m_takenLocatorIndices.erase( std::remove( m_takenLocatorIndices.begin(), m_takenLocatorIndices.end(), data->index ), m_takenLocatorIndices.end() );
				}
			}
			// Trigger if: Repairing Ship && Player Undocks
			else if( m_exit )
			{
				if( m_tunnelBehavior )
				{
					// Drones search for an exit tunnel
					m_tunnelBehavior->UpdateState( true );
				}

				if( m_fxBehavior )
				{
					// Behavior deletes effects
					m_fxBehavior->UpdateState( true );
				}

				agent->playFX = false;
				m_behaviorWeight = 0;
			}
		}
		else
		{
			// Have the drone slowly start moving based on time passed
			data->timePassed += deltaTime;
			data->timePassed = max( data->timePassed, m_seconds );
			desiredVelocity *= Lerp( 0, 1, max(data->timePassed, m_seconds ) / m_seconds );
		}
		agent->acceleration += desiredVelocity - agent->velocity;
	}

	return m_todo;
}

void SeekTarget::SetTarget( ITriTargetable* target )
{
	m_target = target;
}

void SeekTarget::SetExit( bool value )
{
	m_exit = value;
}

void SeekTarget::SetArrivedRadius()
{
	m_arrivedRadius = m_target->GetRadius() / 3;
}

void SeekTarget::SetSlowDownRadius()
{
	m_slowDownRadius = m_target->GetRadius() / 2;
}

Vector3 SeekTarget::GetRandomPosition( int rand )
{
	Vector3 targetPositionWS;
	// Get a damage locator in world position
	m_target->GetDamageLocatorPosition( &targetPositionWS, rand, true );

	return targetPositionWS;
}

Vector3 SeekTarget::GetLocatorDirection( int rand )
{
	Vector3 directionWS;
	m_target->GetDamageLocatorDirection( &directionWS, rand, true );

	return directionWS;
}

void SeekTarget::RenderDebugInfo( ITr2DebugRenderer2& renderer, std::vector<DroneAgent>& agents, Matrix& parentWorldLocation )
{
	if( renderer.HasOption( this, "droneDebug" ) )
	{
		renderer.DrawSphere( this, m_arrivalPoint, m_arrivedRadius, 8, Tr2DebugRenderer::Wireframe, 0xffffffff );
		renderer.DrawSphere( this, m_arrivalPoint, 5, 8, Tr2DebugRenderer::Wireframe, 0xff0000ff );
		renderer.DrawSphere( this, m_arrivalPoint, m_slowDownRadius, 8, Tr2DebugRenderer::Wireframe, 0xffcc11ff );
	}
}