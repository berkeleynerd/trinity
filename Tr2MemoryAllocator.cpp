#include "StdAfx.h"

#if( TRINITY_PLATFORM==TRINITY_ORBIS )

#include "Tr2MemoryAllocator.h"
#include "ALLog.h"

namespace
{

class Region
{
public:
	uint64_t m_typeAndStart;
	uint32_t m_size;
	uint32_t m_guard;
	Region *m_next;
};

class RegionAllocator 
{
private:
	Region *m_freeRegions;
	Region *m_usedRegions;
	Region *m_allRegions;
	uint32_t m_numRegions;
	uint64_t m_memoryStart;
	uint64_t m_memoryEnd;

	uint32_t m_availableMemory;

	Region* FindUnusedRegion() const;
	Region* AllocateRegion( uint32_t size, uint64_t type, uint32_t alignment );
	void ReleaseRegion( uint64_t start, uint64_t type );
public:
	void Init( uint64_t memStart, uint32_t memSize, Region* regions, uint32_t numRegions );

	void* Allocate( uint32_t size, sce::Gnm::AlignmentType alignment );
	void Release( void* );

	uint32_t GetAvailableMemory() const
	{
		return m_availableMemory;
	}
};

static const uint64_t kRegionTypeMask = 0xE000000000000000ULL;
static const uint64_t kRegionStartMask = 0x1FFFFFFFFFFFFFFFULL;

static const uint64_t kUnusedRegionType      = 0x0000000000000000ULL;
static const uint64_t kSystemRegionType      = 0x2000000000000000ULL;
static const uint64_t kSharedVideoRegionType = 0x4000000000000000ULL;
static const uint64_t kPrivateVideoRegionType= 0x8000000000000000ULL;


void MapMemory(uint64_t* system, uint32_t* systemSize, uint64_t* gpuShared, uint32_t* gpuSharedSize)
{
	CCP_ASSERT(system && systemSize && gpuShared && gpuSharedSize);
	//----------------------------------------------------------------------
	// Setup the memory pools:
#ifdef GNM_EARLY_LIMITATION
	const uint32_t	systemPoolSize	   = 1024 * 1024 * 256;
	const uint32_t	sharedGpuPoolSize  = 1024 * 1024 * 512;
#else // GNM_EARLY_LIMITATION
	const uint32_t	systemPoolSize	   = 1024 * 1024 * 1024;
	const uint32_t	sharedGpuPoolSize  = 1024 * 1024 * 512;
#endif // GNM_EARLY_LIMITATION

	const uint32_t  systemAlignment		= 2 * 1024 * 1024;
	const uint32_t  shaderGpuAlignment	= 2 * 1024 * 1024;

	off_t systemPoolOffset;		// TODO: store these value to be able to free them later.
	off_t sharedGpuPoolOffset;
	
	void*	systemPoolPtr	  = NULL;
	void*	sharedGpuPoolPtr  = NULL;

	uint32_t result = S_OK;

	int retSys = sceKernelAllocateDirectMemory(0,
		SCE_KERNEL_MAIN_DMEM_SIZE,
		systemPoolSize,
		systemAlignment, // alignment
		SCE_KERNEL_WB_ONION,
		&systemPoolOffset);
	if ( !retSys )
	{
		retSys = sceKernelMapDirectMemory(&systemPoolPtr,
			systemPoolSize,
			SCE_KERNEL_PROT_CPU_READ|SCE_KERNEL_PROT_CPU_WRITE|SCE_KERNEL_PROT_GPU_ALL,
			0,						//flags
			systemPoolOffset,
			systemAlignment);
	}
	if ( retSys )
	{
		CCP_AL_LOGERR( "sce::Gnm::Initialize Error: System Memory Allocation Failed (%d)", retSys );
		result = E_FAIL;
	}


	//


	int retShr = sceKernelAllocateDirectMemory(0,
		SCE_KERNEL_MAIN_DMEM_SIZE,
		sharedGpuPoolSize,
		shaderGpuAlignment,	// alignment
		SCE_KERNEL_WC_GARLIC,
		&sharedGpuPoolOffset);

	if ( !retShr )
	{
		retShr = sceKernelMapDirectMemory(&sharedGpuPoolPtr,
			sharedGpuPoolSize,
			SCE_KERNEL_PROT_CPU_READ|SCE_KERNEL_PROT_CPU_WRITE|SCE_KERNEL_PROT_GPU_ALL,
			0,						//flags
			sharedGpuPoolOffset,
			shaderGpuAlignment);
	}
	if ( retShr )
	{
		CCP_AL_LOGERR( "sce::Gnm::Initialize Error: Shared Gpu Memory Allocation Failed (%d)", retShr );
		result = E_FAIL;
	}
	//

	if ( result == S_OK )
	{
		
		*gpuShared = (uint64_t)sharedGpuPoolPtr;
		*gpuSharedSize    = sharedGpuPoolSize;

		*system = (uint64_t)systemPoolPtr;
		*systemSize    = systemPoolSize;
	}
	
	CCP_AL_LOG( "Shared System size    : 0x%010lX", static_cast<uint64_t>( systemPoolSize ) );
	CCP_AL_LOG( "Shared System CPU Addr: 0x%010lX", uintptr_t( system ) );
	CCP_AL_LOG( "Shared Video size     : 0x%010lX", static_cast<uint64_t>( sharedGpuPoolSize ) );
	CCP_AL_LOG( "Shared Video CPU Addr : 0x%010lX", uintptr_t( gpuShared ) );
}


void RegionAllocator::Init(uint64_t memStart, uint32_t memSize, Region* regions, uint32_t numRegions)
{
	m_memoryStart = memStart;
	m_memoryEnd = memStart + memSize;
	m_allRegions = regions;
	m_numRegions = numRegions;
	m_freeRegions = NULL;
	m_usedRegions = NULL;

	memset(m_allRegions,0,sizeof(Region)*m_numRegions);
	if( memSize )
	{
		Region* r = FindUnusedRegion();
		r->m_next = NULL;
		r->m_guard = 0;
		r->m_typeAndStart = m_memoryStart | kSystemRegionType;
		r->m_size = memSize;
		m_freeRegions = r;
		m_usedRegions = NULL;
	}

	m_availableMemory = memSize;
}

Region* RegionAllocator::FindUnusedRegion() const
{
	for (uint32_t i = 0; i < m_numRegions; ++i)
	{
		Region *r = &m_allRegions[i];
		if ((r->m_typeAndStart & kRegionTypeMask) == kUnusedRegionType)
			return r;
	}
	return 0;
}

void* RegionAllocator::Allocate(uint32_t size, sce::Gnm::AlignmentType alignment)
{
	Region *r = AllocateRegion( size, kSystemRegionType, alignment);
	if (r != NULL)
	{
		uint32_t *start = (uint32_t *)uintptr_t(r->m_typeAndStart & kRegionStartMask);
		// We've had reports of allocate*Memory() returning blocks that aren't fully within GPU-visible RAM.
		// Let's catch that!
#if GNM_MEMORY_DEBUG_OVERFLOW_CHECK==1
		SCE_GNM_ASSERT(alignment == r->m_guard);

		// Note: end may not be aligned to a word size!!
		uintptr_t start_u8 = (uintptr_t)start;
		uintptr_t end_u8   = start_u8 + (size + alignment);
		uint32_t  *end     = (uint32_t*)end_u8;

		for (uint32_t i=0; i<alignment/4; ++i)
		{
			start[i] = 0xdea110c8;
			end[i] = 0xdea110c8;
		}
		start += alignment/4;
#endif
		return start;
	}
	else
		return NULL;
}

void RegionAllocator::Release(void* ptr)
{
	if(ptr)
	{
		uint64_t gpuAddr = uintptr_t(ptr);
		ReleaseRegion(gpuAddr, kSystemRegionType);
	}
}

Region* RegionAllocator::AllocateRegion(uint32_t size, uint64_t type, uint32_t alignment)
{
	// must be power of 2
	SCE_GNM_ASSERT((alignment & (alignment-1)) == 0);

	uint32_t guard = 0;
#if GNM_MEMORY_DEBUG_OVERFLOW_CHECK==1
	guard = alignment;
	size += alignment * 2;
#endif

	Region* r;
	Region* prevRegion = nullptr;
	for( r = m_freeRegions; r; prevRegion = r, r = r->m_next )
	{
		// the following code ensures that all allocations with alignment<512 are aligned to 512, skipped
		// forward by 1024, then skipped back by the requested alignment. this ensures MISALIGNMENT
		// to (alignment..512] which makes alignment-truncated addresses point as far backwards
		// into memory "landfill" as is practical.

		uint32_t miss;
#if GNM_MEMORY_DEBUG_ALIGNMENT_CHECK==1
		enum { kMaximumIntentionalMisalignment = 256 };
		if( alignment < kMaximumIntentionalMisalignment*2 )
		{
			const uint32_t begin = (uint32_t)(r->m_typeAndStart & Toolkit::kRegionStartMask);
			uint32_t end = begin;
			end &= ~(kMaximumIntentionalMisalignment*2-1); // align to 512
			end +=   kMaximumIntentionalMisalignment*2*2;  // skip forward by 1024
			end -= alignment; // subtract desired alignment
			miss = end - begin; // calculate difference
		}
		else
#endif
		{
			miss = static_cast<uint32_t>(r->m_typeAndStart & (alignment-1));
			if(miss != 0)
				miss = alignment - miss;
		}

		if( r->m_size >= size + miss )
		{
			if( miss != 0 )
			{
				// Align the region start
				Region *n = FindUnusedRegion();
				CCP_ASSERT( n );

				n->m_size = miss;
				n->m_typeAndStart = r->m_typeAndStart;
				n->m_next = r;
				n->m_guard = 0;
				CCP_ASSERT((r->m_typeAndStart & kRegionStartMask) + miss <= kRegionStartMask); // optimization check
				r->m_typeAndStart += miss;
				r->m_size -= miss;
				if( prevRegion )
				{
					prevRegion->m_next = n;
				}
				else
				{
					m_freeRegions = n;
				}
				prevRegion = n;
			}

			uint64_t off = r->m_typeAndStart & kRegionStartMask;
			CCP_ASSERT((r->m_typeAndStart & kRegionStartMask) + size <= kRegionStartMask); // optimization check
			r->m_typeAndStart += size;
			r->m_size -= size;
			CCP_ASSERT((off & (alignment-1)) == 0);

			CCP_ASSERT( m_availableMemory >= size );
			m_availableMemory -= size;

			if( r->m_size == 0 )
			{
				// Unlink me
				if( prevRegion )
				{
					prevRegion->m_next = r->m_next;
				}
				else
				{
					m_freeRegions = r->m_next;
				}

				// Re-link as used region
				Region *u = r;
				u->m_next = m_usedRegions;
				u->m_size = size;
				u->m_typeAndStart = off | type;
				u->m_guard = guard;
				m_usedRegions = u;

				return u;
			}
			else
			{
				// Link as used region
				Region *u = FindUnusedRegion();
				CCP_ASSERT( u );

				u->m_next = m_usedRegions;
				u->m_size = size;
				u->m_guard = guard;
				u->m_typeAndStart = off | type;
				m_usedRegions = u;

				return u;
			}
		}
	}

	return nullptr;
}

void RegionAllocator::ReleaseRegion(uint64_t start, uint64_t type)
{
	SCE_GNM_UNUSED(type);

	// Find the region in the used list
	Region* r;
	Region* prevRegion = nullptr;
	for( r = m_usedRegions; r; prevRegion = r, r = r->m_next )
	{
		if ((r->m_typeAndStart & kRegionStartMask) == start)
		{
			CCP_ASSERT((r->m_typeAndStart & kRegionTypeMask) == type);// "Released with different type than allocated with! %08x %08x", r->m_typeAndStart & Toolkit::kRegionTypeMask, type
			//printf("f: %08x %08x\n", r->m_typeAndStart, r->m_size);

			m_availableMemory += r->m_size;

			// Unlink
			if( prevRegion )
			{
				prevRegion->m_next = r->m_next;
			}
			else
			{
				m_usedRegions = r->m_next;
			}

			uint64_t re = (r->m_typeAndStart & kRegionStartMask) + r->m_size;

			// Add to free list, find insertion point
			Region* f;
			Region *previousFreeRegion = nullptr;
			for( f = m_freeRegions; f; previousFreeRegion = f, f = f->m_next )
			{
				if ((f->m_typeAndStart & kRegionStartMask) > start)
				{
					// Merge nearby blocks?
					if (previousFreeRegion && ((previousFreeRegion->m_typeAndStart & kRegionStartMask) + previousFreeRegion->m_size == (r->m_typeAndStart & kRegionStartMask)))
					{
						//printf("p\n");
						// Merge with previous
						previousFreeRegion->m_size += r->m_size;
						r->m_typeAndStart &= kRegionStartMask;

						// Merge prev with next?
						if ((previousFreeRegion->m_typeAndStart & kRegionStartMask) + previousFreeRegion->m_size == (f->m_typeAndStart & kRegionStartMask))
						{
							//printf("pn\n");
							previousFreeRegion->m_size += f->m_size;
							previousFreeRegion->m_next = f->m_next;
							f->m_typeAndStart &= kRegionStartMask;
						}
					}
					else if (re == (f->m_typeAndStart & kRegionStartMask))
					{
						//printf("n\n");
						// Merge with next
						f->m_typeAndStart -= r->m_size;
						f->m_size += r->m_size;
						r->m_typeAndStart &= kRegionStartMask;
					}
					else
					{
						// Insert
						r->m_next = f;
						if (previousFreeRegion)
							previousFreeRegion->m_next = r;
						else
							m_freeRegions = r;
					}
					break;
				}
			}

			if (f == NULL)
			{
				// Add to end
				r->m_next = NULL;
				if (previousFreeRegion)
					previousFreeRegion->m_next = r;
				else
					m_freeRegions = r;
			}

			return;
		}
	}

	CCP_ASSERT( !"Pointer Not Allocated With GNM" );
}



uint64_t s_onionMemAddr = 0;
uint64_t s_garlicMemAddr = 0;
uint32_t s_onionMemSize = 0;
uint32_t s_garlicMemSize = 0;

RegionAllocator s_onionAllocator;
RegionAllocator s_garlicAllocator;

static const uint32_t MAX_REGIONS = 16384;

Region s_onionRegions[MAX_REGIONS];
Region s_garlicRegions[MAX_REGIONS];



}

