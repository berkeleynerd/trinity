#pragma once
#ifndef Wander_H
#define Wander_H
#include "Eve/SpaceObject/Children/EveChildBehaviorSystem.h"
#include "IBehavior.h"

BLUE_CLASS( Wander ) :
	public IBehavior
{
public:
	EXPOSE_TO_BLUE();
	Wander( IRoot* lockobj = nullptr );
	~Wander();

	virtual std::vector<Vector3> CalculateBehavior(std::vector<DroneAgent>& agents, const float deltaTime,
	                                               BehaviorGroup& sys, EveChildBehaviorSystem& system);

	void RenderDebugInfo(Tr2DebugRenderer& renderer, std::vector<DroneAgent>& agents, Matrix& parentWorldLocation);

private:
	float m_freq;
	float m_weightWander;	//priority of behavior

	//debugging purposes
	float rand1, rand2, rand3;
};

TYPEDEF_BLUECLASS( Wander );

#endif