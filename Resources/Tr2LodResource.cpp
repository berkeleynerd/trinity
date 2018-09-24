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

bool Tr2LodResource::OnModified( Be::Var* value )
{
	if( IsMatch( value, m_resPath[m_currentLod] ) )
	{
		m_requested = nullptr;
		auto& respath = m_resPath[m_currentLod];
		if( respath.empty() )
		{
			m_requested = nullptr;
			m_active = nullptr;
		}
		else
		{
			BeResMan->GetResource( respath, "", m_requested );
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
	// This function can be called on multiple threads simultaneously,
	// we don't want to mutate ref counts in such scenario
	if( BeResMan->IsOnMainThread() && m_requested )
	{
		if( m_requested->IsGood() )
		{
			m_active = m_requested;
			m_requested.Unlock();
		}
	}
	
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
		m_requested = nullptr;
		m_active = nullptr;
	}
	else
	{
		BeResMan->GetResource( respath, "", m_requested );
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
