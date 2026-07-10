// Copyright (c) 2026 CCP Games

#include "StdAfx.h"

#include <Blue.h>

#include "Eve/EveSpaceScene.h"
#include "Eve/EveSpaceSceneRenderDriver.h"
#include "Eve/EveStarfield.h"
#include "Eve/IEveSpaceObject2.h"
#include "Eve/SpaceObjectFactory/EveSOFData.h"
#include "ITr2Renderable.h"
#include "PostProcess/Tr2PostProcess2.h"
#include "Tr2ProfileTimer.h"
#include "Tr2PerObjectData.h"
#include "Tr2Renderer.h"
#include "Resources/TriTextureRes.h"
#include "Shader/Tr2Effect.h"
#include "Shader/Parameter/TriTextureParameter.h"
#include "Shader/Tr2Shader.h"
#include "TrinityStandaloneCmfModel.h"
#include "TriRenderBatch.h"
#include "TriDevice.h"
#include "TriProjection.h"
#include "TriView.h"

#include <IBluePaths.h>
#include <IBluePersist.h>
#include <IBlueResMan.h>

#include <cstdio>
#include <cstdint>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <set>
#include <string>
#include <vector>

extern "C" void TrinityStandaloneStartup();
extern "C" bool BlueInitializeResourceLoading();

#ifndef IRootReader_H
#define IRootReader_H
BLUE_INTERFACE( IRootReader ) : public IRoot
{
	virtual IRoot* ReadFromStream( IBlueStream* stream ) = 0;
	virtual bool ReadForCachingFromStream( IBlueStream* stream ) = 0;
	virtual void SetFileName( const wchar_t* name ) = 0;
	virtual void SetDoInitialize( bool value ) = 0;
	virtual void SetTimeSlice( float seconds ) = 0;
	virtual void GetErrorMessage( std::string& message ) = 0;
};
BLUE_DEFINE_INTERFACE_IMPL( IRootReader );
#endif

#ifdef _WIN32
#define TRINITY_STANDALONE_EXPORT extern "C" __declspec( dllexport )
#else
#define TRINITY_STANDALONE_EXPORT extern "C" __attribute__( ( visibility( "default" ) ) )
#endif

namespace
{
bool ConfigureAsteroEveV5Effect( Tr2Effect& effect, std::string& error );

class StandaloneEveV5PerObjectData : public Tr2PerObjectData
{
public:
	void SetPerObjectDataToDevice( Tr2ConstantBufferAL** buffers, unsigned constantTypeMask, Tr2RenderContext& renderContext ) const override
	{
		if( constantTypeMask & SHADER_TYPE_EXISTS( Tr2RenderContextEnum::VERTEX_SHADER ) )
		{
			FillAndSetConstants(
				*buffers[Tr2RenderContextEnum::VERTEX_SHADER],
				&m_vsData,
				sizeof( m_vsData ),
				Tr2RenderContextEnum::VERTEX_SHADER,
				Tr2Renderer::GetPerObjectVSStartRegister(),
				renderContext );
		}
		if( constantTypeMask & SHADER_TYPE_EXISTS( Tr2RenderContextEnum::PIXEL_SHADER ) )
		{
			FillAndSetConstants(
				*buffers[Tr2RenderContextEnum::PIXEL_SHADER],
				&m_psData,
				sizeof( m_psData ),
				Tr2RenderContextEnum::PIXEL_SHADER,
				Tr2Renderer::GetPerObjectPSStartRegister(),
				renderContext );
		}
	}

	void ApplyConstantBuffers( Tr2IndirectDrawBufferWriter& writer, Tr2RenderContext& ) const override
	{
		writer.SetPerObjectData( Tr2RenderContextEnum::VERTEX_SHADER, &m_vsData, sizeof( m_vsData ) );
		writer.SetPerObjectData( Tr2RenderContextEnum::PIXEL_SHADER, &m_psData, sizeof( m_psData ) );
	}

	EveSpaceObjectVSData m_vsData = {};
	EveSpaceObjectPSData m_psData = {};
};
}

