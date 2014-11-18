////////////////////////////////////////////////////////////
//
//    Created:   November 2014
//    Copyright: CCP 2014
//
#include "StdAfx.h"
#include "EveSceneStaticParticles.h"

#include "..\EveTransform.h"
#include "..\..\Tr2InstancedMesh.h"

// --------------------------------------------------------------------------------
// Description:
// --------------------------------------------------------------------------------
EveSceneStaticParticles::EveSceneStaticParticles( IRoot* lockobj )
{

}

// --------------------------------------------------------------------------------
// Description:
//   tear down
// --------------------------------------------------------------------------------
EveSceneStaticParticles::~EveSceneStaticParticles()
{
}

// --------------------------------------------------------------------------------
// Description:
//   Adds a cluster of particles.
// --------------------------------------------------------------------------------
void EveSceneStaticParticles::AddCluster(Vector3 position, int count, float posDeviation, Color color1, Color color2)
{
	if( !m_transform )
	{
		return;
	}

}

// --------------------------------------------------------------------------------
// Description:
//   Adds a single particle.
// --------------------------------------------------------------------------------
void EveSceneStaticParticles::AddSingle(Vector3 position, float size, Color color)
{
	if( !m_transform )
	{
		return;
	}
}


// --------------------------------------------------------------------------------
// Description:
//   REset things once the red file is fully loaded
// --------------------------------------------------------------------------------
bool EveSceneStaticParticles::Initialize()
{
	return true;
}

