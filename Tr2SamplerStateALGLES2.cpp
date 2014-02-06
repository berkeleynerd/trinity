#include "StdAfx.h"

#if( TRINITY_PLATFORM==TRINITY_OPENGLES2 )

#include "Tr2SamplerStateALGLES2.h"

#ifndef GL_TEXTURE_3D
#define GL_TEXTURE_3D GL_TEXTURE_3D_OES
#define GL_TEXTURE_WRAP_R GL_TEXTURE_WRAP_R_OES
#endif

// Force anisotropic filtering switch:
// 0 means use whatever is specified in the effect (default)
// 1 means turn off anisotropic filtering (fallback to linear)
// >1 - force max anisotropy to specified number
uint32_t g_forceAnisotropy = 0;

using namespace Tr2RenderContextEnum;

Tr2SamplerStateAL::Tr2SamplerStateAL()
:   m_isValid( false )
{
	m_magFilter = GL_LINEAR;
	m_minFilter = GL_LINEAR;
	m_minFilterNoMips = GL_LINEAR;
	m_wrapT = GL_REPEAT;
	m_wrapS = GL_REPEAT;
	m_wrapR = GL_REPEAT;
	m_anisotropy = 0;
#if !defined(TRINITY_AL_MOBILE)
	m_minLod = 0.f;
	m_maxLod = std::numeric_limits<float>::max();
#endif
}

ALResult Tr2SamplerStateAL::Create( 
	Tr2RenderContextAL& /*renderContext*/,
	const Tr2SamplerDescription& description )
{
	AL_FUZZ( OT_SAMPLER_STATE );

	switch( description.m_minFilter )
	{
	case TF_POINT:
		switch( description.m_mipFilter )
		{
		case TF_NONE:
			m_minFilter = GL_NEAREST;
			break;
		case TF_POINT:
			m_minFilter = GL_NEAREST_MIPMAP_NEAREST;
			break;
		default:
			m_minFilter = GL_NEAREST_MIPMAP_LINEAR;
		}
		m_minFilterNoMips = GL_NEAREST;
		break;
	default:
		switch( description.m_mipFilter )
		{
		case TF_NONE:
			m_minFilter = GL_LINEAR;
			break;
		case TF_POINT:
			m_minFilter = GL_LINEAR_MIPMAP_NEAREST;
			break;
		default:
			m_minFilter = GL_LINEAR_MIPMAP_LINEAR;
		}
		m_minFilterNoMips = GL_LINEAR;
	}
	if( description.m_magFilter == TF_POINT )
	{
		m_magFilter = GL_NEAREST;
	}
	else
	{
		m_magFilter = GL_LINEAR;
	}

	static const int wrapModes[] = {
		0,					// ---
		GL_REPEAT,          // TA_WRAP
		GL_MIRRORED_REPEAT, // TA_MIRROR
		GL_CLAMP_TO_EDGE,   // TA_CLAMP
		GL_CLAMP_TO_EDGE,   // TA_BORDER
		GL_CLAMP_TO_EDGE,   // TA_MIRROR_ONCE
	};

	m_wrapT = wrapModes[description.m_addressU];
	m_wrapS = wrapModes[description.m_addressV];
	m_wrapR = wrapModes[description.m_addressW];

	if( description.m_minFilter == TF_ANISOTROPIC || description.m_magFilter == TF_ANISOTROPIC || description.m_mipFilter == TF_ANISOTROPIC )
	{
		m_anisotropy = g_forceAnisotropy ? g_forceAnisotropy : std::max( description.m_maxAnisotropy, 1u );
	}
	else
	{
		m_anisotropy = 1;
	}
#if !defined(TRINITY_AL_MOBILE)
	m_minLod = description.m_minLOD;
	m_maxLod = description.m_maxLOD;
#endif
    m_isValid = true;
	return S_OK;
}

bool Tr2SamplerStateAL::IsValid() const
{
	return m_isValid;
}

bool Tr2SamplerStateAL::operator==( const Tr2SamplerStateAL& sampler ) const
{
	return this == &sampler;
}

long Tr2SamplerStateAL::Apply( GLenum textureType, bool hasMipLevels )
{
	AL_FUZZ( OT_SAMPLER_STATE );

	CR_GL( glTexParameteri( textureType, GL_TEXTURE_MIN_FILTER, hasMipLevels ? m_minFilter : m_minFilterNoMips ) );
	CR_GL( glTexParameteri( textureType, GL_TEXTURE_MAG_FILTER, m_magFilter ) );
	CR_GL( glTexParameteri( textureType, GL_TEXTURE_WRAP_S, textureType == GL_TEXTURE_CUBE_MAP ? GL_CLAMP_TO_EDGE : m_wrapT ) );
	CR_GL( glTexParameteri( textureType, GL_TEXTURE_WRAP_T, textureType == GL_TEXTURE_CUBE_MAP ? GL_CLAMP_TO_EDGE : m_wrapS ) );
	if( textureType == GL_TEXTURE_3D )
	{
		CR_GL( glTexParameteri( textureType, GL_TEXTURE_WRAP_R, m_wrapR ) );
	}
	// setting max anisotropy might fail if ext. is not there, but it is
	// ok, since then we'll have nice fallback to linear/nearest
	GL_IGNORE_ERROR( glTexParameterf( textureType, GL_TEXTURE_MAX_ANISOTROPY_EXT, GLfloat( std::max( m_anisotropy, 1 ) ) ) );
#if !defined(TRINITY_AL_MOBILE)
	CR_GL( glTexParameterf( textureType, GL_TEXTURE_MIN_LOD, m_minLod ) );
	CR_GL( glTexParameterf( textureType, GL_TEXTURE_MAX_LOD, m_maxLod ) );
#endif
	return S_OK;
}

#endif // TRINITY_PLATFORM==TRINITY_OPENGLES2