namespace Tr2MemoryAllocator
{



void Initialize()
{
	static bool initialized = false;
	if( initialized )
	{
		return;
	}
	initialized = true;

	memset( s_onionRegions, 0, sizeof( s_onionRegions ) );
	memset( s_garlicRegions, 0, sizeof( s_garlicRegions ) );

	MapMemory( &s_onionMemAddr, &s_onionMemSize, &s_garlicMemAddr, &s_garlicMemSize );

	s_onionAllocator.Init( s_onionMemAddr, s_onionMemSize, s_onionRegions, MAX_REGIONS );
    s_garlicAllocator.Init( s_garlicMemAddr, s_garlicMemSize, s_garlicRegions, MAX_REGIONS );
}

void* Allocate( MemoryType memoryType, size_t size, size_t align )
{
	if( memoryType == GARLIC )
	{
		return AllocateGarlic( size, align );
	}
	else
	{
		return AllocateOnion( size, align );
	}
}

void* AllocateOnion( size_t size, size_t align )
{
	return s_onionAllocator.Allocate( size, align );
}

void* AllocateGarlic( size_t size, size_t align )
{
	return s_garlicAllocator.Allocate( size, align );
}

void* Allocate( MemoryType memoryType, sce::Gnm::SizeAlign sizeAlign )
{
	if( memoryType == GARLIC )
	{
		return AllocateGarlic( sizeAlign );
	}
	else
	{
		return AllocateOnion( sizeAlign );
	}
}

void* AllocateOnion( sce::Gnm::SizeAlign sizeAlign )
{
	return AllocateOnion( sizeAlign.m_size, sizeAlign.m_align );
}

void* AllocateGarlic( sce::Gnm::SizeAlign sizeAlign )
{
	return AllocateGarlic( sizeAlign.m_size, sizeAlign.m_align );
}

void Release( void *ptr )
{
	if( !ptr )
	{
		return;
	}

	uint64_t address = reinterpret_cast<uint64_t>(ptr);
	if( address >= s_onionMemAddr && address < s_onionMemAddr + s_onionMemSize )
	{
		s_onionAllocator.Release( ptr );
	}
	else if( address >= s_garlicMemAddr && address < s_garlicMemAddr + s_garlicMemSize )
	{
		s_garlicAllocator.Release( ptr );
	}
	else
	{
		CCP_AL_LOGERR( "Tr2MemoryAllocator: Unknown memory range: 0x%016lX", address );
	}
}

size_t GetOnionMemorySize()
{
	return s_onionMemSize;
}

size_t GetGarlicMemorySize()
{
	return s_garlicMemSize;
}

uint32_t GetGarlicAvailableMemory()
{
	return s_garlicAllocator.GetAvailableMemory();
}

uint32_t GetOnionAvailableMemory()
{
	return s_onionAllocator.GetAvailableMemory();
}

};


#endif