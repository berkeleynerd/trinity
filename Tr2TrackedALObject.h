////////////////////////////////////////////////////////////
//
//    Created:   May 2012
//    Copyright: CCP 2012
//

#pragma once
#ifndef Tr2TrackedALObject_H
#define Tr2TrackedALObject_H

// Define TRACK_AL_RESOURCES as 0 to disable tracking
#ifndef TRACK_AL_RESOURCES
#define TRACK_AL_RESOURCES 1
#endif

enum Tr2ALMemoryType
{
	AL_MEMORY_VIDEO = 1 << 0,  // resources created on video card memory
	AL_MEMORY_MANAGED = 1 << 1, // resources created in device memory
};

typedef int Tr2ALMemoryTypes;

#if AL_TACK_RESOURCE_USAGE && TRACK_AL_RESOURCES
#define AL_UPDATE_RESOURCE_FRAME_USAGE( resource )	\
	{ if( ( resource ).IsValid() ) { extern uint64_t g_trackCurrentFrame; ( resource ).m_trackFrameUsed = g_trackCurrentFrame; } };
#else
#define AL_UPDATE_RESOURCE_FRAME_USAGE( resource ) ((void)(resource));
#endif

#if( TRACK_AL_RESOURCES == 0 )

class Tr2TrackedALObjectBase
{
public:
	static void LogAllLiveResources( Tr2ALMemoryTypes flags = AL_MEMORY_VIDEO | AL_MEMORY_MANAGED ) 
	{
	}
	
	bool operator==( const Tr2TrackedALObjectBase& other ) const
	{
		return this == &other;
	}

	template<typename Operation> static void GetAllObjectDescriptions( Tr2ALMemoryTypes flags, Operation& operation )
	{
	}
};

template<Tr2RenderContextEnum::ObjectType Type> 
class Tr2TrackedALObject: public Tr2TrackedALObjectBase
{
public:
	template<typename Operation> static void EnumerateResources( Operation& operation )
	{
	}
};
#else

// --------------------------------------------------------------------------------------
// Description:
//   Base class of AL tracked objects. Maintains lists of live AL objects per object
//   type. Contains static functions to log all live AL objects and to return that
//   information as Python structure.
//   Individual AL classes descend from Tr2TrackedALObjectBase through class-specific
//   Tr2TrackedALObject proxies.
// See Also:
//   Tr2TrackedALObject
// --------------------------------------------------------------------------------------
class Tr2TrackedALObjectBase
{
public:
	static void LogAllLiveResources( Tr2ALMemoryTypes flags = AL_MEMORY_VIDEO | AL_MEMORY_MANAGED );

	template<typename Operation> static void GetAllObjectDescriptions( Tr2ALMemoryTypes flags, Operation& operation );

	bool operator==( const Tr2TrackedALObjectBase& ) const;
#if AL_TACK_RESOURCE_USAGE
	Tr2TrackedALObjectBase(): m_trackFrameUsed( 0 ) {}
	mutable uint64_t m_trackFrameUsed;
#endif
protected:
	static CcpMutex& GetLiveObjectMutex();
	static std::set<Tr2TrackedALObjectBase*>& GetLiveObjects( Tr2RenderContextEnum::ObjectType type );

	template<Tr2RenderContextEnum::ObjectType Type> struct ObjectInfo;

	template<Tr2RenderContextEnum::ObjectType Type, typename Operation> class GetAllObjectDescriptionsHelper;
};

// --------------------------------------------------------------------------------------
// Description:
//   Template object used for tracking live AL objects. Registers/unregisters itself
//   on construction/destruction.
// See Also:
//   Tr2TrackedALObjectBase
// --------------------------------------------------------------------------------------
template<Tr2RenderContextEnum::ObjectType Type> 
class Tr2TrackedALObject: public Tr2TrackedALObjectBase
{
public:
	template<typename Operation> static void EnumerateResources( Operation& operation );
protected:
	Tr2TrackedALObject();
	Tr2TrackedALObject( const Tr2TrackedALObject<Type>& other );
	Tr2TrackedALObject( Tr2TrackedALObject<Type>&& other );
	Tr2TrackedALObject& operator=( const Tr2TrackedALObject& other );
	~Tr2TrackedALObject();
private:
	template<typename Operation> static void EnumerateResources( Operation& operation, std::true_type isCopiable );
	template<typename Operation> static void EnumerateResources( Operation& operation, std::false_type isCopiable );
};

#endif

#endif // Tr2TrackedALObject_H