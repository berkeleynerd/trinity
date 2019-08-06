#include "StdAfx.h"
#include "Wander.h"
#include "Include/TriMath.h"
#include "include/TriQuaternion.h"

Wander::Wander(IRoot* lockobj) :
	m_weightWander(66.f),
	m_freq(0.01f),
	rand1( 13.6842f ),
	rand2( 27.5111456 ),
	rand3( 256.654f )
{
}

Wander::~Wander()
{
}

std::vector<Vector3> Wander::CalculateBehavior(std::vector<DroneAgent>& agents, const float deltaTime,
                                               BehaviorGroup& group, EveChildBehaviorSystem& system)
{	
	std::vector<Vector3> forceVectors;

	for (auto agent = agents.begin(); agent != agents.end(); ++agent)
	{
		float seed = agent->lifetime + agent->id;

		Vector3 p = Vector3( seed * rand1, seed * rand2, seed * rand3 ) * m_freq;

		Vector3 force( float( PerlinNoise1D( p.x, 2, 1, 1 ) ), float( PerlinNoise1D( p.y, 2, 1, 1 ) ), float( PerlinNoise1D( p.z, 2, 1, 1 ) ) );
		Vector3 forceOffset = Normalize(force) * group.GetBoundingSphereRadius();
		forceVectors.push_back( agent->position + forceOffset );
		force *= m_weightWander;
		forceVectors.push_back( force );
		agent->acceleration += force;
	}
	
	return forceVectors;
}

void Wander::RenderDebugInfo(Tr2DebugRenderer& renderer, std::vector<DroneAgent>& agents, Matrix& parentWorldLocation)
{
}
