#include "StdAfx.h"
#include "Inertia.h"
#include "include/TriMath.h"

Inertia::Inertia( IRoot* lockobj ) :
	m_inertiaWeight( 0.5 ),
	m_maxRotationSpeed( 3.14 )
{
}

Inertia::~Inertia()
{
}

std::vector<Vector3> Inertia::CalculateBehavior(std::vector<DroneAgent>& agents, const float deltaTime,
                                                BehaviorGroup& group, EveChildBehaviorSystem& system)
{
	for (auto agent = agents.begin(); agent != agents.end(); ++agent)
	{
		auto lastAccelNormalized = Normalize( agent->lastAcceleration );
		auto lastAccelLength = Length( agent->lastAcceleration );
		auto accelNormalized = Normalize( agent->acceleration );
		auto accelLength = Length( agent->acceleration );

		if ( Length( lastAccelNormalized ) != 0 && m_maxRotationSpeed > 0 )
		{
			auto c = Normalize( Cross( lastAccelNormalized, accelNormalized ) );
			if ( Length( c ) == 0 )
			{
				c = Vector3( 0, 1, 0 );
			}
			auto angle = AngleFromNormalized( lastAccelNormalized, accelNormalized );
			float step = m_maxRotationSpeed * deltaTime;
			angle = min( angle, step );
			if ( angle > 0 )
			{
				auto quat = RotationQuaternion( c, angle );
				TriVectorRotateQuaternion( &agent->acceleration, &lastAccelNormalized, &quat);
			}
			
			agent->acceleration = Normalize(agent->acceleration) * Lerp(lastAccelLength, accelLength,
			                                                            TriClamp(m_inertiaWeight, 0, 1) * deltaTime);
		}
		agent->lastAcceleration = agent->acceleration;
	}
	std::vector<Vector3> noNeedToReturnForces;
	return noNeedToReturnForces;
}

void Inertia::RenderDebugInfo(Tr2DebugRenderer& renderer, std::vector<DroneAgent>& agents, Matrix& parentWorldLocation)
{
}