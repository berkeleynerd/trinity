#pragma once
#ifndef BackAndForth_H
#define BackAndForth_H
#include "Eve/SpaceObject/Children/EveChildBehaviorSystem.h"
#include "IBehavior.h"

BLUE_CLASS( BackAndForth ) :
	public IBehavior
{
public:
	EXPOSE_TO_BLUE();
	BackAndForth( IRoot* lockobj = nullptr );
	~BackAndForth();

	virtual std::vector<Vector3> CalculateBehavior(std::vector<DroneAgent>& agents, const float deltaTime,
	                                               BehaviorGroup& sys, EveChildBehaviorSystem& system);
	void RenderDebugInfo(Tr2DebugRenderer& renderer, std::vector<DroneAgent>& agents, Matrix& parentWorldLocation);

private:
	float m_arrivedRadius;
	float m_slowDownRadius;
	int m_rand;
	float m_backAndForthWeight;
};

TYPEDEF_BLUECLASS( BackAndForth );

#endif