#pragma once
#ifndef DroneAgent_h
#define DroneAgent_h

struct DroneAgent
{	
	DroneAgent() :					
		rotation( 0, 0, 0, 1 ),		
		position( 0, 0, 0 ),		
		acceleration( 0, 0, 0 ),
		velocity( 0, 0, 0 ),		
		target( 0, 0, 0 ),
		lifetime( 0.f ),
		id( 0 ),
		playFX( false ),
		fxStartTime( -1 ),
		// LOD
		xfade( 0.f ),				
		screenSize( 0.f ),			
		isVisible( false ),
		closestAgentInGroup( nullptr )
	{}						
	DroneAgent* closestAgentInGroup;
	Quaternion rotation;
	Vector3 position;
	Vector3 acceleration;
	Vector3 velocity;
	Vector3 target;
	int id;
	float lifetime;
	bool playFX;
	Be::Time fxStartTime;

	float xfade; // Crossfade between mesh and sprite. 1.0 = mesh, 0.0 = sprite
	bool isVisible; // Don't render agents off-screen
	float screenSize;
};

#endif
