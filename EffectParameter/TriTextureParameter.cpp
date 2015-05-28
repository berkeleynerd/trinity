#include "StdAfx.h"
#include "TriTextureParameter.h"
#include "Resources/TriTextureRes.h"
#include "ITr2ShaderState.h"
#include "Tr2Renderer.h"

TriTextureParameter::TriTextureParameter(IRoot* lockobj):
	m_isUsedByEffect( false ),
	m_resourceType( Tr2EffectResource::TEXTURE_TYPELESS )
{
}


TriTextureParameter::~TriTextureParameter()
{
}

const char* TriTextureParameter::GetParameterName() const
{
	return m_name.c_str();
}

void TriTextureParameter::SetParameterName( const BlueSharedString& name )
{
	m_name = name;
}

unsigned TriTextureParameter::GetHashValue( unsigned startingHash ) const
{
	if( m_resource )
	{
		startingHash = CcpHashFNV1( m_resource->GetPath(), wcslen( m_resource->GetPath() ) * sizeof( wchar_t ), startingHash );
	}
	auto name = m_name.c_str();
	return CcpHashFNV1( &name, sizeof( name ), startingHash );
}

// -------------------------------------------------------------
// Description:
//   Returns the respath to the currently used texture. Might
//   be LOD based.
// -------------------------------------------------------------
const wchar_t* TriTextureParameter::GetResourcePath() const
{
	const TriTextureRes* currentRes = GetResource();
	if( !currentRes )
	{
		return L"";
	}
	return currentRes->GetPath();
}

void TriTextureParameter::SetResourcePath( const char* resourcePath )
{
	m_resourcePath = resourcePath;
	OnModified( (Be::Var*)&m_resourcePath );
}


// --------------------------------------------------------------------------------------
// Description:
//   Determines whether the value of this texture parameter is NULL and be ignored when
//   building the material situation.
// Return Value:
//   true, if the texture is NULL
//   false, otherwise
// --------------------------------------------------------------------------------------
bool TriTextureParameter::IsZeroOrNull( void ) const
{
	return m_resource == NULL;
}

void TriTextureParameter::ReloadResources()
{
	if ( m_resource )
	{
		m_resource->ReloadResources();
	}
}

bool TriTextureParameter::OnModified(	Be::Var* val )
{
	UnloadResources();

	Initialize();

	RebuildEffectHandles( m_cachedEffect );

	return true;
}

// --------------------------------------------------------------------------------------
// Description:
//   Applies texture to the render context.
// Arguments:
//   inputType - shader type
//   destHandle - reinterpreted as a sampler index
//   resourceFlags - union of ITr2EffectValue::ResourceFlags
//   renderContext - current render context
// --------------------------------------------------------------------------------------
void TriTextureParameter::CopyValueToEffect(	Tr2RenderContextEnum::ShaderType inputType, 
												unsigned char* destHandle, 
												size_t resourceFlags,
												Tr2RenderContext &renderContext ) const
{
	unsigned int ix = *destHandle;
	bool isUav = ( resourceFlags & RESOURCE_FLAG_UAV ) != 0;
	TriTextureRes* resource = GetResource();
	if( Tr2TextureAL* tex = ( resource ? resource->GetTexture() : nullptr ) )
	{		
		if( isUav )
		{
			renderContext.SetUav( inputType, ix, *tex );
		}
		else
		{
			bool isSrgb = ( resourceFlags & RESOURCE_FLAG_SRGB ) != 0;
			auto colorSpace = isSrgb ? Tr2RenderContextEnum::COLOR_SPACE_SRGB : Tr2RenderContextEnum::COLOR_SPACE_LINEAR;
			renderContext.m_esm.ApplyTexture( inputType, ix, *tex, colorSpace );
		}
	}
	else
	{
		if( isUav )
		{
			// TODO: Fix the signature of SetUav to take a const reference
			renderContext.SetUav( inputType, ix, const_cast<Tr2TextureAL&>( nullTX ) );
		}
		else
		{
			Tr2Renderer::ApplyFallbackTexture( inputType, ix, m_resourceType, m_name.c_str(), renderContext );
		}
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Checks if the texture resource is prepared.
// Return value:
//   true If the resource path is empty or resource is good
//	 false Otherwise
// --------------------------------------------------------------------------------------
bool TriTextureParameter::IsPrepared() const
{
	if( *m_resourcePath.c_str() == 0 || ( m_resource && m_resource->IsPrepared() ) )
	{
		return true;
	}
	return false;
}

// ---------------------------------------------------------------
bool TriTextureParameter::Initialize()
{
	LoadResources();
	return true;
}

// --------------------------------------------------------------------------------------
// Description:
//   Let go of our current resource, and take ownership of newRes instead.
// --------------------------------------------------------------------------------------
void TriTextureParameter::SetResource( TriTextureRes* newRes )
{
	m_resource = newRes;
	RebuildEffectHandles( m_cachedEffect );
}

TriTextureRes* TriTextureParameter::GetResource() const
{
	return m_resource;
}

void* TriTextureParameter::GetResourcePointer() const
{
	return m_resource ? m_resource->GetTexture() : nullptr;
}

bool TriTextureParameter::LoadResources()
{
	if ( !m_resourcePath.empty() )
	{
		m_resource = nullptr;
		BeResMan->GetResource( m_resourcePath.c_str(), "", BlueInterfaceIID<TriTextureRes>(), (void**)&m_resource );
		if( !m_resource )
		{
			return true;
		}
		return m_resource->IsPrepared();
	}
	else
	{
		UnloadResources();
	}
	return true;
}

void TriTextureParameter::UnloadResources()
{
	m_resource.Unlock();
}

// --------------------------------------------------------------------------------------
//  Description:
//    Copies any resource that was dynamically assigned to the parameter to the new copy
// --------------------------------------------------------------------------------------
bool TriTextureParameter::AssignTo( ICopierCustomAssignment* other, 
											  ICopier* copier )
{
	if( m_resourcePath.empty() && m_resource )
	{
		// texture that was dynamically assigned
		TriTextureParameter* dest = (TriTextureParameter*)other;
		dest->SetResource( m_resource );
	}
	return true;
}

void TriTextureParameter::RebuildEffectHandles( ITr2ShaderState* effectRes )
{
	m_cachedEffect = effectRes;

	m_isUsedByEffect = false;
	if ( m_name.empty() || !effectRes )
	{
		return;
	}

	auto resource = effectRes->GetResource( m_name.c_str() );
	if( !resource )
	{
		return;
	}
	m_resourceType = resource->type;
	m_isUsedByEffect = true;
}
