#pragma once
#ifndef CollisionAvoidance_H
#define CollisionAvoidance_H
#include "Eve/SpaceObject/Children/EveChildBehaviorSystem.h"
#include "IBehavior.h"
#include "Eve/Volume/IEveVolume.h"

BLUE_DECLARE_INTERFACE( IEveVolume );
BLUE_DECLARE_IVECTOR( IEveVolume );

struct CollisionAvoidanceData
{
	CollisionAvoidanceData() :
		collisionStrength( 0.0f )
	{
	}
	float collisionStrength;
};


BLUE_CLASS( CollisionAvoidance ) :
	public IBehavior
{
public:
	EXPOSE_TO_BLUE();
	CollisionAvoidance( IRoot* lockobj = nullptr );
	~CollisionAvoidance();

	virtual std::vector<Vector3> CalculateBehavior(std::vector<DroneAgent>& agents, void* scratchData, const float deltaTime,
	                                               BehaviorGroup& group, EveChildBehaviorSystem& system, const std::vector<std::vector<DroneAgent*>>& dronesInSearchRadius);
	void GetDebugOptions( Tr2DebugRendererOptions& options );
	void RenderDebugInfo( ITr2DebugRenderer2& renderer, std::vector<DroneAgent>& agents, Matrix& parentWorldLocation );
	virtual size_t GetScratchMemorySize() const;
	virtual void InitializeScratch( void* scratchMemory );
	int GetProcessPriority();

private:
	PIEveVolumeVector m_exclusionVolumes;

	float m_collisionAvoidanceScalar;
};

TYPEDEF_BLUECLASS( CollisionAvoidance );

#endif