#include "StdAfx.h"

#include "TriVariable.h"

#include "include/TriMath.h"
#include "include/ITr2GpuBuffer.h"

#include "Resources/TriTextureRes.h"

#include "Tr2DepthStencil.h"
#include "Tr2RenderTarget.h"
#include "Tr2VariableStore.h"

#include "Shader/Tr2Material.h"
#include <Shader/Tr2Effect.h>

BLUE_DEFINE_NONEXPOSED( TriVariable );

const Be::ClassInfo* TriVariable::ExposeToBlue()
{
	EXPOSURE_BEGIN( TriVariable, "" )
		MAP_INTERFACE( IRoot )
	EXPOSURE_END()
}

//TriVariable::~TriVariable()
//{
//	//for( auto it = begin( m_materials ); it != end( m_materials ); ++it )
//	//{
//	//	if( !!( *it ) )
//	//	{
//	//		( *it )->OnVariableDestruction( this );
//	//	}
//	//	//else
//	//	//{
//	//	//	std::swap( *it, m_materials.back() );
//	//	//	m_materials.pop_back();
//	//	//}
//	//}
//}

bool TriVariable::CopyToResourceSet(
	Tr2ResourceSetDescriptionAL& resourceDesc,
	Tr2RenderContextEnum::ShaderType stage,
	uint32_t registerIndex,
	ResourceFlags flags ) const
{
	switch( m_type )
	{
	case TRIVARIABLE_TEXTURE_RES:
	{
		auto colorSpace = ( flags & RESOURCE_FLAG_SRGB ) != 0 ? Tr2RenderContextEnum::COLOR_SPACE_SRGB : Tr2RenderContextEnum::COLOR_SPACE_LINEAR;
		Tr2TextureAL* tex = nullptr;
		if( m_texture )
		{
			tex = m_texture->GetTexture();
		}
		if( tex )
		{
			return resourceDesc.SetSrv( stage, registerIndex, *tex, colorSpace );
		}
		else
		{
			return resourceDesc.SetSrv( stage, registerIndex, Tr2TextureAL(), colorSpace );
		}
	}
	case TRIVARIABLE_GPUBUFFER:
	{
		Tr2BufferAL* buffer = nullptr;
		if( m_gpuBuffer )
		{
			buffer = m_gpuBuffer->GetGpuBuffer( 0 );
		}
		if( buffer )
		{
			return resourceDesc.SetSrv( stage, registerIndex, *buffer );
		}
		else
		{
			return resourceDesc.SetSrv( stage, registerIndex, Tr2BufferAL() );
		}
	}
	default:
		return false;
	}
}

bool TriVariable::ApplyUav(
	Tr2ResourceSetDescriptionAL& resourceDesc,
	Tr2RenderContextEnum::ShaderType stage,
	uint32_t registerIndex ) const
{
	switch( m_type )
	{
	case TRIVARIABLE_TEXTURE_RES:
	{
		Tr2TextureAL* tex = nullptr;
		if( m_texture )
		{
			tex = m_texture->GetTexture();
		}
		if( tex )
		{
			return resourceDesc.SetUav( stage, registerIndex, *tex );
		}
		else
		{
			return resourceDesc.SetUav( stage, registerIndex, Tr2BufferAL() );
		}
		break;
	}
	case TRIVARIABLE_GPUBUFFER:
	{
		Tr2BufferAL* buffer = nullptr;
		if( m_gpuBuffer )
		{
			buffer = m_gpuBuffer->GetGpuBuffer( 0 );
		}
		if( buffer )
		{
			return resourceDesc.SetUav( stage, registerIndex, *buffer );
		}
		else
		{
			return resourceDesc.SetUav( stage, registerIndex, Tr2BufferAL() );
		}
		break;
	}
	default:
		break;
	}
	return resourceDesc.SetUav( stage, registerIndex, Tr2BufferAL() );
}

