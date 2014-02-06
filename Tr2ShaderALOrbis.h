#pragma once
#ifndef Tr2ShaderALOrbis_H
#define Tr2ShaderALOrbis_H

#if( TRINITY_PLATFORM==TRINITY_ORBIS )

#include "Tr2VertexDefinition.h"
#include <gnmx/shaderbinary.h>

class Tr2RenderContextAL;


// -------------------------------------------------------------
// Description:
//   A low level wrapper around shaders / shader programs. 
//   32bit - no support for shader blobs > 4 gig
// -------------------------------------------------------------
class Tr2ShaderAL : 
	public Tr2TrackedALObject<Tr2RenderContextEnum::OT_SHADER>
{
public:
	Tr2ShaderAL();
	~Tr2ShaderAL();

	ALResult Create( 
		Tr2RenderContextAL& renderContext, 
		Tr2RenderContextEnum::ShaderType type, 
		const void* bytecode, 
		uint32_t bytecodeSize, 
		const void* patchedBytecode, 
		uint32_t patchedBytecodeSize, 
		const Tr2ShaderInputDefinition& inputDefinition );

	void Destroy();

	bool IsValid() const;
	bool operator==( const Tr2ShaderAL& shader ) const;
	Tr2RenderContextEnum::ShaderType GetType() const;
	ALResult Apply( Tr2RenderContextAL& renderContext ) const;
	ALResult ApplyPatchedShader( Tr2RenderContextAL& renderContext ) const;
	ALResult GetBytecode( const void*& bytecode, uint32_t& size ) const;
	const Tr2ShaderInputDefinition& GetInputDefinition() const;

	Tr2ALMemoryType GetMemoryClass() const { return AL_MEMORY_MANAGED; }

	void SetNullShaderType( Tr2RenderContextEnum::ShaderType type );
private:
	struct Shader
	{
		union
		{
			sce::Gnmx::VsShader* m_vsShader;
			sce::Gnmx::PsShader* m_psShader;
			sce::Gnmx::CsShader* m_csShader;
		};
		void* m_bytecode;
	};

	Tr2ShaderAL( const Tr2ShaderAL& shader );
	Tr2ShaderAL& operator=( const Tr2ShaderAL& shader );

	Shader CreateShader( sce::Gnmx::ShaderType type, const void* bytecode );

	Tr2RenderContextEnum::ShaderType m_type;
	CcpMallocBuffer	m_bytecode;
	Tr2ShaderInputDefinition m_inputDefinition;

	Shader m_shader;
	Shader m_pachedShader;
	mutable uint32_t m_frameUsed;

	friend class Tr2RenderContextAL;
};

#endif

#endif