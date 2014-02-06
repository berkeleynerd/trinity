#include "StdAfx.h"
#include "Tr2AutoResetObjectAL.h"

namespace
{

// --------------------------------------------------------------------------------------
// Description:
//   Returns a mutex shared by all Tr2AutoResetObjectAL instances and used to access
//   GetAllObjects set.
// Return value:
//   Mutex to be used to access GetAllObjects set
// --------------------------------------------------------------------------------------
CcpMutex& GetAutoResetObjectMutex()
{
	static CcpMutex s_mutex( "GetAutoResetObjectMutex", "s_mutex" );
	return s_mutex;
}

TrackableStdHashSet<Tr2AutoResetObjectAL*>& GetAllObjects()
{
	static TrackableStdHashSet<Tr2AutoResetObjectAL*> objects( "Tr2AutoResetObjectAL::objects" );
	return objects;
}

}

Tr2AutoResetObjectAL::Tr2AutoResetObjectAL()
{
	CcpAutoMutex autoMutex( GetAutoResetObjectMutex() );
	GetAllObjects().insert( this );
}

Tr2AutoResetObjectAL::~Tr2AutoResetObjectAL()
{
	CcpAutoMutex autoMutex( GetAutoResetObjectMutex() );
	GetAllObjects().erase( this );
}

Tr2AutoResetObjectAL::Tr2AutoResetObjectAL( const Tr2AutoResetObjectAL& )
{
	CcpAutoMutex autoMutex( GetAutoResetObjectMutex() );
	GetAllObjects().insert( this );
}

void Tr2AutoResetObjectAL::ReleaseALResources()
{
	CcpAutoMutex autoMutex( GetAutoResetObjectMutex() );
	auto& all = GetAllObjects();
	for( auto it = all.begin(); it != all.end(); ++it )
	{
		( *it )->ReleaseALResource();
	}
}

void Tr2AutoResetObjectAL::PrepareALResources( Tr2PrimaryRenderContextAL& renderContext )
{
	CcpAutoMutex autoMutex( GetAutoResetObjectMutex() );
	auto& all = GetAllObjects();
	for( auto it = all.begin(); it != all.end(); ++it )
	{
		( *it )->PrepareALResource( renderContext );
	}
}