void TriVariable::CopyValueToEffect(	Tr2RenderContextEnum::ShaderType inputType, 
										unsigned char* destHandle, 
										size_t size,
										Tr2RenderContext &renderContext ) const
{
	switch( m_type )
	{
	case TRIVARIABLE_INVALID:
	case TRIVARIABLE_UNKNOWN_FLOAT:
	case TRIVARIABLE_TEXTURE_RES:
	case TRIVARIABLE_GPUBUFFER:
		// Do Nothing
		break;
	case TRIVARIABLE_FLOAT4X4:
		{
			size_t ts = GetTypeSize();
			// column_major for shaders, pay attention to size of registers
			TriMatrixTranspose( 
				reinterpret_cast<Matrix*>( destHandle ), 
				reinterpret_cast<const Matrix*>( m_value ), 
				(unsigned int)(size < ts ? size : ts) );
			break;
		}
	default:
		{
			size_t ts = GetTypeSize();
			memcpy( destHandle, m_value, size < ts ? size : ts );
		}
	}
}

void TriVariable::Invalidate()
{
	Clear();
	m_type = TRIVARIABLE_INVALID;
}

void TriVariable::Clear()
{
	if( m_texture )
	{
		m_texture->OnTextureChange().UnregisterListener( this );
	}
	//if( m_gpuBuffer )
	//{
	//	m_gpuBuffer->OnBufferChange().UnregisterListener( this );
	//}

	m_texture = nullptr;	
	m_gpuBuffer = nullptr;

	memset( m_value, 0, GetTypeSize() );

	OnTextureOrBufferChanged();
}

void TriVariable::OnAddedToMaterial( Tr2Effect* material )
{
	CCP_ASSERT_M( find( begin( m_materials ), end( m_materials ), material ) == end( m_materials ), "TriVariable already contains material!" );
	m_materials.push_back( material );
}

void TriVariable::OnRemovedFromMaterial( Tr2Effect* material )
{
	// after the device hung/lost, this also causes issues for some reason,
	// with this variable claiming it has no entry for the material, which makes no sense...
	
	// TODO: intern, remove this! it's only here to be able to set a breakpoint, since the assert for some reason is not showing the callstack...
	if ( find(begin(m_materials), end(m_materials), material) == end(m_materials) )
	{
		CCP_LOGERR( "WTF!!!! TriVariable::OnRemovedFromMaterial has no entry for the material!" );
	}

	CCP_ASSERT_M( find( begin( m_materials ), end( m_materials ), material ) != end( m_materials ), "TriVariable does not contain material!" );
	// TODO: intern, swap and pop
	m_materials.erase( find( begin( m_materials ), end( m_materials ), material ), end( m_materials ) );
}

void TriVariable::SetValueTextureRes( ITr2TextureProvider*& value )
{
	CCP_ASSERT( m_type == GetVariableType( value ) );

	if( m_texture != value )
	{
		if( m_texture )
		{
			m_texture->OnTextureChange().UnregisterListener( this );
		}
		m_texture = value;
		if( m_texture )
		{
			m_texture->OnTextureChange().RegisterListener<TriVariable, &TriVariable::OnTextureOrBufferChanged>( this );
		}
	}

	m_type = TRIVARIABLE_TEXTURE_RES;
}

void TriVariable::SetValueGpuBuffer( ITr2GpuBuffer*& value )
{
	CCP_ASSERT( m_type == GetVariableType( value ) );
	
	// TODO: intern, talk with Filipp! we probably need this as well! which means introducing OnBufferChange to GpuBuffer!
	/*
	if( m_gpuBuffer != value )
	{
		if( m_gpuBuffer )
		{
			m_gpuBuffer->OnBufferChange().UnregisterListener( this );
		}
		m_gpuBuffer = value;
		if( m_gpuBuffer )
		{
			m_gpuBuffer->OnBufferChange().RegisterListener<TriVariable, &TriVariable::OnTextureOrBufferChanged>( this );
		}
	}
	*/
	m_gpuBuffer = value;

	m_type = TRIVARIABLE_GPUBUFFER;
}


void TriVariable::OnTextureOrBufferChanged()
{
	for( size_t i = 0; i < m_materials.size(); )
	{
		if( !!m_materials[i] )
		{
			m_materials[i]->InvalidateResourceSets();
			++i;
		}
		else
		{
			m_materials[i] = m_materials[m_materials.size() - 1];
			m_materials.pop_back();
		}
	}

	//for( auto it = begin( m_materials ); it != end( m_materials ); )
	//{
	//	if( !!( *it ) )
	//	{
	//		( *it )->InvalidateResourceSets();
	//		++it;
	//	}
	//	else
	//	{
	//		std::swap( *it, m_materials.back() );
	//		m_materials.pop_back();
	//	}
	//}
}