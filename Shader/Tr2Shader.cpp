#include "StdAfx.h"
#include "Tr2Shader.h"


using namespace Tr2RenderContextEnum;

// --------------------------------------------------------------------------------------
// Description:
//   Constructor.  Initializes members to default values.
// --------------------------------------------------------------------------------------
Tr2Shader::Tr2Shader( IRoot* lockobj )
	:m_shaderTypeMask( 0 ),
	m_sortValue( 0 )
{	
}

// --------------------------------------------------------------------------------------
// Description:
//   Frees the dynamically-allocated compiler defines array.
// --------------------------------------------------------------------------------------
Tr2Shader::~Tr2Shader()
{
}

// --------------------------------------------------------------------------------------
// Description:
//   Gets the number of passes in the shader.
// Return Value:
//   The number of passes supported by the shader
// --------------------------------------------------------------------------------------
unsigned int Tr2Shader::GetPassCount( void ) const
{
	return static_cast<unsigned int>( m_effect.passes.size() );
}

// --------------------------------------------------------------------------------------
// Description:
//   Searches for a constant/uniform by its name.
// Arguments:
//   name - Constant name.
// Return value:
//   Pointer to constant (temporary) or nullptr if the constant is not found
// --------------------------------------------------------------------------------------
const Tr2EffectConstant* Tr2Shader::GetConstant( const char* name ) const
{
	for( auto pass = m_effect.passes.begin(); pass != m_effect.passes.end(); ++pass )
	{
		for( unsigned i = 0; i < Tr2RenderContextEnum::SHADER_TYPE_COUNT; ++i )
		{
			for( auto constant = pass->stageInputs[i].constants.begin(); constant != pass->stageInputs[i].constants.end(); ++constant )
			{
				if( strcmp( constant->name.c_str(), name ) == 0 )
				{
					return &*constant;
				}
			}
		}
	}
	return nullptr;
}

// --------------------------------------------------------------------------------------
// Description:
//   Gets the sort value for the compiled shader.  The sort value is created when the
//   shader is compiled and analyzed.
// Return Value:
//   The shader sort value
// See Also:
//   Process Effect
// --------------------------------------------------------------------------------------
unsigned int Tr2Shader::GetSortValue( void ) const
{
	return m_sortValue;
}