BLUE_CLASS( TrinityStandaloneRenderable ) :
	public IEveSpaceObject2,
	public ITr2Renderable
{
public:
	EXPOSE_TO_BLUE();

	TrinityStandaloneRenderable( IRoot* lockobj = nullptr )
	{
	}

	bool LoadAsset( const std::string& path, float aspect, std::string& error )
	{
		m_aspect = aspect;
		return m_model.Load( path, error );
	}

	bool InitializeGpu( Tr2PrimaryRenderContext& renderContext, int materialView, int materialMode )
	{
		using namespace Tr2RenderContextEnum;
		m_useEveV5Material = materialMode != 0;
		if( !m_useEveV5Material &&
			( !CreateTexture( m_model.BaseColorTexture(), m_baseColorTexture, renderContext ) ||
			!CreateTexture( m_model.NormalTexture(), m_normalTexture, renderContext ) ||
			!CreateTexture( m_model.RoughnessTexture(), m_roughnessTexture, renderContext ) ||
			!CreateTexture( m_model.MaterialTexture(), m_materialTexture, renderContext ) ||
			!CreateTexture( m_model.GlowTexture(), m_glowTexture, renderContext ) ||
			!CreateTexture( m_model.DirtTexture(), m_dirtTexture, renderContext ) ||
			!CreateTexture( m_model.MaskTexture(), m_maskTexture, renderContext ) ||
			!CreateTexture( m_model.PaintMaskTexture(), m_paintMaskTexture, renderContext ) ) )
		{
			return false;
		}

		const uint32_t vertexStride = m_useEveV5Material ? sizeof( TrinityStandaloneEveV5Vertex ) : sizeof( TrinityStandaloneCmfVertex );
		const void* vertexData = m_useEveV5Material
			? static_cast<const void*>( m_model.EveV5Vertices().data() )
			: static_cast<const void*>( m_model.Vertices().data() );
		if( FAILED( m_vertexBuffer.Create(
				vertexStride,
				m_model.VertexCount(),
				Tr2GpuUsage::VERTEX_BUFFER,
				Tr2CpuUsage::NONE,
				vertexData,
				renderContext ) ) )
		{
			return false;
		}
		if( FAILED( m_indexBuffer.Create(
				sizeof( uint32_t ),
				m_model.IndexCount(),
				Tr2GpuUsage::INDEX_BUFFER,
				Tr2CpuUsage::NONE,
				m_model.Indices().data(),
				renderContext ) ) )
		{
			return false;
		}

		Tr2VertexDefinition vertexDefinition;
		vertexDefinition.Add( Tr2VertexDefinition::FLOAT32_3, Tr2VertexDefinition::POSITION );
		if( m_useEveV5Material )
		{
			vertexDefinition.Add( Tr2VertexDefinition::USHORT_4, Tr2VertexDefinition::BLENDINDICES );
			vertexDefinition.Add( Tr2VertexDefinition::FLOAT32_2, Tr2VertexDefinition::TEXCOORD );
			vertexDefinition.Add( Tr2VertexDefinition::FLOAT32_4, Tr2VertexDefinition::TANGENT );
		}
		else
		{
			vertexDefinition.Add( Tr2VertexDefinition::FLOAT32_3, Tr2VertexDefinition::NORMAL );
			vertexDefinition.Add( Tr2VertexDefinition::FLOAT32_4, Tr2VertexDefinition::TANGENT );
			vertexDefinition.Add( Tr2VertexDefinition::FLOAT32_2, Tr2VertexDefinition::TEXCOORD );
			vertexDefinition.Add( Tr2VertexDefinition::FLOAT32_4, Tr2VertexDefinition::COLOR );
		}
		m_vertexDeclaration = Tr2EffectStateManager::GetVertexDeclarationHandle( vertexDefinition );
		if( m_vertexDeclaration == Tr2EffectStateManager::UNINITIALIZED_DECLARATION )
		{
			return false;
		}

		m_effect.CreateInstance();
		m_effect->StartUpdate();
		if( m_useEveV5Material )
		{
			m_effect->SetOption( BlueSharedString( "SPACE_OBJECT_TRANSPARENCY" ), BlueSharedString( "SOT_OPAQUE" ) );
			m_effect->SetOption( BlueSharedString( "SPACE_OBJECT_PPT_ENABLED" ), BlueSharedString( "SOPPT_DISABLED" ) );
			m_effect->SetEffectPathName( "res:/graphics/effect/managed/space/spaceobject/v5/quad/quadv5.fx" );
			std::string materialError;
			if( !ConfigureAsteroEveV5Effect( *m_effect, materialError ) )
			{
				std::fprintf( stderr, "Failed to configure authored Astero material: %s\n", materialError.c_str() );
				return false;
			}
		}
		else
		{
			m_effect->SetEffectPathName( "res:/graphics/effect/managed/space/spaceobject/probe/asteropbr.fx" );
		}
		Tr2EffectRes* effectResource = m_effect->GetEffectRes();
		if( effectResource )
		{
			effectResource->ForceSynchronousLoad();
			effectResource->Reload();
		}
		if( !m_useEveV5Material )
		{
			m_effect->SetParameter( BlueSharedString( "BaseColorMap" ), m_baseColorTexture );
			m_effect->SetParameter( BlueSharedString( "NormalMap" ), m_normalTexture );
			m_effect->SetParameter( BlueSharedString( "RoughnessMap" ), m_roughnessTexture );
			m_effect->SetParameter( BlueSharedString( "MaterialMap" ), m_materialTexture );
			m_effect->SetParameter( BlueSharedString( "GlowMap" ), m_glowTexture );
			m_effect->SetParameter( BlueSharedString( "DirtMap" ), m_dirtTexture );
			m_effect->SetParameter( BlueSharedString( "DistortionLayerMap" ), m_maskTexture );
			m_effect->SetParameter( BlueSharedString( "PaintMaskMap" ), m_paintMaskTexture );
			m_effect->SetParameter( BlueSharedString( "MaterialView" ), static_cast<float>( materialView ) );
			float centerScale[4];
			m_model.GetCenterAndScale( centerScale );
			m_effect->SetParameter( BlueSharedString( "ModelCenterScale" ), Vector4( centerScale[0], centerScale[1], centerScale[2], centerScale[3] ) );
		}
		UpdateEffectParameters();
		m_effect->EndUpdate();
		if( m_useEveV5Material )
		{
			const char* textureNames[] = {
				"AlbedoMap", "NormalMap", "RoughnessMap", "MaterialMap", "GlowMap", "DirtMap", "PaintMaskMap", "DustNoiseMap"
			};
			for( const char* textureName : textureNames )
			{
				TriTextureParameterPtr parameter = BlueCastPtr( m_effect->GetResourceByName( textureName ) );
				TriTextureResPtr texture;
				if( parameter )
				{
					texture = BlueCastPtr( parameter->GetResource() );
				}
				if( texture )
				{
					texture->ForceSynchronousLoad();
					texture->Reload();
				}
				std::fprintf(
					stderr,
					"Authored texture %s: parameter=%s resource=%s good=%s path=%ls\n",
					textureName,
					parameter ? "yes" : "no",
					texture ? "yes" : "no",
					texture && texture->IsGood() ? "yes" : "no",
					texture ? texture->GetPath() : L"" );
			}
		}

		if( !effectResource || !effectResource->IsGood() )
		{
			if( effectResource )
			{
				std::fprintf(
					stderr,
					"Standalone material effect failed: path=%ls file=%ls loading=%d prepared=%d\n",
					effectResource->GetPath(),
					effectResource->GetFilePath().c_str(),
					effectResource->IsLoading() ? 1 : 0,
					effectResource->IsPrepared() ? 1 : 0 );
			}
			else
			{
			std::fprintf( stderr, "Standalone material effect resource was not created\n" );
			}
			return false;
		}
		Tr2Shader* shader = m_effect->GetShaderStateInterface();
		if( !shader )
		{
			std::fprintf( stderr, "Standalone material effect did not create a shader state interface\n" );
			return false;
		}
		uint32_t mainTechnique = 0;
		uint32_t depthTechnique = 0;
		if( !shader->GetTechniqueIndex( BlueSharedString( "Main" ), mainTechnique ) ||
			!shader->GetTechniqueIndex( BlueSharedString( "Depth" ), depthTechnique ) )
		{
			std::fprintf( stderr, "Standalone material effect is missing the Main or Depth technique\n" );
			return false;
		}
		std::fprintf(
			stderr,
			"Standalone material effect ready: mode=%s Main=%u Depth=%u vertices=%u indices=%u\n",
			m_useEveV5Material ? "eve-v5" : "probe",
			mainTechnique,
			depthTechnique,
			m_model.VertexCount(),
			m_model.IndexCount() );
		return true;
	}

	void UpdateSyncronous( const EveUpdateContext& updateContext ) override
	{
		m_time = updateContext.GetTime();
		UpdateEffectParameters();
	}

	void UpdateAsyncronous( const EveUpdateContext& ) override
	{
	}

	void UpdateVisibility( const EveUpdateContext&, const Matrix& ) override
	{
	}

	void GetRenderables( std::vector<ITr2Renderable*>& renderables, Tr2ImpostorManager* ) override
	{
		renderables.push_back( this );
	}

	bool GetBoundingSphere( Vector4& sphere, BoundingSphereQuery ) const override
	{
		sphere = Vector4( 0.0f, 0.0f, 0.0f, 2.0f );
		return true;
	}

	void UpdateModelCenterWorldPosition( Vector3& position, Be::Time ) override
	{
		position = Vector3( 0.0f, 0.0f, 0.0f );
	}

	void GetModelCenterWorldPosition( Vector3& position ) const override
	{
		position = Vector3( 0.0f, 0.0f, 0.0f );
	}

	bool GetLocalBoundingBox( Vector3& minimum, Vector3& maximum ) override
	{
		minimum = Vector3( -1.0f, -1.0f, -1.0f );
		maximum = Vector3( 1.0f, 1.0f, 1.0f );
		return true;
	}

	void GetLocalToWorldTransform( Matrix& transform ) const override
	{
		transform = m_worldTransform;
	}

	void GetBatches( ITriRenderBatchAccumulator* batches, TriBatchType batchType, const Tr2PerObjectData*, Tr2RenderReason ) override
	{
		if( batchType != TRIBATCHTYPE_OPAQUE )
		{
			return;
		}
		if( !m_effect || !m_effect->GetShaderStateInterface() )
		{
			return;
		}
		Tr2RenderBatch batch;
		batch.SetMaterial( m_effect );
		batch.SetGeometry(
			m_vertexDeclaration,
			m_vertexBuffer,
			m_useEveV5Material ? sizeof( TrinityStandaloneEveV5Vertex ) : sizeof( TrinityStandaloneCmfVertex ),
			m_indexBuffer,
			sizeof( uint32_t ) );
		if( m_useEveV5Material )
		{
			batch.SetPerObjectData( &m_eveV5PerObjectData );
		}
		batch.SetDrawIndexedInstanced( m_model.IndexCount(), 1, 0, 0, 0 );
		batch.SetRenderingMode( Tr2EffectStateManager::RM_OPAQUE );
		batches->Commit( batch );
		if( !m_reportedBatch )
		{
			std::fprintf( stderr, "Probe native opaque batch committed\n" );
			m_reportedBatch = true;
		}
	}

	bool HasTransparentBatches() override
	{
		return false;
	}

	float GetSortValue() override
	{
		return 0.0f;
	}

	Tr2PerObjectData* GetPerObjectData( ITriRenderBatchAccumulator* ) override
	{
		return m_useEveV5Material ? &m_eveV5PerObjectData : nullptr;
	}

	bool DrawFailed() const
	{
		return m_drawFailed;
	}

private:
	static bool CreateTexture( const TrinityStandaloneCmfTexture& source, Tr2TextureAL& texture, Tr2PrimaryRenderContext& renderContext )
	{
		Tr2SubresourceData data = {};
		data.m_sysMem = source.rgba.data();
		data.m_sysMemPitch = source.width * 4;
		data.m_sysMemSlicePitch = static_cast<uint32_t>( source.rgba.size() );
		return SUCCEEDED( texture.Create(
			Tr2BitmapDimensions( source.width, source.height, 1, Tr2RenderContextEnum::PIXEL_FORMAT_R8G8B8A8_UNORM ),
			Tr2GpuUsage::SHADER_RESOURCE,
			&data,
			renderContext ) );
	}

	void UpdateEffectParameters()
	{
		if( !m_effect )
		{
			return;
		}
		const float seconds = static_cast<float>( m_time ) / 10000000.0f;
		const float yaw = seconds * 0.38f - 0.8f;
		const float pitch = -0.28f;
		if( m_useEveV5Material )
		{
			m_worldTransform = RotationYMatrix( yaw ) * RotationXMatrix( pitch );
			const Matrix world = Transpose( m_worldTransform );
			const Matrix inverseWorld = Transpose( Inverse( m_worldTransform ) );
			m_eveV5PerObjectData.m_vsData.worldTransform = world;
			m_eveV5PerObjectData.m_vsData.worldTransformLast = world;
			m_eveV5PerObjectData.m_vsData.invWorldTransform = inverseWorld;
			m_eveV5PerObjectData.m_vsData.shipData = Vector4( 1.0f, 1.0f, 0.0f, 2.0f );
			m_eveV5PerObjectData.m_vsData.ellpsoidRadii = Vector4( -1.0f, -1.0f, -1.0f, 0.0f );
			m_eveV5PerObjectData.m_psData.worldTransform = world;
			m_eveV5PerObjectData.m_psData.worldTransformLast = world;
			m_eveV5PerObjectData.m_psData.invWorldTransform = inverseWorld;
			m_eveV5PerObjectData.m_psData.shipData = m_eveV5PerObjectData.m_vsData.shipData;
		}
		else
		{
			m_effect->SetParameter( BlueSharedString( "RotationTerms" ), Vector4( std::sin( yaw ), std::cos( yaw ), std::sin( pitch ), std::cos( pitch ) ) );
			m_effect->SetParameter( BlueSharedString( "ProjectionTerms" ), Vector4( std::max( m_aspect, 0.1f ), 1.9f, 4.8f, 20.0f ) );
		}
	}

	Be::Time m_time = 0;
	float m_aspect = 1.0f;
	bool m_drawFailed = false;
	bool m_reportedBatch = false;
	bool m_useEveV5Material = false;
	Matrix m_worldTransform = IdentityMatrix();
	TrinityStandaloneCmfModel m_model;
	StandaloneEveV5PerObjectData m_eveV5PerObjectData;
	Tr2BufferAL m_vertexBuffer;
	Tr2BufferAL m_indexBuffer;
	uint32_t m_vertexDeclaration = Tr2EffectStateManager::UNINITIALIZED_DECLARATION;
	Tr2EffectPtr m_effect;
	Tr2TextureAL m_baseColorTexture;
	Tr2TextureAL m_normalTexture;
	Tr2TextureAL m_roughnessTexture;
	Tr2TextureAL m_materialTexture;
	Tr2TextureAL m_glowTexture;
	Tr2TextureAL m_dirtTexture;
	Tr2TextureAL m_maskTexture;
	Tr2TextureAL m_paintMaskTexture;
};

