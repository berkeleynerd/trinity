////////////////////////////////////////////////////////////
//
//    Created:   June 2017
//    Copyright: CCP 2017
//

#include "StdAfx.h"
#include "Tr2Material.h"
#include "ITriReroutable.h"
#include "Tr2Shader.h"
#include "ITr2EffectValue.h"
#include "Tr2VariableStore.h"

CCP_STATS_DECLARE( effectCBLocks, "Trinity/effectCBLocks", true, CST_COUNTER_LOW, "number of CB locks for effect parameters" );
CCP_STATS_DECLARE( effectResourceSetCreated, "Trinity/effectResourceSetCreated", true, CST_COUNTER_LOW, "number of resource sets created" );

namespace
{

CcpAtomic<uint32_t> s_materialId( 0 );

}

Tr2EffectPassParameters::Tr2EffectPassParameters()
	:m_resourceSetDirty( true )
{
}

Tr2EffectPassParameters::~Tr2EffectPassParameters()
{
	for( auto it = m_reroutedParameters.begin(); it != m_reroutedParameters.end(); ++it )
	{
		( *it )->SetDestination( NULL, 0 );
		( *it )->Unlock();
	}
}

void Tr2EffectPassParameters::AllocateConstantMirror( Tr2RenderContextEnum::ShaderType type, unsigned int size )
{
	if( size )
	{
		// fix the size to be multiple of Vector4s
		if( size % sizeof( Vector4 ) )
		{
			size += sizeof( Vector4 ) - size % sizeof( Vector4 );
		}

		USE_MAIN_THREAD_RENDER_CONTEXT();
		if( !m_stageInput[type].m_constantBuffer )
		{
			m_stageInput[type].m_constantBuffer.reset( CCP_NEW( "Tr2EffectPassParameters::m_stageInput::m_constantBuffer" ) Tr2ConstantBufferAL );
		}
		m_stageInput[type].m_constantBuffer->Create(
			size,
			Tr2RenderContextEnum::USAGE_CPU_WRITE | Tr2RenderContextEnum::USAGE_LOCK_FREQUENTLY,
			nullptr,
			renderContext );
	}
}

Tr2Material::Tr2Material( IRoot* lockobj )
{
	m_sortValue = s_materialId++;
}

Tr2Material::~Tr2Material()
{
}

void Tr2Material::ApplyMaterialDataForPass( uint32_t techniqueIndex, unsigned int passIndex, Tr2RenderContext& renderContext ) const
{
	if( !m_shader )
	{
		return;
	}
	unsigned mask = m_shader->GetShaderTypeMask( techniqueIndex );
	auto& pp = *m_parametersForPasses[techniqueIndex][passIndex];
	bool descChanged = pp.m_resourceSetDirty;
	for( unsigned i = 0; i != Tr2RenderContextEnum::SHADER_TYPE_COUNT && mask; ++i )
	{
		if( mask & ( 1 << i ) )
		{
			descChanged |= ApplyShaderInputs( techniqueIndex, passIndex, Tr2RenderContextEnum::ShaderType( i ), renderContext );
			mask &= ~( 1 << i );
		}
	}

	if( descChanged || !pp.m_resourceSet.IsValid() )
	{
		USE_MAIN_THREAD_RENDER_CONTEXT();

		CCP_STATS_INC( effectResourceSetCreated );

		auto sp = renderContext.m_esm.GetShaderProgram( m_shader->GetEffect().techniques[techniqueIndex].passes[passIndex].shaderProgram );
		if( !sp )
		{
			return;
		}
		pp.m_resourceSet.Create( pp.m_resourceSetDesc, *sp, renderContext );
		pp.m_resourceSetDirty = false;
	}

	renderContext.SetResourceSet( pp.m_resourceSet );
}

bool Tr2Material::ApplyShaderInputs( uint32_t techniqueIndex, unsigned int passIndex, Tr2RenderContextEnum::ShaderType shaderType, Tr2RenderContext& renderContext ) const
{
	auto& pp = *m_parametersForPasses[techniqueIndex][passIndex];
	auto& input = pp.m_stageInput[shaderType];

	auto cb = input.m_constantBuffer.get();
	if( cb && cb->GetSize() )
	{
		CCP_STATS_INC( effectCBLocks );
		uint8_t* const mirror = reinterpret_cast<uint8_t*>( cb->GetBufferMirror( renderContext ) );
		if( cb->IsValid() && mirror )
		{
			const auto endVS = input.m_shaderParameters.cend();
			for( auto it = input.m_shaderParameters.cbegin(); it != endVS; ++it )
			{
				size_t size = it->m_registerCount;
				uint8_t* const dst = mirror + it->m_registerIndex;
				it->m_sourceValue->CopyValueToEffect( shaderType, dst, size, renderContext );
			}
			cb->UpdateFromMirror( renderContext );

			renderContext.SetConstants( *cb, shaderType, Tr2RenderContextEnum::CONSTANT_BUFFER_FOR_EFFECT_PARAMETERS );
		}
	}


	bool descChanged = false;

	for( auto it = input.m_textures.cbegin(); it != input.m_textures.cend(); ++it )
	{
		descChanged |= it->m_sourceValue->CopyToResourceSet( pp.m_resourceSetDesc, shaderType, it->m_registerIndex, ITr2EffectValue::ResourceFlags( it->m_registerCount ) );
	}

	for( auto it = input.m_uavs.cbegin(); it != input.m_uavs.cend(); ++it )
	{
		it->m_sourceValue->ApplyUav( pp.m_resourceSetDesc, shaderType, it->m_registerIndex, it->m_initialCount );
	}

	return descChanged;
}
unsigned int Tr2Material::GetSortValue() const
{
	if( m_shader )
	{
		return m_shader->GetSortValue();
	}
	else
	{
		return m_sortValue;
	}
}

Tr2Shader* Tr2Material::GetShaderStateInterface() const
{
	return m_shader;
}

Tr2EffectPassParameters* Tr2Material::GetPassDescription( uint32_t techniqueIndex, uint32_t passIndex )
{
	return m_parametersForPasses[techniqueIndex][passIndex].get();
}

void Tr2Material::InvalidateResourceSets()
{
	for( auto tit = begin( m_parametersForPasses ); tit != end( m_parametersForPasses ); ++tit )
	{
		for( auto pit = begin( *tit ); pit != end( *tit ); ++pit )
		{
			auto params = pit->get();
			params->m_resourceSet = Tr2ResourceSetAL();
			params->m_resourceSetDesc.ClearResources();
			params->m_resourceSetDirty = true;
		}
	}
}