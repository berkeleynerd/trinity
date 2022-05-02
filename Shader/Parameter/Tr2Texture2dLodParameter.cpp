////////////////////////////////////////////////////////////////////////////////
//
// Created:		August 2014
// Copyright:	CCP 2014
//

#include "StdAfx.h"
#include "Tr2Texture2dLodParameter.h"
#include "ITr2TextureProvider.h"
#include "Resources/Tr2LodResource.h"
#include "Shader/Tr2Material.h"


Tr2Texture2dLodParameter::Tr2Texture2dLodParameter( IRoot* lockobj /*= nullptr */ )
{

}

Tr2Texture2dLodParameter::~Tr2Texture2dLodParameter()
{
	if( m_lodResource )
	{
		m_lodResource->RemoveNotifyTarget( this );
	}
}

bool Tr2Texture2dLodParameter::Initialize()
{
	if( m_lodResource )
	{
		m_resourcePath = m_lodResource->GetResourcePath( TR2_LOD_HIGH );
	}
	return TriTextureParameter::Initialize();
}

Tr2LodResourcePtr Tr2Texture2dLodParameter::GetLodResource() const
{
	return m_lodResource;
}

void Tr2Texture2dLodParameter::SetLodResource( Tr2LodResource* newLodResource )
{
	if( m_lodResource )
	{
		m_lodResource->RemoveNotifyTarget( this );
	}
	m_lodResource = newLodResource;
	if( m_lodResource )
	{
		m_lodResource->AddNotifyTarget( this );
	}
}

void Tr2Texture2dLodParameter::OnAddedToMaterial( Tr2Material* material )
{
	m_materials.push_back( material );
}

void Tr2Texture2dLodParameter::OnRemovedFromMaterial( Tr2Material* material )
{
	auto found = find( begin( m_materials ), end( m_materials ), material );
	if( found != end( m_materials ) )
	{
		m_materials.erase( found );
	}
}

void Tr2Texture2dLodParameter::OnLodResourceChanged( Tr2LodResource* resource )
{
}