TYPEDEF_BLUECLASS( TrinityStandaloneRenderable );
BLUE_DEFINE_NONEXPOSED( TrinityStandaloneRenderable );

const Be::ClassInfo* TrinityStandaloneRenderable::ExposeToBlue()
{
	EXPOSURE_BEGIN( TrinityStandaloneRenderable, "" )
		MAP_INTERFACE( IEveSpaceObject2 )
		MAP_INTERFACE( ITr2Renderable )
	EXPOSURE_END()
}

namespace
{

enum StandaloneProbeRung
{
	STANDALONE_PROBE_RUNG_SHELL = 0,
	STANDALONE_PROBE_RUNG_SCENE = 2,
	STANDALONE_PROBE_RUNG_MODEL = 3,
	STANDALONE_PROBE_RUNG_HDR_BLIT = 4,
	STANDALONE_PROBE_RUNG_HDR_POST = 5,
	STANDALONE_PROBE_RUNG_HDR_EXPOSURE = 6,
};

struct StandaloneProbe
{
	~StandaloneProbe()
	{
		driver.Unlock();
		scene.Unlock();
		renderable.Unlock();
		view.Unlock();
		projection.Unlock();
		renderContext = nullptr;

		if( device )
		{
			TriDevice* rawDevice = device;
			device->InvalidateAndUnregisterForTicks();
			if( gTriDev == rawDevice )
			{
				gTriDev = nullptr;
			}
			device.Unlock();

			// TriDeviceLock deliberately takes one lifetime reference in addition
			// to gTriDev. Standalone probes must release it during explicit teardown.
			reinterpret_cast<IRoot*>( rawDevice )->Unlock();
		}
	}

