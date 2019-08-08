#pragma once
#ifndef Inertia_H
#define Inertia_H
#include "Eve/SpaceObject/Children/EveChildBehaviorSystem.h"
#include "IBehavior.h"

BLUE_CLASS( Inertia ) :
	public IBehavior
{
public:
	EXPOSE_TO_BLUE();
	Inertia( IRoot* lockobj = nullptr );
	~Inertia();

	virtual std::vector<Vector3> CalculateBehavior(std::vector<DroneAgent>& agents, const float deltaTime,
	                                               BehaviorGroup& sys, EveChildBehaviorSystem& system);
	void RenderDebugInfo(Tr2DebugRenderer& renderer, std::vector<DroneAgent>& agents, Matrix& parentWorldLocation);

private:
	float m_maxAcceleration;
	float m_inertiaWeight;
	float m_maxRotationSpeed;
};
TYPEDEF_BLUECLASS( Inertia );

#endif