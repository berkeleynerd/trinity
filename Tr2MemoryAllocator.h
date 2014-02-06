#pragma once
#ifndef Tr2MemoryAllocator_H
#define Tr2MemoryAllocator_H

#if( TRINITY_PLATFORM==TRINITY_ORBIS )

#include <gnm/gpumem.h>

namespace Tr2MemoryAllocator
{

enum MemoryType
{
	GARLIC,
	ONION,
};

void Initialize();
void* Allocate( MemoryType memoryType, size_t size, size_t align );
void* AllocateOnion( size_t size, size_t align );
void* AllocateGarlic( size_t size, size_t align );
void* Allocate( MemoryType memoryType, sce::Gnm::SizeAlign sizeAlign );
void* AllocateOnion( sce::Gnm::SizeAlign sizeAlign );
void* AllocateGarlic( sce::Gnm::SizeAlign sizeAlign );
void Release( void *ptr );

size_t GetOnionMemorySize();
size_t GetGarlicMemorySize();
uint32_t GetGarlicAvailableMemory();
uint32_t GetOnionAvailableMemory();

}



#endif

#endif