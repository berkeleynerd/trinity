////////////////////////////////////////////////////////////
//
//    Created:   November 2014
//    Copyright: CCP 2014
//

#pragma once
#ifndef EveSceneStaticParticles_H
#define EveSceneStaticParticles_H

#include "ITr2Renderable.h"
#include "ITr2GeometryProvider.h"
#include "Eve/IEveSpaceObject2.h"
#include "EveMetaballItem.h"
#include "Tr2Renderer.h"
#include "Tr2PersistentPerObjectData.h"

BLUE_DECLARE( EveTransform );


// --------------------------------------------------------------------------------
// Description:
// --------------------------------------------------------------------------------
BLUE_CLASS( EveSceneStaticParticles ) :
	public IInitialize
{
public:
	EXPOSE_TO_BLUE();

	EveSceneStaticParticles(IRoot* lockobj = NULL);
	~EveSceneStaticParticles();

	//////////////////////////////////////////////////////////////////////////
	// IInitialize
	virtual bool Initialize();

	void AddCluster(Vector3 position, int count, float posDeviation, Color color1, Color color2);
	void AddSingle(Vector3 position, float size, Color color);

private:
	float m_minSize;
	float m_maxSize;
	int m_maxParticleCount;

	EveTransformPtr m_transform;
};

TYPEDEF_BLUECLASS( EveSceneStaticParticles );
BLUE_DECLARE_VECTOR( EveSceneStaticParticles );

#endif