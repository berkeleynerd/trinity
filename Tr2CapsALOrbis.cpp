#include "StdAfx.h"
#if( TRINITY_PLATFORM==TRINITY_ORBIS )
#include "Tr2CapsALOrbis.h"

Tr2CapsAL::Tr2CapsAL()
{
}

bool Tr2CapsAL::SupportsFloat16() const
{
	return true;
}

bool Tr2CapsAL::SupportsGpuBuffer() const
{
	return true;
}

bool Tr2CapsAL::SupportsStandaloneSwapChain() const
{
	return false;
}

uint32_t Tr2CapsAL::GetShaderVersion() const
{
	return 1;
}

bool Tr2CapsAL::SupportsVertexShaderTextures() const
{
	return true;
}

#endif
