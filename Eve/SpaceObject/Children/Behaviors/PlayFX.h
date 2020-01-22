#pragma once
#ifndef PlayFX_H
#define PlayFX_H

#include "Eve/SpaceObject/Children/EveChildBehaviorSystem.h"
#include "IBehavior.h"
#include "ITr2Renderable.h"

struct PlayFXData
{
	PlayFXData() :
		effectPlayed( false ),
		display( false ),
		seconds( 10 )
	{
	}

	bool effectPlayed;
	bool display;
	int seconds;
};

BLUE_DECLARE_INTERFACE( IEveFiringEffectElement );
BLUE_DECLARE_IVECTOR( IEveFiringEffectElement );

BLUE_CLASS( PlayFX ) :
	public IBehavior
{
public:
	EXPOSE_TO_BLUE();
	PlayFX( IRoot* lockobj = nullptr );
	~PlayFX();

	/////////////////////////////////////////////////////////////////////////////////////
	// PlayFX
	virtual size_t GetScratchMemorySize() const;
	virtual void InitializeScratch( const DroneAgent& drone, void* scratchMemory );
	virtual std::vector<Vector3> CalculateBehavior( std::vector<DroneAgent>& agents, void* scratchData, const float deltaTime,
		BehaviorGroup& group, EveChildBehaviorSystem& system, const std::vector<std::vector<DroneAgent*>>& dronesInSearchRadius );
	void RenderDebugInfo( ITr2DebugRenderer2& renderer, std::vector<DroneAgent>& agents, Matrix& parentWorldLocation );
	std::string GetBehaviorName();
	int GetProcessPriority();

	void GetRenderables( std::vector<ITr2Renderable*>& renderables );
	void Update( EveUpdateContext& updateContext, BehaviorGroup& group );

	void UpdateState( bool state ) { m_stop = state; }

private:
	void ClearEffects();
	void CheckCount( size_t agentSize );

	size_t m_count;
	float m_behaviorWeight;
	float m_intensity;
	float m_delay;
	int m_low;
	int m_high;
	bool m_stop;
	bool m_display;
	bool debugDisplay;

	IEveFiringEffectElementPtr m_firingEffect;
	PIEveFiringEffectElementVector m_firingEffects;

	std::vector<Vector3> m_todo;
};

TYPEDEF_BLUECLASS( PlayFX );

#endif