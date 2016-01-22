////////////////////////////////////////////////////////////
//
//    Created:   December 2013
//    Copyright: CCP 2013
//
#pragma once
#ifndef EveAnimationData_H
#define EveAnimationData_H

enum EveAnimationStateProgress {
	EVE_ANIM_INACTIVE = 1,
	EVE_ANIM_RUNNING = 2,
	EVE_ANIM_FINALIZING = 4,
	EVE_ANIM_DONE = 8
};

enum EveAnimationCmd
{
	ANIM_CMD_NONE = 0,
	ANIM_CMD_PLAYACTIVATION,				// 1
	ANIM_CMD_DEACTIVATE_TURRETS,			// 2
	ANIM_CMD_ACTIVATE_TURRETS,				// 3
	ANIM_CMD_ACTIVATION_STRENGTH_ZERO,		// 4
	ANIM_CMD_ACTIVATION_STRENGTH_ONE,		// 5
	ANIM_CMD_TURNOFF_BOOSTERS,				// 6
	ANIM_CMD_TURNON_BOOSTERS,				// 7
	ANIM_CMD_PLAY_CLIPSPHERE,				// 8
	ANIM_CMD_STOP_CLIPSPHERE,				// 9
	ANIM_CMD_SET_CLIPSPHERE,				// 10

	ANIM_CMD_COUNT,
};

BLUE_CLASS( EveAnimation ) : public IRoot
{
public:
	EveAnimation( IRoot* lockobj = NULL );
	~EveAnimation() {}
	
	EXPOSE_TO_BLUE();

	std::string m_name;
	int m_loops;
	float m_delay;
	float m_speed;
};
TYPEDEF_BLUECLASS( EveAnimation );
BLUE_DECLARE_VECTOR( EveAnimation );


BLUE_CLASS( EveAnimationCurve ) : public IRoot
{
public:
	EveAnimationCurve( IRoot* lockobj = NULL );
	~EveAnimationCurve() {}
	
	EXPOSE_TO_BLUE();

	std::string m_name;
};
TYPEDEF_BLUECLASS( EveAnimationCurve );
BLUE_DECLARE_VECTOR( EveAnimationCurve );


BLUE_CLASS( EveAnimationCommand ) : public IRoot
{
public:
	EveAnimationCommand( IRoot* lockobj = NULL );
	~EveAnimationCommand() {}
	
	EXPOSE_TO_BLUE();

	EveAnimationCmd m_command;
	std::string m_data;
};
TYPEDEF_BLUECLASS( EveAnimationCommand );
BLUE_DECLARE_VECTOR( EveAnimationCommand );


#endif