// Copyright (c) 2026 CCP Games

#include "StdAfx.h"

#include <Blue.h>

#include "Eve/EveSpaceScene.h"
#include "Eve/EveSpaceSceneRenderDriver.h"
#include "Eve/EveEntity.h"
#include "Eve/EveLensflare.h"
#include "Eve/EveOccluder.h"
#include "Eve/EveStarfield.h"
#include "Eve/IEveSpaceObject2.h"
#include "Eve/SpaceObjectFactory/EveSOFData.h"
#include "Eve/SpaceObjectFactory/EveSOFDataMgr.h"
#include "Eve/SpaceObject/Attachments/Sets/EveBannerSet.h"
#include "Eve/SpaceObject/Attachments/Sets/EveHazeSet.h"
#include "Eve/SpaceObject/Attachments/Sets/EvePlaneSet.h"
#include "Eve/SpaceObject/Attachments/Sets/EveSpotlightSet.h"
#include "Eve/SpaceObject/Attachments/Sets/EveSpriteLineSet.h"
#include "Eve/SpaceObject/Attachments/Sets/EveSpriteLineSetItem.h"
#include "Eve/SpaceObject/Attachments/Sets/EveSpriteSet.h"
#include "Eve/SpaceObject/Children/EveChildContainer.h"
#include "Eve/SpaceObject/Children/EveChildLineSet.h"
#include "Eve/SpaceObject/Children/EveChildParticleSystem.h"
#include "Eve/SpaceObject/Children/EveChildQuad.h"
#include "Eve/SpaceObject/Children/EveChildRef.h"
#include "Eve/SpaceObject/Children/EveChildSocket.h"
#include "ITr2Renderable.h"
#include "Lights/ITr2LightOwner.h"
#include "Lights/Tr2Light.h"
#include "Lights/Tr2PointLight.h"
#include "Lights/Tr2SpotLight.h"
#include "Lights/Tr2TexturedPointLight.h"
#include "PostProcess/Tr2PostProcess2.h"
#include "Tr2Mesh.h"
#include "Tr2ProfileTimer.h"
#include "Tr2MeshBase.h"
#include "Tr2PerObjectData.h"
#include "Tr2Renderer.h"
#include "Tr2ShLightingManager.h"
#include "Resources/TriTextureRes.h"
#include "Resources/TriGeometryRes.h"
#include "Resources/Tr2LightProfileRes.h"
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
#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <functional>
#include <sstream>
#include <set>
#include <string>
#include <vector>

extern "C" void TrinityStandaloneStartup();
extern "C" bool BlueInitializeResourceLoading();
extern bool g_eveSpaceSceneDynamicLighting;
extern bool g_useDynamicLightsShadows;

