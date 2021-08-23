////////////////////////////////////////////////////////////////////////////////
//
// Created:		August 2014
// Copyright:	CCP 2014
//

#include "StdAfx.h"
#include "Tr2LodResource.h"
#include "TriSettingsRegistrar.h"

bool g_lodLevelUltraEnabled = false;
TRI_REGISTER_SETTING( "lodLevelUltraEnabled", g_lodLevelUltraEnabled );

Tr2LodResource::Tr2LodResource( IRoot* lockobj ) :
	m_currentLod( TR2_LOD_UNSPECIFIED )
{
}

Tr2LodResource::~Tr2LodResource()
{
	SetRequested( nullptr );
	SetActive( nullptr );
}

bool Tr2LodResource::OnModified( Be::Var* value )
{
	if( IsMatch( value, m_resPath[m_currentLod] ) )
	{
		SetRequested( nullptr );

		auto& respath = m_resPath[m_currentLod];
		if( respath.empty() )
		{
			SetActive( nullptr );
		}
		else
		{
			IBlueResourcePtr requested;
			BeResMan->GetResource( respath, "", requested );
			SetRequested( requested );
		}
	}
	return true;
}

// --------------------------------------------------------------------------------
// Description:
//   Set the individual lod's resource path.
// Arguments:
//   lod - which lod level?
//   resPath - the new respath for that lod
// --------------------------------------------------------------------------------
void Tr2LodResource::SetResourcePath( Tr2Lod lod, const char* resPath )
{
	m_resPath[ lod ] = resPath;
}

IBlueResource* Tr2LodResource::GetResource()
{
	if( m_active )
	{
		return m_active;
	}

	if( m_requested )
	{
		return m_requested;
	}

	return nullptr;
}

void Tr2LodResource::SelectLod( Tr2Lod lod )
{
	if( lod == TR2_LOD_ULTRA && !g_lodLevelUltraEnabled )
	{
		lod = TR2_LOD_HIGH;
	}

	if( lod == m_currentLod )
	{
		return;
	}
	
	m_currentLod = lod;
	auto& respath = m_resPath[ lod ];
	if( respath.empty() )
	{
		SetRequested( nullptr );
		SetActive( nullptr );
	}
	else
	{
		IBlueResourcePtr requested;
		BeResMan->GetResource( respath, "", requested );
		SetRequested( requested );
	}
}

const char* Tr2LodResource::GetName() const
{
	return m_name.c_str();
}

void Tr2LodResource::SetName( const BlueSharedString& val )
{
	m_name = val;
}

void Tr2LodResource::AddNotifyTarget( ITr2LodResourceListener* p )
{
	m_notifies.push_back( p );
}

void Tr2LodResource::RemoveNotifyTarget( ITr2LodResourceListener* p )
{
	auto found = std::find( begin( m_notifies ), end( m_notifies ), p );
	if( found != end( m_notifies ) )
	{
		m_notifies.erase( found );
	}
}

void Tr2LodResource::SetRequested( IBlueResource* requested )
{
	if( m_requested == requested )
	{
		return;
	}
	if( m_requested && m_requested != m_active )
	{
		m_requested->RemoveNotifyTarget( this );
	}
	m_requested = dynamic_cast<BlueAsyncRes*>( requested );
	if( m_requested && m_requested != m_active )
	{
		m_requested->AddNotifyTarget( this );
	}
}

void Tr2LodResource::SetActive( BlueAsyncRes* active )
{
	if( m_active == active )
	{
		return;
	}
	if( m_active && m_requested != m_active )
	{
		m_active->RemoveNotifyTarget( this );
	}
	m_active = active;
	if( m_active )
	{
		if( m_requested != m_active )
		{
			m_active->AddNotifyTarget( this );
		}
		else
		{
			NotifyListeners();
		}
	}
	if( !m_active )
	{
		NotifyListeners();
	}
}

void Tr2LodResource::NotifyListeners()
{
	for( auto it = begin( m_notifies ); it != end( m_notifies ); ++it )
	{
		( *it )->OnLodResourceChanged( this );
	}
}

void Tr2LodResource::ReleaseCachedData( BlueAsyncRes* p )
{
	if( p == m_active )
	{
		NotifyListeners();
	}
	else if( p == m_requested && !m_active )
	{
		NotifyListeners();
	}
}

void Tr2LodResource::RebuildCachedData( BlueAsyncRes* p )
{
	if( p == m_active )
	{
		NotifyListeners();
	}
	if( p == m_requested )
	{
		if( p->IsGood() )
		{
			SetActive( p );
			SetRequested( nullptr );
		}
	}
}

bool Tr2LodResource::IsUsingSelectedLod() const
{
	return m_requested == nullptr;
}