	BluePtr<TriDevice> device;
	Tr2RenderContext* renderContext = nullptr;
	EveSpaceScenePtr scene;
	EveSpaceSceneRenderDriverPtr driver;
	BluePtr<TrinityStandaloneRenderable> renderable;
	TriViewPtr view;
	TriProjectionPtr projection;
	uint32_t renderWidth = 0;
	uint32_t renderHeight = 0;
};

std::wstring ToWide( const char* value )
{
	if( !value )
	{
		return {};
	}
	return std::wstring( value, value + strlen( value ) );
}

bool CheckHresult( HRESULT result, const char* operation )
{
	if( SUCCEEDED( result ) )
	{
		return true;
	}
	CCP_LOGERR( "%s failed with HRESULT 0x%08x", operation, static_cast<uint32_t>( result ) );
	return false;
}

const char* AreaTypeName( EveSOFDataArea::AreaType type )
{
	switch( type )
	{
	case EveSOFDataArea::TYPE_PRIMARY:
		return "Primary";
	case EveSOFDataArea::TYPE_GLASS:
		return "Glass";
	case EveSOFDataArea::TYPE_SAILS:
		return "Sails";
	case EveSOFDataArea::TYPE_REACTOR:
		return "Reactor";
	case EveSOFDataArea::TYPE_DARKHULL:
		return "DarkHull";
	case EveSOFDataArea::TYPE_WRECK:
		return "Wreck";
	case EveSOFDataArea::TYPE_ROCK:
		return "Rock";
	case EveSOFDataArea::TYPE_MONUMENT:
		return "Monument";
	case EveSOFDataArea::TYPE_ORNAMENT:
		return "Ornament";
	case EveSOFDataArea::TYPE_SIMPLEPRIMARY:
		return "SimplePrimary";
	case EveSOFDataArea::TYPE_TURRET:
		return "Turret";
	default:
		return "Unknown";
	}
}

const char* VertexUsageName( Tr2VertexDefinition::UsageCode usage )
{
	static const char* names[] = {
		"POSITION", "COLOR", "NORMAL", "TANGENT", "BITANGENT", "TEXCOORD", "BLENDINDICES", "BLENDWEIGHTS"
	};
	return usage < Tr2VertexDefinition::NUM_USAGE_CODE ? names[usage] : "UNKNOWN";
}

const char* ShaderStageName( Tr2RenderContextEnum::ShaderType stage )
{
	switch( stage )
	{
	case Tr2RenderContextEnum::VERTEX_SHADER:
		return "vertex";
	case Tr2RenderContextEnum::PIXEL_SHADER:
		return "pixel";
	case Tr2RenderContextEnum::GEOMETRY_SHADER:
		return "geometry";
	case Tr2RenderContextEnum::HULL_SHADER:
		return "hull";
	case Tr2RenderContextEnum::DOMAIN_SHADER:
		return "domain";
	case Tr2RenderContextEnum::COMPUTE_SHADER:
		return "compute";
	default:
		return "unknown";
	}
}

const char* PipelineInputTypeName( Tr2ShaderPipelineInputAL::Type type )
{
	switch( type )
	{
	case Tr2ShaderPipelineInputAL::FLOAT:
		return "float";
	case Tr2ShaderPipelineInputAL::INT:
		return "int";
	case Tr2ShaderPipelineInputAL::UINT:
		return "uint";
	default:
		return "other";
	}
}

void WriteVector4( std::ostream& output, const Vector4& value )
{
	output << std::fixed << std::setprecision( 5 )
		   << value.x << ", " << value.y << ", " << value.z << ", " << value.w;
}

void WriteColor( std::ostream& output, const Color& value )
{
	output << std::fixed << std::setprecision( 5 )
		   << value.r << ", " << value.g << ", " << value.b << ", " << value.a;
}

void WriteAreaList( std::ostream& output, const char* label, const PEveSOFDataHullAreaVector& areas )
{
	output << "### " << label << " areas (" << areas.size() << ")\n\n";
	if( areas.empty() )
	{
		output << "None.\n\n";
		return;
	}

	for( const auto& area : areas )
	{
		output << "- `" << area->m_name.c_str() << "`: index=" << area->m_index
			   << ", count=" << area->m_count
			   << ", type=" << AreaTypeName( area->m_areaType )
			   << ", shader=`" << area->m_shader.c_str() << "`, blockedMaterials=0x"
			   << std::hex << area->m_blockedMaterials << std::dec << "\n";
		for( const auto& texture : area->m_textures )
		{
			output << "  - texture `" << texture->m_name.c_str() << "` = `" << texture->m_resFilePath << "`\n";
		}
		for( const auto& parameter : area->m_parameters )
		{
			output << "  - parameter `" << parameter->m_name.c_str() << "` = (";
			WriteVector4( output, parameter->m_value );
			output << ")\n";
		}
	}
	output << "\n";
}

template <typename T>
BluePtr<T> LoadBlackObjectWithoutYield( const char* path, std::string& error )
{
	const std::wstring widePath = ToWide( path );
	IBlueStreamPtr stream;
	if( !BePaths->GetStreamFromPathW( widePath.c_str(), &stream ) || !stream )
	{
		error = std::string( "Unable to open " ) + path;
		return nullptr;
	}

	BluePtr<IRootReader> reader;
	if( !BeClasses->CreateInstanceFromName(
			"BlackReader",
			BlueInterfaceIID<IRootReader>(),
			reinterpret_cast<void**>( &reader.p ) ) ||
		!reader )
	{
		error = "Carbon BlackReader is not registered";
		return nullptr;
	}

	reader->SetFileName( widePath.c_str() );
	reader->SetDoInitialize( false );

#if BLUE_WITH_PYTHON
	PyThreadState* pythonState = nullptr;
	if( Py_IsInitialized() && PyGILState_Check() )
	{
		pythonState = PyEval_SaveThread();
	}
	IRoot* rawObject = reader->ReadFromStream( stream );
	if( pythonState )
	{
		PyEval_RestoreThread( pythonState );
	}
#else
	IRoot* rawObject = reader->ReadFromStream( stream );
#endif

	if( !rawObject )
	{
		reader->GetErrorMessage( error );
		if( error.empty() )
		{
			error = std::string( "BlackReader failed for " ) + path;
		}
		return nullptr;
	}

	IRootPtr object;
	object.Attach( rawObject );
	auto typed = BluePtr<T>( BlueCastPtr( object ) );
	if( !typed )
	{
		error = std::string( "Unexpected root type in " ) + path;
	}
	return typed;
}

std::string ToNarrowPath( const wchar_t* path )
{
	if( !path )
	{
		return {};
	}
	const std::wstring widePath( path );
	return std::string( widePath.begin(), widePath.end() );
}

Vector3 RgbToHsv( float red, float green, float blue )
{
	const float minimum = std::min( { red, green, blue } );
	const float maximum = std::max( { red, green, blue } );
	const float delta = maximum - minimum;
	if( delta == 0.0f )
	{
		return Vector3( 0.0f, 0.0f, maximum );
	}
	if( maximum == 0.0f )
	{
		return Vector3( -1.0f, 0.0f, maximum );
	}

	float hue = 0.0f;
	if( red == maximum )
	{
		hue = ( green - blue ) / delta;
	}
	else if( green == maximum )
	{
		hue = 2.0f + ( blue - red ) / delta;
	}
	else
	{
		hue = 4.0f + ( red - green ) / delta;
	}
	hue *= 60.0f;
	if( hue < 0.0f )
	{
		hue += 360.0f;
	}
	return Vector3( hue, delta / maximum, maximum );
}

Color HsvToColor( float hue, float saturation, float value )
{
	if( saturation == 0.0f )
	{
		return Color( value, value, value, 1.0f );
	}

	const float sector = hue / 60.0f;
	const int sectorIndex = static_cast<int>( std::floor( sector ) ) % 6;
	const float fraction = sector - std::floor( sector );
	const float p = value * ( 1.0f - saturation );
	const float q = value * ( 1.0f - saturation * fraction );
	const float t = value * ( 1.0f - saturation * ( 1.0f - fraction ) );
	switch( sectorIndex )
	{
	case 0:
		return Color( value, t, p, 1.0f );
	case 1:
		return Color( q, value, p, 1.0f );
	case 2:
		return Color( p, value, t, 1.0f );
	case 3:
		return Color( p, q, value, 1.0f );
	case 4:
		return Color( t, p, value, 1.0f );
	default:
		return Color( value, p, q, 1.0f );
	}
}

bool PrepareTextureResourceWithoutYield( const std::string& logicalPath, const char* role, std::string& error )
{
	if( logicalPath.empty() )
	{
		return true;
	}

	TriTextureResPtr texture;
	BeResMan->GetResource( logicalPath.c_str(), "", texture );
	if( texture )
	{
		texture->ForceSynchronousLoad();
		texture->Reload();
	}
	if( !texture || !texture->IsGood() )
	{
		error = std::string( role ) + " failed to prepare: " + logicalPath;
		return false;
	}

	std::fprintf( stderr, "EVE scene %s ready: %s\n", role, logicalPath.c_str() );
	return true;
}

bool PrepareEffectResourcesWithoutYield( Tr2Effect& effect, const char* role, std::string& error )
{
	for( auto* resource : effect.m_resources )
	{
		TriTextureParameterPtr parameter = BlueCastPtr( resource );
		if( !parameter )
		{
			continue;
		}
		if( !parameter->Initialize() )
		{
			error = std::string( role ) + " texture parameter failed to initialize: " + parameter->GetParameterName();
			return false;
		}
		const std::string texturePath = ToNarrowPath( parameter->GetResourcePath() );
		TriTextureResPtr texture = BlueCastPtr( parameter->GetResource() );
		if( texture )
		{
			texture->ForceSynchronousLoad();
			texture->Reload();
		}
		if( texturePath.empty() || !texture || !texture->IsGood() )
		{
			error = std::string( role ) + " texture failed to prepare: " +
				( texturePath.empty() ? parameter->GetParameterName() : texturePath );
			return false;
		}
		std::fprintf(
			stderr,
			"EVE %s texture ready: parameter=%s path=%s\n",
			role,
			parameter->GetParameterName(),
			texturePath.c_str() );
	}

	if( !effect.Initialize() )
	{
		error = std::string( role ) + " effect failed to initialize: " + effect.GetEffectPathName();
		return false;
	}
	Tr2EffectRes* effectResource = effect.GetEffectRes();
	if( effectResource )
	{
		effectResource->ForceSynchronousLoad();
		effectResource->Reload();
	}
	Tr2Shader* shader = effect.GetShaderStateInterface();
	if( !effectResource || !effectResource->IsGood() || !shader || shader->GetPassCount( 0 ) == 0 )
	{
		error = std::string( role ) + " effect failed to prepare: " + effect.GetEffectPathName();
		return false;
	}
	std::fprintf(
		stderr,
		"EVE %s effect ready: %s passes=%u\n",
		role,
		effect.GetEffectPathName(),
		shader->GetPassCount( 0 ) );
	return true;
}

bool PrepareSceneBackgroundWithoutYield( EveSpaceScene& scene, const char* scenePath, std::string& error )
{
	const EveSpaceScene::LightingSetup lighting = scene.GetLightingSetup();
	std::fprintf(
		stderr,
		"EVE scene lighting: scene=%s background=%s sunDirection=(%.4f, %.4f, %.4f) "
		"sunColor=(%.4f, %.4f, %.4f) ambient=(%.4f, %.4f, %.4f) nebula=%.4f "
		"reflection=%.4f envRotation=(%.4f, %.4f, %.4f, %.4f) env=%s env1=%s env2=%s sh=%s\n",
		scenePath,
		lighting.backgroundRenderingEnabled ? "enabled" : "disabled",
		lighting.sunDirection.x,
		lighting.sunDirection.y,
		lighting.sunDirection.z,
		lighting.sunColor.r,
		lighting.sunColor.g,
		lighting.sunColor.b,
		lighting.ambientColor.r,
		lighting.ambientColor.g,
		lighting.ambientColor.b,
		lighting.nebulaIntensity,
		lighting.reflectionIntensity,
		lighting.environmentMapRotation.x,
		lighting.environmentMapRotation.y,
		lighting.environmentMapRotation.z,
		lighting.environmentMapRotation.w,
		lighting.environmentMapPath.c_str(),
		lighting.environmentMap1Path.c_str(),
		lighting.environmentMap2Path.c_str(),
		scene.GetShLightingManager() ? "yes" : "no" );

	if( !lighting.backgroundRenderingEnabled )
	{
		return true;
	}

	Tr2EffectPtr backgroundEffect = scene.GetBackgroundEffect();
	if( !backgroundEffect )
	{
		error = std::string( "Background rendering is enabled but no effect was serialized in " ) + scenePath;
		return false;
	}

	if( !PrepareEffectResourcesWithoutYield( *backgroundEffect, "background", error ) )
	{
		return false;
	}

	const std::pair<const std::string*, const char*> sceneTextures[] = {
		{ &lighting.environmentMapPath, "reflection environment" },
		{ &lighting.environmentMap1Path, "environment map 1" },
		{ &lighting.environmentMap2Path, "environment map 2" },
		{ &lighting.lowQualityNebulaPath, "low-quality nebula" },
		{ &lighting.lowQualityNebulaMixPath, "low-quality nebula mix" },
	};
	for( const auto& texture : sceneTextures )
	{
		if( !PrepareTextureResourceWithoutYield( *texture.first, texture.second, error ) )
		{
			return false;
		}
	}
	return true;
}

bool ConfigureNewEdenSystem( EveSpaceScene& scene, std::string& error )
{
	constexpr int32_t systemId = 30005286;
	constexpr int32_t constellationId = 20000773;
	constexpr float securityStatus = 0.2605538070201874f;
	constexpr double starRadius = 158400000.0;
	constexpr double observerX = -1069486940160.0;
	constexpr double observerY = 202669301760.0;
	constexpr double observerZ = 831868968960.0;
	constexpr float emissiveR = 5.0f;
	constexpr float emissiveG = 4.274509906768799f;
	constexpr float emissiveB = 2.3529412746429443f;

	const double observerDistance = std::sqrt(
		observerX * observerX + observerY * observerY + observerZ * observerZ );
	const Vector3 sunDirection(
		static_cast<float>( observerX / observerDistance ),
		static_cast<float>( observerY / observerDistance ),
		static_cast<float>( observerZ / observerDistance ) );
	const float distanceRatio = static_cast<float>(
		1.0 / std::max( 1.0, observerDistance / starRadius * 0.5 ) );

	const Vector3 closeHsv = RgbToHsv( emissiveR, emissiveG, emissiveB );
	const float farSaturation = closeHsv.y * 0.5f;
	const float farValue = 1.5f;
	const Color sunColor = HsvToColor(
		closeHsv.x,
		farSaturation + ( closeHsv.y - farSaturation ) * distanceRatio,
		farValue + ( closeHsv.z - farValue ) * distanceRatio );
	scene.SetSunLighting( sunDirection, sunColor );

	const int32_t starCount = 500 + static_cast<int32_t>( 250.0f * securityStatus );
	auto starfield = LoadBlackObjectWithoutYield<EveStarfield>(
		"res:/dx9/scene/starfield/spritestars.black",
		error );
	if( !starfield )
	{
		return false;
	}
	starfield->Configure( starCount, constellationId, 40.0f, 80.0f );
	Tr2Effect* starfieldEffect = starfield->GetEffect();
	if( !starfieldEffect )
	{
		error = "New Eden sprite starfield has no effect";
		return false;
	}
	if( !PrepareEffectResourcesWithoutYield( *starfieldEffect, "sprite starfield", error ) ||
		!starfield->Initialize() )
	{
		if( error.empty() )
		{
			error = "New Eden sprite starfield failed to initialize";
		}
		return false;
	}
	scene.SetStarfield( starfield );

	std::fprintf(
		stderr,
		"New Eden system active: system=%d constellation=%d security=%.6f observer=Promised Land stargate "
		"sunType=45041 graphic=21480 radius=%.0f ratio=%.8f direction=(%.6f, %.6f, %.6f) "
		"color=(%.6f, %.6f, %.6f) stars=%d seed=%d range=[40, 80]\n",
		systemId,
		constellationId,
		securityStatus,
		starRadius,
		distanceRatio,
		sunDirection.x,
		sunDirection.y,
		sunDirection.z,
		sunColor.r,
		sunColor.g,
		sunColor.b,
		starCount,
		constellationId );
	return true;
}

bool ConfigureAsteroEveV5Effect( Tr2Effect& effect, std::string& error )
{
	const char* materialNames[] = {
		"chrome_metallic", "grey_steel_brushed", "white_ghost_matt", "red_crimson_enamel"
	};
	const char* materialPaths[] = {
		"res:/dx9/model/spaceobjectfactory/materials/chrome_metallic.black",
		"res:/dx9/model/spaceobjectfactory/materials/grey_steel_brushed.black",
		"res:/dx9/model/spaceobjectfactory/materials/white_ghost_matt.black",
		"res:/dx9/model/spaceobjectfactory/materials/red_crimson_enamel.black",
	};

	auto hull = LoadBlackObjectWithoutYield<EveSOFDataHull>(
		"res:/dx9/model/spaceobjectfactory/hulls/soef1_t1.black",
		error );
	if( !hull )
	{
		return false;
	}
	auto faction = LoadBlackObjectWithoutYield<EveSOFDataFaction>(
		"res:/dx9/model/spaceobjectfactory/factions/soebase.black",
		error );
	if( !faction || !faction->m_colorSet )
	{
		if( error.empty() )
		{
			error = "Astero faction has no color set";
		}
		return false;
	}

	Vector4 patternMaterialValues[2][3] = {};
	for( size_t materialIndex = 0; materialIndex < 4; ++materialIndex )
	{
		auto material = LoadBlackObjectWithoutYield<EveSOFDataMaterial>( materialPaths[materialIndex], error );
		if( !material )
		{
			return false;
		}

		const std::string prefix = "Mtl" + std::to_string( materialIndex + 1 );
		for( const auto& parameter : material->m_parameters )
		{
			const std::string parameterName = prefix + parameter->m_name.c_str();
			effect.AddParameterVector4( BlueSharedString( parameterName ), &parameter->m_value );

			if( materialIndex < 2 )
			{
				const char* suffix = parameter->m_name.c_str();
				const size_t patternIndex = materialIndex;
				if( std::strcmp( suffix, "DiffuseColor" ) == 0 )
				{
					patternMaterialValues[patternIndex][0] = parameter->m_value;
				}
				else if( std::strcmp( suffix, "FresnelColor" ) == 0 )
				{
					patternMaterialValues[patternIndex][1] = parameter->m_value;
				}
				else if( std::strcmp( suffix, "Gloss" ) == 0 )
				{
					patternMaterialValues[patternIndex][2] = parameter->m_value;
				}
			}
		}
		const Vector4 noHeatGlow( 0.0f, 0.0f, 0.0f, 0.0f );
		effect.AddParameterVector4( BlueSharedString( prefix + "HeatGlowData" ), &noHeatGlow );
		std::fprintf( stderr, "Authored material Mtl%zu: %s\n", materialIndex + 1, materialNames[materialIndex] );
	}

	const char* patternSuffixes[] = { "DiffuseColor", "FresnelColor", "Gloss" };
	for( size_t materialIndex = 0; materialIndex < 2; ++materialIndex )
	{
		for( size_t parameterIndex = 0; parameterIndex < 3; ++parameterIndex )
		{
			const std::string name = "PMtl" + std::to_string( materialIndex + 1 ) + patternSuffixes[parameterIndex];
			effect.AddParameterVector4( BlueSharedString( name ), &patternMaterialValues[materialIndex][parameterIndex] );
		}
	}

	if( hull->m_opaqueAreas.empty() )
	{
		error = "Astero hull has no opaque SOF areas";
		return false;
	}
	const EveSOFDataHullAreaPtr& hullArea = hull->m_opaqueAreas.front();
	for( const auto& parameter : hullArea->m_parameters )
	{
		effect.AddParameterVector4( parameter->m_name, &parameter->m_value );
	}

	const Color& glow = faction->m_colorSet->m_colors[SOFDataFactionColorChooser::TYPE_HULL];
	const Vector4 glowValue( glow.r, glow.g, glow.b, glow.a );
	effect.AddParameterVector4( BlueSharedString( "GeneralGlowColor" ), &glowValue );
	effect.SetParameter( BlueSharedString( "areaId" ), uint32_t( 0 ) );
	effect.SetParameter( BlueSharedString( "objectId" ), uint32_t( 0 ) );

	for( const auto& texture : hullArea->m_textures )
	{
		effect.AddResourceTexture2D( texture->m_name, texture->m_resFilePath.c_str() );
	}
	effect.AddResourceTexture2D( BlueSharedString( "DustNoiseMap" ), "res:/texture/global/black.dds" );

	std::fprintf(
		stderr,
		"Authored Astero SOF material configured: shader=quad/quadv5.fx textures=%zu glow=(%.3f, %.3f, %.3f)\n",
		hullArea->m_textures.size(),
		glow.r,
		glow.g,
		glow.b );
	return true;
}

bool WriteAsteroClientAssetReport( const char* reportPath )
{
	if( !reportPath || reportPath[0] == '\0' )
	{
		CCP_LOGERR( "Astero client asset inspection requires an output path" );
		return false;
	}

	std::ofstream output( reportPath );
	if( !output )
	{
		CCP_LOGERR( "Unable to create Astero client asset report '%s'", reportPath );
		return false;
	}

	const char* hullPath = "res:/dx9/model/spaceobjectfactory/hulls/soef1_t1.black";
	const char* factionPath = "res:/dx9/model/spaceobjectfactory/factions/soebase.black";
	const char* effectPath = "res:/graphics/effect/managed/space/spaceobject/v5/quad/quadv5.fx";
	const char* materialNames[] = {
		"chrome_metallic", "grey_steel_brushed", "white_ghost_matt", "red_crimson_enamel", "glass_soe"
	};

	std::string hullError;
	std::string factionError;
	auto hull = LoadBlackObjectWithoutYield<EveSOFDataHull>( hullPath, hullError );
	auto faction = LoadBlackObjectWithoutYield<EveSOFDataFaction>( factionPath, factionError );

	output << "# Astero Client Asset Compatibility Report\n\n";
	output << "Generated by the macOS Trinity runtime using Carbon `BlackReader` and Trinity `Tr2EffectRes`.\n\n";
	output << "## Load Status\n\n";
	output << "- Hull Black: " << ( hull ? "loaded" : "FAILED" ) << " (`" << hullPath << "`)\n";
	output << "- Faction Black: " << ( faction ? "loaded" : "FAILED" ) << " (`" << factionPath << "`)\n";
	if( !hullError.empty() )
	{
		output << "- Hull error: `" << hullError << "`\n";
	}
	if( !factionError.empty() )
	{
		output << "- Faction error: `" << factionError << "`\n";
	}

	if( !hull || !faction )
	{
		output << "\nTyped SOF decoding did not complete; effect inspection was not attempted.\n";
		return false;
	}

	output << "\n## Hull\n\n";
	output << "- Name: `" << hull->m_name << "`\n";
	output << "- Geometry: `" << hull->m_geometryResFilePath << "`\n";
	output << "- Category: `" << hull->m_category.c_str() << "`\n";
	output << "- SOF6: " << ( hull->m_sof6 ? "yes" : "no" ) << "\n";
	output << "- Skinned: " << ( hull->m_isSkinned ? "yes" : "no" ) << "\n";
	output << "- Cast shadow: " << ( hull->m_castShadow ? "yes" : "no" ) << "\n";
	output << "- Auxiliary sets: decals=" << hull->m_decalSets.size()
		   << ", sprites=" << hull->m_spriteSets.size()
		   << ", spotlights=" << hull->m_spotlightSets.size()
		   << ", planes=" << hull->m_planeSets.size()
		   << ", lights=" << hull->m_lightSets.size() << "\n\n";

	WriteAreaList( output, "Opaque", hull->m_opaqueAreas );
	WriteAreaList( output, "Decal", hull->m_decalAreas );
	WriteAreaList( output, "Transparent", hull->m_transparentAreas );
	WriteAreaList( output, "Additive", hull->m_additiveAreas );
	WriteAreaList( output, "Distortion", hull->m_distortionAreas );

	output << "## Faction Material Assignment\n\n";
	output << "- Faction: `" << faction->m_name << "`\n";
	output << "- Material usage: " << faction->m_materialUsageMtl1 << ", " << faction->m_materialUsageMtl2
		   << ", " << faction->m_materialUsageMtl3 << ", " << faction->m_materialUsageMtl4 << "\n\n";
	output << "| Area type | Material 1 | Material 2 | Material 3 | Material 4 | Glow color type |\n";
	output << "|---|---|---|---|---|---:|\n";
	if( faction->m_areaTypes )
	{
		for( int type = 0; type < EveSOFDataArea::TYPE_MAX; ++type )
		{
			const auto& materials = faction->m_areaTypes->m_materials[type];
			if( !materials )
			{
				continue;
			}
			output << "| " << AreaTypeName( static_cast<EveSOFDataArea::AreaType>( type ) );
			for( int material = 0; material < EveSOFDataAreaMaterial::MATERIAL_MAX; ++material )
			{
				output << " | `" << materials->m_material[material] << "`";
			}
			output << " | " << static_cast<int>( materials->m_glowColorType ) << " |\n";
		}
	}

	output << "\n### Selected faction colors\n\n";
	if( faction->m_colorSet )
	{
		struct NamedColor
		{
			const char* name;
			SOFDataFactionColorChooser::ColorType type;
		};
		const NamedColor colors[] = {
			{ "Primary", SOFDataFactionColorChooser::TYPE_PRIMARY },
			{ "Secondary", SOFDataFactionColorChooser::TYPE_SECONDARY },
			{ "Tertiary", SOFDataFactionColorChooser::TYPE_TERTIARY },
			{ "White", SOFDataFactionColorChooser::TYPE_WHITE },
			{ "Red", SOFDataFactionColorChooser::TYPE_RED },
			{ "Hull", SOFDataFactionColorChooser::TYPE_HULL },
			{ "Glass", SOFDataFactionColorChooser::TYPE_GLASS },
			{ "DarkHull", SOFDataFactionColorChooser::TYPE_DARKHULL },
			{ "Booster", SOFDataFactionColorChooser::TYPE_BOOSTER },
			{ "PrimaryLight", SOFDataFactionColorChooser::TYPE_PRIMARY_LIGHT },
		};
		for( const NamedColor& color : colors )
		{
			output << "- " << color.name << ": (";
			WriteColor( output, faction->m_colorSet->m_colors[color.type] );
			output << ")\n";
		}
	}

	output << "\n## Material Parameters\n\n";
	bool materialsLoaded = true;
	for( const char* materialName : materialNames )
	{
		const std::string path = std::string( "res:/dx9/model/spaceobjectfactory/materials/" ) + materialName + ".black";
		std::string materialError;
		auto material = LoadBlackObjectWithoutYield<EveSOFDataMaterial>( path.c_str(), materialError );
		output << "### `" << materialName << "`\n\n";
		if( !material )
		{
			output << "Load failed: `" << materialError << "`.\n\n";
			materialsLoaded = false;
			continue;
		}
		for( const auto& parameter : material->m_parameters )
		{
			output << "- `" << parameter->m_name.c_str() << "` = (";
			WriteVector4( output, parameter->m_value );
			output << ")\n";
		}
		output << "\n";
	}

	Tr2EffectPtr effect;
	effect.CreateInstance();
	effect->SetEffectPathName( effectPath );
	Tr2EffectRes* effectResource = effect->GetEffectRes();
	if( effectResource )
	{
		effectResource->ForceSynchronousLoad();
		effectResource->Reload();
	}
	Tr2Shader* shader = effect->GetShaderStateInterface();
	const bool effectLoaded = effectResource && effectResource->IsGood() && shader;

	output << "## Authored `quadv5` Effect\n\n";
	output << "- Logical source path: `" << effectPath << "`\n";
	output << "- Compiled path: `res:/graphics/effect.metal/managed/space/spaceobject/v5/quad/quadv5.sm_hi`\n";
	output << "- Runtime load: " << ( effectLoaded ? "success" : "FAILED" ) << "\n";
	if( !effectLoaded )
	{
		return false;
	}

	output << "- Container format version: " << effectResource->GetFormatVersion() << "\n";
	output << "- Permutation dimensions: " << effectResource->GetPermutations().size() << "\n\n";
	for( const Tr2ShaderPermutation& permutation : effectResource->GetPermutations() )
	{
		output << "- `" << permutation.name.c_str() << "` (default=";
		if( permutation.defaultOption < permutation.options.size() )
		{
			output << '`' << permutation.options[permutation.defaultOption].c_str() << '`';
		}
		else
		{
			output << permutation.defaultOption;
		}
		output << "): ";
		for( size_t i = 0; i < permutation.options.size(); ++i )
		{
			output << ( i == 0 ? "" : ", " ) << '`' << permutation.options[i].c_str() << '`';
		}
		output << "\n";
	}

	const std::set<std::pair<int, int>> bridgeVertexInputs = {
		{ Tr2VertexDefinition::POSITION, 0 },
		{ Tr2VertexDefinition::NORMAL, 0 },
		{ Tr2VertexDefinition::TANGENT, 0 },
		{ Tr2VertexDefinition::TEXCOORD, 0 },
		{ Tr2VertexDefinition::COLOR, 0 },
	};
	std::set<std::pair<int, int>> effectVertexInputs;
	std::set<std::string> explicitConstants;
	std::set<std::string> automaticConstants;
	std::set<std::string> explicitResources;
	std::set<std::string> automaticResources;

	const Tr2EffectDescription& description = shader->GetEffectDescription();
	output << "\n### Techniques (" << description.techniques.size() << ")\n\n";
	for( const Tr2EffectTechnique& technique : description.techniques )
	{
		output << "- `" << technique.name.c_str() << "`: passes=" << technique.passes.size() << "\n";
		for( size_t passIndex = 0; passIndex < technique.passes.size(); ++passIndex )
		{
			const Tr2Pass& pass = technique.passes[passIndex];
			for( int stageIndex = 0; stageIndex < Tr2RenderContextEnum::SHADER_TYPE_COUNT; ++stageIndex )
			{
				const auto stageType = static_cast<Tr2RenderContextEnum::ShaderType>( stageIndex );
				const Tr2EffectStageInput& stage = pass.stageInputs[stageIndex];
				if( !stage.m_exists )
				{
					continue;
				}
				output << "  - pass " << passIndex << " " << ShaderStageName( stageType )
					   << ": constants=" << stage.constants.size()
					   << ", resources=" << stage.resources.size()
					   << ", samplers=" << stage.samplers.size()
					   << ", inputs=" << stage.signature.pipelineInputs.size() << "\n";
				for( const Tr2EffectConstant& constant : stage.constants )
				{
					( constant.isAutoregister ? automaticConstants : explicitConstants ).insert( constant.name.c_str() );
				}
				for( const auto& resource : stage.resources )
				{
					( resource.second.isAutoregister ? automaticResources : explicitResources ).insert( resource.second.name );
				}
				if( stageType == Tr2RenderContextEnum::VERTEX_SHADER )
				{
					for( const Tr2ShaderPipelineInputAL& input : stage.signature.pipelineInputs )
					{
						effectVertexInputs.insert( { input.usage, input.usageIndex } );
						output << "    - input `" << VertexUsageName( input.usage ) << input.usageIndex
							   << "`: " << PipelineInputTypeName( input.type ) << input.dimension
							   << ", attribute=" << input.registerIndex << ", mask=0x"
							   << std::hex << input.usedMask << std::dec << "\n";
					}
				}
			}
		}
	}

	auto WriteNames = [&]( const char* title, const std::set<std::string>& names ) {
		output << "\n### " << title << " (" << names.size() << ")\n\n";
		for( const std::string& name : names )
		{
			output << "- `" << name << "`\n";
		}
	};
	WriteNames( "Explicit constants", explicitConstants );
	WriteNames( "Automatic Trinity constants", automaticConstants );
	WriteNames( "Explicit resources", explicitResources );
	WriteNames( "Automatic Trinity resources", automaticResources );

	output << "\n## Existing Bridge Compatibility\n\n";
	output << "The current bridge provides `POSITION0`, `NORMAL0`, `TANGENT0`, `TEXCOORD0`, and `COLOR0`.\n\n";
	bool missingVertexInputs = false;
	for( const auto& input : effectVertexInputs )
	{
		const auto usage = static_cast<Tr2VertexDefinition::UsageCode>( input.first );
		const bool available = bridgeVertexInputs.count( input ) != 0;
		output << "- `" << VertexUsageName( usage ) << input.second << "`: " << ( available ? "available" : "MISSING" ) << "\n";
		missingVertexInputs = missingVertexInputs || !available;
	}
	output << "\n- Typed Black/SOF decode: compatible\n";
	output << "- Compiled Metal effect container: compatible\n";
	output << "- Extracted material Black files: " << ( materialsLoaded ? "compatible" : "incomplete" ) << "\n";
	output << "- Existing bridge vertex declaration: " << ( missingVertexInputs ? "requires expansion" : "compatible with default permutation" ) << "\n";
	output << "- Material/global bindings: require the explicit and automatic inputs inventoried above\n";

	return materialsLoaded;
}

bool DrawShellFrame( Tr2RenderContext& renderContext )
{
	using namespace Tr2RenderContextEnum;

	Tr2Renderer::BeginFrame();
	bool ok = CheckHresult( Tr2Renderer::BeginRenderContext(), "Tr2Renderer::BeginRenderContext" );
	if( ok )
	{
		ok = CheckHresult( renderContext.SetRenderTarget( renderContext.GetDefaultBackBuffer() ), "SetRenderTarget" ) && ok;
		ok = CheckHresult( renderContext.SetDepthStencil( Tr2TextureAL() ), "SetDepthStencil" ) && ok;
		ok = CheckHresult( renderContext.Clear( CLEARFLAGS_TARGET, 0xff000000, 1.0f ), "Clear" ) && ok;
		ok = CheckHresult( Tr2Renderer::EndRenderContext(), "Tr2Renderer::EndRenderContext" ) && ok;
	}
	Tr2Renderer::EndFrame();

	if( !ok )
	{
		return false;
	}
	return CheckHresult( renderContext.Present(), "Present" );
}

bool DrawDriverFrame( Tr2RenderContext& renderContext, EveSpaceSceneRenderDriver& driver, Be::Time realTime, Be::Time simTime )
{
	const Tr2TextureAL& destination = renderContext.GetDefaultBackBuffer();
	const Tr2BitmapDimensions destinationDimensions = destination.GetDesc();

	ITr2RenderNode::Span<const Tr2BitmapDimensions> destinationDimensionSpan = { &destinationDimensions, 1 };
	ITr2RenderNode::Span<const BlueSharedString> requestedOutputs = {};
	if( !driver.Validate( destinationDimensionSpan, requestedOutputs, realTime, simTime ) )
	{
		CCP_LOGERR( "EveSpaceSceneRenderDriver::Validate failed" );
		return false;
	}

	Tr2Renderer::BeginFrame();
	bool ok = CheckHresult( Tr2Renderer::BeginRenderContext(), "Tr2Renderer::BeginRenderContext" );
	if( ok )
	{
		const Tr2TextureAL* destinationTexture = &destination;
		ITr2RenderNode::Span<const Tr2TextureAL> destinationSpan = { destinationTexture, 1 };
		ITr2RenderNode::Span<ITr2RenderNode::TempOutput> outputSpan = {};
		Tr2ProfileTimer rootTimer;
		driver.Execute( destinationSpan, outputSpan, realTime, simTime, rootTimer, renderContext );
		ok = CheckHresult( Tr2Renderer::EndRenderContext(), "Tr2Renderer::EndRenderContext" ) && ok;
	}
	Tr2Renderer::EndFrame();

	if( !ok )
	{
		return false;
	}
	return CheckHresult( renderContext.Present(), "Present" );
}

bool ConfigureDriverScene( StandaloneProbe& probe, int qualityRung, const char* assetPath, int materialView, int materialMode, const char* sceneResourcePath, int sceneFixture )
{
	if( qualityRung >= STANDALONE_PROBE_RUNG_HDR_BLIT )
	{
		struct RequiredEffect
		{
			const char* sourcePath;
			const wchar_t* compiledPath;
		};
		const RequiredEffect requiredEffects[] = {
			{ "res:/Graphics/Effect/Managed/Space/PostProcess/ToneMapping.fx", L"res:/graphics/effect.metal/managed/space/postprocess/tonemapping.sm_hi" },
			{ "res:/Graphics/Effect/Managed/Space/System/Blit.fx", L"res:/graphics/effect.metal/managed/space/system/blit.sm_hi" },
			{ "res:/Graphics/Effect/Managed/Space/System/BlitFiltered.fx", L"res:/graphics/effect.metal/managed/space/system/blitfiltered.sm_hi" },
		};
		for( const RequiredEffect& required : requiredEffects )
		{
			if( !BePaths->FileExistsLocally( required.compiledPath ) )
			{
				std::fprintf( stderr, "HDR/post rung requires compiled effect: %ls\n", required.compiledPath );
				CCP_LOGERR( "HDR/post rung requires compiled effect: %S", required.compiledPath );
				return false;
			}

			Tr2EffectPtr effect;
			effect.CreateInstance();
			effect->SetEffectPathName( required.sourcePath );
			Tr2EffectRes* resource = effect->GetEffectRes();
			if( resource )
			{
				resource->ForceSynchronousLoad();
				resource->Reload();
			}
			Tr2Shader* shader = effect->GetShaderStateInterface();
			if( !resource || !resource->IsGood() || !shader || shader->GetPassCount( 0 ) == 0 )
			{
				std::fprintf( stderr, "HDR/post effect failed to prepare: %s\n", required.sourcePath );
				CCP_LOGERR( "HDR/post effect failed to prepare: %s", required.sourcePath );
				return false;
			}
			std::fprintf( stderr, "HDR/post effect ready: %s passes=%u\n", required.sourcePath, shader->GetPassCount( 0 ) );
		}

		if( qualityRung >= STANDALONE_PROBE_RUNG_HDR_EXPOSURE )
		{
			const RequiredEffect exposureEffects[] = {
				{ "res:/Graphics/Effect/Managed/Space/PostProcess/CreateHistograms.fx", L"res:/graphics/effect.metal/managed/space/postprocess/createhistograms.sm_hi" },
				{ "res:/Graphics/Effect/Managed/Space/PostProcess/MergeHistograms.fx", L"res:/graphics/effect.metal/managed/space/postprocess/mergehistograms.sm_hi" },
				{ "res:/Graphics/Effect/Managed/Space/PostProcess/MeasureExposure.fx", L"res:/graphics/effect.metal/managed/space/postprocess/measureexposure.sm_hi" },
			};
			for( const RequiredEffect& required : exposureEffects )
			{
				if( !BePaths->FileExistsLocally( required.compiledPath ) )
				{
					CCP_LOGERR( "Dynamic exposure requires compiled effect: %S", required.compiledPath );
					return false;
				}
				Tr2EffectPtr effect;
				effect.CreateInstance();
				effect->SetEffectPathName( required.sourcePath );
				Tr2EffectRes* resource = effect->GetEffectRes();
				if( resource )
				{
					resource->ForceSynchronousLoad();
					resource->Reload();
				}
				Tr2Shader* shader = effect->GetShaderStateInterface();
				if( !resource || !resource->IsGood() || !shader || shader->GetPassCount( 0 ) == 0 )
				{
					CCP_LOGERR( "Dynamic exposure effect failed to prepare: %s", required.sourcePath );
					return false;
				}
				std::fprintf( stderr, "Dynamic exposure effect ready: %s passes=%u\n", required.sourcePath, shader->GetPassCount( 0 ) );
			}
		}

		TriTextureResPtr neutralTexture;
		BeResMan->GetResource( "res:/texture/global/black.dds", "", neutralTexture );
		if( neutralTexture )
		{
			neutralTexture->ForceSynchronousLoad();
			neutralTexture->Reload();
		}
		if( !neutralTexture || !neutralTexture->IsGood() )
		{
			CCP_LOGERR( "HDR/post neutral texture failed to prepare: res:/texture/global/black.dds" );
			return false;
		}
		std::fprintf( stderr, "HDR/post neutral texture ready: res:/texture/global/black.dds\n" );
	}

	if( sceneResourcePath && sceneResourcePath[0] != '\0' )
	{
		std::string sceneError;
		probe.scene = LoadBlackObjectWithoutYield<EveSpaceScene>(
			sceneResourcePath,
			sceneError );
		if( !probe.scene )
		{
			std::fprintf( stderr, "Failed to load EVE scene '%s': %s\n", sceneResourcePath, sceneError.c_str() );
			return false;
		}
		if( sceneFixture == 3 && !ConfigureNewEdenSystem( *probe.scene, sceneError ) )
		{
			std::fprintf( stderr, "Failed to configure New Eden system: %s\n", sceneError.c_str() );
			return false;
		}
		if( !PrepareSceneBackgroundWithoutYield( *probe.scene, sceneResourcePath, sceneError ) )
		{
			std::fprintf( stderr, "Failed to prepare EVE scene '%s': %s\n", sceneResourcePath, sceneError.c_str() );
			return false;
		}

		if( qualityRung >= STANDALONE_PROBE_RUNG_HDR_EXPOSURE )
		{
			std::string postProcessError;
			auto postProcess = LoadBlackObjectWithoutYield<Tr2PostProcess2>(
				"res:/dx9/default/postprocess.black",
				postProcessError );
			if( !postProcess )
			{
				std::fprintf( stderr, "Failed to load EVE default postprocess: %s\n", postProcessError.c_str() );
				return false;
			}
			postProcess->SetBloom( nullptr );
			postProcess->SetFilmGrain( nullptr );
			postProcess->SetTaa( nullptr );
			auto exposure = postProcess->GetDynamicExposureIfAvailable();
			if( !exposure || !postProcess->GetTonemappingIfAvailable() )
			{
				CCP_LOGERR( "EVE default postprocess does not provide dynamic exposure and tone mapping" );
				return false;
			}
			probe.scene->m_sceneDefaultPostProcess = postProcess;
			std::fprintf(
				stderr,
				"EVE dynamic exposure active: middle=%.4f influence=%.4f adjustment=%.4f range=[%.4f, %.4f]\n",
				exposure->m_middleValue,
				exposure->m_influence,
				exposure->m_adjustment,
				exposure->m_minExposure,
				exposure->m_maxExposure );
		}
	}
	else if( !probe.scene.CreateInstance() )
	{
		CCP_LOGERR( "Failed to create EveSpaceScene" );
		return false;
	}
	if( !probe.scene->Initialize() )
	{
		CCP_LOGERR( "Failed to initialize EveSpaceScene" );
		return false;
	}
	if( !probe.driver.CreateInstance() )
	{
		CCP_LOGERR( "Failed to create EveSpaceSceneRenderDriver" );
		return false;
	}
	if( !probe.view.CreateInstance() )
	{
		CCP_LOGERR( "Failed to create TriView" );
		return false;
	}
	if( !probe.projection.CreateInstance() )
	{
		CCP_LOGERR( "Failed to create TriProjection" );
		return false;
	}

	const float aspect = probe.renderHeight > 0 ? static_cast<float>( probe.renderWidth ) / static_cast<float>( probe.renderHeight ) : 1.0f;
	probe.view->SetLookAtPosition( Vector3( 0.0f, 0.0f, -5.2f ), Vector3( 0.0f, 0.0f, 0.0f ), Vector3( 0.0f, 1.0f, 0.0f ) );
	probe.projection->PerspectiveFov( 60.0f * 3.1415926535f / 180.0f, aspect, 1.0f, 10000.0f );

	EveSpaceSceneRenderDriver::Settings settings;
	settings.clearColor = Color( 0.0f, 0.0f, 0.0f, 1.0f );
	settings.enableUpscaling = false;
	settings.bypassPostProcessing = qualityRung < STANDALONE_PROBE_RUNG_HDR_BLIT;
	settings.postProcessBlitOnly = qualityRung == STANDALONE_PROBE_RUNG_HDR_BLIT;
	settings.enableDistortion = false;
	settings.shadowQuality = ShadowQuality::SHADOW_DISABLED;
	settings.antiAliasingQuality = EveSpaceSceneRenderDriver::AntiAliasingQuality::Disabled;
	settings.aoQuality = EveSpaceSceneRenderDriver::AmbientOcclusionQuality::Disabled;
	settings.volumetricQuality = Tr2VolumerticQuality::Low;
	settings.postProcessingQuality = qualityRung >= STANDALONE_PROBE_RUNG_HDR_POST ? PostProcess::HIGH : PostProcess::LOW;
	settings.forceNormalMap = qualityRung >= STANDALONE_PROBE_RUNG_MODEL;
	settings.forceOpaqueBuffer = false;
	settings.forceVelocityMap = false;

	probe.driver->SetSettings( settings );
	probe.driver->SetScene( probe.scene );
	probe.driver->SetView( probe.view );
	probe.driver->SetProjection( probe.projection );

	if( qualityRung >= STANDALONE_PROBE_RUNG_MODEL )
	{
		if( !assetPath || assetPath[0] == '\0' )
		{
			CCP_LOGERR( "EVE model rung requires a CMF asset path" );
			return false;
		}
		if( !probe.renderable.CreateInstance() )
		{
			CCP_LOGERR( "Failed to create the standalone EVE renderable" );
			return false;
		}
		std::string loadError;
		if( !probe.renderable->LoadAsset( assetPath, aspect, loadError ) )
		{
			std::fprintf( stderr, "Failed to load standalone CMF model '%s': %s\n", assetPath, loadError.c_str() );
			CCP_LOGERR( "Failed to load standalone CMF model '%s': %s", assetPath, loadError.c_str() );
			return false;
		}
		Tr2PrimaryRenderContext& primaryRenderContext = Tr2RenderContext_GetMainThreadRenderContext();
		if( !probe.renderable->InitializeGpu( primaryRenderContext, materialView, materialMode ) )
		{
			CCP_LOGERR( "Failed to initialize the standalone EVE renderable GPU resources" );
			return false;
		}
		probe.scene->Objects().Insert( -1, probe.renderable->GetRawRoot() );
	}
	return true;
}

Tr2WindowHandle ToWindowHandle( void* windowHandle )
{
#ifdef __APPLE__
	return (__bridge id)windowHandle;
#else
	return reinterpret_cast<Tr2WindowHandle>( windowHandle );
#endif
}

}

