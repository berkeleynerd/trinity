// Copyright © 2017 CCP ehf.

#include "StdAfx.h"
#include "Tr2RuntimeTextureParameter.h"
#include "ITr2TextureProvider.h"
#include "Shader/Tr2Shader.h"
#include "Shader/Tr2Material.h"
#include "Tr2Renderer.h"

// --------------------------------------------------------------------------------------
Tr2RuntimeTextureParameter::Tr2RuntimeTextureParameter( IRoot* lockobj ) :
	m_resourceType( Tr2EffectResource::TEXTURE_TYPELESS ),
	m_uavMipLevel( 0 )
{
}

bool Tr2RuntimeTextureParameter::OnModified( Be::Var* value )
{
	if( IsMatch( value, m_texture ) )
	{
		for( auto it = begin( m_materials ); it != end( m_materials ); ++it )
		{
			( *it )->InvalidateResourceSets();
		}
	}
	return true;
}

// --------------------------------------------------------------------------------------
bool Tr2RuntimeTextureParameter::CopyToResourceSet(
	Tr2ResourceSetDescriptionAL& resourceDesc,
	Tr2RenderContextEnum::ShaderType stage,
	uint32_t registerIndex,
	ResourceFlags flags ) const
{
	bool isSrgb = ( flags & RESOURCE_FLAG_SRGB ) != 0;
	auto colorSpace = isSrgb ? Tr2RenderContextEnum::COLOR_SPACE_SRGB : Tr2RenderContextEnum::COLOR_SPACE_LINEAR;
	if( Tr2TextureAL* tex = ( m_texture ? m_texture->GetTexture() : nullptr ) )
	{
		return resourceDesc.SetSrv( stage, registerIndex, *tex, colorSpace );
	}
	else
	{
		return resourceDesc.SetSrv( stage, registerIndex, Tr2Renderer::GetFallbackTexture( m_resourceType, m_name.c_str() ), colorSpace );
	}
}

// --------------------------------------------------------------------------------------
bool Tr2RuntimeTextureParameter::ApplyUav(
	Tr2ResourceSetDescriptionAL& resourceDesc,
	Tr2RenderContextEnum::ShaderType stage,
	uint32_t registerIndex ) const
{
	if( Tr2TextureAL* tex = ( m_texture ? m_texture->GetTexture() : nullptr ) )
	{
		return resourceDesc.SetUav( stage, registerIndex, *tex, m_uavMipLevel );
	}
	else
	{
		return resourceDesc.SetUav( stage, registerIndex, Tr2TextureAL() );
	}
}

void Tr2RuntimeTextureParameter::AddUsedTexture( Tr2BindlessResourcesAL& usedTextures ) const
{
	if( Tr2TextureAL* tex = ( m_texture ? m_texture->GetTexture() : nullptr ) )
	{
		usedTextures.Add( *tex );
	}
}


// --------------------------------------------------------------------------------------
const char* Tr2RuntimeTextureParameter::GetParameterName() const
{
	return m_name.c_str();
}

// --------------------------------------------------------------------------------------
void Tr2RuntimeTextureParameter::RebuildEffectHandles( Tr2Shader* effectRes )
{
	if( m_name.empty() || !effectRes )
	{
		return;
	}

	auto resource = effectRes->GetResource( m_name.c_str() );
	if( !resource )
	{
		return;
	}
	m_resourceType = resource->type;
}

// --------------------------------------------------------------------------------------
unsigned Tr2RuntimeTextureParameter::GetHashValue( unsigned startingHash ) const
{
	startingHash = CcpHashFNV1( m_texture.p, sizeof( m_texture.p ), startingHash );
	TrinityALImpl::Tr2TextureAL* textureObject = nullptr;
	if( Tr2TextureAL* texture = m_texture ? m_texture->GetTexture() : nullptr )
	{
		textureObject = texture->TrinityALImpl_GetObject();
	}
	startingHash = CcpHashFNV1(
		&textureObject, sizeof( textureObject ), startingHash );
	return CcpHashFNV1( &m_uavMipLevel, sizeof( m_uavMipLevel ), startingHash );
}

// --------------------------------------------------------------------------------------
void Tr2RuntimeTextureParameter::Create( const BlueSharedString& name, ITr2TextureProvider* texture, uint32_t uavMipLevel )
{
	m_name = name;
	m_texture = texture;
	m_uavMipLevel = uavMipLevel;
}

// --------------------------------------------------------------------------------------
void Tr2RuntimeTextureParameter::SetTextureProvider( ITr2TextureProvider* texture )
{
	if( m_texture == texture )
	{
		return;
	}
	m_texture = texture;
	for( auto it = begin( m_materials ); it != end( m_materials ); ++it )
	{
		( *it )->InvalidateResourceSets();
	}
}

ITr2TextureProviderPtr Tr2RuntimeTextureParameter::GetTextureProvider() const
{
	return m_texture;
}

void Tr2RuntimeTextureParameter::SetUavMipLevel( uint32_t mipLevel )
{
	if( m_uavMipLevel == mipLevel )
	{
		return;
	}
	m_uavMipLevel = mipLevel;
	for( Tr2Material* material : m_materials )
	{
		material->InvalidateResourceSets();
	}
}

void Tr2RuntimeTextureParameter::OnAddedToMaterial( Tr2Material* material )
{
	// A material registers at most once; Initialize may run more than once
	// for the same effect (read-time and preparation passes).
	if( find( begin( m_materials ), end( m_materials ), material ) == end( m_materials ) )
	{
		m_materials.push_back( material );
	}
}

void Tr2RuntimeTextureParameter::OnRemovedFromMaterial( Tr2Material* material )
{
	auto found = find( begin( m_materials ), end( m_materials ), material );
	if( found != end( m_materials ) )
	{
		m_materials.erase( found );
	}
}
