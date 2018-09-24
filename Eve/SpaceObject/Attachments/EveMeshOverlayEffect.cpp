////////////////////////////////////////////////////////////
//
//    Created:   January 2012
//    Copyright: CCP 2012
//

#include "StdAfx.h"
#include "EveMeshOverlayEffect.h"
#include "Shader/Tr2Effect.h"
#include "Curves/TriCurveSet.h"


// --------------------------------------------------------------------------------------
// Description:
//   EveMeshOverlayEffect destructor
// --------------------------------------------------------------------------------------
EveMeshOverlayEffect::~EveMeshOverlayEffect()
{
}

// --------------------------------------------------------------------------------------
// Description:
//   EveMeshOverlayEffect constructor
// --------------------------------------------------------------------------------------
EveMeshOverlayEffect::EveMeshOverlayEffect( IRoot* lockobj ):
	m_display( true ),
	m_update( true ),
	PARENTLOCK( m_opaqueEffects ),
	PARENTLOCK( m_decalEffects ),
	PARENTLOCK( m_transparentEffects ),
	PARENTLOCK( m_additiveEffects ),
	PARENTLOCK( m_distortionEffects )
{
}

// --------------------------------------------------------------------------------------
// Description:
//   IInitialize
// --------------------------------------------------------------------------------------
bool EveMeshOverlayEffect::Initialize()
{
	return true;
}

// --------------------------------------------------------------------------------------
// Description:
//   For each batch type we gove back the appropriate overlay type!
// --------------------------------------------------------------------------------------
EveMeshOverlayEffect::OverlayType EveMeshOverlayEffect::GetType( TriBatchType batchType ) const
{
	switch( batchType )
	{
	case TRIBATCHTYPE_OPAQUE:
		return TYPE_OPAQUEONLY;
	default:
		return TYPE_ALL;
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Tell if this overlay effect holds any effects for transparent areas
// --------------------------------------------------------------------------------------
bool EveMeshOverlayEffect::HasTransparentArea() const
{
	return !m_transparentEffects.empty();
}

inline void SetIndividualShaderOption( const PTr2EffectVector& effectVector, const BlueSharedString& name, const BlueSharedString& value )
{
	for (auto it = effectVector.begin(); it != effectVector.end(); ++it)
	{
		Tr2Effect *effect = *it;
		effect->SetOption( name, value );
	}
}

void EveMeshOverlayEffect::SetShaderOption( const BlueSharedString& name, const BlueSharedString& value )
{
	SetIndividualShaderOption( m_opaqueEffects, name, value );
	SetIndividualShaderOption( m_decalEffects, name, value );
	SetIndividualShaderOption( m_transparentEffects, name, value );
	SetIndividualShaderOption( m_additiveEffects, name, value );
	SetIndividualShaderOption( m_distortionEffects, name, value );
}


// --------------------------------------------------------------------------------------
// Description:
//   GetEffect. 
// Return Value:
//   A Tr2EffectVector of effects.
// --------------------------------------------------------------------------------------
const PTr2EffectVector& EveMeshOverlayEffect::GetEffects(TriBatchType batchType, bool& success) const
{
	if ( m_display )
	{
		if ( batchType == TRIBATCHTYPE_OPAQUE )
		{
			success = true;
			return m_opaqueEffects;
		}
		else if ( batchType == TRIBATCHTYPE_DECAL )
		{
			success = true;
			return m_decalEffects;
		}
		else if ( batchType == TRIBATCHTYPE_TRANSPARENT )
		{
			success = true;
			return m_transparentEffects;
		}
		else if ( batchType == TRIBATCHTYPE_ADDITIVE )
		{
			success = true;
			return m_additiveEffects;
		}
		else if ( batchType == TRIBATCHTYPE_DISTORTION )
		{
			success = true;
			return m_distortionEffects;
		}
	}
	success = false;
	return m_opaqueEffects;
}


// --------------------------------------------------------------------------------------
// Description:
//   Update
// --------------------------------------------------------------------------------------
void EveMeshOverlayEffect::Update( Be::Time realTime, Be::Time simTime )
{
	if( !m_update || !m_curveSet )
	{
		return;
	}

	m_curveSet->Update( realTime, simTime );
}