TRINITY_STANDALONE_EXPORT bool TrinityStandaloneProbeStartup( int argc, const char* const* argv, const char* executableDirectory )
{
	static bool started = false;
	if( started )
	{
		return true;
	}

	std::vector<std::wstring> startupArgs;
	startupArgs.reserve( static_cast<size_t>( argc ) );
	for( int i = 0; i < argc; ++i )
	{
		startupArgs.push_back( ToWide( argv[i] ) );
	}

#if BLUE_WITH_PYTHON
	if( !Py_IsInitialized() )
	{
		Py_Initialize();
	}
#endif

	BlueModuleStartup();
	BlueSetStartupArgs( startupArgs );

	const std::string executablePath = executableDirectory && executableDirectory[0] != '\0' ? executableDirectory : ".";
	if( !BlueInitializePaths( ToWide( executablePath.c_str() ) ) )
	{
		CCP_LOGERR( "BlueInitializePaths failed for TrinityStandaloneProbe" );
		return false;
	}

	{
		const std::string assetDirectory = executablePath + "/Assets";
		std::vector<std::wstring> searchPaths;
		searchPaths.push_back( ToWide( ( "root=" + executablePath ).c_str() ) );
		searchPaths.push_back( ToWide( ( "bin=" + executablePath ).c_str() ) );
		searchPaths.push_back( ToWide( ( "res=" + assetDirectory ).c_str() ) );
		if( !BlueSetSearchPaths( searchPaths ) )
		{
			CCP_LOGERR( "BlueSetSearchPaths failed for TrinityStandaloneProbe" );
			return false;
		}
	}

	if( !BeOS )
	{
		CCP_LOGERR( "BlueModuleStartup did not initialize BeOS for TrinityStandaloneProbe" );
		return false;
	}
	if( !BlueInitializeResourceLoading() )
	{
		CCP_LOGERR( "BlueInitializeResourceLoading failed for TrinityStandaloneProbe" );
		return false;
	}

	TrinityStandaloneStartup();
	started = true;
	return true;
}

