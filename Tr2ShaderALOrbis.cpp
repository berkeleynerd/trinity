#include "StdAfx.h"

#if( TRINITY_PLATFORM==TRINITY_ORBIS )

#include "Tr2ShaderALOrbis.h"
#include "Tr2MemoryAllocator.h"
#include "ALLog.h"

#include <gnmx/shader_parser.h>
#include <gnmx/fetchshaderhelper.h>

using namespace Tr2RenderContextEnum;

namespace
{

bool GetNaitiveShaderType( Tr2RenderContextEnum::ShaderType type, sce::Gnmx::ShaderType& naitiveType )
{
	switch( type )
	{
	case VERTEX_SHADER:
		naitiveType = sce::Gnmx::kVertexShader;
		break;
	case PIXEL_SHADER:
		naitiveType = sce::Gnmx::kPixelShader;
		break;
	case COMPUTE_SHADER:
		naitiveType = sce::Gnmx::kComputeShader;
		break;
	default:
		return false;
	}
	return true;
}

size_t GetHeaderSize( const sce::Gnmx::ShaderInfo& shaderInfo, sce::Gnmx::ShaderType type )
{
	switch( type )
	{
	case sce::Gnmx::kVertexShader:
		return shaderInfo.m_vsShader->computeSize();
	case sce::Gnmx::kPixelShader:
		return shaderInfo.m_psShader->computeSize();
	case sce::Gnmx::kComputeShader:
		return shaderInfo.m_csShader->computeSize();
	default:
		return 0;
	}
}

}

Tr2ShaderAL::Tr2ShaderAL()
	:m_type( INVALID_SHADER ),
	m_frameUsed( 0 )
{
	m_shader.m_bytecode = nullptr;
	m_shader.m_csShader = nullptr;
	m_pachedShader.m_bytecode = nullptr;
	m_pachedShader.m_csShader = nullptr;
}

Tr2ShaderAL::~Tr2ShaderAL()
{
	Destroy();
}

void Tr2ShaderAL::Destroy()
{
	if( IsValid() )
	{
		Tr2RenderContextAL::InternalDelayDelete( m_frameUsed, m_shader.m_csShader );
		Tr2RenderContextAL::InternalDelayDelete( m_frameUsed, m_shader.m_bytecode );
		Tr2RenderContextAL::InternalDelayDelete( m_frameUsed, m_pachedShader.m_csShader );
		Tr2RenderContextAL::InternalDelayDelete( m_frameUsed, m_pachedShader.m_bytecode );

		m_shader.m_csShader = nullptr;
		m_shader.m_bytecode = nullptr;
		m_pachedShader.m_csShader = nullptr;
		m_pachedShader.m_bytecode = nullptr;
		m_type = INVALID_SHADER;
		m_bytecode.clear();
	}
}

