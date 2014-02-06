#include "StdAfx.h"
#if( TRINITY_PLATFORM==TRINITY_OPENGLES2 )
#include "Tr2CapsALGLES2.h"

Tr2CapsAL::Tr2CapsAL()
	:m_supportsFloat16( false )
{
}

bool Tr2CapsAL::SupportsFloat16() const
{
    return m_supportsFloat16;
}

bool Tr2CapsAL::SupportsGpuBuffer() const
{
	return false;
}

bool Tr2CapsAL::SupportsStandaloneSwapChain() const
{
#if defined(TRINITY_AL_MOBILE)
	return false;
#else
	return true;
#endif
}

uint32_t Tr2CapsAL::GetShaderVersion() const
{
	return 2;
}

bool Tr2CapsAL::SupportsVertexShaderTextures() const
{
	return true;
}

bool Tr2CapsAL::SupportsNoOverwriteLock() const
{
#if defined(TRINITY_AL_MOBILE)
	return false;
#else
	return true;
#endif
}

#endif
