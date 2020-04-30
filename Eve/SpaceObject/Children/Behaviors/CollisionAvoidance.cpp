#include "StdAfx.h"
#include "CollisionAvoidance.h"

CollisionAvoidance::CollisionAvoidance( IRoot* lockobj ) :
	PARENTLOCK( m_exclusionVolumes ),
	m_collisionAvoidanceScalar( 10.f ) 
{
}

CollisionAvoidance::~CollisionAvoidance()
{
}

int CollisionAvoidance::GetProcessPriority()
{
	return PROCESS_LAST;
}

size_t CollisionAvoidance::GetScratchMemorySize() const
{
	return sizeof( CollisionAvoidanceData );
}

void CollisionAvoidance::InitializeScratch( void* scratchMemory )
{
	*static_cast<CollisionAvoidanceData*>( scratchMemory ) = CollisionAvoidanceData();
}

std::vector<Vector3> CollisionAvoidance::CalculateBehavior( std::vector<DroneAgent>& agents, void* scratchData, const float deltaTime,
	BehaviorGroup& group, EveChildBehaviorSystem& system, const std::vector < std::vector<DroneAgent*>>& dronesInSearchRadius)
{
	std::vector<Vector3> forceVectors;
	auto data = static_cast<CollisionAvoidanceData*>( scratchData );
	for( auto agent = agents.begin(); agent != agents.end(); ++agent, ++data )
	{
		Vector3 avoidanceForce( 0, 0, 0 );
		for( auto exclusionVolume = m_exclusionVolumes.begin(); exclusionVolume != m_exclusionVolumes.end(); ++exclusionVolume )
		{
			float intensity = ( *exclusionVolume )->GetIntensity( agent->position );
			Vector3 pos = ( *exclusionVolume )->GetBoundingSphere().GetXYZ();

			// This is when drones realize they're in the outer radius
			if( intensity > 0.0f )
			{
				avoidanceForce = agent->position - pos;
				Vector3 force = Normalize( avoidanceForce );
				Vector3 velocity = agent->velocity;
				velocity = Normalize( velocity );
				float dotProduct = Dot( force, velocity );

				if( dotProduct < 0.f )
				{
					data->collisionStrength = ( intensity * m_collisionAvoidanceScalar * -dotProduct);
				}
			}

			Vector3 forceOffset = Normalize( avoidanceForce ) * group.GetBoundingSphereRadius();
			forceVectors.push_back( agent->position + forceOffset );

			avoidanceForce = Normalize( avoidanceForce ) * ( data->collisionStrength );
			agent->acceleration += avoidanceForce;

			forceVectors.push_back( avoidanceForce );
		}
	}
	return forceVectors;
}

void CollisionAvoidance::GetDebugOptions( Tr2DebugRendererOptions& options )
{
	options.insert( "ExclusionVolumes" );
}

void CollisionAvoidance::RenderDebugInfo( ITr2DebugRenderer2& renderer, std::vector<DroneAgent>& agents, Matrix& parentWorldLocation )
{
	if( renderer.HasOption( this, "ExclusionVolumes" ) )
	{
		for( auto volume = m_exclusionVolumes.begin(); volume != m_exclusionVolumes.end(); ++volume )
		{
			( *volume )->RenderDebugInfo( renderer, parentWorldLocation );
		}
	}
}