Tr2ShaderAL::Shader Tr2ShaderAL::CreateShader( sce::Gnmx::ShaderType type, const void* bytecode )
{
	Shader result;
	result.m_bytecode = nullptr;
	result.m_csShader = nullptr;

	sce::Gnmx::ShaderInfo shaderInfo;
	sce::Gnmx::parseShader( &shaderInfo, bytecode, type );

	size_t headerSize = GetHeaderSize( shaderInfo, type );
	if( headerSize == 0 )
	{
		CCP_ASSERT( false );
		return result;
	}
	void *shaderHeader = Tr2MemoryAllocator::AllocateOnion( headerSize, sce::Gnm::kAlignmentOfBufferInBytes );
	void *shaderBinary = Tr2MemoryAllocator::AllocateGarlic( shaderInfo.m_gpuShaderCodeSize, sce::Gnm::kAlignmentOfShaderInBytes );

	if( !shaderBinary || !shaderHeader )
	{
		CCP_ASSERT( false );
		Tr2MemoryAllocator::Release( shaderBinary );
		Tr2MemoryAllocator::Release( shaderHeader );
		return result;
	}

	memcpy( shaderBinary, shaderInfo.m_gpuShaderCode, shaderInfo.m_gpuShaderCodeSize );
	memcpy( shaderHeader, shaderInfo.m_vsShader, headerSize );

	switch( type )
	{
	case sce::Gnmx::kVertexShader:
		result.m_vsShader = static_cast<sce::Gnmx::VsShader*>( shaderHeader );
		result.m_vsShader->patchShaderGpuAddress( shaderBinary );
		break;
	case sce::Gnmx::kPixelShader:
		result.m_psShader = static_cast<sce::Gnmx::PsShader*>( shaderHeader );
		result.m_psShader->patchShaderGpuAddress( shaderBinary );
		break;
	case sce::Gnmx::kComputeShader:
		result.m_csShader = static_cast<sce::Gnmx::CsShader*>( shaderHeader );
		result.m_csShader->patchShaderGpuAddress( shaderBinary );
		break;
	default:
		CCP_ASSERT( false );
		Tr2MemoryAllocator::Release( shaderBinary );
		Tr2MemoryAllocator::Release( shaderHeader );
		return result;
	}
	result.m_bytecode = shaderBinary;

	return result;
}

ALResult Tr2ShaderAL::Create( 
	Tr2RenderContextAL& renderContext, 
	Tr2RenderContextEnum::ShaderType type, 
	const void* bytecode, 
	uint32_t bytecodeSize, 
	const void* patchedBytecode, 
	uint32_t patchedBytecodeSize, 
	const Tr2ShaderInputDefinition& inputDefinition )
{
	CCP_STATS_ZONE( __FUNCTION__ );
	
	Destroy();

	AL_FUZZ( OT_SHADER );

	if( !renderContext.IsValid() )
	{
		return E_INVALIDCALL;
	}

	sce::Gnmx::ShaderType naitiveType;
	if( !GetNaitiveShaderType( type, naitiveType ) )
	{
		CCP_AL_LOGERR( "Tr2ShaderAL::Create: unsupported shader type %i", int( type ) );
		return E_INVALIDARG;
	}

	m_shader = CreateShader( naitiveType, bytecode );
	if( !m_shader.m_bytecode )
	{
		return E_FAIL;
	}
	if( patchedBytecodeSize )
	{
		m_pachedShader = CreateShader( naitiveType, patchedBytecode );
		if( !m_pachedShader.m_bytecode )
		{
			Destroy();
			return E_FAIL;
		}
	}

	m_type = type;
	m_bytecode.resize( "Tr2ShaderAL::m_byteCode", bytecodeSize );
	if( !m_bytecode.empty() )
	{
		memcpy( m_bytecode.get(), bytecode, bytecodeSize );
	}
	m_inputDefinition = inputDefinition;
	m_frameUsed = renderContext.InternalGetCurentFrameIndex() + renderContext.InternalGetMaxFrameLatency() + 1;

	return S_OK;
}

bool Tr2ShaderAL::IsValid() const
{
	return m_type != INVALID_SHADER;
}

bool Tr2ShaderAL::operator==( const Tr2ShaderAL& shader ) const
{
	return &shader == this;
}

Tr2RenderContextEnum::ShaderType Tr2ShaderAL::GetType() const
{
	return m_type;
}

ALResult Tr2ShaderAL::GetBytecode( const void*& bytecode, uint32_t& size ) const
{
	if( !IsValid() )
	{
		return E_INVALIDCALL;
	}
	bytecode = m_bytecode.get();
	size = m_bytecode.size();
	return S_OK;
}

const Tr2ShaderInputDefinition& Tr2ShaderAL::GetInputDefinition() const
{
	return m_inputDefinition;
}

void Tr2ShaderAL::SetNullShaderType( Tr2RenderContextEnum::ShaderType type )
{
	Destroy();
	m_type = type;
}

#endif