// --------------------------------------------------------------------------------------
// Description:
//   Searches for a sampler by its texture name.
// Arguments:
//   name - Texture name.
// Return value:
//   Pointer to sampler (temporary) or nullptr if the sampler is not found
// --------------------------------------------------------------------------------------
const Tr2EffectResource* Tr2Shader::GetResource( const char* name ) const
{
	for( auto pass = m_effect.passes.begin(); pass != m_effect.passes.end(); ++pass )
	{
		for( unsigned i = 0; i < Tr2RenderContextEnum::SHADER_TYPE_COUNT; ++i )
		{
			for( auto constant = pass->stageInputs[i].resources.begin(); constant != pass->stageInputs[i].resources.end(); ++constant )
			{
				if( strcmp( constant->second.name, name ) == 0 )
				{
					return &constant->second;
				}
			}
			for( auto constant = pass->stageInputs[i].uavs.begin(); constant != pass->stageInputs[i].uavs.end(); ++constant )
			{
				if( strcmp( constant->second.name, name ) == 0 )
				{
					return &constant->second;
				}
			}
		}
	}
	return nullptr;
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns map of effect annotations for a given parameter or nullptr if the parameter
//   is not found or it doesn't contain any annotations.  
// Arguments:
//   parameterName - Name of the effect parameter
// Return Value:
//   Map of effect annotations or nullptr
// --------------------------------------------------------------------------------------
const Tr2EffectParameterAnnotationMap* Tr2Shader::GetParameterAnnotations( const char* parameterName ) const
{
	auto it = std::find_if( m_effect.annotations.begin(), m_effect.annotations.end(), [&]( Tr2EffectAnnotationMap::const_reference key ) { return strcmp( key.first, parameterName ) == 0; } );
	return it == m_effect.annotations.end() ? nullptr : &it->second;
}


const Tr2EffectDescription& Tr2Shader::GetEffectDescription() const
{
	return m_effect;
}

// --------------------------------------------------------------------------------------
// Description:
//   Applies the vertex and pixel shaders, and all the sampler and render states for the
//   given pass.
// Arguments:
//   passIndex - The index of the pass for which to apply state.
// --------------------------------------------------------------------------------------
void Tr2Shader::ApplyAllStateForPass( 
										uint32_t passIndex,
										Tr2RenderContext &renderContext ) const
{
	const Tr2Pass& pass = m_effect.passes[passIndex];

	for( unsigned i = SHADER_TYPE_FIRST; i != SHADER_TYPE_COUNT; ++i )
	{
		if( m_shaderTypeMask & ( 1 << i ) )
		{
			renderContext.m_esm.ApplyShader( ShaderType( i ), pass.stageInputs[i].m_shader );
			for( Tr2SamplerSetupMap::const_iterator it = pass.stageInputs[i].samplers.begin(); 
				it != pass.stageInputs[i].samplers.end(); ++it )
			{
				renderContext.m_esm.ApplySamplerSetup( 
					Tr2RenderContextEnum::ShaderType( i ), 
					it->first, 
					it->second.handle );
			}
		}
	}

	renderContext.m_esm.ApplyRenderStates( pass.renderStates );
}

// --------------------------------------------------------------------------------------
// Description:
//   Applies all the render states for the given pass.
// Arguments:
//   passIndex - The index of the pass for which to apply render state.
// --------------------------------------------------------------------------------------
void Tr2Shader::ApplyRenderStates( 
										uint32_t passIndex,
										Tr2RenderContext &renderContext ) const
{
	const Tr2Pass& pass = m_effect.passes[passIndex];

	renderContext.m_esm.ApplyRenderStates( pass.renderStates );
}

// --------------------------------------------------------------------------------------
// Description:
//   Applies all the sampler states for the given pass.
// Arguments:
//   passIndex - The index of the pass for which to apply sampler state.
// --------------------------------------------------------------------------------------
void Tr2Shader::ApplySamplerStates( 
										uint32_t passIndex, 
										Tr2RenderContextEnum::ShaderType type,
										Tr2RenderContext &renderContext ) const
{
	const Tr2Pass& pass = m_effect.passes[passIndex];

	for( Tr2SamplerSetupMap::const_iterator it = pass.stageInputs[type].samplers.begin(); 
		it != pass.stageInputs[type].samplers.end(); ++it )
	{
		renderContext.m_esm.ApplySamplerSetup( 
			type, 
			it->first, 
			it->second.handle );
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Applies the shader for the given pass.
// Arguments:
//   passIndex - The index of the pass for which to apply the shader.
//   type - Shader type to apply
// --------------------------------------------------------------------------------------
void Tr2Shader::ApplyShader( 
									uint32_t passIndex, 
									Tr2RenderContextEnum::ShaderType type,
									Tr2RenderContext &renderContext ) const
{
	const Tr2Pass& pass = m_effect.passes[passIndex];
	renderContext.m_esm.ApplyShader( type, pass.stageInputs[type].m_shader );
}

unsigned Tr2Shader::GetShaderTypeMask()
{
	return m_shaderTypeMask;
}

// --------------------------------------------------------------------------------------
// Description:
//   Sets the compiled D3DX effect for this shader and rebuilds annotation map.
// Arguments:
//   effect - The compiled effect
// --------------------------------------------------------------------------------------
Tr2EffectDescription& Tr2Shader::GetEffect()
{
	return m_effect;
}

// --------------------------------------------------------------------------------------
// Description:
//   Refreshes effect-dependent variables.
// --------------------------------------------------------------------------------------
void Tr2Shader::ProcessEffect()
{
	m_sortValue = 0;
	if( !m_effect.passes.empty() )
	{
		// Construct sort value so that the following parameters affect it, in the order given:
		// 1) Number of passes
		// 2) Pixel shader in the first pass
		// 3) Vertex shader in the first pass
		// 4) Render states set in the first pass

		unsigned int ps = m_effect.passes[0].stageInputs[PIXEL_SHADER].m_shader & 0x3ff;
		unsigned int vs = m_effect.passes[0].stageInputs[VERTEX_SHADER].m_shader & 0x3ff;
		unsigned int states = m_effect.passes[0].renderStates & 0x3ff;
		unsigned int numPasses = m_effect.passes.size() & 0x3;

		m_sortValue = (numPasses << 30) | (ps << 20) | (vs << 10) | states;
	}

	m_shaderTypeMask = 0;
	for( auto pass = m_effect.passes.cbegin(); pass != m_effect.passes.cend(); ++pass )
	{
		for( unsigned i = SHADER_TYPE_FIRST; i != SHADER_TYPE_COUNT; ++i )
		{
			if( pass->stageInputs[i].m_shader != Tr2EffectStageInput::INVALID )
			{
				m_shaderTypeMask |= 1u << i;
			}
		}
	}
}
