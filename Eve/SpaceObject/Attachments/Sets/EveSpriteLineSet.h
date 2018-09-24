////////////////////////////////////////////////////////////
//
//    Created:   January 2016
//    Copyright: CCP 2016
//
#pragma once
#ifndef EveSpriteLineSet_H
#define EveSpriteLineSet_H

#include "IEveSpaceObjectAttachment.h"
#include "EveSpriteLineSetItem.h"
#include "Eve/SpaceObject/Attachments/Sets/EveSpriteSet.h"

// forwards
BLUE_DECLARE( EveSpriteLineSet );
BLUE_DECLARE( Tr2QuadRenderer );
BLUE_DECLARE( Tr2Effect );

// --------------------------------------------------------------------------------
// Description:
//   This class is for rendering a line made of sprite set blinkies
// --------------------------------------------------------------------------------
BLUE_CLASS( EveSpriteLineSet ):
	public IEveSpaceObjectAttachment,
	public IInitialize,
	public Tr2DeviceResource
{
public:
	EXPOSE_TO_BLUE();

	EveSpriteLineSet( IRoot* lockobj = NULL );
	~EveSpriteLineSet();

	//////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize();

	//////////////////////////////////////////////////////////////////////////////////////
	// ITriDeviceResource
	void ReleaseResources( TriStorage s );
private:
	bool OnPrepareResources();

public:

	//////////////////////////////////////////////////////////////////////////////////////
	// IEveSpaceObjectAttachment
	virtual void RegisterWithQuadRenderer( Tr2QuadRenderer& quadRenderer );
	virtual void AddToQuadRenderer( Tr2QuadRenderer& quadRenderer, const Matrix& parentTransform, float activation, float boosterGain, const granny_matrix_3x4* bones, size_t boneCount );
	void SetShaderOption( const BlueSharedString& name, const BlueSharedString& value ) override;

	// setup
	void Setup( Tr2EffectPtr effect, bool isSkinned );
	void Add( EveSpriteLineSetItemPtr newItem );

	// rebuild resources
	void Rebuild();

private:
	// general data
	std::string m_name;
	bool m_display;
	bool m_skinned;

	// shader
	unsigned int m_effectHash;
	Tr2EffectPtr m_effect;

	// buffers for globals quadrenderer
	TrackableStdVector<EveSpriteSet::PoolVertex> m_buffer;
	TrackableStdVector<EveSpriteSet::SpriteData> m_spriteData;

	// all the line set
	PEveSpriteLineSetItemVector m_spriteLines;
};

TYPEDEF_BLUECLASS( EveSpriteLineSet );

#endif // EveSpriteLineSet_H