#ifndef IRootReader_H
#define IRootReader_H
BLUE_INTERFACE( IRootReader ) : public IRoot
{
	virtual IRoot* ReadFromStream( IBlueStream * stream ) = 0;
	virtual bool ReadForCachingFromStream( IBlueStream * stream ) = 0;
	virtual void SetFileName( const wchar_t* name ) = 0;
	virtual void SetDoInitialize( bool value ) = 0;
	virtual void SetTimeSlice( float seconds ) = 0;
	virtual void GetErrorMessage( std::string & message ) = 0;
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
bool ConfigureAsteroEveV5Effect( Tr2Effect& effect, uint32_t sourceGroup, int normalMapMode, std::string& error );
std::string ToNarrowPath( const wchar_t* path );
bool PrepareTextureResourceWithoutYield( const std::string& logicalPath, const char* role, std::string& error );

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
	public ITr2Renderable,
	public ITr2ShLightingReceiver,
	public ITr2LightOwner,
	public EveEntity
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

	bool ConfigureLocalLights( int mode, const EveSOFDataHull& hull, const EveSOFDataFaction& faction, std::string& error )
	{
		m_localLightMode = mode;
		if( mode == 0 )
		{
			std::fprintf( stderr, "Astero local lights disabled\n" );
			return true;
		}
		if( mode == 2 )
		{
			LightData data;
			data.position = Vector3( 0.35f, 0.55f, -1.25f );
			data.radius = 3.5f;
			data.innerRadius = 0.15f;
			data.color = Color( 15.0f, 2.4f, 0.6f, 1.0f );
			data.brightness = 6.0f;
			Tr2PointLightPtr light;
			light.CreateInstance();
			light->SetLightData( data );
			AddDirectLight( light, LIGHT_FAMILY_VALIDATION );
			m_lightStats[LIGHT_FAMILY_VALIDATION].declared = 1;
			m_lightStats[LIGHT_FAMILY_VALIDATION].constructed = 1;
			std::fprintf( stderr, "Synthetic local-light validation point enabled: position=(0.35,0.55,-1.25) radius=3.5\n" );
			return true;
		}
		if( mode != 1 || !faction.m_colorSet || !faction.m_visibilityGroupSet )
		{
			error = mode != 1 ? "Invalid local-light mode" : "Astero faction is missing color or visibility data";
			return false;
		}

		std::set<std::string> visibilityGroups;
		for( const auto& value : faction.m_visibilityGroupSet->m_visibilityGroups )
		{
			visibilityGroups.insert( value->m_str );
		}
		std::fprintf( stderr, "Astero active SOF visibility groups:" );
		for( const auto& value : visibilityGroups )
		{
			std::fprintf( stderr, " %s", value.c_str() );
		}
		std::fprintf( stderr, "\n" );

		auto isActive = [&]( const BlueSharedString& group ) {
			return visibilityGroups.count( group.c_str() ) != 0;
		};
		auto colorFor = [&]( SOFDataFactionColorChooser::ColorType type, Color& color ) {
			const int index = static_cast<int>( type );
			if( index < 0 || index >= SOFDataFactionColorChooser::TYPE_MAX )
			{
				error = "Astero local light has invalid faction color index " + std::to_string( index );
				return false;
			}
			color = faction.m_colorSet->m_colors[index];
			return true;
		};

		uint32_t index = 0;
		for( const auto& setData : hull.m_spriteSets )
		{
			const bool active = isActive( setData->m_visibilityGroup );
			EveSpriteSetPtr set;
			for( const auto& item : setData->m_items )
			{
				if( !item->m_light )
					continue;
				DeclareLight( LIGHT_FAMILY_SPRITE, active );
				if( !active )
					continue;
				Color color;
				if( !colorFor( item->m_colorType, color ) || !ValidateLightProfile( item->m_light->m_lightProfilePath, error ) )
					return false;
				color = Saturate( item->m_intensity * color, item->m_saturation * item->m_light->m_saturation );
				EveSOFDataMgr::PointLightAttachment attachment( *item->m_light );
				LightData data = attachment.AsLightData( color, 1.0f );
				data.position += item->m_position;
				data.boneIndex = item->m_boneIndex;
				NormalizeAuthoredLight( data );
				if( !set )
					set.CreateInstance();
				set->AddLightFromSOF( EveSpriteLight( data, item->m_blinkPhase, item->m_blinkRate, item->m_minScale, item->m_maxScale, index++, item->m_light->m_lightProfilePath ) );
				ConstructLight( LIGHT_FAMILY_SPRITE, data.boneIndex );
			}
			if( set )
				m_spriteLightSets.push_back( set );
		}

		for( const auto& setData : hull.m_spriteLineSets )
		{
			const bool active = isActive( setData->m_visibilityGroup );
			EveSpriteLineSetPtr set;
			for( const auto& item : setData->m_items )
			{
				if( !item->m_light )
					continue;
				EveSpriteLineSetItemPtr runtimeItem;
				runtimeItem.CreateInstance();
				runtimeItem->m_position = item->m_position;
				runtimeItem->m_rotation = item->m_rotation;
				runtimeItem->m_scaling = item->m_scaling;
				runtimeItem->m_spacing = item->m_spacing;
				runtimeItem->m_isCircle = item->m_isCircle;
				const auto positions = runtimeItem->GetPositions();
				for( size_t positionIndex = 0; positionIndex < positions.size(); ++positionIndex )
					DeclareLight( LIGHT_FAMILY_SPRITE_LINE, active );
				if( !active )
					continue;
				Color color;
				if( !colorFor( item->m_colorType, color ) || !ValidateLightProfile( item->m_light->m_lightProfilePath, error ) )
					return false;
				color = Saturate( item->m_intensity * color, item->m_saturation * item->m_light->m_saturation );
				EveSOFDataMgr::PointLightAttachment attachment( *item->m_light );
				LightData data = attachment.AsLightData( color, 1.0f );
				data.boneIndex = item->m_boneIndex;
				uint32_t positionIndex = 0;
				for( const Vector3& position : positions )
				{
					LightData positioned = data;
					positioned.position = item->m_light->m_translation + position + item->m_position;
					positioned.rotation = Normalize( positioned.rotation * item->m_rotation );
					NormalizeAuthoredLight( positioned );
					if( !set )
						set.CreateInstance();
					set->AddLightFromSOF( EveSpriteLight( positioned, item->m_blinkPhase + item->m_blinkPhaseShift * positionIndex++, item->m_blinkRate, item->m_minScale, item->m_maxScale, index++, item->m_light->m_lightProfilePath ) );
					ConstructLight( LIGHT_FAMILY_SPRITE_LINE, positioned.boneIndex );
				}
			}
			if( set )
				m_spriteLineLightSets.push_back( set );
		}

		for( const auto& setData : hull.m_spotlightSets )
		{
			const bool active = isActive( setData->m_visibilityGroup );
			EveSpotlightSetPtr set;
			for( const auto& item : setData->m_items )
			{
				if( !item->m_light )
					continue;
				DeclareLight( LIGHT_FAMILY_SPOTLIGHT, active );
				if( !active )
					continue;
				Color color;
				if( !colorFor( item->m_colorType, color ) || !ValidateLightProfile( item->m_light->m_lightProfilePath, error ) )
					return false;
				Vector3 scale, position;
				Quaternion rotation;
				Decompose( scale, rotation, position, item->m_transform );
				scale = Vector3( std::abs( scale.x ), std::abs( scale.y ), std::abs( scale.z ) );
				const float angle = scale.z > 0.0f ? std::atan( std::max( scale.x, scale.y ) / ( 2.0f * scale.z ) ) * 180.0f / 3.1415926535f : 0.0f;
				color = Saturate( color, item->m_saturation * item->m_light->m_saturation );
				EveSOFDataMgr::SpotLightAttachment attachment( *item->m_light );
				LightData data = attachment.AsLightData( color, scale.z, angle, angle );
				data.position = ( TranslationMatrix( data.position ) * RotationMatrix( rotation ) * TranslationMatrix( position ) ).GetTranslation();
				data.rotation = rotation;
				data.brightness *= item->m_coneIntensity;
				data.boneIndex = item->m_boneIndex;
				NormalizeAuthoredLight( data );
				if( !set )
					set.CreateInstance();
				set->AddLightFromSOF( EveSpotlightLight( data, index++, item->m_light->m_lightProfilePath, item->m_boosterGainInfluence ) );
				ConstructLight( LIGHT_FAMILY_SPOTLIGHT, data.boneIndex );
			}
			if( set )
				m_spotlightLightSets.push_back( set );
		}

		for( const auto& setData : hull.m_planeSets )
		{
			const bool active = isActive( setData->m_visibilityGroup );
			EvePlaneSetPtr set;
			for( const auto& item : setData->m_items )
			{
				for( const auto& rawLight : item->m_lights )
				{
					DeclareLight( LIGHT_FAMILY_PLANE, active );
					if( !active )
						continue;
					Color color;
					if( !colorFor( item->m_colorType, color ) || !ValidateLightProfile( rawLight->m_lightProfilePath, error ) )
						return false;
					color = Saturate( item->m_intensity * color, item->m_saturation * rawLight->m_saturation );
					EveSOFDataMgr::PointLightAttachment attachment( *rawLight );
					const float scale = std::max( item->m_scaling.x, std::max( item->m_scaling.y, item->m_scaling.z ) );
					LightData data = attachment.AsLightData( color, scale );
					data.position = Transform( data.position, RotationMatrix( item->m_rotation ) ) + item->m_position;
					data.rotation = Normalize( data.rotation * item->m_rotation );
					data.boneIndex = item->m_boneIndex;
					NormalizeAuthoredLight( data );
					if( !set )
						set.CreateInstance();
					set->AddLightFromSOF( EvePlaneLight( data, rawLight->m_saturation, index++, rawLight->m_lightProfilePath, static_cast<EveSpaceObjectAttachmentUtils::FadeType>( item->m_blinkMode ), item->m_phase, item->m_rate ) );
					ConstructLight( LIGHT_FAMILY_PLANE, data.boneIndex );
				}
			}
			if( set )
				m_planeLightSets.push_back( set );
		}

		for( const auto& setData : hull.m_hazeSets )
		{
			const bool active = isActive( setData->m_visibilityGroup );
			EveHazeSetPtr set;
			for( const auto& item : setData->m_items )
			{
				for( const auto& rawLight : item->m_lights )
				{
					DeclareLight( LIGHT_FAMILY_HAZE, active );
					if( !active )
						continue;
					Color color;
					if( !colorFor( item->m_colorType, color ) || !ValidateLightProfile( rawLight->m_lightProfilePath, error ) )
						return false;
					color = Saturate( color, rawLight->m_saturation );
					EveSOFDataMgr::PointLightAttachment attachment( *rawLight );
					const float scale = std::max( item->m_scaling.x, std::max( item->m_scaling.y, item->m_scaling.z ) );
					LightData data = attachment.AsLightData( color, scale );
					data.position = Transform( data.position, RotationMatrix( item->m_rotation ) ) + item->m_position;
					data.rotation = Normalize( data.rotation * item->m_rotation );
					data.boneIndex = item->m_boneIndex;
					NormalizeAuthoredLight( data );
					if( !set )
						set.CreateInstance();
					set->AddLightFromSOF( EveHazeSetLight( data, index++, rawLight->m_lightProfilePath, item->m_boosterGainInfluence ) );
					ConstructLight( LIGHT_FAMILY_HAZE, data.boneIndex );
				}
			}
			if( set )
				m_hazeLightSets.push_back( set );
		}

		for( const auto& setData : hull.m_bannerSets )
		{
			const bool active = isActive( setData->m_visibilityGroup );
			EveBannerSetPtr set;
			TriTextureParameterPtr imageMap;
			for( const auto& item : setData->m_banners )
			{
				if( !item->m_light )
					continue;
				DeclareLight( LIGHT_FAMILY_BANNER, active );
				std::fprintf( stderr, "Astero banner descriptor: name=%s usage=%d visibility=%s active=%s\n", item->m_name.c_str(), static_cast<int>( item->m_usage ), setData->m_visibilityGroup.c_str(), active ? "yes" : "no" );
				if( !active )
					continue;
				if( !ValidateLightProfile( item->m_light->m_lightProfilePath, error ) )
					return false;
				if( !set )
				{
					const char* bannerPath = "res:/ui/texture/classes/banners/factional/various/soe_banner_base.dds";
					if( !PrepareTextureResourceWithoutYield( bannerPath, "Astero Sisters of EVE banner", error ) )
						return false;
					set.CreateInstance();
					imageMap.CreateInstance();
					imageMap->SetParameterName( BlueSharedString( "ImageMap" ) );
					imageMap->SetResourcePath( bannerPath );
					set->SetPrimaryTextureParameter( imageMap );
					std::fprintf( stderr, "Astero dynamic banner texture: deterministic SoE fixture=%s client-no-identity-fallback=res:/texture/global/black.dds\n", bannerPath );
				}
				Color color( 0.0f, 0.0f, 0.0f, 0.0f );
				EveSOFDataMgr::PointLightAttachment attachment( *item->m_light );
				const float scale = std::max( item->m_scaling.x, std::max( item->m_scaling.y, item->m_scaling.z ) );
				LightData data = attachment.AsLightData( color, scale );
				data.position = Transform( data.position, RotationMatrix( item->m_rotation ) ) + item->m_position;
				data.rotation = Normalize( data.rotation * item->m_rotation );
				data.boneIndex = item->m_boneIndex;
				NormalizeAuthoredLight( data );
				set->AddLightFromSOF( EveBannerLight( data, item->m_light->m_saturation, index++, item->m_light->m_lightProfilePath ) );
				ConstructLight( LIGHT_FAMILY_BANNER, data.boneIndex );
			}
			if( set )
				m_bannerLightSets.push_back( set );
		}

		for( const auto& setData : hull.m_lightSets )
		{
			const bool active = isActive( setData->m_visibilityGroup );
			for( const auto& item : setData->m_items )
			{
				DeclareLight( LIGHT_FAMILY_EXPLICIT, active );
				if( !active )
					continue;
				const auto& source = item->m_data;
				Color color;
				if( !colorFor( source.lightColor, color ) )
					return false;
				LightData data;
				data.position = source.position;
				data.rotation = source.rotation;
				data.radius = source.radius;
				data.innerRadius = source.innerRadius;
				data.color = color;
				data.brightness = source.brightness;
				data.noiseAmplitude = source.noiseAmplitude;
				data.noiseFrequency = source.noiseFrequency;
				data.noiseOctaves = source.noiseOctaves;
				data.innerAngle = source.innerAngle;
				data.outerAngle = source.outerAngle;
				data.texturePath = source.texturePath;
				data.boneIndex = source.boneIndex;
				data.flags = source.flags;
				NormalizeAuthoredLight( data );
				Tr2LightPtr light;
				if( source.type == EveSOFDataHullLightSetItem::POINT_LIGHT )
				{
					Tr2PointLightPtr point;
					point.CreateInstance();
					light = point;
				}
				else if( source.type == EveSOFDataHullLightSetItem::TEXTURED_POINT_LIGHT )
				{
					if( !PrepareTextureResourceWithoutYield( ToNarrowPath( source.texturePath.c_str() ), "Astero textured local light", error ) )
						return false;
					Tr2TexturedPointLightPtr point;
					point.CreateInstance();
					light = point;
				}
				else if( source.type == EveSOFDataHullLightSetItem::SPOT_LIGHT )
				{
					Tr2SpotLightPtr spot;
					spot.CreateInstance();
					light = spot;
				}
				else
				{
					error = "Unsupported Astero explicit light type " + std::to_string( static_cast<int>( source.type ) );
					return false;
				}
				light->SetLightData( data );
				AddDirectLight( light, LIGHT_FAMILY_EXPLICIT );
				ConstructLight( LIGHT_FAMILY_EXPLICIT, data.boneIndex );
			}
		}

		ReportLightStats( "constructed" );
		if( TotalConstructedLights() == 0 )
		{
			error = "Astero authored-light reconstruction produced zero active lights";
			return false;
		}
		return true;
	}

	bool InitializeGpu( Tr2PrimaryRenderContext & renderContext, int materialView, int materialMode, int areaView, int normalMapMode )
	{
		using namespace Tr2RenderContextEnum;
		m_useEveV5Material = materialMode != 0;
		m_areaView = areaView;
		m_normalMapMode = normalMapMode;
		for( size_t maskIndex = 0; maskIndex < EVE_SPACEOBJECT_CUSTOWMASK_MAX; ++maskIndex )
		{
			m_eveV5PerObjectData.m_vsData.customMaskMatrix[maskIndex] = IdentityMatrix();
		}
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
		const void* vertexData = m_useEveV5Material ? static_cast<const void*>( m_model.EveV5Vertices().data() ) : static_cast<const void*>( m_model.Vertices().data() );
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

		if( m_useEveV5Material )
		{
			for( const TrinityStandaloneCmfSection& section : m_model.Sections() )
			{
				if( section.sourceGroup < m_sectionsByGroup.size() )
				{
					m_sectionsByGroup[section.sourceGroup] = &section;
				}
				std::fprintf( stderr, "CMF area section: name=%s group=%u firstIndex=%u indexCount=%u\n", section.name.c_str(), section.sourceGroup, section.firstIndex, section.indexCount );
			}
			for( uint32_t group = 0; group < m_sectionsByGroup.size(); ++group )
			{
				if( !m_sectionsByGroup[group] )
				{
					std::fprintf( stderr, "Astero CMF is missing authored GR2 group %u\n", group );
					return false;
				}
			}

			if( !InitializeAsteroEffect( m_distortionEffect, 0, "res:/graphics/effect/managed/space/spaceobject/v5/fx/fxdistortionv5.fx", false ) ||
				!InitializeAsteroEffect( m_hullEffect, 1, "res:/graphics/effect/managed/space/spaceobject/v5/quad/quadv5.fx", true ) ||
				!InitializeAsteroEffect( m_boosterEffect, 2, "res:/graphics/effect/managed/space/spaceobject/v5/quad/quadheatv5.fx", true ) )
			{
				return false;
			}
		}
		else
		{
			m_effect.CreateInstance();
			m_effect->StartUpdate();
			m_effect->SetEffectPathName( "res:/graphics/effect/managed/space/spaceobject/probe/asteropbr.fx" );
			Tr2EffectRes* effectResource = m_effect->GetEffectRes();
			if( effectResource )
			{
				effectResource->ForceSynchronousLoad();
				effectResource->Reload();
			}
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
			m_effect->EndUpdate();
			if( !ValidateEffect( *m_effect, "probe", true ) )
			{
				return false;
			}
		}

		UpdateEffectParameters();
		std::fprintf( stderr, "Standalone material contract ready: mode=%s vertices=%u indices=%u sections=%zu areaView=%d\n", m_useEveV5Material ? "eve-v5" : "probe", m_model.VertexCount(), m_model.IndexCount(), m_model.Sections().size(), m_areaView );
		return true;
	}

	void UpdateSyncronous( const EveUpdateContext& updateContext ) override
	{
		m_time = updateContext.GetTime();
		UpdateEffectParameters();
		UpdateAttachmentLights();
	}

	void UpdateAsyncronous( const EveUpdateContext& ) override
	{
	}

	void UpdateVisibility( const EveUpdateContext&, const Matrix& ) override
	{
	}

	void GetRenderables( std::vector<ITr2Renderable*> & renderables, Tr2ImpostorManager* ) override
	{
		renderables.push_back( this );
	}

	bool GetBoundingSphere( Vector4 & sphere, BoundingSphereQuery ) const override
	{
		sphere = Vector4( 0.0f, 0.0f, 0.0f, 2.0f );
		return true;
	}

	void UpdateModelCenterWorldPosition( Vector3 & position, Be::Time ) override
	{
		position = Vector3( 0.0f, 0.0f, 0.0f );
	}

	void GetModelCenterWorldPosition( Vector3 & position ) const override
	{
		position = Vector3( 0.0f, 0.0f, 0.0f );
	}

	bool GetLocalBoundingBox( Vector3 & minimum, Vector3 & maximum ) override
	{
		minimum = Vector3( -1.0f, -1.0f, -1.0f );
		maximum = Vector3( 1.0f, 1.0f, 1.0f );
		return true;
	}

	void GetLocalToWorldTransform( Matrix & transform ) const override
	{
		transform = m_worldTransform;
	}

	void GetBatches( ITriRenderBatchAccumulator * batches, TriBatchType batchType, const Tr2PerObjectData*, Tr2RenderReason ) override
	{
		if( m_useEveV5Material )
		{
			if( batchType == TRIBATCHTYPE_OPAQUE )
			{
				CommitAreaBatch( batches, 1, m_hullEffect, Tr2EffectStateManager::RM_OPAQUE );
				CommitAreaBatch( batches, 2, m_boosterEffect, Tr2EffectStateManager::RM_OPAQUE );
			}
			else if( batchType == TRIBATCHTYPE_DISTORTION )
			{
				CommitAreaBatch( batches, 0, m_distortionEffect, Tr2EffectStateManager::RM_ALPHA_ADDITIVE );
			}
			return;
		}

		if( batchType != TRIBATCHTYPE_OPAQUE || !m_effect || !m_effect->GetShaderStateInterface() )
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
		uint32_t firstIndex = 0;
		uint32_t indexCount = m_model.IndexCount();
		if( m_areaView != 0 )
		{
			const uint32_t requestedGroup = static_cast<uint32_t>( m_areaView - 1 );
			const auto section = std::find_if( m_model.Sections().begin(), m_model.Sections().end(), [requestedGroup]( const TrinityStandaloneCmfSection& candidate ) { return candidate.sourceGroup == requestedGroup; } );
			if( section == m_model.Sections().end() )
			{
				return;
			}
			firstIndex = section->firstIndex;
			indexCount = section->indexCount;
		}
		batch.SetDrawIndexedInstanced( indexCount, 1, firstIndex, 0, 0 );
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

	void SetShLightingEnabled( bool enabled )
	{
		m_shLightingEnabled = enabled;
		if( !enabled )
		{
			ClearShLighting();
		}
	}

	void UpdateShLighting( Tr2ShLightingManager & manager, const EveUpdateContext& ) override
	{
		ClearShLighting();
		if( !m_shLightingEnabled || !m_useEveV5Material )
		{
			return;
		}

		manager.GetLighting(
			Vector3( 0.0f, 0.0f, 0.0f ),
			1.0f,
			0.6f,
			m_eveV5PerObjectData.m_psData.shLightingCoefficients );

		float maximumMagnitude = 0.0f;
		for( size_t index = 0; index < Tr2ShLightingManager::PACKED_COEFFICIENT_COUNT; ++index )
		{
			const Vector4& coefficient = m_eveV5PerObjectData.m_psData.shLightingCoefficients[index];
			maximumMagnitude = std::max( maximumMagnitude, std::abs( coefficient.x ) );
			maximumMagnitude = std::max( maximumMagnitude, std::abs( coefficient.y ) );
			maximumMagnitude = std::max( maximumMagnitude, std::abs( coefficient.z ) );
			if( index != Tr2ShLightingManager::PACKED_COEFFICIENT_COUNT - 1 )
			{
				maximumMagnitude = std::max( maximumMagnitude, std::abs( coefficient.w ) );
			}
		}
		++m_shUpdateCount;
		if( !m_reportedShLighting && ( maximumMagnitude > 0.0f || m_shUpdateCount >= 2 ) )
		{
			std::fprintf( stderr, "Trinity SH receiver coefficients: physicalMax=%e sentinelW=%g status=%s\n", maximumMagnitude, m_eveV5PerObjectData.m_psData.shLightingCoefficients[Tr2ShLightingManager::PACKED_COEFFICIENT_COUNT - 1].w, maximumMagnitude > 0.0f ? "contributing" : "zero-or-culled" );
			for( size_t index = 0; index < Tr2ShLightingManager::PACKED_COEFFICIENT_COUNT; ++index )
			{
				const Vector4& coefficient = m_eveV5PerObjectData.m_psData.shLightingCoefficients[index];
				std::fprintf( stderr, "  sh[%zu]=(%e, %e, %e, %e)\n", index, coefficient.x, coefficient.y, coefficient.z, coefficient.w );
			}
			m_reportedShLighting = true;
		}
	}

	void ClearShLighting() override
	{
		std::memset(
			m_eveV5PerObjectData.m_psData.shLightingCoefficients,
			0,
			sizeof( m_eveV5PerObjectData.m_psData.shLightingCoefficients ) );
	}

	void GetLights( Tr2LightManager & lightManager ) const override
	{
		auto addFamily = [&]( LightFamily family, const auto& lightSet ) {
			const size_t before = lightManager.GetCurrentThreadPendingLightCount();
			lightSet->GetLights( lightManager );
			m_lightStats[family].frustumAccepted += static_cast<uint32_t>( lightManager.GetCurrentThreadPendingLightCount() - before );
		};
		for( const auto& set : m_spriteLightSets )
			addFamily( LIGHT_FAMILY_SPRITE, set );
		for( const auto& set : m_spriteLineLightSets )
			addFamily( LIGHT_FAMILY_SPRITE_LINE, set );
		for( const auto& set : m_spotlightLightSets )
			addFamily( LIGHT_FAMILY_SPOTLIGHT, set );
		for( const auto& set : m_planeLightSets )
			addFamily( LIGHT_FAMILY_PLANE, set );
		for( const auto& set : m_hazeLightSets )
			addFamily( LIGHT_FAMILY_HAZE, set );
		for( const auto& set : m_bannerLightSets )
			addFamily( LIGHT_FAMILY_BANNER, set );
		for( const DirectLight& direct : m_directLights )
		{
			const size_t before = lightManager.GetCurrentThreadPendingLightCount();
			direct.light->AddLight( lightManager, m_worldTransform, 1.0f );
			m_lightStats[direct.family].frustumAccepted += static_cast<uint32_t>( lightManager.GetCurrentThreadPendingLightCount() - before );
		}
		if( !m_reportedAcceptedLights )
		{
			ReportLightStats( "frustum-gather" );
			m_reportedAcceptedLights = true;
		}
	}

private:
	enum LightFamily
	{
		LIGHT_FAMILY_EXPLICIT,
		LIGHT_FAMILY_SPRITE,
		LIGHT_FAMILY_SPRITE_LINE,
		LIGHT_FAMILY_SPOTLIGHT,
		LIGHT_FAMILY_PLANE,
		LIGHT_FAMILY_HAZE,
		LIGHT_FAMILY_BANNER,
		LIGHT_FAMILY_VALIDATION,
		LIGHT_FAMILY_COUNT,
	};

	struct LightStats
	{
		uint32_t declared = 0;
		uint32_t visibilityExcluded = 0;
		uint32_t constructed = 0;
		mutable uint32_t frustumAccepted = 0;
	};

	struct DirectLight
	{
		Tr2LightPtr light;
		LightFamily family;
	};

	static const char* LightFamilyName( LightFamily family )
	{
		static const char* names[] = { "explicit", "sprite", "sprite-line", "spotlight", "plane", "haze", "banner", "validation" };
		return names[family];
	}

	void DeclareLight( LightFamily family, bool active )
	{
		++m_lightStats[family].declared;
		if( !active )
			++m_lightStats[family].visibilityExcluded;
	}

	void ConstructLight( LightFamily family, int32_t boneIndex )
	{
		++m_lightStats[family].constructed;
		if( boneIndex >= 0 )
		{
			std::fprintf( stderr, "Astero authored local light uses static identity bone delta: family=%s bone=%d\n", LightFamilyName( family ), boneIndex );
		}
	}

	void AddDirectLight( Tr2Light * light, LightFamily family )
	{
		m_directLights.push_back( DirectLight{ light, family } );
	}

	uint32_t TotalConstructedLights() const
	{
		uint32_t count = 0;
		for( const LightStats& stats : m_lightStats )
			count += stats.constructed;
		return count;
	}

	void ReportLightStats( const char* phase ) const
	{
		uint32_t declared = 0, excluded = 0, constructed = 0, accepted = 0;
		std::fprintf( stderr, "Astero local-light counts (%s):\n", phase );
		for( int family = 0; family < LIGHT_FAMILY_COUNT; ++family )
		{
			const LightStats& stats = m_lightStats[family];
			declared += stats.declared;
			excluded += stats.visibilityExcluded;
			constructed += stats.constructed;
			accepted += stats.frustumAccepted;
			std::fprintf( stderr, "  %-11s declared=%u visibility-excluded=%u constructed=%u frustum-accepted=%u\n", LightFamilyName( static_cast<LightFamily>( family ) ), stats.declared, stats.visibilityExcluded, stats.constructed, stats.frustumAccepted );
		}
		std::fprintf( stderr, "  total       declared=%u visibility-excluded=%u constructed=%u frustum-accepted=%u\n", declared, excluded, constructed, accepted );
	}

	bool ValidateLightProfile( const std::wstring& path, std::string& error )
	{
		if( path.empty() )
			return true;
		const std::string logicalPath = ToNarrowPath( path.c_str() );
		if( !BePaths->FileExistsLocally( path.c_str() ) )
		{
			error = "Missing authored Astero light profile: " + logicalPath;
			return false;
		}
		Tr2LightProfileResPtr profile;
		BeResMan->GetResource( path, L"lp", profile );
		if( profile )
		{
			profile->ForceSynchronousLoad();
			profile->Reload();
		}
		if( !profile || !profile->IsGood() )
		{
			error = "Failed to prepare authored Astero light profile: " + logicalPath;
			return false;
		}
		std::fprintf( stderr, "Astero authored light profile ready: %s\n", logicalPath.c_str() );
		return true;
	}

	void NormalizeAuthoredLight( LightData & data ) const
	{
		float centerScale[4];
		m_model.GetCenterAndScale( centerScale );
		data.position = ( data.position - Vector3( centerScale[0], centerScale[1], centerScale[2] ) ) * centerScale[3];
		data.radius *= centerScale[3];
		data.innerRadius *= centerScale[3];
		data.castsShadows = PerLightShadowSetting::DISABLED;
		data.isVolumetric = false;
		data.flags &= ~Tr2LightManager::FLAG_CASTS_SHADOWS;
		data.flags &= ~Tr2LightManager::FLAG_IS_VOLUMETRIC;
	}

	void UpdateAttachmentLights()
	{
		for( const auto& set : m_spriteLightSets )
			set->UpdateLights( m_worldTransform, nullptr, 0, 1.0f, 1.0f );
		for( const auto& set : m_spriteLineLightSets )
			set->UpdateLights( m_worldTransform, nullptr, 0, 1.0f, 1.0f );
		for( const auto& set : m_spotlightLightSets )
			set->UpdateLights( m_worldTransform, nullptr, 0, 1.0f, 1.0f );
		for( const auto& set : m_planeLightSets )
			set->UpdateLights( m_worldTransform, nullptr, 0, 1.0f, 1.0f );
		for( const auto& set : m_hazeLightSets )
			set->UpdateLights( m_worldTransform, nullptr, 0, 1.0f, 1.0f );
		for( const auto& set : m_bannerLightSets )
			set->UpdateLights( m_worldTransform, nullptr, 0, 1.0f, 1.0f );
	}

	void RegisterComponents() override
	{
		if( GetComponentRegistry() && TotalConstructedLights() > 0 )
		{
			GetComponentRegistry()->RegisterComponent<ITr2LightOwner>( this );
		}
	}

	bool ValidateEffect( Tr2Effect & effect, const char* label, bool requireDepth )
	{
		Tr2EffectRes* resource = effect.GetEffectRes();
		if( resource )
		{
			resource->ForceSynchronousLoad();
			resource->Reload();
		}
		if( !resource || !resource->IsGood() )
		{
			std::fprintf( stderr, "Astero %s effect resource failed\n", label );
			return false;
		}

		Tr2Shader* shader = effect.GetShaderStateInterface();
		uint32_t mainTechnique = 0;
		uint32_t depthTechnique = 0;
		if( !shader || !shader->GetTechniqueIndex( BlueSharedString( "Main" ), mainTechnique ) ||
			( requireDepth && !shader->GetTechniqueIndex( BlueSharedString( "Depth" ), depthTechnique ) ) )
		{
			std::fprintf( stderr, "Astero %s effect is missing its required techniques\n", label );
			return false;
		}
		if( requireDepth && m_localLightMode != 0 )
		{
			std::fprintf( stderr,
						  "Astero %s local-light shader contract: LightBuffer=%s LightIndexBuffer=%s consumption-gate=%s\n",
						  label,
						  shader->GetResource( "LightBuffer" ) ? "present" : "absent",
						  shader->GetResource( "LightIndexBuffer" ) ? "present" : "absent",
						  shader->GetResource( "LightBuffer" ) && shader->GetResource( "LightIndexBuffer" ) ? "open" : "blocked" );
		}
		std::fprintf( stderr, "Astero %s effect ready: Main=%u Depth=%s\n", label, mainTechnique, requireDepth ? std::to_string( depthTechnique ).c_str() : "n/a" );
		return true;
	}

	bool InitializeAsteroEffect( Tr2EffectPtr & effect, uint32_t sourceGroup, const char* effectPath, bool requireDepth )
	{
		effect.CreateInstance();
		effect->StartUpdate();
		if( requireDepth )
		{
			effect->SetOption( BlueSharedString( "SPACE_OBJECT_CLIPPING" ), BlueSharedString( "SOC_DISABLED" ) );
			effect->SetOption( BlueSharedString( "SPACE_OBJECT_PPT_ENABLED" ), BlueSharedString( "SOPPT_DISABLED" ) );
			effect->SetOption( BlueSharedString( "SPACE_OBJECT_TRANSPARENCY" ), BlueSharedString( "SOT_OPAQUE" ) );
			effect->SetOption( BlueSharedString( "SPACE_OBJECT_INSTANCED_ATTACHMENT" ), BlueSharedString( "SOIA_DISABLED" ) );
		}
		effect->SetEffectPathName( effectPath );
		std::string error;
		if( !ConfigureAsteroEveV5Effect( *effect, sourceGroup, m_normalMapMode, error ) )
		{
			std::fprintf( stderr, "Failed to configure Astero group %u: %s\n", sourceGroup, error.c_str() );
			return false;
		}
		effect->EndUpdate();

		const char* opaqueTextures[] = {
			"AlbedoMap", "NormalMap", "RoughnessMap", "MaterialMap", "GlowMap", "DirtMap", "PaintMaskMap", "DustNoiseMap"
		};
		const char* distortionTextures[] = { "DistortionMap", "Layer2Map" };
		const char** textureNames = requireDepth ? opaqueTextures : distortionTextures;
		const size_t textureCount = requireDepth ? std::size( opaqueTextures ) : std::size( distortionTextures );
		for( size_t textureIndex = 0; textureIndex < textureCount; ++textureIndex )
		{
			const char* textureName = textureNames[textureIndex];
			TriTextureParameterPtr parameter = BlueCastPtr( effect->GetResourceByName( textureName ) );
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
			std::fprintf( stderr, "Astero group %u texture %s: %s path=%ls\n", sourceGroup, textureName, texture && texture->IsGood() ? "ready" : "FAILED", texture ? texture->GetPath() : L"" );
			if( !texture || !texture->IsGood() )
			{
				return false;
			}
		}
		return ValidateEffect( *effect, sourceGroup == 0 ? "distortion" : ( sourceGroup == 1 ? "hull" : "booster" ), requireDepth );
	}

	void CommitAreaBatch( ITriRenderBatchAccumulator * batches, uint32_t sourceGroup, Tr2Effect* effect, Tr2EffectStateManager::RenderingMode renderMode )
	{
		if( sourceGroup >= m_sectionsByGroup.size() || !m_sectionsByGroup[sourceGroup] ||
			!effect || !effect->GetShaderStateInterface() || ( m_areaView != 0 && m_areaView != static_cast<int>( sourceGroup + 1 ) ) )
		{
			return;
		}

		const TrinityStandaloneCmfSection& section = *m_sectionsByGroup[sourceGroup];
		Tr2RenderBatch batch;
		batch.SetMaterial( effect );
		batch.SetGeometry( m_vertexDeclaration, m_vertexBuffer, sizeof( TrinityStandaloneEveV5Vertex ), m_indexBuffer, sizeof( uint32_t ) );
		batch.SetPerObjectData( &m_eveV5PerObjectData );
		batch.SetDrawIndexedInstanced( section.indexCount, 1, section.firstIndex, 0, 0 );
		batch.SetRenderingMode( renderMode );
		batches->Commit( batch );
		if( !m_reportedAreaBatch[sourceGroup] )
		{
			std::fprintf( stderr, "Astero area batch committed: group=%u indices=%u batch=%s\n", sourceGroup, section.indexCount, sourceGroup == 0 ? "distortion" : "opaque" );
			m_reportedAreaBatch[sourceGroup] = true;
		}
	}

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
		if( !m_useEveV5Material && !m_effect )
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
			m_eveV5PerObjectData.m_vsData.clipData = Vector4( 0.0f, 0.0f, 0.0f, 0.0f );
			m_eveV5PerObjectData.m_vsData.ellpsoidRadii = Vector4( -1.0f, -1.0f, -1.0f, 0.0f );
			m_eveV5PerObjectData.m_vsData.ellpsoidCenter = Vector4( 0.0f, 0.0f, 0.0f, 0.0f );
			m_eveV5PerObjectData.m_psData.worldTransform = world;
			m_eveV5PerObjectData.m_psData.worldTransformLast = world;
			m_eveV5PerObjectData.m_psData.invWorldTransform = inverseWorld;
			m_eveV5PerObjectData.m_psData.shipData = m_eveV5PerObjectData.m_vsData.shipData;
			m_eveV5PerObjectData.m_psData.clipSphereCenter = Vector3( 0.0f, 0.0f, 0.0f );
			m_eveV5PerObjectData.m_psData.clipRadiusSq = 0.0f;
			m_eveV5PerObjectData.m_psData.clipRadius2Sq = 0.0f;
			m_eveV5PerObjectData.m_psData.impactDataOffset = 0.0f;
			m_eveV5PerObjectData.m_psData.clipSphereFactor2 = 0.0f;
			m_eveV5PerObjectData.m_psData.clipSphereFactor = 0.0f;
			m_eveV5PerObjectData.m_psData.screenSize = Vector4( 0.0f, 0.0f, 0.0f, 0.0f );
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
	bool m_shLightingEnabled = true;
	bool m_reportedShLighting = false;
	mutable bool m_reportedAcceptedLights = false;
	int m_localLightMode = 0;
	int m_normalMapMode = 0;
	uint32_t m_shUpdateCount = 0;
	int m_areaView = 0;
	Matrix m_worldTransform = IdentityMatrix();
	TrinityStandaloneCmfModel m_model;
	std::array<const TrinityStandaloneCmfSection*, 3> m_sectionsByGroup = {};
	std::array<bool, 3> m_reportedAreaBatch = {};
	StandaloneEveV5PerObjectData m_eveV5PerObjectData;
	Tr2BufferAL m_vertexBuffer;
	Tr2BufferAL m_indexBuffer;
	uint32_t m_vertexDeclaration = Tr2EffectStateManager::UNINITIALIZED_DECLARATION;
	Tr2EffectPtr m_effect;
	Tr2EffectPtr m_hullEffect;
	Tr2EffectPtr m_boosterEffect;
	Tr2EffectPtr m_distortionEffect;
	Tr2TextureAL m_baseColorTexture;
	Tr2TextureAL m_normalTexture;
	Tr2TextureAL m_roughnessTexture;
	Tr2TextureAL m_materialTexture;
	Tr2TextureAL m_glowTexture;
	Tr2TextureAL m_dirtTexture;
	Tr2TextureAL m_maskTexture;
	Tr2TextureAL m_paintMaskTexture;
	std::array<LightStats, LIGHT_FAMILY_COUNT> m_lightStats = {};
	std::vector<EveSpriteSetPtr> m_spriteLightSets;
	std::vector<EveSpriteLineSetPtr> m_spriteLineLightSets;
	std::vector<EveSpotlightSetPtr> m_spotlightLightSets;
	std::vector<EvePlaneSetPtr> m_planeLightSets;
	std::vector<EveHazeSetPtr> m_hazeLightSets;
	std::vector<EveBannerSetPtr> m_bannerLightSets;
	std::vector<DirectLight> m_directLights;
};

TYPEDEF_BLUECLASS( TrinityStandaloneRenderable );
BLUE_DEFINE_NONEXPOSED( TrinityStandaloneRenderable );

const Be::ClassInfo* TrinityStandaloneRenderable::ExposeToBlue(){
	EXPOSURE_BEGIN( TrinityStandaloneRenderable, "" )
		MAP_INTERFACE( IEveSpaceObject2 )
			MAP_INTERFACE( ITr2Renderable )
				MAP_INTERFACE( ITr2ShLightingReceiver )
					MAP_INTERFACE( ITr2LightOwner )
						MAP_INTERFACE( EveEntity )
							EXPOSURE_END()
}

BLUE_CLASS( TrinityStandaloneSecondaryLight ) :
	public IEveSpaceObject2,
	public ITr2SecondaryLightSource
{
public:
	EXPOSE_TO_BLUE();

	TrinityStandaloneSecondaryLight( IRoot* lockobj = nullptr )
	{
	}

	void Configure( const Vector3& position, float radius, const Color& albedo, const Color& emissive )
	{
		m_position = position;
		m_radius = radius;
		m_albedo = albedo;
		m_emissive = emissive;
	}

	void UpdateSyncronous( const EveUpdateContext& ) override
	{
	}
	void UpdateAsyncronous( const EveUpdateContext& ) override
	{
	}
	void UpdateVisibility( const EveUpdateContext&, const Matrix& ) override
	{
	}
	void GetRenderables( std::vector<ITr2Renderable*>&, Tr2ImpostorManager* ) override
	{
	}

	bool GetBoundingSphere( Vector4 & sphere, BoundingSphereQuery ) const override
	{
		sphere = Vector4( m_position.x, m_position.y, m_position.z, m_radius );
		return true;
	}

	void UpdateModelCenterWorldPosition( Vector3 & position, Be::Time ) override
	{
		position = m_position;
	}
	void GetModelCenterWorldPosition( Vector3 & position ) const override
	{
		position = m_position;
	}

	bool GetLocalBoundingBox( Vector3 & minimum, Vector3 & maximum ) override
	{
		minimum = Vector3( -m_radius, -m_radius, -m_radius );
		maximum = Vector3( m_radius, m_radius, m_radius );
		return true;
	}

	void GetLocalToWorldTransform( Matrix & transform ) const override
	{
		transform = TranslationMatrix( m_position );
	}

	void RegisterSecondaryLightSource( Tr2ShLightingManager & manager ) override
	{
		manager.RegisterSecondaryLightSource( &m_position, &m_radius, &m_albedo, &m_emissive );
	}

	void UnregisterSecondaryLightSource( Tr2ShLightingManager & manager ) override
	{
		manager.UnregisterSecondaryLightSource( &m_position );
	}

private:
	Vector3 m_position = Vector3( 0.0f, 0.0f, 0.0f );
	float m_radius = 0.0f;
	Color m_albedo = Color( 0.0f, 0.0f, 0.0f, 1.0f );
	Color m_emissive = Color( 0.0f, 0.0f, 0.0f, 1.0f );
};

TYPEDEF_BLUECLASS( TrinityStandaloneSecondaryLight );
BLUE_DEFINE_NONEXPOSED( TrinityStandaloneSecondaryLight );

const Be::ClassInfo* TrinityStandaloneSecondaryLight::ExposeToBlue()
{
	EXPOSURE_BEGIN( TrinityStandaloneSecondaryLight, "" )
		MAP_INTERFACE( IEveSpaceObject2 )
		MAP_INTERFACE( ITr2SecondaryLightSource )
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

enum StandaloneLightingView
{
	STANDALONE_LIGHTING_COMBINED = 0,
	STANDALONE_LIGHTING_DIRECT = 1,
	STANDALONE_LIGHTING_SH = 2,
	STANDALONE_LIGHTING_LOCAL = 3,
};

enum StandaloneLocalLights
{
	STANDALONE_LOCAL_LIGHTS_OFF = 0,
	STANDALONE_LOCAL_LIGHTS_AUTHORED = 1,
	STANDALONE_LOCAL_LIGHTS_VALIDATION = 2,
};

enum StandaloneShSource
{
	STANDALONE_SH_SOURCE_NEW_EDEN_CELESTIALS = 0,
	STANDALONE_SH_SOURCE_VALIDATION = 1,
	STANDALONE_SH_SOURCE_NONE = 2,
};

enum StandaloneReflectionCorrection
{
	STANDALONE_REFLECTION_CORRECTION_OFF = 0,
	STANDALONE_REFLECTION_CORRECTION_CLIENT = 1,
};

enum StandaloneNormalMapMode
{
	STANDALONE_NORMAL_MAP_AUTHORED = 0,
	STANDALONE_NORMAL_MAP_FLAT = 1,
};

enum StandaloneCaptureProduct
{
	STANDALONE_CAPTURE_COLOR = 1 << 0,
	STANDALONE_CAPTURE_DEPTH = 1 << 1,
	STANDALONE_CAPTURE_NORMAL = 1 << 2,
	STANDALONE_CAPTURE_FREEZE_SCENE = 1 << 8,
};

enum StandaloneCameraView
{
	STANDALONE_CAMERA_MODEL = 0,
	STANDALONE_CAMERA_CELESTIALS = 1,
	STANDALONE_CAMERA_PLANET = 2,
};

enum StandaloneSceneComposition
{
	STANDALONE_COMPOSITION_SYSTEM = 0,
	STANDALONE_COMPOSITION_CINEMATIC = 1,
};

enum StandalonePlanetLayers
{
	STANDALONE_PLANET_SURFACE = 0,
	STANDALONE_PLANET_ATMOSPHERE = 1,
	STANDALONE_PLANET_CLOUDS = 2,
	STANDALONE_PLANET_ALL = 3,
};

enum StandaloneSunEffects
{
	STANDALONE_SUN_EFFECTS_AUTO = 0,
	STANDALONE_SUN_EFFECTS_OFF = 1,
	STANDALONE_SUN_EFFECTS_FLARE = 2,
	STANDALONE_SUN_EFFECTS_GOD_RAYS = 3,
	STANDALONE_SUN_EFFECTS_ALL = 4,
};

const char* SunEffectsName( int effects )
{
	switch( effects )
	{
	case STANDALONE_SUN_EFFECTS_OFF:
		return "off";
	case STANDALONE_SUN_EFFECTS_FLARE:
		return "flare";
	case STANDALONE_SUN_EFFECTS_GOD_RAYS:
		return "god-rays";
	case STANDALONE_SUN_EFFECTS_ALL:
		return "all";
	default:
		return "invalid";
	}
}

struct StandaloneProbe
{
	~StandaloneProbe()
	{
		if( scene )
		{
			scene->SetShLightingManager( nullptr );
		}
		renderProductVisualizer.Unlock();
		driver.Unlock();
		scene.Unlock();
		renderable.Unlock();
		secondaryLights.clear();
		shLightingManager.Unlock();
		g_eveSpaceSceneDynamicLighting = false;
		g_useDynamicLightsShadows = false;
		Tr2LightManager::DeleteInstance();
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
	std::vector<BluePtr<TrinityStandaloneSecondaryLight>> secondaryLights;
	Tr2ShLightingManagerPtr shLightingManager;
	TriViewPtr view;
	TriProjectionPtr projection;
	Tr2EffectPtr renderProductVisualizer;
	uint32_t renderWidth = 0;
	uint32_t renderHeight = 0;
	int localLights = STANDALONE_LOCAL_LIGHTS_OFF;
	bool reportedResolvedLights = false;
	uint64_t renderedFrameCount = 0;
	bool reportedCameraOrbit = false;
	int cameraView = STANDALONE_CAMERA_MODEL;
	EvePlanet* celestialInspectionTarget = nullptr;
	const char* celestialInspectionName = nullptr;
	float celestialExpectedPixelDiameter = 0.0f;
	float celestialInspectionFovRadians = 0.0f;
	float celestialInspectionScale = 1.0f;
	Vector3 celestialInspectionDirection;
	bool celestialInspectionValidated = false;
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

class Python27Random
{
public:
	explicit Python27Random( uint32_t seed )
	{
		Seed( seed );
	}

	uint32_t RandInt( uint32_t first, uint32_t last )
	{
		return first + static_cast<uint32_t>( Random() * ( last - first + 1 ) );
	}

	double Random()
	{
		const uint32_t high = Next() >> 5;
		const uint32_t low = Next() >> 6;
		return ( high * 67108864.0 + low ) / 9007199254740992.0;
	}

private:
	static constexpr size_t kStateSize = 624;

	void Seed( uint32_t seed )
	{
		m_state[0] = 19650218U;
		for( size_t index = 1; index < kStateSize; ++index )
		{
			m_state[index] = 1812433253U * ( m_state[index - 1] ^ ( m_state[index - 1] >> 30 ) ) +
				static_cast<uint32_t>( index );
		}
		size_t stateIndex = 1;
		for( size_t count = kStateSize; count > 0; --count )
		{
			m_state[stateIndex] = ( m_state[stateIndex] ^
									( ( m_state[stateIndex - 1] ^ ( m_state[stateIndex - 1] >> 30 ) ) * 1664525U ) ) +
				seed;
			if( ++stateIndex >= kStateSize )
			{
				m_state[0] = m_state[kStateSize - 1];
				stateIndex = 1;
			}
		}
		for( size_t count = kStateSize - 1; count > 0; --count )
		{
			m_state[stateIndex] = ( m_state[stateIndex] ^
									( ( m_state[stateIndex - 1] ^ ( m_state[stateIndex - 1] >> 30 ) ) * 1566083941U ) ) -
				static_cast<uint32_t>( stateIndex );
			if( ++stateIndex >= kStateSize )
			{
				m_state[0] = m_state[kStateSize - 1];
				stateIndex = 1;
			}
		}
		m_state[0] = 0x80000000U;
		m_index = kStateSize;
	}

	uint32_t Next()
	{
		if( m_index >= kStateSize )
		{
			constexpr uint32_t matrix = 0x9908b0dfU;
			for( size_t index = 0; index < kStateSize; ++index )
			{
				const uint32_t combined = ( m_state[index] & 0x80000000U ) |
					( m_state[( index + 1 ) % kStateSize] & 0x7fffffffU );
				m_state[index] = m_state[( index + 397 ) % kStateSize] ^ ( combined >> 1 ) ^
					( ( combined & 1U ) ? matrix : 0U );
			}
			m_index = 0;
		}
		uint32_t value = m_state[m_index++];
		value ^= value >> 11;
		value ^= ( value << 7 ) & 0x9d2c5680U;
		value ^= ( value << 15 ) & 0xefc60000U;
		value ^= value >> 18;
		return value;
	}

	std::array<uint32_t, kStateSize> m_state;
	size_t m_index = kStateSize;
};

struct PlanetCloudSelection
{
	std::string cloudPath;
	std::string capPath;
	float brightness = 0.0f;
	float transparency = 0.0f;
};

PlanetCloudSelection SelectPlanetClouds( int year, int month, int day, int32_t itemId )
{
	Python27Random random( static_cast<uint32_t>( year + month * 30 + day + itemId ) );
	const bool useDense = random.RandInt( 1, 5 ) % 5 == 0;
	(void)useDense;
	const uint32_t cloudIndex = random.RandInt( 0, 3 );
	const uint32_t capIndex = random.RandInt( 0, 3 );
	const char* cloudPaths[] = {
		"res:/dx9/model/worldobject/planet/sandstorm/dust01_m_hi.dds",
		"res:/dx9/model/worldobject/planet/sandstorm/dust02_m_hi.dds",
		"res:/dx9/model/worldobject/planet/sandstorm/dust03_m_hi.dds",
		"res:/dx9/model/worldobject/planet/sandstorm/dust04_m_hi.dds",
	};
	const char* capPaths[] = {
		"res:/dx9/model/worldobject/planet/sandstorm/dustcap01_m_hi.dds",
		"res:/dx9/model/worldobject/planet/sandstorm/dustcap02_m_hi.dds",
		"res:/dx9/model/worldobject/planet/sandstorm/dustcap03_m_hi.dds",
		"res:/dx9/model/worldobject/planet/sandstorm/dustcap04_m_hi.dds",
	};
	return {
		cloudPaths[cloudIndex],
		capPaths[capIndex],
		static_cast<float>( random.Random() * 0.4 + 0.6 ),
		static_cast<float>( random.Random() * 2.0 + 1.0 ),
	};
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

bool PrepareMeshWithoutYield( Tr2Mesh& mesh, const char* cmfPath, const char* role, std::string& error )
{
	mesh.SetMeshResPath( cmfPath );
	TriGeometryRes* geometry = mesh.GetGeometryResource();
	if( geometry )
	{
		geometry->ForceSynchronousLoad();
		geometry->Reload();
	}
	std::fprintf(
		stderr,
		"New Eden %s geometry state: path=%s exists=%s loading=%s prepared=%s good=%s\n",
		role,
		cmfPath,
		BePaths->FileExistsLocally( CA2W( cmfPath ) ) ? "yes" : "no",
		geometry && geometry->IsLoading() ? "yes" : "no",
		geometry && geometry->IsPrepared() ? "yes" : "no",
		geometry && geometry->IsGood() ? "yes" : "no" );
	if( !geometry || !geometry->IsGood() || !geometry->IsUsingCMF() )
	{
		error = std::string( role ) + " geometry failed to prepare: " + cmfPath;
		return false;
	}
	const int meshIndex = mesh.GetMeshIndex();
	if( meshIndex < 0 || static_cast<unsigned int>( meshIndex ) >= geometry->GetMeshCount() )
	{
		error = std::string( role ) + " serialized mesh index is outside converted CMF geometry";
		return false;
	}
	const unsigned int geometryAreaCount = geometry->GetAreaCount( static_cast<unsigned int>( meshIndex ) );

	for( int batchType = 0; batchType < TRIBATCHTYPE_COUNT_OF_BATCH_TYPES; ++batchType )
	{
		const Tr2MeshAreaVector* areas = mesh.GetAreas( static_cast<TriBatchType>( batchType ) );
		if( !areas )
		{
			continue;
		}
		for( Tr2MeshArea* area : *areas )
		{
			const int areaIndex = area->GetIndex();
			const int areaCount = std::max( 1, area->GetCount() );
			if( areaIndex < 0 || static_cast<unsigned int>( areaIndex + areaCount ) > geometryAreaCount )
			{
				error = std::string( role ) + " serialized area range is outside converted CMF geometry: " + area->GetName();
				return false;
			}
			Tr2Effect* effect = area->GetMaterialInterface();
			if( !effect )
			{
				error = std::string( role ) + " area has no effect: " + area->GetName();
				return false;
			}
			if( !PrepareEffectResourcesWithoutYield( *effect, role, error ) )
			{
				return false;
			}
		}
	}
	std::fprintf(
		stderr,
		"New Eden %s mesh ready: geometry=%s meshIndex=%d meshCount=%u areaCount=%u\n",
		role,
		cmfPath,
		meshIndex,
		geometry->GetMeshCount(),
		geometryAreaCount );
	return true;
}

bool PrepareCelestialMeshWithoutYield( EveChildMesh& child, const char* cmfPath, const char* role, std::string& error )
{
	Tr2MeshPtr mesh = BlueCastPtr( child.GetMesh() );
	if( !mesh )
	{
		error = std::string( role ) + " does not contain a serialized Tr2Mesh";
		return false;
	}
	if( !PrepareMeshWithoutYield( *mesh, cmfPath, role, error ) )
	{
		return false;
	}

	if( !child.Initialize() )
	{
		error = std::string( role ) + " child failed to initialize";
		return false;
	}
	std::fprintf( stderr, "New Eden %s ready: geometry=%s\n", role, cmfPath );
	return true;
}

bool PrepareLensFlareTransformWithoutYield( EveTransform& transform, const char* cmfPath, const char* role, std::string& error )
{
	Tr2MeshPtr mesh = BlueCastPtr( transform.GetMesh() );
	if( mesh && !PrepareMeshWithoutYield( *mesh, cmfPath, role, error ) )
	{
		return false;
	}
	if( !transform.Initialize() )
	{
		error = std::string( role ) + " transform failed to initialize";
		return false;
	}
	return true;
}

bool PrepareNewEdenLensFlareWithoutYield( EveLensflare& lensFlare, std::string& error )
{
	Tr2MeshPtr mesh = lensFlare.GetMesh();
	if( !mesh )
	{
		error = "New Eden lens flare has no serialized mesh";
		return false;
	}
	if( !PrepareMeshWithoutYield( *mesh, "res:/fisfx/lensflare/yellow_small.cmf", "lens flare", error ) )
	{
		return false;
	}

	size_t flareTransformCount = 0;
	for( EveTransform* flare : lensFlare.Flares() )
	{
		if( !flare || !PrepareLensFlareTransformWithoutYield( *flare, "res:/fisfx/lensflare/yellow_small.cmf", "lens flare transform", error ) )
		{
			return false;
		}
		++flareTransformCount;
	}

	size_t foregroundSpriteCount = 0;
	size_t backgroundSpriteCount = 0;
	auto prepareOccluders = [&]( const PEveOccluderVector& occluders, const char* role, size_t& spriteCount ) {
		for( EveOccluder* occluder : occluders )
		{
			if( !occluder )
			{
				error = std::string( role ) + " contains a null occluder";
				return false;
			}
			for( EveTransform* sprite : occluder->Sprites() )
			{
				if( !sprite || !PrepareLensFlareTransformWithoutYield( *sprite, "res:/model/global/zsprite.cmf", role, error ) )
				{
					return false;
				}
				++spriteCount;
			}
		}
		return true;
	};
	if( !prepareOccluders( lensFlare.Occluders(), "lens flare foreground occluder", foregroundSpriteCount ) ||
		!prepareOccluders( lensFlare.BackgroundOccluders(), "lens flare background occluder", backgroundSpriteCount ) )
	{
		return false;
	}

	Tr2EffectPtr managementEffect;
	if( !managementEffect.CreateInstance() )
	{
		error = "Failed to create lens flare occlusion-management effect";
		return false;
	}
	managementEffect->SetEffectPathName( "res:/graphics/effect/managed/space/specialfx/lensflares/occludermanagement.fx" );
	if( !PrepareEffectResourcesWithoutYield( *managementEffect, "lens flare occlusion management", error ) )
	{
		return false;
	}
	if( !lensFlare.Initialize() )
	{
		error = "New Eden lens flare failed to initialize";
		return false;
	}
	lensFlare.StartControllers();
	std::fprintf(
		stderr,
		"New Eden lens flare ready: graphicId=1247 path=res:/fisfx/lensflare/yellow_small.black "
		"flareTransforms=%zu foregroundOccluders=%zu foregroundSprites=%zu backgroundOccluders=%zu backgroundSprites=%zu\n",
		flareTransformCount,
		lensFlare.Occluders().size(),
		foregroundSpriteCount,
		lensFlare.BackgroundOccluders().size(),
		backgroundSpriteCount );
	return true;
}

bool PrepareNewEdenCelestialsWithoutYield(
	EvePlanet& sun,
	EvePlanet& planet,
	int sunEffects,
	int planetLayers,
	int cloudYear,
	int cloudMonth,
	int cloudDay,
	std::string& error )
{
	EveChildContainerPtr sunBodyContainer;
	PIEveSpaceObjectChildVector& sunChildren = sun.GetChildren();
	for( IEveSpaceObjectChild* child : sunChildren )
	{
		if( std::strcmp( child->GetName(), "MediumAndHigh_Quality_SunHalfSizer" ) == 0 )
		{
			sunBodyContainer = BlueCastPtr( child );
			break;
		}
	}
	if( !sunBodyContainer )
	{
		error = "New Eden sun body container is missing";
		return false;
	}
	for( ssize_t index = static_cast<ssize_t>( sunChildren.size() ) - 1; index >= 0; --index )
	{
		if( sunChildren[index] != sunBodyContainer->GetRawRoot() )
		{
			sunChildren.Remove( index );
		}
	}

	const bool retainSunFlare = sunEffects == STANDALONE_SUN_EFFECTS_FLARE || sunEffects == STANDALONE_SUN_EFFECTS_ALL;
	EveChildMeshPtr sunBody;
	EveChildMeshPtr sunFlare;
	for( IEveSpaceObjectChild* child : sunBodyContainer->m_objects )
	{
		if( std::strcmp( child->GetName(), "Sun" ) == 0 )
		{
			sunBody = BlueCastPtr( child );
		}
		else if( std::strcmp( child->GetName(), "SunFlares" ) == 0 )
		{
			sunFlare = BlueCastPtr( child );
		}
	}
	if( !sunBody )
	{
		error = "New Eden sun sphere child is missing";
		return false;
	}
	if( retainSunFlare && !sunFlare )
	{
		error = "New Eden authored SunFlares child is missing";
		return false;
	}
	for( ssize_t index = static_cast<ssize_t>( sunBodyContainer->m_objects.size() ) - 1; index >= 0; --index )
	{
		IRoot* child = sunBodyContainer->m_objects[index];
		if( child != sunBody->GetRawRoot() && ( !retainSunFlare || child != sunFlare->GetRawRoot() ) )
		{
			sunBodyContainer->m_objects.Remove( index );
		}
	}

	PIEveSpaceObjectChildVector& planetChildren = planet.GetChildren();
	EveChildMeshPtr planetBody;
	EveChildContainerPtr atmosphereContainer;
	EveChildMeshPtr cloudLayer;
	for( IEveSpaceObjectChild* child : planetChildren )
	{
		if( std::strcmp( child->GetName(), "Planet" ) == 0 || std::strcmp( child->GetName(), "planet" ) == 0 )
		{
			planetBody = BlueCastPtr( child );
		}
		else if( std::strcmp( child->GetName(), "Atmosphere" ) == 0 )
		{
			atmosphereContainer = BlueCastPtr( child );
		}
		else if( std::strcmp( child->GetName(), "CloudLayer" ) == 0 )
		{
			cloudLayer = BlueCastPtr( child );
		}
	}
	if( !planetBody )
	{
		error = "New Eden planet sphere child is missing";
		return false;
	}
	if( !atmosphereContainer || !cloudLayer )
	{
		error = "New Eden planet atmosphere container or CloudLayer mesh is missing";
		return false;
	}

	EveChildMeshPtr atmosphereMeshChild;
	for( IEveSpaceObjectChild* child : atmosphereContainer->m_objects )
	{
		EveChildMeshPtr meshChild = BlueCastPtr( child );
		std::fprintf(
			stderr,
			"New Eden atmosphere graph: child=%s mesh=%s\n",
			child->GetName(),
			meshChild ? "yes" : "no" );
		if( std::strcmp( child->GetName(), "Atmo" ) == 0 )
		{
			atmosphereMeshChild = meshChild;
		}
	}
	if( !atmosphereMeshChild )
	{
		error = "New Eden planet atmosphere graph is missing the Atmo mesh";
		return false;
	}
	const bool retainAtmosphere = planetLayers == STANDALONE_PLANET_ATMOSPHERE || planetLayers == STANDALONE_PLANET_ALL;
	const bool retainClouds = planetLayers == STANDALONE_PLANET_CLOUDS || planetLayers == STANDALONE_PLANET_ALL;
	for( ssize_t index = static_cast<ssize_t>( planetChildren.size() ) - 1; index >= 0; --index )
	{
		IRoot* child = planetChildren[index];
		if( child != planetBody->GetRawRoot() &&
			( child != atmosphereContainer->GetRawRoot() || !retainAtmosphere ) &&
			( child != cloudLayer->GetRawRoot() || !retainClouds ) )
		{
			planetChildren.Remove( index );
		}
	}
	for( ssize_t index = static_cast<ssize_t>( atmosphereContainer->m_objects.size() ) - 1; index >= 0; --index )
	{
		if( atmosphereContainer->m_objects[index] != atmosphereMeshChild->GetRawRoot() )
		{
			atmosphereContainer->m_objects.Remove( index );
		}
	}

	const PlanetCloudSelection cloudSelection = SelectPlanetClouds( cloudYear, cloudMonth, cloudDay, 40334264 );
	std::fprintf(
		stderr,
		"New Eden planet cloud selection: date=%04d-%02d-%02d cloud=%s cap=%s brightness=%.10f transparency=%.10f\n",
		cloudYear,
		cloudMonth,
		cloudDay,
		cloudSelection.cloudPath.c_str(),
		cloudSelection.capPath.c_str(),
		cloudSelection.brightness,
		cloudSelection.transparency );
	if( cloudYear == 2026 && cloudMonth == 7 && cloudDay == 10 &&
		( cloudSelection.cloudPath.find( "dust04_m_hi.dds" ) == std::string::npos ||
		  cloudSelection.capPath.find( "dustcap02_m_hi.dds" ) == std::string::npos ||
		  std::abs( cloudSelection.brightness - 0.9530833834f ) > 1e-6f ||
		  std::abs( cloudSelection.transparency - 2.7395450457f ) > 1e-6f ) )
	{
		error = "Python 2.7 cloud randomization parity check failed for 2026-07-10";
		return false;
	}

	Tr2MeshPtr planetMesh = BlueCastPtr( planetBody->GetMesh() );
	if( !planetMesh )
	{
		error = "New Eden planet surface does not contain a serialized Tr2Mesh";
		return false;
	}
	for( int batchType = 0; batchType < TRIBATCHTYPE_COUNT_OF_BATCH_TYPES; ++batchType )
	{
		const Tr2MeshAreaVector* areas = planetMesh->GetAreas( static_cast<TriBatchType>( batchType ) );
		if( !areas )
		{
			continue;
		}
		for( Tr2MeshArea* area : *areas )
		{
			Tr2Effect* effect = area->GetMaterialInterface();
			if( !effect )
			{
				error = "New Eden planet surface area has no effect";
				return false;
			}
			TriTextureParameterPtr cloudTexture = BlueCastPtr( effect->GetResourceByName( "CloudsTexture" ) );
			TriTextureParameterPtr cloudCapTexture = BlueCastPtr( effect->GetResourceByName( "CloudCapTexture" ) );
			Tr2ConstantEffectParameter* cloudFactors = nullptr;
			for( auto& parameter : effect->m_constParameters )
			{
				if( std::strcmp( parameter.name.c_str(), "CloudsFactors" ) == 0 )
				{
					cloudFactors = &parameter;
					break;
				}
			}
			if( !cloudTexture || !cloudCapTexture || !cloudFactors )
			{
				error = "New Eden planet surface is missing authored cloud parameters";
				return false;
			}
			cloudTexture->SetResourcePath( cloudSelection.cloudPath.c_str() );
			cloudCapTexture->SetResourcePath( cloudSelection.capPath.c_str() );
			cloudFactors->value.y = cloudSelection.brightness;
			cloudFactors->value.z = cloudSelection.transparency;
			for( ssize_t index = static_cast<ssize_t>( effect->m_resources.size() ) - 1; index >= 0; --index )
			{
				if( std::strcmp( effect->m_resources[index]->GetParameterName(), "HeightMap" ) == 0 )
				{
					effect->m_resources.Remove( index );
				}
			}
			if( !effect->AddResourceTexture2D(
					BlueSharedString( "NormalHeight1" ),
					"res:/dx9/model/worldobject/planet/terrestrial/terrestrial04_h_hi.dds" ) ||
				!effect->AddResourceTexture2D(
					BlueSharedString( "NormalHeight2" ),
					"res:/dx9/model/worldobject/planet/moon/moon01_h.dds" ) ||
				!effect->AddParameterFloat( BlueSharedString( "Random" ), 64.0f ) )
			{
				error = "New Eden planet surface parameter injection failed";
				return false;
			}
			std::fprintf(
				stderr,
				"New Eden planet authored preset: graphic=4321 path=res:/dx9/model/worldobject/planet/"
				"template_hi/sandstorm/p_sandstorm_11.black heightMap1=3843 heightMap2=3903 random=64 "
				"cloudDate=%04d-%02d-%02d clouds=%s cap=%s brightness=%.10f transparency=%.10f\n",
				cloudYear,
				cloudMonth,
				cloudDay,
				cloudSelection.cloudPath.c_str(),
				cloudSelection.capPath.c_str(),
				cloudSelection.brightness,
				cloudSelection.transparency );
		}
	}

	if( !PrepareCelestialMeshWithoutYield(
			*sunBody,
			"res:/graphics/generic/unitsphere/unitsphere_4k_01a.cmf",
			"sun body",
			error ) ||
		!PrepareCelestialMeshWithoutYield(
			*planetBody,
			"res:/dx9/model/worldobject/planet/planetsphere.cmf",
			"planet body",
			error ) )
	{
		return false;
	}
	if( retainSunFlare && !PrepareCelestialMeshWithoutYield( *sunFlare, "res:/graphics/generic/unitplane/unitplane.cmf", "authored sun flare", error ) )
	{
		return false;
	}
	if( retainAtmosphere && !PrepareCelestialMeshWithoutYield( *atmosphereMeshChild, "res:/dx9/model/worldobject/planet/planetring.cmf", "planet atmosphere", error ) )
	{
		return false;
	}
	if( retainClouds && !PrepareCelestialMeshWithoutYield( *cloudLayer, "res:/dx9/model/worldobject/planet/planetring.cmf", "planet cloud layer", error ) )
	{
		return false;
	}
	if( retainAtmosphere && !atmosphereContainer->Initialize() )
	{
		error = "New Eden planet atmosphere container failed to initialize";
		return false;
	}
	std::fprintf(
		stderr,
		"New Eden authored sun flare: enabled=%s child=SunFlares geometry=res:/graphics/generic/unitplane/unitplane.cmf\n",
		retainSunFlare ? "yes" : "no" );
	std::fprintf(
		stderr,
		"New Eden planet layers ready: surface=yes atmosphere=%s clouds=%s aurora=no dataDrivenFx=no\n",
		retainAtmosphere ? "yes" : "no",
		retainClouds ? "yes" : "no" );
	return sunBodyContainer->Initialize() && sun.Initialize() && planet.Initialize();
}

bool ConfigureNewEdenSystem(
	StandaloneProbe& probe,
	EveSpaceScene& scene,
	int cameraView,
	int composition,
	int sunEffects,
	int planetLayers,
	int cloudYear,
	int cloudMonth,
	int cloudDay,
	std::string& error )
{
	constexpr int32_t systemId = 30005286;
	constexpr int32_t constellationId = 20000773;
	constexpr float securityStatus = 0.2605538070201874f;
	constexpr double starRadius = 158400000.0;
	constexpr double planetRadius = 2630000.0;
	constexpr double asteroCollisionRadius = 35.0;
	constexpr double asteroMeshRadius = 54.55741723174651;
	constexpr double metersPerSceneUnit = 1000000.0;
	constexpr double observerX = -1069486940160.0;
	constexpr double observerY = 202669301760.0;
	constexpr double observerZ = 831868968960.0;
	constexpr float emissiveR = 5.0f;
	constexpr float emissiveG = 4.274509906768799f;
	constexpr float emissiveB = 2.3529412746429443f;

	const double observerDistance = std::sqrt(
		observerX * observerX + observerY * observerY + observerZ * observerZ );
	Vector3 sunDirection(
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
	Vector3 sunPosition( 1069486940160.0f, -202669301760.0f, -831868968960.0f );
	Vector3 planetPosition( 1083758787326.0f, -205372890997.0f, -787280443197.0f );
	if( composition == STANDALONE_COMPOSITION_CINEMATIC )
	{
		// EVE applies the selectable warp range around a deterministic planet-specific
		// destination. Reproduce eveCfg.GetPlanetWarpInPoint's radial distance here.
		constexpr double factor = 20.0;
		double warpScale = std::pow(
							   factor,
							   ( 5.0 - std::log10( planetRadius / metersPerSceneUnit ) - 0.5 ) / factor ) *
			factor;
		warpScale = std::min( 10.0, std::max( 0.0, warpScale ) ) + 0.5;
		const double warpSurfaceClearance = metersPerSceneUnit + planetRadius * warpScale;
		const double warpCenterDistance = planetRadius + warpSurfaceClearance;
		const double contactCenterDistance = planetRadius + asteroCollisionRadius;
		const double visualContactCenterDistance = planetRadius + asteroMeshRadius;
		constexpr double cinematicSunCenterDistance = 1000000000.0;
		sunPosition = Normalize( Vector3( -0.65f, 0.32f, 1.0f ) ) *
			static_cast<float>( cinematicSunCenterDistance );
		planetPosition = Normalize( Vector3( 0.55f, -0.12f, 1.0f ) ) *
			static_cast<float>( warpCenterDistance );
		sunDirection = -Normalize( sunPosition );
		std::fprintf(
			stderr,
			"New Eden cinematic composition: authored bodies retained; standard planet warp-in "
			"radius=%.3f km surfaceClearance=%.3f km centerDistance=%.3f km "
			"contactCenterDistance=%.3f km visualContactCenterDistance=%.3f km\n",
			planetRadius / 1000.0,
			warpSurfaceClearance / 1000.0,
			warpCenterDistance / 1000.0,
			contactCenterDistance / 1000.0,
			visualContactCenterDistance / 1000.0 );
		std::fprintf(
			stderr,
			"New Eden cinematic placement: sun=(%.3f, %.3f, %.3f) planet=(%.3f, %.3f, %.3f)\n",
			sunPosition.x,
			sunPosition.y,
			sunPosition.z,
			planetPosition.x,
			planetPosition.y,
			planetPosition.z );
	}
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

	auto sun = LoadBlackObjectWithoutYield<EvePlanet>(
		"res:/dx9/model/celestial/sun/sun_yellow_small_01b.black",
		error );
	if( !sun )
	{
		return false;
	}
	auto planet = LoadBlackObjectWithoutYield<EvePlanet>(
		"res:/dx9/model/worldobject/planet/template_hi/sandstorm/p_sandstorm_11.black",
		error );
	if( !planet )
	{
		return false;
	}
	sun->SetStandalonePlacement(
		sunPosition,
		static_cast<float>( starRadius ),
		Color( 0.0f, 0.0f, 0.0f, 1.0f ),
		Color( emissiveR, emissiveG, emissiveB, 1.0f ) );
	planet->SetStandalonePlacement(
		planetPosition,
		static_cast<float>( planetRadius ),
		Color( 1.0f, 0.8901960849761963f, 0.6627451181411743f, 1.0f ),
		Color( 0.0f, 0.0f, 0.0f, 1.0f ) );
	if( !PrepareNewEdenCelestialsWithoutYield(
			*sun,
			*planet,
			sunEffects,
			planetLayers,
			cloudYear,
			cloudMonth,
			cloudDay,
			error ) )
	{
		if( error.empty() )
		{
			error = "New Eden celestial graph failed to initialize";
		}
		return false;
	}
	const bool enableLensFlare = sunEffects == STANDALONE_SUN_EFFECTS_FLARE || sunEffects == STANDALONE_SUN_EFFECTS_ALL;
	if( enableLensFlare && cameraView != STANDALONE_CAMERA_PLANET )
	{
		auto lensFlare = LoadBlackObjectWithoutYield<EveLensflare>(
			"res:/fisfx/lensflare/yellow_small.black",
			error );
		if( !lensFlare )
		{
			return false;
		}
		if( !PrepareNewEdenLensFlareWithoutYield( *lensFlare, error ) )
		{
			return false;
		}
		lensFlare->SetTranslationCurve( sun->GetTranslationCurve() );
		scene.Lensflares().Insert( -1, lensFlare->GetRawRoot() );
	}
	else if( enableLensFlare )
	{
		std::fprintf( stderr, "New Eden planet camera isolation: lens flare submission disabled\n" );
	}
	if( cameraView != STANDALONE_CAMERA_PLANET )
	{
		scene.Planets().Insert( -1, sun->GetRawRoot() );
	}
	else
	{
		std::fprintf( stderr, "New Eden planet camera isolation: sun submission disabled\n" );
	}
	scene.Planets().Insert( -1, planet->GetRawRoot() );
	if( composition == STANDALONE_COMPOSITION_SYSTEM )
	{
		const float sixtyDegreeFov = 60.0f * 3.1415926535f / 180.0f;
		auto pixelDiameter = [&]( float radius, const Vector3& position, float fov ) {
			return static_cast<float>( probe.renderHeight ) * ( radius / Length( position ) ) / std::tan( fov * 0.5f );
		};
		std::fprintf(
			stderr,
			"New Eden exact-system observer view: fov=60.000000 degrees sunPixels=%.8f planetPixels=%.8f "
			"planetScale=1000000 planetCameraScale=1000000\n",
			pixelDiameter( static_cast<float>( starRadius ), sunPosition, sixtyDegreeFov ),
			pixelDiameter( static_cast<float>( planetRadius ), planetPosition, sixtyDegreeFov ) );

		if( cameraView == STANDALONE_CAMERA_CELESTIALS || cameraView == STANDALONE_CAMERA_PLANET )
		{
			constexpr float inspectionDistance = 10000.0f;
			constexpr float targetHeightFraction = 0.25f;
			const bool inspectPlanet = cameraView == STANDALONE_CAMERA_PLANET;
			const Vector3 selectedPosition = inspectPlanet ? planetPosition : sunPosition;
			const float selectedRadius = inspectPlanet ? static_cast<float>( planetRadius ) : static_cast<float>( starRadius );
			const float selectedDistance = Length( selectedPosition );
			const float angularRadius = std::asin( std::min( 1.0f, selectedRadius / selectedDistance ) );
			probe.celestialInspectionTarget = inspectPlanet ? planet.p : sun.p;
			probe.celestialInspectionName = inspectPlanet ? "planet" : "sun";
			probe.celestialExpectedPixelDiameter = static_cast<float>( probe.renderHeight ) * targetHeightFraction;
			probe.celestialInspectionFovRadians = 2.0f * std::atan( std::tan( angularRadius ) / targetHeightFraction );
			probe.celestialInspectionScale = selectedDistance / inspectionDistance;
			probe.celestialInspectionDirection = Normalize( selectedPosition );
			scene.SetPlanetScale( probe.celestialInspectionScale );
			scene.SetPlanetCameraScale( probe.celestialInspectionScale );
			std::fprintf(
				stderr,
				"New Eden celestial inspection configured: target=%s authoredDistance=%.3f authoredRadius=%.3f "
				"renderDistance=%.3f scale=%.6f fovRadians=%.10f fovDegrees=%.8f expectedPixels=%.3f\n",
				probe.celestialInspectionName,
				selectedDistance,
				selectedRadius,
				inspectionDistance,
				probe.celestialInspectionScale,
				probe.celestialInspectionFovRadians,
				probe.celestialInspectionFovRadians * 180.0f / 3.1415926535f,
				probe.celestialExpectedPixelDiameter );
		}
	}
	auto describeCelestial = []( const char* role, EvePlanet& celestial ) {
		std::function<void( IEveSpaceObjectChild*, int )> describeChild;
		describeChild = [&]( IEveSpaceObjectChild* child, int depth ) {
			EveChildContainerPtr container = BlueCastPtr( child );
			EveChildSocketPtr socket = BlueCastPtr( child );
			EveChildRefPtr childRef = BlueCastPtr( child );
			EveChildMeshPtr meshChild = BlueCastPtr( child );
			EveChildLineSetPtr lineSet = BlueCastPtr( child );
			EveChildQuadPtr quad = BlueCastPtr( child );
			EveChildParticleSystemPtr particles = BlueCastPtr( child );
			Tr2MeshBase* mesh = meshChild ? meshChild->GetMesh() : ( lineSet ? lineSet->GetMesh() : nullptr );
			Tr2MeshPtr serializedMesh = BlueCastPtr( mesh );
			const char* type = container ? "container" :
										   ( socket ? "socket" :
													  ( childRef ? "ref" :
																   ( meshChild ? "mesh" :
																				 ( lineSet ? "line-set" :
																							 ( quad ? "quad" : ( particles ? "particles" : "other" ) ) ) ) ) );
			std::fprintf(
				stderr,
				"New Eden %s graph: depth=%d type=%s name=%s resource=%s mesh=%s\n",
				role,
				depth,
				type,
				child->GetName(),
				socket ? socket->GetPlugResPath() : ( childRef ? childRef->GetResPath() : "" ),
				serializedMesh ? serializedMesh->GetMeshResPath() : "" );
			if( mesh )
			{
				for( int batchType = 0; batchType < TRIBATCHTYPE_COUNT_OF_BATCH_TYPES; ++batchType )
				{
					const Tr2MeshAreaVector* areas = mesh->GetAreas( static_cast<TriBatchType>( batchType ) );
					if( !areas )
					{
						continue;
					}
					for( Tr2MeshArea* area : *areas )
					{
						Tr2Effect* effect = area->GetMaterialInterface();
						std::fprintf(
							stderr,
							"New Eden %s area: depth=%d child=%s batch=%d area=%s effect=%s\n",
							role,
							depth,
							child->GetName(),
							batchType,
							area->GetName().c_str(),
							effect ? effect->GetEffectPathName() : "" );
						if( effect )
						{
							for( IRoot* resource : effect->m_resources )
							{
								TriTextureParameterPtr texture = BlueCastPtr( resource );
								if( texture )
								{
									std::fprintf(
										stderr,
										"New Eden %s texture: child=%s area=%s parameter=%s path=%s\n",
										role,
										child->GetName(),
										area->GetName().c_str(),
										texture->GetParameterName(),
										texture->GetAuthoredResourcePath() );
								}
							}
						}
					}
				}
			}
			if( container )
			{
				for( IEveSpaceObjectChild* nested : container->m_objects )
				{
					describeChild( nested, depth + 1 );
				}
			}
		};
		for( IEveSpaceObjectChild* child : celestial.GetChildren() )
		{
			describeChild( child, 0 );
		}
	};
	describeCelestial( "sun", *sun );
	describeCelestial( "planet", *planet );
	std::fprintf(
		stderr,
		"New Eden visible celestials loaded: sunChildren=%zu planetChildren=%zu\n",
		sun->GetChildren().size(),
		planet->GetChildren().size() );

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

bool ConfigureShLighting(
	StandaloneProbe& probe,
	int lightingView,
	int shSource,
	int sceneFixture,
	std::string& error )
{
	if( lightingView < STANDALONE_LIGHTING_COMBINED || lightingView > STANDALONE_LIGHTING_LOCAL )
	{
		error = "Invalid standalone lighting view";
		return false;
	}
	if( shSource < STANDALONE_SH_SOURCE_NEW_EDEN_CELESTIALS || shSource > STANDALONE_SH_SOURCE_NONE )
	{
		error = "Invalid standalone SH source";
		return false;
	}

	if( lightingView == STANDALONE_LIGHTING_SH || lightingView == STANDALONE_LIGHTING_LOCAL )
	{
		const EveSpaceScene::LightingSetup lighting = probe.scene->GetLightingSetup();
		probe.scene->SetSunLighting( lighting.sunDirection, Color( 0.0f, 0.0f, 0.0f, 1.0f ) );
		std::fprintf( stderr, "Lighting isolation: %s only; direct scene sun color is zero\n", lightingView == STANDALONE_LIGHTING_SH ? "SH" : "local" );
	}
	else
	{
		std::fprintf( stderr, "Lighting isolation: %s\n", lightingView == STANDALONE_LIGHTING_DIRECT ? "direct only" : "combined direct plus SH" );
	}

	if( lightingView == STANDALONE_LIGHTING_DIRECT || lightingView == STANDALONE_LIGHTING_LOCAL || shSource == STANDALONE_SH_SOURCE_NONE )
	{
		std::fprintf( stderr, "Trinity SH manager disabled for this comparison\n" );
		return true;
	}

	if( !probe.shLightingManager.CreateInstance() )
	{
		error = "Failed to create Tr2ShLightingManager";
		return false;
	}
	probe.scene->SetShLightingManager( probe.shLightingManager );

	auto addSecondarySource = [&]( const Vector3& position, float radius, const Color& albedo, const Color& emissive ) {
		BluePtr<TrinityStandaloneSecondaryLight> source;
		if( !source.CreateInstance() )
		{
			error = "Failed to create standalone SH secondary source";
			return false;
		}
		source->Configure( position, radius, albedo, emissive );
		probe.scene->Objects().Insert( -1, source->GetRawRoot() );
		probe.secondaryLights.push_back( source );
		return true;
	};

	if( shSource == STANDALONE_SH_SOURCE_NEW_EDEN_CELESTIALS )
	{
		if( sceneFixture != 3 )
		{
			std::fprintf( stderr, "Trinity SH sources skipped: New Eden celestials are not part of the selected scene fixture\n" );
			return true;
		}

		// The client adds both bodies to scene.planets. EvePlanet registers them
		// with Tr2ShLightingManager after applying its 1,000,000:1 scene scale.
		if( !addSecondarySource(
				Vector3( 1069486.940160f, -202669.301760f, -831868.968960f ),
				158.4f,
				Color( 0.0f, 0.0f, 0.0f, 1.0f ),
				Color( 5.0f, 4.274509906768799f, 2.3529412746429443f, 1.0f ) ) ||
			!addSecondarySource(
				Vector3( 1083758.787326f, -205372.890997f, -787280.443197f ),
				2.63f,
				Color( 1.0f, 0.8901960849761963f, 0.6627451181411743f, 1.0f ),
				Color( 0.0f, 0.0f, 0.0f, 1.0f ) ) )
		{
			return false;
		}
		std::fprintf(
			stderr,
			"Trinity SH sources: New Eden sun 40334263/type 45041/graphic 21480 and planet "
			"40334264/type 2016/graphic 3837; exact client albedo/emissive, expected contribution below runtime cutoff\n" );
	}
	else
	{
		const Vector3 sunDirection = Normalize( probe.scene->GetLightingSetup().sunDirection );
		if( !addSecondarySource(
				sunDirection * 8.0f,
				7.0f,
				Color( 4.0f, 3.0f, 2.0f, 1.0f ),
				Color( 0.0f, 0.0f, 0.0f, 1.0f ) ) )
		{
			return false;
		}
		std::fprintf(
			stderr,
			"Trinity SH source: synthetic stress sphere, distance=8 radius=7 albedo=(4,3,2); capability evidence only\n" );
	}

	return true;
}

bool ConfigureAsteroEveV5Effect( Tr2Effect& effect, uint32_t sourceGroup, int normalMapMode, std::string& error )
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

	EveSOFDataHullAreaPtr selectedArea;
	for( const auto& area : hull->m_opaqueAreas )
	{
		if( area->m_index == sourceGroup )
		{
			selectedArea = area;
			break;
		}
	}
	if( !selectedArea )
	{
		for( const auto& area : hull->m_distortionAreas )
		{
			if( area->m_index == sourceGroup )
			{
				selectedArea = area;
				break;
			}
		}
	}
	if( !selectedArea )
	{
		error = "Astero SOF has no area for GR2 group " + std::to_string( sourceGroup );
		return false;
	}

	const bool opaqueArea = sourceGroup == 1 || sourceGroup == 2;
	Vector4 patternMaterialValues[2][3] = {};
	if( opaqueArea )
	{
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
					if( std::strcmp( suffix, "DiffuseColor" ) == 0 )
					{
						patternMaterialValues[materialIndex][0] = parameter->m_value;
					}
					else if( std::strcmp( suffix, "FresnelColor" ) == 0 )
					{
						patternMaterialValues[materialIndex][1] = parameter->m_value;
					}
					else if( std::strcmp( suffix, "Gloss" ) == 0 )
					{
						patternMaterialValues[materialIndex][2] = parameter->m_value;
					}
				}
			}
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
	}

	for( const auto& parameter : selectedArea->m_parameters )
	{
		effect.AddParameterVector4( parameter->m_name, &parameter->m_value );
	}
	if( opaqueArea )
	{
		const Vector4 noHeatGlow( 0.0f, 0.0f, 0.0f, 0.0f );
		for( size_t materialIndex = 0; materialIndex < 4; ++materialIndex )
		{
			const std::string heatName = "Mtl" + std::to_string( materialIndex + 1 ) + "HeatGlowData";
			bool hasAuthoredHeat = false;
			for( const auto& parameter : selectedArea->m_parameters )
			{
				hasAuthoredHeat = hasAuthoredHeat || std::strcmp( parameter->m_name.c_str(), heatName.c_str() ) == 0;
			}
			if( !hasAuthoredHeat )
			{
				effect.AddParameterVector4( BlueSharedString( heatName ), &noHeatGlow );
			}
		}
	}

	const auto glowType = sourceGroup == 2 ? SOFDataFactionColorChooser::TYPE_BOOSTER : SOFDataFactionColorChooser::TYPE_HULL;
	const Color& glow = faction->m_colorSet->m_colors[glowType];
	const Vector4 glowValue( glow.r, glow.g, glow.b, glow.a );
	effect.AddParameterVector4( BlueSharedString( "GeneralGlowColor" ), &glowValue );
	effect.SetParameter( BlueSharedString( "areaId" ), sourceGroup );
	effect.SetParameter( BlueSharedString( "objectId" ), uint32_t( 0 ) );

	for( const auto& texture : selectedArea->m_textures )
	{
		const bool useFlatNormal = opaqueArea && normalMapMode == STANDALONE_NORMAL_MAP_FLAT &&
			std::strcmp( texture->m_name.c_str(), "NormalMap" ) == 0;
		effect.AddResourceTexture2D(
			texture->m_name,
			useFlatNormal ? "res:/texture/global/flatnormal.dds" : texture->m_resFilePath.c_str() );
	}
	if( opaqueArea )
	{
		effect.AddResourceTexture2D( BlueSharedString( "DustNoiseMap" ), "res:/texture/global/black.dds" );
	}

	std::fprintf(
		stderr,
		"Authored Astero area configured: group=%u name=%s shader=%s textures=%zu glow=(%.3f, %.3f, %.3f)\n",
		sourceGroup,
		selectedArea->m_name.c_str(),
		selectedArea->m_shader.c_str(),
		selectedArea->m_textures.size(),
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
	const char* shaderSuffix = Tr2Renderer::GetShaderModel() == TR2SM_3_0_DEPTH ? "sm_depth" :
																				  ( Tr2Renderer::GetShaderModel() == TR2SM_3_0_HI ? "sm_hi" : "sm_lo" );
	output << "- Shader model: `" << Tr2Renderer::GetShaderModelString( Tr2Renderer::GetShaderModel() ) << "`\n";
	output << "- Compiled path: `res:/graphics/effect.metal/managed/space/spaceobject/v5/quad/quadv5." << shaderSuffix << "`\n";
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
		{ Tr2VertexDefinition::BLENDINDICES, 0 },
		{ Tr2VertexDefinition::TANGENT, 0 },
		{ Tr2VertexDefinition::TEXCOORD, 0 },
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
	const bool hasLightBuffer = shader->GetResource( "LightBuffer" ) != nullptr;
	const bool hasLightIndexBuffer = shader->GetResource( "LightIndexBuffer" ) != nullptr;
	output << "\n### Tiled-light shader inputs\n\n";
	output << "- `LightBuffer`: " << ( hasLightBuffer ? "present" : "ABSENT" ) << "\n";
	output << "- `LightIndexBuffer`: " << ( hasLightIndexBuffer ? "present" : "ABSENT" ) << "\n";
	output << "- Opaque local-light consumption: "
		   << ( hasLightBuffer && hasLightIndexBuffer ? "eligible for visual A/B validation" : "not accepted for this Metal payload" ) << "\n";

	output << "\n## Existing Bridge Compatibility\n\n";
	output << "The V5 bridge provides `POSITION0`, `BLENDINDICES0`, `TANGENT0`, and `TEXCOORD0`. "
			  "The tangent stream preserves the authored GR2 frame with CMF `PackedTangentLegacy`. "
			  "The Astero GR2 has no authored `TEXCOORD1` stream, so TrinityAL supplies its missing-attribute zero value.\n\n";
	bool missingVertexInputs = false;
	for( const auto& input : effectVertexInputs )
	{
		const auto usage = static_cast<Tr2VertexDefinition::UsageCode>( input.first );
		const bool available = bridgeVertexInputs.count( input ) != 0;
		const bool synthesizedNeutral = usage == Tr2VertexDefinition::TEXCOORD && input.second == 1;
		output << "- `" << VertexUsageName( usage ) << input.second << "`: "
			   << ( available ? "available" : ( synthesizedNeutral ? "synthesized zero (authored stream absent)" : "MISSING" ) ) << "\n";
		missingVertexInputs = missingVertexInputs || ( !available && !synthesizedNeutral );
	}
	output << "\n- Typed Black/SOF decode: compatible\n";
	output << "- Compiled Metal effect container: compatible\n";
	output << "- Extracted material Black files: " << ( materialsLoaded ? "compatible" : "incomplete" ) << "\n";
	output << "- Existing bridge vertex declaration: " << ( missingVertexInputs ? "requires expansion" : "compatible with default permutation" ) << "\n";
	output << "- Material/global bindings: all opaque-area explicit resources are synchronously validated at startup\n";

	output << "\n## RC-05 Runtime Area Contract\n\n";
	output << "| GR2 group | SOF area | Batch | Authored effect | Runtime status |\n";
	output << "|---:|---|---|---|---|\n";
	output << "| 0 | `area_fx_distortion` | Distortion | `fx/fxdistortionv5.fx` | Geometry, effect, `DistortionMap`, and `Layer2Map` validated; submission deferred until the separately gated distortion compositor. |\n";
	output << "| 1 | `area_hull` | Opaque | `quad/quadv5.fx` | Rendered as an independent indexed batch. |\n";
	output << "| 2 | `area_booster` | Opaque | `quad/quadheatv5.fx` | Rendered independently with four authored heat parameter sets and booster glow color. |\n\n";
	output << "The hull declares no decal, transparent, or additive mesh areas. The root-object shader options explicitly disable clipping, projected-pattern textures, and instanced-attachment mode. "
			  "The ten decal sets, four sprite sets, two spotlight sets, four plane sets, and one light set are auxiliary SOF attachments rather than retained GR2 mesh groups. "
			  "The standalone bridge now reconstructs their active light descriptors through `ITr2LightOwner` and Trinity's tiled light manager without submitting attachment geometry. "
			  "Visible sprites, cones, planes, haze, and banners remain separately observable follow-up work.\n\n";
	output << "## RC-06 Dynamic-Light Contract\n\n";
	output << "The current `soebase` payload enables `primary` and `soe`; the Capsuleer Day explicit light strip is outside those groups and is excluded. "
			  "Active point and spotlight attachments retain Trinity blink, fade, noise, profile, and cone conversion through the native attachment-light wrappers. "
			  "The static CMF bridge uses identity rest-pose bone deltas and applies the same model center/fit normalization as its vertices. "
			  "Banner lights use native `EveBannerSet` average-color sampling from the staged Sisters of EVE faction banner fixture. The installed client `LogoLoader` would normally replace these alliance/corporation slots from its photo cache and otherwise uses `res:/texture/global/black.dds`. ";
	if( hasLightBuffer && hasLightIndexBuffer )
	{
		output << "The selected Metal V5 payload declares both tiled-light buffers, so opaque consumption is eligible for visual A/B acceptance. ";
	}
	else
	{
		output << "The selected Metal V5 payload does not declare both tiled-light buffers, so manager/list generation remains capability-only evidence. ";
	}
	output << "Local shadows, volumetrics, AO, and all visible attachment geometry remain disabled.\n\n";

	output << "## Per-Object Field Contract\n\n";
	output << "| Field family | Probe value | Classification |\n";
	output << "|---|---|---|\n";
	output << "| World/inverse/previous transforms | Rotating root transform | Derived each frame; previous equals current until velocity/TAA work. |\n";
	output << "| `shipData` | `(1, 1, 0, 2)` | Root scale, activation, authored default dirt, fitted radius. |\n";
	output << "| Clip sphere and custom masks | Disabled/zero; custom-mask matrices identity | Neutral because clipping and PPT permutations are disabled. |\n";
	output << "| Shape ellipsoid | `(-1, -1, -1, 0)` | Matches the unconfigured `EveSpaceObject2` sentinel. |\n";
	output << "| Bone and morph offsets | Zero | Static bind-pose bridge; no runtime animation or morph targets. |\n";
	output << "| SH coefficients | Scene manager or zero by isolation mode | RC-06 proves generation/upload; the selected opaque V5 passes do not consume the physical payload. |\n";
	output << "| Tiled local lights | Authored, validation, or disabled by isolation mode | Real `Tr2LightManager` buffers and `ComputeLightLists`; "
		   << ( hasLightBuffer && hasLightIndexBuffer ? "selected Metal V5 bindings present; visual A/B required for acceptance." : "selected Metal V5 bindings incomplete; opaque consumption unaccepted." )
		   << " |\n";
	output << "| Screen/custom/impact data | Zero | Neutral for the selected opaque permutations and absent impact/custom systems. |\n";

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

bool PrepareRenderProductVisualizer( StandaloneProbe& probe )
{
	if( probe.renderProductVisualizer )
	{
		return true;
	}
	probe.renderProductVisualizer.CreateInstance();
	probe.renderProductVisualizer->StartUpdate();
	probe.renderProductVisualizer->SetEffectPathName(
		"res:/graphics/effect/managed/space/spaceobject/probe/renderproductvisualizer.fx" );
	probe.renderProductVisualizer->SetParameter( BlueSharedString( "RenderProductMode" ), 1.0f );
	probe.renderProductVisualizer->EndUpdate();
	Tr2EffectRes* resource = probe.renderProductVisualizer->GetEffectRes();
	if( resource )
	{
		resource->ForceSynchronousLoad();
		resource->Reload();
	}
	Tr2Shader* shader = probe.renderProductVisualizer->GetShaderStateInterface();
	uint32_t technique = 0;
	if( !resource || !resource->IsGood() || !shader ||
		!shader->GetTechniqueIndex( BlueSharedString( "Main" ), technique ) )
	{
		std::fprintf( stderr, "EVE render-product visualization effect is unavailable\n" );
		probe.renderProductVisualizer.Unlock();
		return false;
	}
	std::fprintf( stderr, "EVE render-product visualization effect ready: Main=%u\n", technique );
	return true;
}

bool DrawDriverFrame( StandaloneProbe& probe, Be::Time realTime, Be::Time simTime, int captureProducts )
{
	Tr2RenderContext& renderContext = *probe.renderContext;
	EveSpaceSceneRenderDriver& driver = *probe.driver;
	const Tr2TextureAL& destination = renderContext.GetDefaultBackBuffer();
	const Tr2BitmapDimensions destinationDimensions = destination.GetDesc();
	std::array<BlueSharedString, 2> requestedNames;
	std::array<ITr2RenderNode::TempOutput, 2> outputStorage;
	size_t requestedCount = 0;
	if( captureProducts & STANDALONE_CAPTURE_DEPTH )
	{
		requestedNames[requestedCount] = BlueSharedString( "DepthMap" );
		outputStorage[requestedCount].name = requestedNames[requestedCount];
		++requestedCount;
	}
	if( captureProducts & STANDALONE_CAPTURE_NORMAL )
	{
		requestedNames[requestedCount] = BlueSharedString( "NormalMap" );
		outputStorage[requestedCount].name = requestedNames[requestedCount];
		++requestedCount;
	}

	ITr2RenderNode::Span<const Tr2BitmapDimensions> destinationDimensionSpan = { &destinationDimensions, 1 };
	ITr2RenderNode::Span<const BlueSharedString> requestedOutputs = { requestedNames.data(), requestedCount };
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
		ITr2RenderNode::Span<ITr2RenderNode::TempOutput> outputSpan = { outputStorage.data(), requestedCount };
		Tr2ProfileTimer rootTimer;
		driver.Execute( destinationSpan, outputSpan, realTime, simTime, rootTimer, renderContext );

		const char* selectedName = captureProducts == STANDALONE_CAPTURE_DEPTH ? "DepthMap" :
																				 ( captureProducts == STANDALONE_CAPTURE_NORMAL ? "NormalMap" : nullptr );
		if( selectedName )
		{
			const Tr2TextureAL* selectedTexture = nullptr;
			for( size_t outputIndex = 0; outputIndex < requestedCount; ++outputIndex )
			{
				if( outputStorage[outputIndex].name == BlueSharedString( selectedName ) && outputStorage[outputIndex].texture.IsValid() )
				{
					selectedTexture = &outputStorage[outputIndex].texture.Get();
					break;
				}
			}
			if( !selectedTexture )
			{
				std::fprintf( stderr, "Requested EVE render product '%s' was not produced\n", selectedName );
				ok = false;
			}
			else
			{
				if( PrepareRenderProductVisualizer( probe ) )
				{
					probe.renderProductVisualizer->SetParameter(
						BlueSharedString( "RenderProductMode" ),
						captureProducts == STANDALONE_CAPTURE_DEPTH ? 1.0f : 2.0f );
					renderContext.m_esm.SetRenderTarget( 0, destination );
					renderContext.m_esm.SetDepthStencilBuffer( {} );
					renderContext.RenderPassHint( { Tr2LoadAction::DONT_CARE, Tr2StoreAction::STORE }, {} );
					renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_FULLSCREEN );
					ok = Tr2Renderer::DrawTexture( renderContext, probe.renderProductVisualizer, *selectedTexture );
				}
				else
				{
					ok = false;
				}
				if( !ok )
				{
					std::fprintf( stderr, "Failed to visualize EVE render product '%s'\n", selectedName );
				}
			}
		}
		ok = CheckHresult( Tr2Renderer::EndRenderContext(), "Tr2Renderer::EndRenderContext" ) && ok;
	}
	Tr2Renderer::EndFrame();
	if( !ok )
	{
		return false;
	}
	return ok && CheckHresult( renderContext.Present(), "Present" );
}

void UpdateProbeCamera( StandaloneProbe& probe )
{
	constexpr uint64_t kStaticCameraFrames = 180;
	constexpr float kCameraRadius = 5.2f;
	constexpr float kOrbitFrames = 900.0f;
	if( !probe.view || !probe.renderable || probe.cameraView != STANDALONE_CAMERA_MODEL || probe.renderedFrameCount < kStaticCameraFrames )
	{
		return;
	}

	const float orbitFrame = static_cast<float>( probe.renderedFrameCount - kStaticCameraFrames + 1 );
	const float angle = orbitFrame * ( 2.0f * 3.1415926535f / kOrbitFrames );
	const Vector3 eye(
		kCameraRadius * std::sin( angle ),
		0.0f,
		-kCameraRadius * std::cos( angle ) );
	probe.view->SetLookAtPosition( eye, Vector3( 0.0f, 0.0f, 0.0f ), Vector3( 0.0f, 1.0f, 0.0f ) );
	if( !probe.reportedCameraOrbit )
	{
		std::fprintf( stderr, "EVE probe camera orbit active after %llu static frames: radius=%.1f period=%.1f seconds\n", static_cast<unsigned long long>( kStaticCameraFrames ), kCameraRadius, kOrbitFrames / 60.0f );
		probe.reportedCameraOrbit = true;
	}
}

bool ConfigureDriverScene( StandaloneProbe& probe, int qualityRung, const char* assetPath, int materialView, int materialMode, int areaView, const char* sceneResourcePath, int sceneFixture, int lightingView, int shSource, int localLights, int reflectionCorrection, int normalMapMode, int cameraView, int composition, int planetLayers, int cloudYear, int cloudMonth, int cloudDay, int sunEffects )
{
	if( localLights < STANDALONE_LOCAL_LIGHTS_OFF || localLights > STANDALONE_LOCAL_LIGHTS_VALIDATION )
	{
		CCP_LOGERR( "Invalid standalone local-light mode" );
		return false;
	}
	if( reflectionCorrection < STANDALONE_REFLECTION_CORRECTION_OFF || reflectionCorrection > STANDALONE_REFLECTION_CORRECTION_CLIENT )
	{
		CCP_LOGERR( "Invalid standalone reflection-correction mode" );
		return false;
	}
	if( normalMapMode < STANDALONE_NORMAL_MAP_AUTHORED || normalMapMode > STANDALONE_NORMAL_MAP_FLAT )
	{
		CCP_LOGERR( "Invalid standalone normal-map mode" );
		return false;
	}
	if( planetLayers < STANDALONE_PLANET_SURFACE || planetLayers > STANDALONE_PLANET_ALL ||
		cloudYear < 1 || cloudMonth < 1 || cloudMonth > 12 || cloudDay < 1 || cloudDay > 31 )
	{
		CCP_LOGERR( "Invalid standalone planet layer or cloud-date selection" );
		return false;
	}
	if( cameraView < STANDALONE_CAMERA_MODEL || cameraView > STANDALONE_CAMERA_PLANET )
	{
		CCP_LOGERR( "Invalid standalone camera view" );
		return false;
	}
	if( composition < STANDALONE_COMPOSITION_SYSTEM || composition > STANDALONE_COMPOSITION_CINEMATIC )
	{
		CCP_LOGERR( "Invalid standalone scene composition" );
		return false;
	}
	if( sunEffects < STANDALONE_SUN_EFFECTS_OFF || sunEffects > STANDALONE_SUN_EFFECTS_ALL )
	{
		CCP_LOGERR( "Invalid standalone sun-effects mode" );
		return false;
	}
	if( sceneFixture != 3 && sunEffects != STANDALONE_SUN_EFFECTS_OFF )
	{
		CCP_LOGERR( "Standalone sun effects require the New Eden scene fixture" );
		return false;
	}
	const bool enableGodRays = sunEffects == STANDALONE_SUN_EFFECTS_GOD_RAYS || sunEffects == STANDALONE_SUN_EFFECTS_ALL;
	if( enableGodRays && qualityRung < STANDALONE_PROBE_RUNG_HDR_POST )
	{
		CCP_LOGERR( "Standalone god rays require the hdr-post or hdr-exposure quality rung" );
		return false;
	}
	if( sceneFixture == 3 )
	{
		std::fprintf(
			stderr,
			"New Eden sun effects: mode=%s graphicId=1247 flare=res:/fisfx/lensflare/yellow_small.black "
			"godRayColor=(0.270588189, 0.278431386, 0.352941185, 1.0)\n",
			SunEffectsName( sunEffects ) );
	}
	probe.cameraView = cameraView;
	if( lightingView == STANDALONE_LIGHTING_DIRECT || lightingView == STANDALONE_LIGHTING_SH )
	{
		localLights = STANDALONE_LOCAL_LIGHTS_OFF;
	}
	probe.localLights = localLights;
	if( localLights != STANDALONE_LOCAL_LIGHTS_OFF )
	{
		const wchar_t* compiledPath = Tr2Renderer::GetShaderModel() == TR2SM_3_0_DEPTH ? L"res:/graphics/effect.metal/managed/space/system/computelightlists.sm_depth" : L"res:/graphics/effect.metal/managed/space/system/computelightlists.sm_hi";
		const char* effectPath = "res:/graphics/effect/managed/space/system/computelightlists.fx";
		if( !BePaths->FileExistsLocally( compiledPath ) )
		{
			CCP_LOGERR( "Local lights require compiled effect: %S", compiledPath );
			return false;
		}
		Tr2EffectPtr effect;
		effect.CreateInstance();
		effect->SetEffectPathName( effectPath );
		Tr2EffectRes* resource = effect->GetEffectRes();
		if( resource )
		{
			resource->ForceSynchronousLoad();
			resource->Reload();
		}
		Tr2Shader* shader = effect->GetShaderStateInterface();
		if( !resource || !resource->IsGood() || !shader || shader->GetPassCount( 0 ) == 0 )
		{
			CCP_LOGERR( "Local-light compute effect failed to prepare: %s", effectPath );
			return false;
		}
		g_eveSpaceSceneDynamicLighting = true;
		g_useDynamicLightsShadows = false;
		Tr2LightManager::DeleteInstance();
		if( !Tr2LightManager::GetOrCreateInstance( effectPath ) )
		{
			CCP_LOGERR( "Failed to create Tr2LightManager" );
			return false;
		}
		std::fprintf( stderr, "Trinity tiled local-light path ready: effect=%s passes=%u shadows=disabled\n", effectPath, shader->GetPassCount( 0 ) );
	}
	else
	{
		g_eveSpaceSceneDynamicLighting = false;
		Tr2LightManager::DeleteInstance();
	}
	if( qualityRung >= STANDALONE_PROBE_RUNG_HDR_BLIT )
	{
		struct RequiredEffect
		{
			const char* sourcePath;
			const wchar_t* compiledPath;
		};
		const RequiredEffect requiredEffects[] = {
			{ "res:/Graphics/Effect/Managed/Space/PostProcess/ToneMapping.fx", Tr2Renderer::GetShaderModel() == TR2SM_3_0_DEPTH ? L"res:/graphics/effect.metal/managed/space/postprocess/tonemapping.sm_depth" : L"res:/graphics/effect.metal/managed/space/postprocess/tonemapping.sm_hi" },
			{ "res:/Graphics/Effect/Managed/Space/System/Blit.fx", Tr2Renderer::GetShaderModel() == TR2SM_3_0_DEPTH ? L"res:/graphics/effect.metal/managed/space/system/blit.sm_depth" : L"res:/graphics/effect.metal/managed/space/system/blit.sm_hi" },
			{ "res:/Graphics/Effect/Managed/Space/System/BlitFiltered.fx", Tr2Renderer::GetShaderModel() == TR2SM_3_0_DEPTH ? L"res:/graphics/effect.metal/managed/space/system/blitfiltered.sm_depth" : L"res:/graphics/effect.metal/managed/space/system/blitfiltered.sm_hi" },
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
		if( enableGodRays )
		{
			const RequiredEffect godRayEffects[] = {
				{ "res:/Graphics/Effect/Managed/Space/PostProcess/Godrays.fx", Tr2Renderer::GetShaderModel() == TR2SM_3_0_DEPTH ? L"res:/graphics/effect.metal/managed/space/postprocess/godrays.sm_depth" : L"res:/graphics/effect.metal/managed/space/postprocess/godrays.sm_hi" },
				{ "res:/Graphics/Effect/Managed/Space/PostProcess/DownSampleDepth.fx", Tr2Renderer::GetShaderModel() == TR2SM_3_0_DEPTH ? L"res:/graphics/effect.metal/managed/space/postprocess/downsampledepth.sm_depth" : L"res:/graphics/effect.metal/managed/space/postprocess/downsampledepth.sm_hi" },
			};
			for( const RequiredEffect& required : godRayEffects )
			{
				if( !BePaths->FileExistsLocally( required.compiledPath ) )
				{
					CCP_LOGERR( "God rays require compiled effect: %S", required.compiledPath );
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
					CCP_LOGERR( "God-ray effect failed to prepare: %s", required.sourcePath );
					return false;
				}
				std::fprintf( stderr, "God-ray effect ready: %s passes=%u\n", required.sourcePath, shader->GetPassCount( 0 ) );
			}
			std::string noiseError;
			if( !PrepareTextureResourceWithoutYield( "res:/texture/global/noise.dds", "god-ray noise", noiseError ) )
			{
				std::fprintf( stderr, "Failed to prepare god-ray noise: %s\n", noiseError.c_str() );
				return false;
			}
		}

		if( qualityRung >= STANDALONE_PROBE_RUNG_HDR_EXPOSURE )
		{
			const RequiredEffect exposureEffects[] = {
				{ "res:/Graphics/Effect/Managed/Space/PostProcess/CreateHistograms.fx", Tr2Renderer::GetShaderModel() == TR2SM_3_0_DEPTH ? L"res:/graphics/effect.metal/managed/space/postprocess/createhistograms.sm_depth" : L"res:/graphics/effect.metal/managed/space/postprocess/createhistograms.sm_hi" },
				{ "res:/Graphics/Effect/Managed/Space/PostProcess/MergeHistograms.fx", Tr2Renderer::GetShaderModel() == TR2SM_3_0_DEPTH ? L"res:/graphics/effect.metal/managed/space/postprocess/mergehistograms.sm_depth" : L"res:/graphics/effect.metal/managed/space/postprocess/mergehistograms.sm_hi" },
				{ "res:/Graphics/Effect/Managed/Space/PostProcess/MeasureExposure.fx", Tr2Renderer::GetShaderModel() == TR2SM_3_0_DEPTH ? L"res:/graphics/effect.metal/managed/space/postprocess/measureexposure.sm_depth" : L"res:/graphics/effect.metal/managed/space/postprocess/measureexposure.sm_hi" },
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
		if( sceneFixture == 3 && !ConfigureNewEdenSystem( probe, *probe.scene, cameraView, composition, sunEffects, planetLayers, cloudYear, cloudMonth, cloudDay, sceneError ) )
		{
			std::fprintf( stderr, "Failed to configure New Eden system: %s\n", sceneError.c_str() );
			return false;
		}
		if( !PrepareSceneBackgroundWithoutYield( *probe.scene, sceneResourcePath, sceneError ) )
		{
			std::fprintf( stderr, "Failed to prepare EVE scene '%s': %s\n", sceneResourcePath, sceneError.c_str() );
			return false;
		}

		Tr2PostProcess2Ptr postProcess;
		if( qualityRung >= STANDALONE_PROBE_RUNG_HDR_EXPOSURE )
		{
			std::string postProcessError;
			postProcess = LoadBlackObjectWithoutYield<Tr2PostProcess2>(
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
			std::fprintf(
				stderr,
				"EVE dynamic exposure active: middle=%.4f influence=%.4f adjustment=%.4f range=[%.4f, %.4f]\n",
				exposure->m_middleValue,
				exposure->m_influence,
				exposure->m_adjustment,
				exposure->m_minExposure,
				exposure->m_maxExposure );
		}
		else if( enableGodRays && !postProcess.CreateInstance() )
		{
			CCP_LOGERR( "Failed to create standalone postprocess container for god rays" );
			return false;
		}
		if( enableGodRays )
		{
			Tr2PPGodRaysEffectPtr godRays;
			if( !godRays.CreateInstance() )
			{
				CCP_LOGERR( "Failed to create Tr2PPGodRaysEffect" );
				return false;
			}
			godRays->m_godRayColor = Color( 0.2705881894f, 0.2784313858f, 0.3529411852f, 1.0f );
			godRays->m_intensity = 1.0f;
			godRays->m_noiseTexturePath = BlueSharedString( "res:/texture/global/noise.dds" );
			postProcess->SetGodRays( godRays );
			std::fprintf(
				stderr,
				"New Eden god rays active: graphicId=1247 color=(%.9f, %.9f, %.9f, %.1f) "
				"intensity=%.1f noise=%s factors=(%.1f, %.1f, %.1f, %.1f)\n",
				godRays->m_godRayColor.r,
				godRays->m_godRayColor.g,
				godRays->m_godRayColor.b,
				godRays->m_godRayColor.a,
				godRays->m_intensity,
				godRays->m_noiseTexturePath.c_str(),
				godRays->grFactors.x,
				godRays->grFactors.y,
				godRays->grFactors.z,
				godRays->grFactors.w );
		}
		if( postProcess )
		{
			probe.scene->m_sceneDefaultPostProcess = postProcess;
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
	std::string shLightingError;
	if( !ConfigureShLighting( probe, lightingView, shSource, sceneFixture, shLightingError ) )
	{
		std::fprintf( stderr, "Failed to configure Trinity SH lighting: %s\n", shLightingError.c_str() );
		return false;
	}
	if( !probe.driver.CreateInstance() )
	{
		CCP_LOGERR( "Failed to create EveSpaceSceneRenderDriver" );
		return false;
	}
	const bool reflectionCorrectionEnabled = reflectionCorrection == STANDALONE_REFLECTION_CORRECTION_CLIENT;
	const std::string reflectionCorrectionPath = reflectionCorrectionEnabled ? "res:/texture/reflectioncorrection/128x128.dds" : "res:/texture/global/black.dds";
	std::string reflectionCorrectionError;
	if( !PrepareTextureResourceWithoutYield(
			reflectionCorrectionPath,
			reflectionCorrectionEnabled ? "client reflection correction" : "disabled reflection correction fallback",
			reflectionCorrectionError ) )
	{
		std::fprintf( stderr, "Failed to prepare reflection correction: %s\n", reflectionCorrectionError.c_str() );
		return false;
	}
	probe.driver->SetReflectionCorrectionEnabled( reflectionCorrectionEnabled );
	std::fprintf( stderr, "EVE reflection correction: mode=%s path=%s\n", reflectionCorrectionEnabled ? "client" : "off", reflectionCorrectionPath.c_str() );
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
	constexpr float newEdenPlanetWarpCenterDistance = 31245000.0f;
	const Vector3 planetPosition = composition == STANDALONE_COMPOSITION_CINEMATIC ? Normalize( Vector3( 0.55f, -0.12f, 1.0f ) ) * newEdenPlanetWarpCenterDistance : Vector3( 1083758787326.0f, -205372890997.0f, -787280443197.0f );
	const Vector3 eye( 0.0f, 0.0f, -5.2f );
	const bool exactSystemInspection = composition == STANDALONE_COMPOSITION_SYSTEM &&
		probe.celestialInspectionTarget && probe.celestialInspectionFovRadians > 0.0f;
	const Vector3 target = exactSystemInspection ? eye + probe.celestialInspectionDirection : ( cameraView == STANDALONE_CAMERA_CELESTIALS ? eye + Normalize( Vector3( 1069486940160.0f, -202669301760.0f, -831868968960.0f ) ) : ( cameraView == STANDALONE_CAMERA_PLANET ? planetPosition : Vector3( 0.0f, 0.0f, 0.0f ) ) );
	probe.view->SetLookAtPosition( eye, target, Vector3( 0.0f, 1.0f, 0.0f ) );
	const char* cameraName = cameraView == STANDALONE_CAMERA_CELESTIALS ? "celestials" :
																		  ( cameraView == STANDALONE_CAMERA_PLANET ? "planet" : "model" );
	const float cameraFovRadians = exactSystemInspection ? probe.celestialInspectionFovRadians : 60.0f * 3.1415926535f / 180.0f;
	std::fprintf( stderr, "EVE camera view: %s fov=%.8f degrees diagnostic=%s\n", cameraName, cameraFovRadians * 180.0f / 3.1415926535f, exactSystemInspection ? "yes" : "no" );
	probe.projection->PerspectiveFov( cameraFovRadians, aspect, 1.0f, 10000.0f );

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
	if( materialMode != 0 )
	{
		std::fprintf( stderr, "EVE distortion compositor disabled: Astero group 0 is validated but not submitted until RC-12\n" );
	}

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
		if( localLights != STANDALONE_LOCAL_LIGHTS_OFF )
		{
			auto hull = LoadBlackObjectWithoutYield<EveSOFDataHull>(
				"res:/dx9/model/spaceobjectfactory/hulls/soef1_t1.black", loadError );
			auto faction = LoadBlackObjectWithoutYield<EveSOFDataFaction>(
				"res:/dx9/model/spaceobjectfactory/factions/soebase.black", loadError );
			if( !hull || !faction || !probe.renderable->ConfigureLocalLights( localLights, *hull, *faction, loadError ) )
			{
				std::fprintf( stderr, "Failed to configure Astero local lights: %s\n", loadError.c_str() );
				return false;
			}
		}
		Tr2PrimaryRenderContext& primaryRenderContext = Tr2RenderContext_GetMainThreadRenderContext();
		if( !probe.renderable->InitializeGpu( primaryRenderContext, materialView, materialMode, areaView, normalMapMode ) )
		{
			CCP_LOGERR( "Failed to initialize the standalone EVE renderable GPU resources" );
			return false;
		}
		probe.renderable->SetShLightingEnabled( lightingView == STANDALONE_LIGHTING_COMBINED || lightingView == STANDALONE_LIGHTING_SH );
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

TRINITY_STANDALONE_EXPORT void* TrinityStandaloneProbeCreateDevice( void* windowHandle, uint32_t renderWidth, uint32_t renderHeight, int shaderTier )
{
	if( shaderTier < 0 || shaderTier > 1 )
	{
		CCP_LOGERR( "Invalid standalone shader tier" );
		return nullptr;
	}
	Tr2Renderer::SetShaderModel( shaderTier == 1 ? TR2SM_3_0_DEPTH : TR2SM_3_0_HI );
	std::fprintf( stderr, "Trinity standalone shader model: %s (%s client tier)\n", Tr2Renderer::GetShaderModelString( Tr2Renderer::GetShaderModel() ), shaderTier == 1 ? "high" : "medium" );

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

TRINITY_STANDALONE_EXPORT bool TrinityStandaloneProbeCreateEveScene( void* opaqueProbe, int qualityRung, const char* assetPath, int materialView, int materialMode, int areaView, const char* sceneResourcePath, int sceneFixture, int lightingView, int shSource, int localLights, int reflectionCorrection, int normalMapMode, int cameraView, int composition, int planetLayers, int cloudYear, int cloudMonth, int cloudDay, int sunEffects )
{
	auto* probe = static_cast<StandaloneProbe*>( opaqueProbe );
	if( !probe )
	{
		return false;
	}
	return ConfigureDriverScene( *probe, qualityRung, assetPath, materialView, materialMode, areaView, sceneResourcePath, sceneFixture, lightingView, shSource, localLights, reflectionCorrection, normalMapMode, cameraView, composition, planetLayers, cloudYear, cloudMonth, cloudDay, sunEffects );
}

TRINITY_STANDALONE_EXPORT bool TrinityStandaloneProbeRenderFrame( void* opaqueProbe, int qualityRung, int64_t realTime, int64_t simTime, int captureProducts )
{
	auto* probe = static_cast<StandaloneProbe*>( opaqueProbe );
	if( !probe || !probe->renderContext )
	{
		return false;
	}
	const bool freezeScene = ( captureProducts & STANDALONE_CAPTURE_FREEZE_SCENE ) != 0;
	const int captureProduct = captureProducts & ~STANDALONE_CAPTURE_FREEZE_SCENE;
	if( captureProduct != 0 && captureProduct != STANDALONE_CAPTURE_COLOR &&
		captureProduct != STANDALONE_CAPTURE_DEPTH && captureProduct != STANDALONE_CAPTURE_NORMAL )
	{
		CCP_LOGERR( "Invalid standalone render-product selection" );
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
	if( !freezeScene )
	{
		UpdateProbeCamera( *probe );
	}
	probe->device->SetAnimationTime( static_cast<float>( simTime ) / 10000000.0f );
	const bool rendered = DrawDriverFrame( *probe, static_cast<Be::Time>( realTime ), static_cast<Be::Time>( simTime ), captureProduct );
	if( rendered && !freezeScene )
	{
		++probe->renderedFrameCount;
	}
	if( rendered && !freezeScene && probe->celestialInspectionTarget &&
		!probe->celestialInspectionValidated && probe->renderedFrameCount >= 2 )
	{
		const float reported = probe->celestialInspectionTarget->GetEstimatedPixelDiameter();
		const float expected = probe->celestialExpectedPixelDiameter;
		const float relativeError = expected > 0.0f ? std::abs( reported - expected ) / expected : 1.0f;
		std::fprintf(
			stderr,
			"New Eden celestial inspection validation: target=%s expectedPixels=%.4f trinityPixels=%.4f "
			"relativeError=%.4f scale=%.6f fovDegrees=%.8f\n",
			probe->celestialInspectionName,
			expected,
			reported,
			relativeError,
			probe->celestialInspectionScale,
			probe->celestialInspectionFovRadians * 180.0f / 3.1415926535f );
		if( !std::isfinite( reported ) || relativeError > 0.05f )
		{
			CCP_LOGERR( "New Eden celestial inspection pixel diameter differs from the derived target by more than five percent" );
			return false;
		}
		probe->celestialInspectionValidated = true;
	}
	if( rendered && probe->localLights != STANDALONE_LOCAL_LIGHTS_OFF )
	{
		Tr2LightManager* manager = Tr2LightManager::GetInstance();
		if( !manager || FAILED( manager->GetLastUpdateResult() ) )
		{
			CCP_LOGERR( "Trinity tiled local-light list generation failed" );
			return false;
		}
		const size_t resolved = manager->GetResolvedLightCount();
		if( probe->localLights == STANDALONE_LOCAL_LIGHTS_VALIDATION && resolved == 0 )
		{
			CCP_LOGERR( "Synthetic validation light resolved to zero tiled lights" );
			return false;
		}
		if( !probe->reportedResolvedLights )
		{
			const uint32_t tilesX = ( probe->renderWidth + 15 ) / 16;
			const uint32_t tilesY = ( probe->renderHeight + 15 ) / 16;
			std::fprintf( stderr, "Trinity tiled local-light result: resolved=%zu tile-grid=%ux%u tile-count=%u update=success\n", resolved, tilesX, tilesY, tilesX * tilesY );
			probe->reportedResolvedLights = true;
		}
	}
	return rendered && ( !probe->renderable || !probe->renderable->DrawFailed() );
}