TRINITY_STANDALONE_EXPORT void* TrinityStandaloneProbeCreateDevice( void* windowHandle, uint32_t renderWidth, uint32_t renderHeight )
{
	auto* probe = CCP_NEW( "TrinityStandaloneProbe" ) StandaloneProbe();
	probe->renderWidth = renderWidth;
	probe->renderHeight = renderHeight;

	if( !probe->device.CreateInstance() )
	{
		CCP_LOGERR( "Failed to create TriDevice for TrinityStandaloneProbe" );
		delete probe;
		return nullptr;
	}

	if( !probe->device->CreateSimpleDevice( ToWindowHandle( windowHandle ), renderWidth, renderHeight, TriDevice::WINDOWED, Tr2RenderContextEnum::PRESENT_INTERVAL_ONE ) )
	{
		CCP_LOGERR( "TriDevice::CreateSimpleDevice failed for TrinityStandaloneProbe" );
		delete probe;
		return nullptr;
	}

	probe->renderContext = probe->device->GetRenderContext();
	if( !probe->renderContext )
	{
		CCP_LOGERR( "TriDevice did not expose a render context for TrinityStandaloneProbe" );
		delete probe;
		return nullptr;
	}

	return probe;
}

TRINITY_STANDALONE_EXPORT void TrinityStandaloneProbeDestroyDevice( void* opaqueProbe )
{
	auto* probe = static_cast<StandaloneProbe*>( opaqueProbe );
	delete probe;
}

