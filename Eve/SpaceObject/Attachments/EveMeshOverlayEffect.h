#pragma once
#ifndef EveMeshOverlayEffect_H
#define EveMeshOverlayEffect_H


#include "Tr2MeshArea.h"
#include "ITr2Renderable.h"

BLUE_DECLARE( Tr2Effect );
BLUE_DECLARE( TriCurveSet );
BLUE_DECLARE_VECTOR( Tr2Effect );

// --------------------------------------------------------------------------------
// Description:
//   This class holds curveSets and Tr2Effects used for overlay effects for SpaceObjects.
//   These effects render all meshes (mesh indices) in the parent's mesh.
// SeeAlso:
//   Tr2SpaceObject2
// --------------------------------------------------------------------------------
BLUE_CLASS( EveMeshOverlayEffect ) :
	public IInitialize
{
public:
	EXPOSE_TO_BLUE();

	enum OverlayType
	{
		TYPE_OPAQUEONLY = 0,
		TYPE_ALL,

		TYPE_COUNT,
	};

	EveMeshOverlayEffect(IRoot* lockobj = NULL);
	~EveMeshOverlayEffect();

	//////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize();
	
	const PTr2EffectVector& GetEffects( TriBatchType batchType, bool& success ) const;
	void Update( Be::Time realTime, Be::Time simTime );

	// type is given by a batchtype
	OverlayType GetType( TriBatchType batchType ) const;

	// do we have transparent areas?
	bool HasTransparentArea() const;

	void SetShaderOption( const BlueSharedString& name, const BlueSharedString& value );

private:
	// general data
	bool m_display;
	bool m_update;
	std::string m_name;

	// all the effects per area
	PTr2EffectVector m_opaqueEffects;
	PTr2EffectVector m_decalEffects;
	PTr2EffectVector m_transparentEffects;
	PTr2EffectVector m_additiveEffects;
	PTr2EffectVector m_distortionEffects;

	// animating this overlay effect
	TriCurveSetPtr m_curveSet;

};

TYPEDEF_BLUECLASS( EveMeshOverlayEffect );
BLUE_DECLARE_VECTOR( EveMeshOverlayEffect );

#endif // EveMeshOverlayEffect_H
