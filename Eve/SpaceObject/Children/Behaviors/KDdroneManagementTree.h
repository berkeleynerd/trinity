////////////////////////////////////////////////////////////
//
//    Created:   August 2019
//    Copyright: CCP 2019
//

#pragma once
#ifndef KDdroneManagementTree_H
#define KDdroneManagementTree_H

#include "Eve/SpaceObject/Children/EveChildBehaviorSystem.h"
#include "DroneAgent.h"

enum planeType
{
	X = 0,
	Y = 1,
	Z = 2,
};

struct searchRange
{
	searchRange() :
		behaviorNbr( 0 ),
		radius( 0 )
	{}
	int behaviorNbr;
	float radius;
};

struct closestDrone
{
	closestDrone() :
		agent( nullptr ),
		rangeBetweenSq( 0 )
	{}
	DroneAgent* agent;
	float rangeBetweenSq;
};

struct AgentRef
{
	AgentRef() :
		agent( nullptr ),
		planeType( X ),
		Left( nullptr ),
		Right( nullptr )
	{}
	
	DroneAgent* agent;
	planeType planeType;
	int b;
	int e;
	AgentRef* Left;
	AgentRef* Right;
};

struct compareRef
{
	bool operator()( const AgentRef& lhs, const AgentRef& rhs ) const
	{
		switch ( rhs.planeType )
		{
		case X:
			return lhs.agent->position.x < rhs.agent->position.x;
			break;
		case Y:
			return lhs.agent->position.y < rhs.agent->position.y;
			break;
		case Z:
			return lhs.agent->position.z < rhs.agent->position.z;
			break;
		}
		return false;
	}

	bool operator()( const searchRange& lhs, const searchRange& rhs ) const
	{
		return lhs.radius > rhs.radius;
	}
};

BLUE_CLASS( KDdroneManagementTree ): public IRoot
{
public:
	EXPOSE_TO_BLUE();
	KDdroneManagementTree( IRoot* lockobj = nullptr );
	~KDdroneManagementTree();

	DroneAgent* findClosestAgent( Vector3 pos );
	std::vector<std::vector<std::vector<DroneAgent*>>> FindDronesInRange( std::vector<DroneAgent>& agents, std::vector<float>& ranges, float& BehaviorGroupboundingSphereRadius);
	void CreateTree(std::vector<DroneAgent>& agents);
	void UpdateTree( const float dt );
	void RenderDebugInfo(Tr2DebugRenderer& renderer, float debugSquareSize, Matrix& parentWorldLocation);

private:
	AgentRef* CompareNodeToChildren(AgentRef* node);
	bool IsBiggestOnAxis(AgentRef* node, float n, planeType pt);
	bool IsSmallestOnAxis(AgentRef* node, float n, planeType pt);
	AgentRef* SplitSort(int b, int e, planeType pt);
	void ChangeAgentsIntoAgentRefs(std::vector<DroneAgent>& agents);
	planeType FindNextSplitAxis(planeType pt);
	
	
	void findClosestAgentRecursive( Vector3& pos, AgentRef* currentNode, closestDrone& agent );
	void searchThroughTree( std::vector<std::vector<std::vector<DroneAgent*>>>& closeAgents, AgentRef* node, std::vector<DroneAgent>& agents, std::vector<searchRange>& ranges, int& activeRange) const;
	void searchThroughTreeHelperFunction( std::vector<std::vector<std::vector<DroneAgent*>>>& closeAgents, AgentRef* node, DroneAgent& agent, std::vector<searchRange>& ranges, int& activeRange, int& c ) const;

	static void AddAgentToSearchLists( std::vector<std::vector<std::vector<DroneAgent*>>>& closeAgents, AgentRef* node,
	                                  float& dist, std::vector<searchRange>& ranges, int& activeRange, int& agentNbr );
	std::vector<AgentRef>& sortByAxis(std::vector<AgentRef>& agents, int b, int e, planeType pt) const;

	// debug
	void DrawTree(Tr2DebugRenderer& renderer, AgentRef* tree, Vector3 debugSquareCorner1, Vector3 debugSquareCorner2, Vector3& pwt );
	void DrawSquareInnerLines(Tr2DebugRenderer& renderer, Vector3& agentPos, Vector3& P1, Vector3& P2, Color C, planeType pt, Vector3& pwt);

	float m_debugSquareSize;
	bool m_isInitialized;
	AgentRef m_tree;
	std::vector< AgentRef > m_agentRefs;
	float m_updateTimeCounter;
	float m_timeBetweenUpdate;
};

TYPEDEF_BLUECLASS( KDdroneManagementTree );

#endif