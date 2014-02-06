////////////////////////////////////////////////////////////
//
//    Created:   April 2012
//    Copyright: CCP 2012
//

#pragma once
#ifndef Tr2SwapChainALOrbis_H
#define Tr2SwapChainALOrbis_H

#if( TRINITY_PLATFORM==TRINITY_ORBIS )

#include "Tr2AutoResetObjectAL.h"

class Tr2RenderContextAL;

class Tr2SwapChainAL: 
	public Tr2AutoResetObjectAL, 
	public Tr2TrackedALObject<Tr2RenderContextEnum::OT_SWAP_CHAIN>
{
public:
	Tr2SwapChainAL() {}

	ALResult Create( Tr2WindowHandle windowHandle, Tr2RenderContextAL& renderContext ) { return E_FAIL; }
	void Destroy() {}

	bool IsValid() const { return false; }

	ALResult Present( Tr2RenderContextAL& renderContext ) { return E_FAIL; }

	int GetWidth() const { return 0; }
	int GetHeight() const { return 0; }

	bool operator==( const Tr2SwapChainAL& other ) const { return this == &other; }

	Tr2ALMemoryType GetMemoryClass() const { return AL_MEMORY_VIDEO; }

	Tr2RenderTargetAL m_backBuffer;
private:
	Tr2SwapChainAL( const Tr2SwapChainAL& ) /* = delete */;
	Tr2SwapChainAL& operator=( const Tr2SwapChainAL& ) /* = delete */;

	virtual void ReleaseALResource() {};
	virtual void PrepareALResource( Tr2PrimaryRenderContextAL& renderContext ) {};
};

#endif

#endif