TRINITY_STANDALONE_EXPORT bool TrinityStandaloneProbeInspectClientAssets( void* opaqueProbe, const char* reportPath )
{
	auto* probe = static_cast<StandaloneProbe*>( opaqueProbe );
	if( !probe || !probe->renderContext )
	{
		CCP_LOGERR( "Client asset inspection requires an initialized render device" );
		return false;
	}
	return WriteAsteroClientAssetReport( reportPath );
}

TRINITY_STANDALONE_EXPORT bool TrinityStandaloneProbeCreateEveScene( void* opaqueProbe, int qualityRung, const char* assetPath, int materialView, int materialMode, const char* sceneResourcePath, int sceneFixture )
{
	auto* probe = static_cast<StandaloneProbe*>( opaqueProbe );
	if( !probe )
	{
		return false;
	}
	return ConfigureDriverScene( *probe, qualityRung, assetPath, materialView, materialMode, sceneResourcePath, sceneFixture );
}

TRINITY_STANDALONE_EXPORT bool TrinityStandaloneProbeRenderFrame( void* opaqueProbe, int qualityRung, int64_t realTime, int64_t simTime )
{
	auto* probe = static_cast<StandaloneProbe*>( opaqueProbe );
	if( !probe || !probe->renderContext )
	{
		return false;
	}

	if( qualityRung <= STANDALONE_PROBE_RUNG_SHELL )
	{
		return DrawShellFrame( *probe->renderContext );
	}
	if( !probe->driver )
	{
		CCP_LOGERR( "TrinityStandaloneProbeRenderFrame called without an EVE scene driver" );
		return false;
	}
	probe->device->SetAnimationTime( static_cast<float>( simTime ) / 10000000.0f );
	const bool rendered = DrawDriverFrame( *probe->renderContext, *probe->driver, static_cast<Be::Time>( realTime ), static_cast<Be::Time>( simTime ) );
	return rendered && ( !probe->renderable || !probe->renderable->DrawFailed() );
}
