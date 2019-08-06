#include "StdAfx.h"
#include "FollowASpline.h"

BLUE_DEFINE_INTERFACE( IBehavior );

BLUE_DEFINE( FollowASpline );

const Be::ClassInfo* FollowASpline::ExposeToBlue()
{
	EXPOSURE_BEGIN( FollowASpline, "" )
		MAP_INTERFACE( FollowASpline )
		MAP_INTERFACE( IBehavior )

		MAP_ATTRIBUTE( "behaviorWeight", m_behaviorWeight, "here you configure how hard it is for other behaviors to interact with this one :jessica-group: followASpline", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "smoothPullFactor", m_smoothPullFactor, "0: pull-to-start point is the same everywhere, 1: pulling force grows the closer you get to the entrance, [0-1] a blend (this var is heavily affected by behavior weight) \n:jessica-group: followASpline", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "cornerSmoothener", m_cornerSmoothener, "The lower this one is the more it factors the next splinePoint, looks more natural when lower but is more reliable for complex tunnels (and sharp turns) \n:jessica-group: followASpline", Be::READWRITE | Be::PERSIST )

	EXPOSURE_END()
}