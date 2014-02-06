#include "StdAfx.h"

#if( TRINITY_PLATFORM==TRINITY_ORBIS )

#include "Tr2SamplerStateALOrbis.h"
#include <cmath>

using namespace Tr2RenderContextEnum;

// Force anisotropic filtering switch:
// 0 means use whatever is specified in the effect (default)
// 1 means turn off anisotropic filtering (fallback to linear)
// >1 - force max anisotropy to specified number
uint32_t g_forceAnisotropy = 0;

namespace
{

sce::Gnm::WrapMode ConvertToWrapMode( TextureAddressMode mode )
{
	switch( mode )
	{
	case TA_MIRROR:
		return sce::Gnm::kWrapModeMirror;
	case TA_CLAMP:
		return sce::Gnm::kWrapModeClampLastTexel;
	case TA_BORDER:
		return sce::Gnm::kWrapModeClampBorder;
	case TA_MIRROR_ONCE:
		return sce::Gnm::kWrapModeMirrorOnceLastTexel;
	default:
		return sce::Gnm::kWrapModeWrap;
	}
}

float ClampToU4_8( float value )
{
	float clampedValue = std::min( value, 15.9999f );
	clampedValue = std::max( clampedValue, 0.0f );

	return clampedValue;
}

}

Tr2SamplerStateAL::Tr2SamplerStateAL()
	:m_isValid( false )
{
}

ALResult Tr2SamplerStateAL::Create(
	Tr2RenderContextAL& renderContext,
	const Tr2SamplerDescription& description )
{
	m_sampler.init();
	sce::Gnm::FilterMode minFilter, magFilter;
	sce::Gnm::ZFilterMode zFilter = sce::Gnm::kZFilterModeLinear;
	switch( description.m_minFilter )
	{
	case TF_LINEAR:
		minFilter = sce::Gnm::kFilterModeBilinear;
		break;
	case TF_ANISOTROPIC:
		minFilter = sce::Gnm::kFilterModeAnisoBilinear;
		break;
	default:
		minFilter = sce::Gnm::kFilterModePoint;
		zFilter = sce::Gnm::kZFilterModePoint;
	}
	switch( description.m_magFilter )
	{
	case TF_LINEAR:
		magFilter = sce::Gnm::kFilterModeBilinear;
		break;
	case TF_ANISOTROPIC:
		magFilter = sce::Gnm::kFilterModeAnisoBilinear;
		break;
	default:
		magFilter = sce::Gnm::kFilterModePoint;
	}
	m_sampler.setXyFilterMode( minFilter, magFilter );
	m_sampler.setZFilterMode( zFilter );

	sce::Gnm::MipFilterMode mipFilter;
	switch( description.m_mipFilter )
	{
	case TF_POINT:
		mipFilter = sce::Gnm::kMipFilterModePoint;
		break;
	case TF_NONE:
		mipFilter = sce::Gnm::kMipFilterModeNone;
		break;
	default:
		mipFilter = sce::Gnm::kMipFilterModeLinear;
	}
	m_sampler.setMipFilterMode( mipFilter );

	sce::Gnm::AnisotropyRatio anisotropy;
	if( description.m_maxAnisotropy >= 16 )
	{
		anisotropy = sce::Gnm::kAnisotropyRatio16;
	}
	else if( description.m_maxAnisotropy >= 8 )
	{
		anisotropy = sce::Gnm::kAnisotropyRatio8;
	}
	else if( description.m_maxAnisotropy >= 4 )
	{
		anisotropy = sce::Gnm::kAnisotropyRatio4;
	}
	else if( description.m_maxAnisotropy >= 2 )
	{
		anisotropy = sce::Gnm::kAnisotropyRatio2;
	}
	else
	{
		anisotropy = sce::Gnm::kAnisotropyRatio1;
	}
	m_sampler.setAnisotropyRatio( anisotropy );

	m_sampler.setWrapMode( 
		ConvertToWrapMode( description.m_addressU ), 
		ConvertToWrapMode( description.m_addressV ), 
		ConvertToWrapMode( description.m_addressW ) );

	m_sampler.setLodBias( sce::Gnmx::convertF32ToS6_8( description.m_mipLODBias ), 0 );

	if( description.m_addressU == TA_BORDER ||
		description.m_addressV == TA_BORDER ||
		description.m_addressW == TA_BORDER )
	{
		m_sampler.setBorderColorTableIndex( renderContext.InternalGetBorderColorIndex( description.m_borderColor ) );
	}
	m_sampler.setLodRange( 

		sce::Gnmx::convertF32ToU4_8( ClampToU4_8( description.m_minLOD ) ),
		sce::Gnmx::convertF32ToU4_8( ClampToU4_8( description.m_maxLOD ) ) );

	if( description.m_isComparisonFilter )
	{
		sce::Gnm::DepthCompare depthCompare;
		switch( description.m_comparisonFunc )
		{
		case CMP_NEVER:
			depthCompare = sce::Gnm::kDepthCompareNever;
			break;
		case CMP_LESS:
			depthCompare = sce::Gnm::kDepthCompareLess;
			break;
		case CMP_EQUAL:
			depthCompare = sce::Gnm::kDepthCompareEqual;
			break;
		case CMP_LESSEQUAL:
			depthCompare = sce::Gnm::kDepthCompareLessEqual;
			break;
		case CMP_GREATER:
			depthCompare = sce::Gnm::kDepthCompareGreater;
			break;
		case CMP_NOTEQUAL:
			depthCompare = sce::Gnm::kDepthCompareNotEqual;
			break;
		case CMP_GREATEREQUAL:
			depthCompare = sce::Gnm::kDepthCompareGreaterEqual;
			break;
		default:
			depthCompare = sce::Gnm::kDepthCompareAlways;
			break;
		}
		m_sampler.setDepthCompareFunction( depthCompare );
	}
	m_isValid = true;
	return S_OK;
}

bool Tr2SamplerStateAL::IsValid() const
{
	return m_isValid;
}

bool Tr2SamplerStateAL::operator==( const Tr2SamplerStateAL& other ) const
{
	return this == &other;
}

#endif