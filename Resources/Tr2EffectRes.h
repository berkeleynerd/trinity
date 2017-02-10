#ifndef TR2EFFECTRES_H
#define TR2EFFECTRES_H


#include "ITr2EffectValue.h"
#include "Tr2DeviceResource.h"
#include "ITriReroutable.h"
#include "ITr2ShaderState.h"
#include "ITr2ShaderMaterial.h"

class TriVariable;
struct ITriEffectResourceParameter;

BLUE_DECLARE( Tr2EffectRes );
BLUE_DECLARE( Tr2Shader );

// Tr2EffectParam describes one parameter for an effect. It provides a mapping from
// the source data.
class Tr2EffectParam
{
public:
	// Name of variable
	std::string m_sourceName;

	ITr2EffectValuePtr m_sourceValue;

	// Offset of parameter value in constant buffer in bytes
	unsigned int m_registerIndex;
	// Size of parameter value in bytes
	unsigned int m_registerCount;
	// Initial count for buffer list UAVs (default -1 means the counter is not reset)
	uint32_t m_initialCount;
};

struct Tr2SamplerOverrideData
{
	uint32_t registerIndex;
	uint32_t handle;
};

typedef std::vector<Tr2SamplerOverrideData> Tr2SamplerOverrideDataVector;

// Tr2EffectPassParameters holds information on parameters for the effect pass.
// * For each vertex/pixel shader parameter there is a Tr2EffectParam instance
//   describing where the value comes from.
// * For each sampler used there is a Tr2SamplerSetup structure.
// * Optionally, there is a block of Vector4 values with the default values
//   for any vertex/pixel shader constants. This is needed when constants are
//   given values in the .fx file.
class Tr2EffectPassParameters
{
public:
	Tr2EffectPassParameters()
	{
	}
	~Tr2EffectPassParameters()
	{
		for( ITriReroutableVector::iterator it = m_reroutedParameters.begin(); it != m_reroutedParameters.end(); ++it )
		{
			(*it)->SetDestination( NULL, 0 );
			(*it)->Unlock();
		}
	}

	void AllocateConstantMirror( Tr2RenderContextEnum::ShaderType type, unsigned int size )
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

	// ----------------------------------------------------------------------------------
	// Description:
	//   Represents per-render-stage inputs.
	// ----------------------------------------------------------------------------------
	struct StageInput
	{
		Tr2EffectParamVector m_shaderParameters;
		Tr2EffectParamVector m_textures;
		Tr2EffectParamVector m_uavs;
		Tr2SamplerOverrideDataVector m_samplers;
		std::unique_ptr<Tr2ConstantBufferAL> m_constantBuffer;
	};

	StageInput m_stageInput[Tr2RenderContextEnum::SHADER_TYPE_COUNT];
	ITriReroutableVector m_reroutedParameters;
};

struct Tr2ShaderPermutation
{
	BlueSharedString name;
	std::vector<BlueSharedString> options;
	size_t defaultOption;
	std::string description;
	uint8_t type;
};

struct Tr2ShaderOption
{
	BlueSharedString name;
	BlueSharedString value;
};

typedef std::vector<std::unique_ptr<Tr2EffectPassParameters>> Tr2EffectPassParametersVector;

BLUE_CLASS( Tr2EffectRes ): 
	public BlueAsyncRes,
	public ICacheable,
	public Tr2DeviceResource
{
public:
	EXPOSE_TO_BLUE();

	Tr2EffectRes( IRoot* lockobj = NULL );
	~Tr2EffectRes();

	ITr2ShaderStatePtr GetShader( const Tr2ShaderOption* options, size_t count );


	//////////////////////////////////////////////////////////////////////////
	// ICacheable
	bool IsMemoryUsageKnown();
	size_t GetMemoryUsage();

	/////////////////////////////////////////////////////////////////////////////////////
	// ITriDeviceResource
	void ReleaseResources( TriStorage s );
#if TRINITYDEV
	virtual void GetDescription( std::string& desc );
#endif
private:
	bool OnPrepareResources();

	// Provide the functions that do the actual work of loading and preparing.
	// The async management itself is done in BlueAsyncRes.
	virtual LoadingResult DoLoad();
	virtual bool DoPrepare();

#if BLUE_WITH_PYTHON
	PyObject* GetPermutationDescription();
#endif

protected:
	// Per-permutation compiled file information
	struct FileRecord
	{
		uint32_t index;
		// Compiled code offset into the file 
		uint32_t offset;
		// Compiled code size
		uint32_t size;
	};

	CcpMallocBuffer m_data;
	uint32_t m_version;

	const char* m_stringTable;
	size_t m_stringTableSize;

	const FileRecord* m_offsets;
	uint32_t m_offsetCount;

	TrackableStdVector<Tr2ShaderPermutation> m_permutations;
	TrackableStdUnorderedMap<uint32_t, BlueWeakRef<Tr2Shader> > m_shaders;

	friend class Tr2EffectStateManager;
};

TYPEDEF_BLUECLASS_WR_SHUTDOWN( Tr2EffectRes );

#endif
