#include "StdAfx.h"
#include "BackAndForth.h"
#include "include/TriMath.h"

const BlueSharedString DELIVER_LOCATOR_SET_NAME( "deliver" );
const BlueSharedString SEEK_LOCATOR_SET_NAME( "seek" );

BackAndForth::BackAndForth( IRoot* lockobj ) :
	m_arrivedRadius( 50.f ),
	m_slowDownRadius( 200.f ),
	m_backAndForthWeight( 100.f )
{
}

BackAndForth::~BackAndForth()
{
}

std::vector<Vector3> BackAndForth::CalculateBehavior(std::vector<DroneAgent>& agents, const float deltaTime,
                                                     BehaviorGroup& sys, EveChildBehaviorSystem& system)
{
	for (auto agent = agents.begin(); agent != agents.end(); ++agent)
	{
		if( agent->arrived )
		{
			if( agent->seek )
			{
				//Get all locators under the "seek" locatorSet
				auto seekLocators = sys.GetLocatorsForSet( SEEK_LOCATOR_SET_NAME );
				m_rand = TriRandInt( 0, (int)seekLocators->size() );
				agent->target = seekLocators[0][m_rand].position;
			
			}
			//If the deliver behavior is active
			else if( agent->deliver )
			{
				//Get all locators under the "deliver" locatorSet
				auto deliverLocators = sys.GetLocatorsForSet( DELIVER_LOCATOR_SET_NAME );
				m_rand = TriRandInt( 0, (int)deliverLocators->size() );
				agent->target = deliverLocators[0][m_rand].position;
				
			}
			agent->arrived = false;
		}
		Vector3 target = agent->target - agent->position;
		float distance = Length( target );
		target = Normalize( target );

		//If we are approaching the target
		if (distance < m_slowDownRadius)
		{
			// make the agent slow down before arriving at target
			target = target * m_backAndForthWeight * ( distance / m_slowDownRadius );
			
			//If the agent has arrived to the target, then switch targets
			if (distance < m_arrivedRadius)
			{
				std::swap( agent->seek, agent->deliver );
				agent->arrived = true;
			}	
		}
		else
		{
			target *= m_backAndForthWeight;
		}

		agent->acceleration += target;
	}
	std::vector<Vector3> todo;
	return todo;
}

void BackAndForth::RenderDebugInfo(Tr2DebugRenderer& renderer, std::vector<DroneAgent>& agents, Matrix& parentWorldLocation)
{
}