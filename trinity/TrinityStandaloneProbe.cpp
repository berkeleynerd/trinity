// Copyright (c) 2026 CCP Games

#include "StdAfx.h"

#include <Blue.h>

#include "Eve/EveSpaceScene.h"
#include "Eve/EveSpaceSceneRenderDriver.h"
#include "Eve/EveEntity.h"
#include "Eve/EveLensflare.h"
#include "Eve/EveOccluder.h"
#include "Eve/EveStarfield.h"
#include "Eve/IEveShadowCaster.h"
#include "Eve/IEveSpaceObject2.h"
#include "Eve/SpaceObject/Attachments/EveSpaceObjectDecal.h"
#include "Eve/SpaceObject/Attachments/IEveSpaceObjectDecalOwner.h"
#include "Eve/SpaceObjectFactory/EveSOFData.h"
#include "Eve/SpaceObjectFactory/EveSOFDataMgr.h"
#include "Eve/SpaceObject/Attachments/Sets/EveBannerSet.h"
#include "Eve/SpaceObject/Attachments/Sets/EveHazeSetItem.h"
#include "Eve/SpaceObject/Attachments/Sets/EveHazeSet.h"
#include "Eve/SpaceObject/Attachments/Sets/EvePlaneSet.h"
#include "Eve/SpaceObject/Attachments/Sets/EvePlaneSetItem.h"
#include "Eve/SpaceObject/Attachments/Sets/EveSpotlightSet.h"
#include "Eve/SpaceObject/Attachments/Sets/EveSpotlightSetItem.h"
#include "Eve/SpaceObject/Attachments/Sets/EveSpriteLineSet.h"
#include "Eve/SpaceObject/Attachments/Sets/EveSpriteLineSetItem.h"
#include "Eve/SpaceObject/Attachments/Sets/EveSpriteSet.h"
#include "Eve/SpaceObject/Attachments/Sets/EveSpriteSetItem.h"
#include "Eve/SpaceObject/Children/EveChildContainer.h"
#include "Eve/SpaceObject/Children/EveChildLineSet.h"
#include "Eve/SpaceObject/Children/EveChildParticleSystem.h"
#include "Eve/SpaceObject/Children/EveChildQuad.h"
#include "Eve/SpaceObject/Children/EveChildRef.h"
#include "Eve/SpaceObject/Children/EveChildSocket.h"
#include "ITr2TextureProvider.h"
#include "ITr2Renderable.h"
#include "Lights/ITr2LightOwner.h"
#include "Lights/Tr2Light.h"
#include "Lights/Tr2PointLight.h"
#include "Lights/Tr2SpotLight.h"
#include "Lights/Tr2TexturedPointLight.h"
#include "PostProcess/Tr2PostProcess2.h"
#include "Tr2Mesh.h"
#include "Tr2ProfileTimer.h"
#include "Tr2ReflectionProbe.h"
#include "Tr2MeshBase.h"
#include "Tr2PerObjectData.h"
#include "Tr2Renderer.h"
#include "Tr2SSAO.h"
#include "Tr2ShLightingManager.h"
#include "Resources/TriTextureRes.h"
#include "Resources/TriGeometryRes.h"
#include "Resources/Tr2LightProfileRes.h"
#include "Shader/Tr2Effect.h"
#include "Shader/Parameter/TriTextureParameter.h"
#include "Shader/Tr2Shader.h"
#include "TrinityStandaloneCmfModel.h"
#include "TrinityStandaloneProbeApi.h"
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
#include <atomic>
#include <cmath>
#include <fstream>
#include <functional>
#include <initializer_list>
#include <map>
#include <sstream>
#include <set>
#include <string>
#include <vector>

extern "C" void TrinityStandaloneStartup();
extern "C" bool BlueInitializeResourceLoading();
extern bool g_eveSpaceSceneDynamicLighting;
extern float g_eveSpaceSceneGammaBrightness;

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
struct StandaloneProbe;
bool ConfigureAsteroEveV5Effect( Tr2Effect& effect, uint32_t sourceGroup, int normalMapMode, std::string& error );
std::string ToNarrowPath( const wchar_t* path );
bool PrepareTextureResourceWithoutYield( const std::string& logicalPath, const char* role, std::string& error );
bool PrepareReflectionProductVisualizer( StandaloneProbe& probe );

enum StandaloneLocalShadowMode
{
	STANDALONE_LOCAL_SHADOWS_OFF = 0,
	STANDALONE_LOCAL_SHADOWS_AUTHORED = 1,
	STANDALONE_LOCAL_SHADOWS_VALIDATION = 2,
};

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

static_assert( sizeof( EveSpaceScene::PerFramePSData ) == 1888, "V5 per-frame shader contract changed" );
static_assert( sizeof( EveSpaceObjectPSData ) == 464, "V5 per-object shader contract changed" );
}

BLUE_CLASS( TrinityStandaloneRenderable ) :
	public IEveSpaceObject2,
	public ITr2Renderable,
	public ITr2ShLightingReceiver,
	public ITr2LightOwner,
	public IEveShadowCaster,
	public IEveSpaceObjectDecalOwner,
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
		if( !m_model.Load( path, error ) )
		{
			return false;
		}
		float centerScale[4];
		m_model.GetCenterAndScale( centerScale );
		m_shadowBoundingRadius = 0.0f;
		for( const TrinityStandaloneEveV5Vertex& vertex : m_model.EveV5Vertices() )
		{
			m_shadowBoundingRadius = std::max(
				m_shadowBoundingRadius,
				Length( Vector3(
					vertex.position[0] - centerScale[0],
					vertex.position[1] - centerScale[1],
					vertex.position[2] - centerScale[2] ) ) );
		}
		m_authoredWorldScale = centerScale[3] > 0.0f ? 1.0f / centerScale[3] : 1.0f;
		std::fprintf(
			stderr,
			"CMF authored coordinate contract: center=(%.8f,%.8f,%.8f) fitScale=%.8f rawRadius=%.8f\n",
			centerScale[0],
			centerScale[1],
			centerScale[2],
			centerScale[3],
			m_shadowBoundingRadius );
		return m_shadowBoundingRadius > 0.0f;
	}

	void SetShadowCastingEnabled( bool enabled )
	{
		m_shadowCastingEnabled = enabled;
	}

	float GetAuthoredWorldScale() const
	{
		return m_authoredWorldScale;
	}

	void SetModelYawDegrees( float degrees )
	{
		m_modelYawOffset = degrees * 3.1415926535f / 180.0f;
		std::fprintf( stderr, "Standalone model yaw offset: %.6f degrees\n", degrees );
	}

	bool ConfigureLocalLights( int mode, int localShadowMode, const EveSOFDataHull& hull, const EveSOFDataFaction& faction, std::string& error )
	{
		m_localLightMode = mode;
		m_localShadowMode = localShadowMode;
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
			if( localShadowMode == STANDALONE_LOCAL_SHADOWS_VALIDATION )
			{
				data.castsShadows = PerLightShadowSetting::ENABLED_ONLY_ON_HIGH_QUALITY;
				++m_lightStats[LIGHT_FAMILY_VALIDATION].shadowEligible;
			}
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

		for( const auto& setData : hull.m_spriteSets )
		{
			const bool active = isActive( setData->m_visibilityGroup );
			EveSpriteSetPtr set;
			uint32_t itemOrdinal = 0;
			for( const auto& item : setData->m_items )
			{
				const uint32_t ordinal = itemOrdinal++;
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
				NormalizeAuthoredLight( data, LIGHT_FAMILY_SPRITE );
				if( !set )
					set.CreateInstance();
				set->AddLightFromSOF( EveSpriteLight( data, item->m_blinkPhase, item->m_blinkRate, item->m_minScale, item->m_maxScale, ordinal, item->m_light->m_lightProfilePath ) );
				ConstructLight( LIGHT_FAMILY_SPRITE, data.boneIndex );
			}
			if( set )
				m_spriteLightSets.push_back( set );
		}

		for( const auto& setData : hull.m_spriteLineSets )
		{
			const bool active = isActive( setData->m_visibilityGroup );
			EveSpriteLineSetPtr set;
			uint32_t itemOrdinal = 0;
			for( const auto& item : setData->m_items )
			{
				const uint32_t ordinal = itemOrdinal++;
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
					NormalizeAuthoredLight( positioned, LIGHT_FAMILY_SPRITE_LINE );
					if( !set )
						set.CreateInstance();
					set->AddLightFromSOF( EveSpriteLight( positioned, item->m_blinkPhase + item->m_blinkPhaseShift * positionIndex++, item->m_blinkRate, item->m_minScale, item->m_maxScale, ordinal, item->m_light->m_lightProfilePath ) );
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
			uint32_t itemOrdinal = 0;
			for( const auto& item : setData->m_items )
			{
				const uint32_t ordinal = itemOrdinal++;
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
				NormalizeAuthoredLight( data, LIGHT_FAMILY_SPOTLIGHT );
				if( !set )
					set.CreateInstance();
				set->AddLightFromSOF( EveSpotlightLight( data, ordinal, item->m_light->m_lightProfilePath, item->m_boosterGainInfluence ) );
				ConstructLight( LIGHT_FAMILY_SPOTLIGHT, data.boneIndex );
			}
			if( set )
				m_spotlightLightSets.push_back( set );
		}

		for( const auto& setData : hull.m_planeSets )
		{
			const bool active = isActive( setData->m_visibilityGroup );
			EvePlaneSetPtr set;
			uint32_t itemOrdinal = 0;
			for( const auto& item : setData->m_items )
			{
				const uint32_t ordinal = itemOrdinal++;
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
					NormalizeAuthoredLight( data, LIGHT_FAMILY_PLANE );
					if( !set )
						set.CreateInstance();
					set->AddLightFromSOF( EvePlaneLight( data, rawLight->m_saturation, ordinal, rawLight->m_lightProfilePath, static_cast<EveSpaceObjectAttachmentUtils::FadeType>( item->m_blinkMode ), item->m_phase, item->m_rate ) );
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
			if( active )
				set.CreateInstance();
			uint32_t itemOrdinal = 0;
			for( const auto& item : setData->m_items )
			{
				const uint32_t ordinal = itemOrdinal++;
				if( active )
				{
					Color itemColor;
					if( !colorFor( item->m_colorType, itemColor ) )
						return false;
					EveHazeSetItemPtr runtime;
					runtime.CreateInstance();
					runtime->m_position = NormalizeAuthoredPosition( item->m_position );
					runtime->m_scaling = item->m_scaling * NormalizeAuthoredLength( 1.0f );
					runtime->m_rotation = item->m_rotation;
					runtime->m_color = Saturate( item->m_hazeBrightness * itemColor, item->m_saturation );
					runtime->m_hazeData = Vector4( item->m_hazeFalloff, item->m_sourceSize, item->m_sourceBrightness, item->m_boosterGainInfluence ? 1.0f : 0.0f );
					runtime->m_boneIndex = item->m_boneIndex;
					set->AddHazeItem( runtime );
				}
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
					NormalizeAuthoredLight( data, LIGHT_FAMILY_HAZE );
					set->AddLightFromSOF( EveHazeSetLight( data, ordinal, rawLight->m_lightProfilePath, item->m_boosterGainInfluence ) );
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
			if( active )
				set.CreateInstance();
			TriTextureParameterPtr imageMap;
			uint32_t itemOrdinal = 0;
			for( const auto& item : setData->m_banners )
			{
				const uint32_t ordinal = itemOrdinal++;
				if( active )
				{
					EveBannerItem runtime;
					runtime.bone = item->m_boneIndex;
					runtime.position = NormalizeAuthoredPosition( item->m_position );
					runtime.rotation = item->m_rotation;
					runtime.scaling = item->m_scaling * NormalizeAuthoredLength( 1.0f );
					runtime.angleX = item->m_angleX;
					runtime.angleY = item->m_angleY;
					runtime.reference = static_cast<int32_t>( item->m_usage );
					set->AddBanner( runtime );
				}
				if( !item->m_light )
					continue;
				DeclareLight( LIGHT_FAMILY_BANNER, active );
				std::fprintf( stderr, "Astero banner descriptor: name=%s usage=%d visibility=%s active=%s\n", item->m_name.c_str(), static_cast<int>( item->m_usage ), setData->m_visibilityGroup.c_str(), active ? "yes" : "no" );
				if( !active )
					continue;
				if( !ValidateLightProfile( item->m_light->m_lightProfilePath, error ) )
					return false;
				if( !imageMap )
				{
					const char* bannerPath = "res:/dx9/model/shared/faction_logos/soe_logo.dds";
					if( !PrepareTextureResourceWithoutYield( bannerPath, "Astero Sisters of EVE square logo", error ) )
						return false;
					imageMap.CreateInstance();
					imageMap->SetParameterName( BlueSharedString( "ImageMap" ) );
					imageMap->SetResourcePath( bannerPath );
					set->SetPrimaryTextureParameter( imageMap );
					std::fprintf( stderr, "Astero dynamic logo texture: deterministic square SoE fixture=%s client-no-identity-fallback=res:/texture/global/black.dds\n", bannerPath );
				}
				Color color( 0.0f, 0.0f, 0.0f, 0.0f );
				EveSOFDataMgr::PointLightAttachment attachment( *item->m_light );
				const float scale = std::max( item->m_scaling.x, std::max( item->m_scaling.y, item->m_scaling.z ) );
				LightData data = attachment.AsLightData( color, scale );
				data.position = Transform( data.position, RotationMatrix( item->m_rotation ) ) + item->m_position;
				data.rotation = Normalize( data.rotation * item->m_rotation );
				data.boneIndex = item->m_boneIndex;
				NormalizeAuthoredLight( data, LIGHT_FAMILY_BANNER );
				set->AddLightFromSOF( EveBannerLight( data, item->m_light->m_saturation, ordinal, item->m_light->m_lightProfilePath ) );
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
				NormalizeAuthoredLight( data, LIGHT_FAMILY_EXPLICIT );
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

	bool ConfigureAttachments( int mode, int view, const EveSOFDataHull& hull, const EveSOFDataFaction& faction, const EveSOFDataGeneric& generic, std::string& error )
	{
		m_attachmentMode = mode;
		m_attachmentView = view;
		if( mode == 1 )
		{
			std::fprintf( stderr, "Astero visible attachments disabled\n" );
			return true;
		}
		if( mode != 2 || !faction.m_colorSet || !faction.m_visibilityGroupSet )
		{
			error = "Invalid authored-attachment configuration";
			return false;
		}

		std::set<std::string> visibilityGroups;
		for( const auto& value : faction.m_visibilityGroupSet->m_visibilityGroups )
			visibilityGroups.insert( value->m_str );
		auto isActive = [&]( const BlueSharedString& group ) { return visibilityGroups.count( group.c_str() ) != 0; };
		auto selected = [&]( int family ) { return view == 0 || view == family; };
		auto colorFor = [&]( SOFDataFactionColorChooser::ColorType type, Color& color ) {
			const int index = static_cast<int>( type );
			if( index < 0 || index >= SOFDataFactionColorChooser::TYPE_MAX )
			{
				error = "Astero attachment has invalid faction color index " + std::to_string( index );
				return false;
			}
			color = faction.m_colorSet->m_colors[index];
			return true;
		};

		Tr2EffectPtr spriteEffect;
		if( selected( 1 ) || selected( 2 ) )
		{
			if( !CreateAttachmentEffect( spriteEffect, "res:/graphics/effect/managed/space/spaceobject/fx/blinkinglightspool.fx", "sprite", error ) )
				return false;
		}

		for( const auto& setData : hull.m_spriteSets )
		{
			CountAttachmentSet( ATTACHMENT_FAMILY_SPRITE, setData->m_items.size(), isActive( setData->m_visibilityGroup ) );
			if( !isActive( setData->m_visibilityGroup ) || !selected( 1 ) )
				continue;
			EveSpriteSetPtr set;
			set.CreateInstance();
			set->SetEffect( spriteEffect );
			set->SetSkinned( false );
			for( const auto& item : setData->m_items )
			{
				Color color;
				if( !colorFor( item->m_colorType, color ) )
					return false;
				EveSpriteSetItemPtr runtime;
				runtime.CreateInstance();
				runtime->m_position = NormalizeAuthoredPosition( item->m_position );
				runtime->m_blinkRate = item->m_blinkRate;
				runtime->m_blinkPhase = item->m_blinkPhase;
				runtime->m_minScale = NormalizeAuthoredLength( item->m_minScale );
				runtime->m_maxScale = NormalizeAuthoredLength( item->m_maxScale );
				runtime->m_falloff = item->m_falloff;
				runtime->m_color = Saturate( item->m_intensity * color, item->m_saturation );
				runtime->m_warpColor = runtime->m_color;
				runtime->m_boneIndex = item->m_boneIndex;
				ReportAttachmentBone( ATTACHMENT_FAMILY_SPRITE, item->m_boneIndex );
				set->Add( runtime );
			}
			set->Rebuild();
			if( !set->Initialize() )
			{
				error = "Failed to initialize Astero sprite attachment set";
				return false;
			}
			m_spriteAttachmentSets.push_back( set );
		}

		for( const auto& setData : hull.m_spriteLineSets )
		{
			CountAttachmentSet( ATTACHMENT_FAMILY_SPRITE_LINE, setData->m_items.size(), isActive( setData->m_visibilityGroup ) );
			if( !isActive( setData->m_visibilityGroup ) || !selected( 2 ) )
				continue;
			EveSpriteLineSetPtr set;
			set.CreateInstance();
			set->Setup( spriteEffect, false );
			for( const auto& item : setData->m_items )
			{
				Color color;
				if( !colorFor( item->m_colorType, color ) )
					return false;
				EveSpriteLineSetItemPtr runtime;
				runtime.CreateInstance();
				runtime->m_position = NormalizeAuthoredPosition( item->m_position );
				runtime->m_rotation = item->m_rotation;
				runtime->m_scaling = item->m_scaling;
				runtime->m_spacing = item->m_spacing;
				if( item->m_isCircle )
					runtime->m_scaling *= NormalizeAuthoredLength( 1.0f );
				else
					runtime->m_spacing = NormalizeAuthoredLength( item->m_spacing );
				runtime->m_isCircle = item->m_isCircle;
				runtime->m_blinkRate = item->m_blinkRate;
				runtime->m_blinkPhase = item->m_blinkPhase;
				runtime->m_blinkPhaseShift = item->m_blinkPhaseShift;
				runtime->m_minScale = NormalizeAuthoredLength( item->m_minScale );
				runtime->m_maxScale = NormalizeAuthoredLength( item->m_maxScale );
				runtime->m_falloff = item->m_falloff;
				runtime->m_color = Saturate( item->m_intensity * color, item->m_saturation );
				runtime->m_boneIndex = item->m_boneIndex;
				ReportAttachmentBone( ATTACHMENT_FAMILY_SPRITE_LINE, item->m_boneIndex );
				set->Add( runtime );
			}
			set->Rebuild();
			if( !set->Initialize() )
			{
				error = "Failed to initialize Astero sprite-line attachment set";
				return false;
			}
			m_spriteLineAttachmentSets.push_back( set );
		}

		for( const auto& setData : hull.m_spotlightSets )
		{
			CountAttachmentSet( ATTACHMENT_FAMILY_SPOTLIGHT, setData->m_items.size(), isActive( setData->m_visibilityGroup ) );
			if( !isActive( setData->m_visibilityGroup ) || !selected( 3 ) )
				continue;
			Tr2EffectPtr cone, glow;
			if( !CreateTexturedAttachmentEffect( cone, "res:/graphics/effect/managed/space/spaceobject/fx/spotlightconepool.fx", setData->m_coneTextureResPath, "spotlight cone", error, setData->m_zOffset ) ||
				!CreateTexturedAttachmentEffect( glow, "res:/graphics/effect/managed/space/spaceobject/fx/spotlightglowpool.fx", setData->m_glowTextureResPath, "spotlight glow", error ) )
				return false;
			EveSpotlightSetPtr set;
			set.CreateInstance();
			set->SetConeEffect( cone );
			set->SetGlowEffect( glow );
			set->SetSkinned( false );
			for( const auto& item : setData->m_items )
			{
				Color color;
				if( !colorFor( item->m_colorType, color ) )
					return false;
				Vector3 scale, position;
				Quaternion rotation;
				Decompose( scale, rotation, position, item->m_transform );
				EveSpotlightSetItemPtr runtime;
				runtime.CreateInstance();
				runtime->m_transform = TransformationMatrix( scale * NormalizeAuthoredLength( 1.0f ), rotation, NormalizeAuthoredPosition( position ) );
				runtime->m_spriteScale = item->m_spriteScale * NormalizeAuthoredLength( 1.0f );
				runtime->m_coneColor = item->m_coneIntensity * ModifyAttachmentColor( color, item->m_saturation * 0.75f, 0.5f );
				runtime->m_flareColor = item->m_flareIntensity * ModifyAttachmentColor( color, item->m_saturation, 1.0f );
				runtime->m_spriteColor = item->m_spriteIntensity * ModifyAttachmentColor( color, item->m_saturation * 0.9f, 0.75f );
				runtime->m_boneIndex = item->m_boneIndex;
				runtime->m_boosterGainInfluence = item->m_boosterGainInfluence;
				ReportAttachmentBone( ATTACHMENT_FAMILY_SPOTLIGHT, item->m_boneIndex );
				set->AddSpotlightItem( runtime );
			}
			set->Rebuild();
			if( !set->Initialize() )
			{
				error = "Failed to initialize Astero spotlight attachment set";
				return false;
			}
			m_spotlightAttachmentSets.push_back( set );
		}

		for( const auto& setData : hull.m_planeSets )
		{
			CountAttachmentSet( ATTACHMENT_FAMILY_PLANE, setData->m_items.size(), isActive( setData->m_visibilityGroup ) );
			if( !isActive( setData->m_visibilityGroup ) || !selected( 4 ) )
				continue;
			if( setData->m_usage == EveSOFDataHullPlaneSet::USAGE_SPACE_VIDEO || setData->m_usage == EveSOFDataHullPlaneSet::USAGE_HANGAR_VIDEO )
			{
				error = "Unsupported active Astero plane-video attachment usage";
				return false;
			}
			Tr2EffectPtr effect;
			EvePlaneSetPtr set;
			set.CreateInstance();
			if( !CreatePlaneAttachmentEffect( effect, *setData, *set, error ) )
				return false;
			set->SetEffect( effect );
			set->SetIsSkinned( false );
			for( const auto& item : setData->m_items )
			{
				Color color;
				if( !colorFor( item->m_colorType, color ) )
					return false;
				EvePlaneSetItemPtr runtime;
				runtime.CreateInstance();
				runtime->m_position = NormalizeAuthoredPosition( item->m_position );
				runtime->m_scaling = item->m_scaling * NormalizeAuthoredLength( 1.0f );
				runtime->m_rotation = item->m_rotation;
				runtime->m_color = Saturate( item->m_intensity * color, item->m_saturation );
				runtime->m_layer1Transform = item->m_layer1Transform;
				runtime->m_layer2Transform = item->m_layer2Transform;
				runtime->m_layer1Scroll = item->m_layer1Scroll;
				runtime->m_layer2Scroll = item->m_layer2Scroll;
				runtime->m_boneIndex = item->m_boneIndex;
				runtime->m_maskAtlasID = static_cast<uint32_t>( item->m_maskMapAtlasIndex );
				runtime->m_blinkData = Vector4( item->m_rate, item->m_phase, item->m_dutyCycle, static_cast<float>( item->m_blinkMode ) );
				ReportAttachmentBone( ATTACHMENT_FAMILY_PLANE, item->m_boneIndex );
				set->AddPlaneItem( runtime );
			}
			set->Rebuild();
			if( !set->Initialize() )
			{
				error = "Failed to initialize Astero plane attachment set";
				return false;
			}
			m_planeAttachmentSets.push_back( set );
		}

		for( const auto& setData : hull.m_hazeSets )
		{
			CountAttachmentSet( ATTACHMENT_FAMILY_HAZE, setData->m_items.size(), isActive( setData->m_visibilityGroup ) );
			if( !isActive( setData->m_visibilityGroup ) || !selected( 5 ) )
				continue;
			if( setData->m_hazeType != EveSOFDataHullHazeSet::TYPE_SPHERICAL )
			{
				error = "Unsupported active Astero half-spherical haze attachment";
				return false;
			}
			Tr2EffectPtr effect;
			if( !CreateAttachmentEffect( effect, "res:/graphics/effect/managed/space/spaceobject/fx/hazespherical.fx", "spherical haze", error ) )
				return false;
			EveHazeSetPtr set;
			set.CreateInstance();
			set->Setup( effect );
			for( const auto& item : setData->m_items )
			{
				Color color;
				if( !colorFor( item->m_colorType, color ) )
					return false;
				EveHazeSetItemPtr runtime;
				runtime.CreateInstance();
				runtime->m_position = NormalizeAuthoredPosition( item->m_position );
				runtime->m_scaling = item->m_scaling * NormalizeAuthoredLength( 1.0f );
				runtime->m_rotation = item->m_rotation;
				runtime->m_color = Saturate( item->m_hazeBrightness * color, item->m_saturation );
				runtime->m_hazeData = Vector4( item->m_hazeFalloff, item->m_sourceSize, item->m_sourceBrightness, item->m_boosterGainInfluence ? 1.0f : 0.0f );
				runtime->m_boneIndex = item->m_boneIndex;
				ReportAttachmentBone( ATTACHMENT_FAMILY_HAZE, item->m_boneIndex );
				set->AddHazeItem( runtime );
			}
			set->Rebuild();
			if( !set->Initialize() )
			{
				error = "Failed to initialize Astero haze attachment set";
				return false;
			}
			m_hazeAttachmentSets.push_back( set );
		}

		if( selected( 6 ) && !ConfigureBannerAttachments( hull, generic, visibilityGroups, error ) )
			return false;
		else if( !selected( 6 ) )
		{
			for( const auto& setData : hull.m_bannerSets )
				CountAttachmentSet( ATTACHMENT_FAMILY_BANNER, setData->m_banners.size(), isActive( setData->m_visibilityGroup ) );
		}

		ReportAttachmentStats();
		if( m_attachmentView > 0 && m_attachmentStats[m_attachmentView - 1].active == 0 && m_attachmentView != 2 )
		{
			error = "Selected Astero attachment family unexpectedly has zero active items";
			return false;
		}
		return true;
	}

	bool ConfigureDecals( int mode,
						  int view,
						  uint32_t killCount,
						  const EveSOFDataHull& hull,
						  const EveSOFDataFaction& faction,
						  const EveSOFDataGeneric& generic,
						  std::string& error )
	{
		m_decalMode = mode;
		m_decalView = view;
		m_killCount = killCount;
		if( mode == 1 )
		{
			std::fprintf( stderr, "Astero indexed decals disabled\n" );
			return true;
		}
		if( mode != 2 || view < 0 || view > 3 || !faction.m_visibilityGroupSet || !faction.m_colorSet )
		{
			error = "Invalid authored Astero decal configuration";
			return false;
		}
		if( !PrepareDecalGeometry( error ) )
		{
			return false;
		}

		std::set<std::string> visibilityGroups;
		for( const auto& value : faction.m_visibilityGroupSet->m_visibilityGroups )
		{
			visibilityGroups.insert( value->m_str );
		}

		uint32_t activeSetCount = 0;
		uint32_t activeItemCount = 0;
		uint32_t excludedSetCount = 0;
		uint32_t excludedItemCount = 0;
		uint32_t activeLod0IndexCount = 0;
		uint32_t maximumLod0Index = 0;
		for( const auto& setData : hull.m_decalSets )
		{
			++m_decalDeclaredSetCount;
			m_decalDeclaredItemCount += static_cast<uint32_t>( setData->m_items.size() );
			const bool active = visibilityGroups.count( setData->m_visibilityGroup.c_str() ) != 0;
			if( active )
			{
				++activeSetCount;
			}
			else
			{
				++excludedSetCount;
				excludedItemCount += static_cast<uint32_t>( setData->m_items.size() );
			}

			for( const auto& item : setData->m_items )
			{
				if( !active )
				{
					continue;
				}
				++activeItemCount;
				if( item->m_boneIndex != -1 )
				{
					error = "Active Astero decal unexpectedly requires bone " + std::to_string( item->m_boneIndex ) + ": " + item->m_name;
					return false;
				}

				DecalFamily family;
				const char* effectName = nullptr;
				if( item->m_usage == EveSOFDataHullDecalSetItem::USAGE_STANDARD )
				{
					family = DECAL_FAMILY_STANDARD;
					effectName = "decalv5.fx";
				}
				else if( item->m_usage == EveSOFDataHullDecalSetItem::USAGE_LOGO )
				{
					family = DECAL_FAMILY_LOGO;
					effectName = "decalv5.fx";
				}
				else if( item->m_usage == EveSOFDataHullDecalSetItem::USAGE_KILLCOUNTER )
				{
					family = DECAL_FAMILY_KILLMARK;
					effectName = "decalcounterv5.fx";
				}
				else
				{
					error = "Unsupported active Astero decal usage " + std::to_string( static_cast<int>( item->m_usage ) ) + ": " + item->m_name;
					return false;
				}

				std::vector<std::vector<uint32_t>> indexBuffers;
				indexBuffers.reserve( item->m_indexBuffers.size() );
				for( const auto& source : item->m_indexBuffers )
				{
					if( source->m_indexBuffer.size() % 3 != 0 )
					{
						error = "Astero decal index count is not divisible by three: " + item->m_name;
						return false;
					}
					indexBuffers.push_back( source->m_indexBuffer );
				}
				if( indexBuffers.empty() || indexBuffers.front().empty() )
				{
					error = "Astero decal has no high-detail index buffer: " + item->m_name;
					return false;
				}
				for( uint32_t index : indexBuffers.front() )
				{
					maximumLod0Index = std::max( maximumLod0Index, index );
					if( index >= m_decalSourceVertexCount )
					{
						error = "Astero decal index exceeds the first CMF source-vertex block: " + item->m_name;
						return false;
					}
				}
				activeLod0IndexCount += static_cast<uint32_t>( indexBuffers.front().size() );
				const uint32_t lod0Triangles = static_cast<uint32_t>( indexBuffers.front().size() / 3 );
				++m_decalStats[family].active;
				m_decalStats[family].activeTriangles += lod0Triangles;

				if( view != 0 && view != static_cast<int>( family ) + 1 )
				{
					continue;
				}
				std::fprintf(
					stderr,
					"Astero indexed decal selected: item=%s family=%s mesh=%d bone=%d position=(%.6f, %.6f, %.6f) rotation=(%.6f, %.6f, %.6f, %.6f) scaling=(%.6f, %.6f, %.6f) lods=%zu lod0-triangles=%u\n",
					item->m_name.c_str(),
					DecalFamilyName( family ),
					item->m_meshIndex,
					item->m_boneIndex,
					item->m_position.x,
					item->m_position.y,
					item->m_position.z,
					item->m_rotation.x,
					item->m_rotation.y,
					item->m_rotation.z,
					item->m_rotation.w,
					item->m_scaling.x,
					item->m_scaling.y,
					item->m_scaling.z,
					indexBuffers.size(),
					lod0Triangles );
				for( const auto& parameter : item->m_parameters )
				{
					std::fprintf(
						stderr,
						"  decal parameter %s=(%.6f, %.6f, %.6f, %.6f)\n",
						parameter->m_name.c_str(),
						parameter->m_value.x,
						parameter->m_value.y,
						parameter->m_value.z,
						parameter->m_value.w );
				}
				const Matrix inverseDecal = Inverse( TransformationMatrix( item->m_scaling, item->m_rotation, item->m_position ) );
				Vector3 localMinimum( std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max() );
				Vector3 localMaximum( -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max() );
				for( uint32_t index : indexBuffers.front() )
				{
					const TrinityStandaloneCmfVertex& vertex = m_model.Vertices()[index];
					const Vector3 local = XMVector3Transform(
						Vector3( vertex.position[0], vertex.position[1], vertex.position[2] ),
						inverseDecal );
					localMinimum = Vector3(
						std::min( localMinimum.x, local.x ),
						std::min( localMinimum.y, local.y ),
						std::min( localMinimum.z, local.z ) );
					localMaximum = Vector3(
						std::max( localMaximum.x, local.x ),
						std::max( localMaximum.y, local.y ),
						std::max( localMaximum.z, local.z ) );
				}
				std::fprintf(
					stderr,
					"  decal indexed local bounds min=(%.6f, %.6f, %.6f) max=(%.6f, %.6f, %.6f)\n",
					localMinimum.x,
					localMinimum.y,
					localMinimum.z,
					localMaximum.x,
					localMaximum.y,
					localMaximum.z );

				Tr2EffectPtr effect;
				if( !CreateDecalEffect( effect, *item, hull, faction, generic, effectName, error ) )
				{
					return false;
				}
				EveSpaceObjectDecalPtr decal;
				decal.CreateInstance();
				decal->SetPosition( item->m_position );
				decal->SetRotation( item->m_rotation );
				decal->SetScaling( item->m_scaling );
				decal->SetBoneIndex( item->m_boneIndex );
				decal->SetMinScreenSize( 10.0f );
				decal->SetHighDetailDecalState( true );
				decal->SetIndices( indexBuffers );
				decal->SetEffect( effect );
				decal->SetBatchType( TRIBATCHTYPE_DECAL );
				if( !decal->Initialize() )
				{
					error = "Failed to initialize Astero decal: " + item->m_name;
					return false;
				}
				AddDecal( decal );
				m_decalEntries.push_back( { decal, family, lod0Triangles, item->m_name } );
				++m_decalStats[family].selected;
				m_decalStats[family].selectedTriangles += lod0Triangles;
			}
		}

		if( m_decalDeclaredSetCount != 10 || m_decalDeclaredItemCount != 43 || activeSetCount != 2 ||
			activeItemCount != 11 || excludedSetCount != 8 || excludedItemCount != 32 ||
			activeLod0IndexCount != 84 || maximumLod0Index != 6764 )
		{
			error = "Astero decal inventory no longer matches the verified primary/soe fixture";
			return false;
		}
		if( m_decalStats[DECAL_FAMILY_STANDARD].active != 6 || m_decalStats[DECAL_FAMILY_STANDARD].activeTriangles != 14 ||
			m_decalStats[DECAL_FAMILY_LOGO].active != 4 || m_decalStats[DECAL_FAMILY_LOGO].activeTriangles != 12 ||
			m_decalStats[DECAL_FAMILY_KILLMARK].active != 1 || m_decalStats[DECAL_FAMILY_KILLMARK].activeTriangles != 2 )
		{
			error = "Astero decal family inventory no longer matches the verified fixture";
			return false;
		}
		if( m_decalEntries.empty() )
		{
			error = "Selected Astero decal family produced zero native decals";
			return false;
		}

		std::fprintf(
			stderr,
			"Astero indexed-decal inventory: sets=%u active-sets=%u visibility-excluded-sets=%u "
			"items=%u active-items=%u visibility-excluded-items=%u lod0-indices=%u lod0-triangles=%u max-index=%u source-vertices=%u kill-count=%u\n",
			m_decalDeclaredSetCount,
			activeSetCount,
			excludedSetCount,
			m_decalDeclaredItemCount,
			activeItemCount,
			excludedItemCount,
			activeLod0IndexCount,
			activeLod0IndexCount / 3,
			maximumLod0Index,
			m_decalSourceVertexCount,
			m_killCount );
		for( int familyIndex = 0; familyIndex < DECAL_FAMILY_COUNT; ++familyIndex )
		{
			const DecalStats& stats = m_decalStats[familyIndex];
			std::fprintf(
				stderr,
				"  %-10s active=%u triangles=%u selected=%u selected-triangles=%u\n",
				DecalFamilyName( static_cast<DecalFamily>( familyIndex ) ),
				stats.active,
				stats.activeTriangles,
				stats.selected,
				stats.selectedTriangles );
		}
		return true;
	}

	void AddDecal( EveSpaceObjectDecalPtr decal ) override
	{
		decal->SetPriority( static_cast<uint32_t>( m_decals.size() ) );
		m_decals.push_back( decal );
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
		m_shadowCullTests = 0;
		m_shadowAcceptedCascades = 0;
		m_shadowCommittedBatches = 0;
		m_time = updateContext.GetTime();
		if( !m_decalEntries.empty() )
		{
			++m_decalWarmupFrames;
		}
		UpdateEffectParameters();
		UpdateAttachmentLights();
	}

	void UpdateAsyncronous( const EveUpdateContext& ) override
	{
	}

	void UpdateVisibility( const EveUpdateContext& updateContext, const Matrix& ) override
	{
		for( const auto& set : m_spriteAttachmentSets )
			set->UpdateVisibility( updateContext, m_worldTransform, nullptr, 0 );
		for( const auto& set : m_spriteLineAttachmentSets )
			set->UpdateVisibility( updateContext, m_worldTransform, nullptr, 0 );
		for( const auto& set : m_spotlightAttachmentSets )
			set->UpdateVisibility( updateContext, m_worldTransform, nullptr, 0 );
		for( const auto& set : m_planeAttachmentSets )
			set->UpdateVisibility( updateContext, m_worldTransform, nullptr, 0 );
		for( const auto& set : m_hazeAttachmentSets )
			set->UpdateVisibility( updateContext, m_worldTransform, nullptr, 0 );
		for( const auto& set : m_bannerAttachmentSets )
			set->UpdateVisibility( updateContext, m_worldTransform, nullptr, 0 );

		if( !m_decalEntries.empty() )
		{
			IEveSpaceObject2::ParentData parentData;
			GetDecalParentData( parentData );
			for( const DecalEntry& entry : m_decalEntries )
			{
				entry.decal->UpdateVisibility( updateContext, &parentData );
			}
		}
	}

	void RegisterWithQuadRenderer( Tr2QuadRenderer & quadRenderer ) override
	{
		for( const auto& set : m_spriteAttachmentSets )
			set->RegisterWithQuadRenderer( quadRenderer );
		for( const auto& set : m_spriteLineAttachmentSets )
			set->RegisterWithQuadRenderer( quadRenderer );
		for( const auto& set : m_spotlightAttachmentSets )
			set->RegisterWithQuadRenderer( quadRenderer );
		for( const auto& set : m_planeAttachmentSets )
			set->RegisterWithQuadRenderer( quadRenderer );
	}

	void AddQuadsToQuadRenderer( const TriFrustum&, Tr2QuadRenderer& quadRenderer ) override
	{
		for( const auto& set : m_spriteAttachmentSets )
			set->AddToQuadRenderer( quadRenderer, m_worldTransform, 1.0f, 1.0f, nullptr, 0 );
		for( const auto& set : m_spriteLineAttachmentSets )
			set->AddToQuadRenderer( quadRenderer, m_worldTransform, 1.0f, 1.0f, nullptr, 0 );
		for( const auto& set : m_spotlightAttachmentSets )
			set->AddToQuadRenderer( quadRenderer, m_worldTransform, 1.0f, 1.0f, nullptr, 0 );
		for( const auto& set : m_planeAttachmentSets )
			set->AddToQuadRenderer( quadRenderer, m_worldTransform, 1.0f, 1.0f, nullptr, 0 );
	}

	void GetRenderables( std::vector<ITr2Renderable*> & renderables, Tr2ImpostorManager* ) override
	{
		renderables.push_back( this );
		if( !m_decalGeometry || m_decalEntries.empty() )
		{
			return;
		}

		DecalMeshCache meshCache;
		uint32_t submitted = 0;
		uint32_t submittedTriangles = 0;
		uint32_t committed = 0;
		float minimumVisibility = 1.0f;
		for( const DecalEntry& entry : m_decalEntries )
		{
			const size_t before = renderables.size();
			entry.decal->GetRenderables(
				renderables,
				meshCache,
				m_decalGeometry,
				std::numeric_limits<float>::max() );
			if( renderables.size() != before + 1 )
			{
				continue;
			}
			const std::vector<uint32_t> primitiveCounts = entry.decal->GetDecalPrimitiveCounts();
			if( primitiveCounts.empty() || primitiveCounts.front() != entry.lod0Triangles )
			{
				std::fprintf(
					stderr,
					"Astero indexed decal primitive mismatch: item=%s expected=%u actual=%u\n",
					entry.name.c_str(),
					entry.lod0Triangles,
					primitiveCounts.empty() ? 0 : primitiveCounts.front() );
				m_drawFailed = true;
				continue;
			}
			if( entry.family == DECAL_FAMILY_KILLMARK && entry.decal->GetResolvedKillCount() != m_killCount )
			{
				std::fprintf(
					stderr,
					"Astero killmark display-data mismatch: expected=%u actual=%u\n",
					m_killCount,
					entry.decal->GetResolvedKillCount() );
				m_drawFailed = true;
				continue;
			}
			++submitted;
			submittedTriangles += primitiveCounts.front();
			if( entry.decal->GetCommittedBatchCount() != 0 )
			{
				++committed;
			}
			minimumVisibility = std::min( minimumVisibility, entry.decal->GetResolvedVisibility() );
		}
		if( m_decalWarmupFrames >= 2 && ( submitted != m_decalEntries.size() || committed != m_decalEntries.size() ) )
		{
			if( !m_reportedDecalFailure )
			{
				std::fprintf(
					stderr,
					"Astero indexed decal render contract failed: configured=%zu submitted=%u committed=%u visibility=%.6f warmup-frames=%u\n",
					m_decalEntries.size(),
					submitted,
					committed,
					minimumVisibility,
					m_decalWarmupFrames );
				m_reportedDecalFailure = true;
			}
			m_drawFailed = true;
		}
		else if( submitted == m_decalEntries.size() && committed == m_decalEntries.size() && !m_reportedDecalSubmission )
		{
			std::fprintf(
				stderr,
				"Astero indexed decals ready: configured=%zu submitted=%u committed=%u lod0-triangles=%u visibility=%.6f batch=decal\n",
				m_decalEntries.size(),
				submitted,
				committed,
				submittedTriangles,
				minimumVisibility );
			m_reportedDecalSubmission = true;
		}
	}

	bool GetBoundingSphere( Vector4 & sphere, BoundingSphereQuery ) const override
	{
		sphere = Vector4( 0.0f, 0.0f, 0.0f, m_shadowBoundingRadius );
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

	void GetParentData( IEveSpaceObject2::ParentData * parentData ) const override
	{
		if( !parentData )
		{
			return;
		}
		GetDecalParentData( *parentData );
	}

	void GetBatches( ITriRenderBatchAccumulator * batches, TriBatchType batchType, const Tr2PerObjectData* perObjectData, Tr2RenderReason reason ) override
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
			ForwardAttachmentBatches( batches, batchType, perObjectData, reason );
			return;
		}

		if( batchType != TRIBATCHTYPE_OPAQUE || !m_effect || !m_effect->GetShaderStateInterface() )
		{
			ForwardAttachmentBatches( batches, batchType, perObjectData, reason );
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
		ForwardAttachmentBatches( batches, batchType, perObjectData, reason );
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

	bool IsCastingShadow( const TriFrustum& cameraFrustum, const IEveShadowFrustum& shadowFrustum, Tr2RenderReason, float& sizeInShadow ) const override
	{
		const bool directional = dynamic_cast<const TriShadowOrthoFrustum*>( &shadowFrustum ) != nullptr;
		if( directional )
		{
			++m_shadowCullTests;
		}
		sizeInShadow = 0.0f;
		if( !m_shadowCastingEnabled || !m_useEveV5Material || m_shadowBoundingRadius <= 0.0f )
		{
			return false;
		}
		const Vector4 sphere(
			TransformCoord( Vector3( 0.0f, 0.0f, 0.0f ), m_worldTransform ),
			m_shadowBoundingRadius );
		if( shadowFrustum.IsVisible( cameraFrustum, sphere ) )
		{
			sizeInShadow = shadowFrustum.GetSizeInShadow( sphere );
		}
		if( sizeInShadow <= 15.0f )
		{
			return false;
		}
		if( directional )
		{
			++m_shadowAcceptedCascades;
		}
		return true;
	}

	void GetShadowBatches( ITriRenderBatchAccumulator * batches, const Tr2PerObjectData* perObjectData, float ) override
	{
		if( !m_shadowCastingEnabled || !m_useEveV5Material )
		{
			return;
		}
		uint32_t committed = 0;
		committed += CommitShadowAreaBatch( batches, 1, m_hullEffect, perObjectData ) ? 1u : 0u;
		committed += CommitShadowAreaBatch( batches, 2, m_boosterEffect, perObjectData ) ? 1u : 0u;
		m_shadowCommittedBatches += committed;
	}

	Tr2PerObjectData* GetShadowPerObjectData( ITriRenderBatchAccumulator* ) override
	{
		return m_useEveV5Material ? &m_eveV5PerObjectData : nullptr;
	}

	uint32_t GetShadowCullTests() const
	{
		return m_shadowCullTests.load();
	}

	uint32_t GetShadowAcceptedCascades() const
	{
		return m_shadowAcceptedCascades.load();
	}

	uint32_t GetShadowCommittedBatches() const
	{
		return m_shadowCommittedBatches.load();
	}

	uint32_t GetHazeShadowEligibleCount() const
	{
		return m_lightStats[LIGHT_FAMILY_HAZE].shadowEligible;
	}

	uint32_t GetBannerShadowEligibleCount() const
	{
		return m_lightStats[LIGHT_FAMILY_BANNER].shadowEligible;
	}

	uint32_t GetValidationShadowEligibleCount() const
	{
		return m_lightStats[LIGHT_FAMILY_VALIDATION].shadowEligible;
	}

	bool DrawFailed() const
	{
		return m_drawFailed;
	}

	uint32_t GetCommittedOpaqueBatchCount() const
	{
		return ( m_reportedAreaBatch[1] ? 1u : 0u ) + ( m_reportedAreaBatch[2] ? 1u : 0u );
	}

	uint32_t GetAttachmentActiveCount( uint32_t family ) const
	{
		return family < m_attachmentStats.size() ? m_attachmentStats[family].active : 0;
	}

	uint32_t GetSelectedDecalCount() const
	{
		return static_cast<uint32_t>( m_decalEntries.size() );
	}

	uint32_t GetSelectedDecalTriangleCount() const
	{
		uint32_t triangles = 0;
		for( const DecalEntry& entry : m_decalEntries )
		{
			triangles += entry.lod0Triangles;
		}
		return triangles;
	}

	bool HaveDecalsCommitted() const
	{
		return m_reportedDecalSubmission && !m_reportedDecalFailure;
	}

	uint32_t GetConstructedLightCount() const
	{
		return TotalConstructedLights();
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
	enum DecalFamily
	{
		DECAL_FAMILY_STANDARD,
		DECAL_FAMILY_LOGO,
		DECAL_FAMILY_KILLMARK,
		DECAL_FAMILY_COUNT,
	};

	struct DecalStats
	{
		uint32_t active = 0;
		uint32_t activeTriangles = 0;
		uint32_t selected = 0;
		uint32_t selectedTriangles = 0;
	};

	struct DecalEntry
	{
		EveSpaceObjectDecalPtr decal;
		DecalFamily family;
		uint32_t lod0Triangles;
		std::string name;
	};

	static const char* DecalFamilyName( DecalFamily family )
	{
		static const char* names[] = { "standard", "logos", "killmarks" };
		return names[family];
	}

	void GetDecalParentData( IEveSpaceObject2::ParentData & parentData ) const
	{
		parentData = IEveSpaceObject2::ParentData{};
		parentData.transform = m_decalWorldTransform;
		parentData.killCount = m_killCount;
		parentData.shipData = m_eveV5PerObjectData.m_psData.shipData;
		parentData.clipSphereCenter = m_eveV5PerObjectData.m_psData.clipSphereCenter;
		parentData.clipRadiusSq = m_eveV5PerObjectData.m_psData.clipRadiusSq;
		parentData.clipRadius2Sq = m_eveV5PerObjectData.m_psData.clipRadius2Sq;
		parentData.clipFactor = m_eveV5PerObjectData.m_psData.clipSphereFactor;
		parentData.clipFactor2 = m_eveV5PerObjectData.m_psData.clipSphereFactor2;
		parentData.shLighting = m_eveV5PerObjectData.m_psData.shLightingCoefficients;
	}

	bool PrepareDecalGeometry( std::string & error )
	{
		if( m_model.Sections().size() != 3 || m_model.VertexCount() != 21582 || m_model.VertexCount() % 3 != 0 )
		{
			error = "Astero CMF does not contain the verified three equal source-vertex blocks";
			return false;
		}
		for( uint32_t group = 0; group < 3; ++group )
		{
			if( m_model.Sections()[group].sourceGroup != group )
			{
				error = "Astero CMF source-group order is incompatible with indexed decals";
				return false;
			}
		}
		m_decalSourceVertexCount = m_model.VertexCount() / 3;
		if( m_decalSourceVertexCount != 7194 )
		{
			error = "Astero CMF first source-vertex block no longer contains 7194 vertices";
			return false;
		}

		BeResMan->GetResource( "res:/Astero.cmf", "", m_decalGeometry );
		if( m_decalGeometry )
		{
			m_decalGeometry->ForceSynchronousLoad();
			m_decalGeometry->Reload();
		}
		std::fprintf(
			stderr,
			"Astero native decal geometry state: path=res:/Astero.cmf exists=%s resource=%s loading=%s prepared=%s good=%s cmf=%s meshes=%u\n",
			BePaths->FileExistsLocally( L"res:/Astero.cmf" ) ? "yes" : "no",
			m_decalGeometry ? "yes" : "no",
			m_decalGeometry && m_decalGeometry->IsLoading() ? "yes" : "no",
			m_decalGeometry && m_decalGeometry->IsPrepared() ? "yes" : "no",
			m_decalGeometry && m_decalGeometry->IsGood() ? "yes" : "no",
			m_decalGeometry && m_decalGeometry->IsUsingCMF() ? "yes" : "no",
			m_decalGeometry ? m_decalGeometry->GetMeshCount() : 0 );
		if( !m_decalGeometry || !m_decalGeometry->IsGood() || !m_decalGeometry->IsUsingCMF() ||
			m_decalGeometry->GetMeshCount() != 3 )
		{
			error = "Failed to prepare res:/Astero.cmf as native decal geometry";
			return false;
		}
		for( uint32_t meshIndex = 0; meshIndex < 3; ++meshIndex )
		{
			TriGeometryResMeshData* mesh = m_decalGeometry->GetMeshData( meshIndex );
			if( !mesh || mesh->m_lods.size() != 1 || mesh->m_lods.front()->m_vertexCount != m_decalSourceVertexCount )
			{
				error = "Native Astero decal geometry does not contain three equal 7194-vertex source blocks";
				return false;
			}
		}
		std::fprintf(
			stderr,
			"Astero native decal geometry ready: path=res:/Astero.cmf merged-vertices=%u native-meshes=3 source-vertices=%u lods=1\n",
			m_model.VertexCount(),
			m_decalSourceVertexCount );
		return true;
	}

	bool FindDecalParentTexture( const EveSOFDataHull& hull,
								 int32_t meshIndex,
								 const std::string& textureName,
								 std::string& path ) const
	{
		if( meshIndex < 0 )
		{
			return false;
		}
		for( const auto& area : hull.m_opaqueAreas )
		{
			if( meshIndex < static_cast<int32_t>( area->m_index ) ||
				meshIndex >= static_cast<int32_t>( area->m_index + area->m_count ) )
			{
				continue;
			}
			for( const auto& texture : area->m_textures )
			{
				if( texture->m_name == BlueSharedString( textureName.c_str() ) )
				{
					path = texture->m_resFilePath;
					return !path.empty();
				}
			}
		}
		return false;
	}

	bool CreateDecalEffect( Tr2EffectPtr & effect,
							const EveSOFDataHullDecalSetItem& item,
							const EveSOFDataHull& hull,
							const EveSOFDataFaction& faction,
							const EveSOFDataGeneric& generic,
							const char* effectName,
							std::string& error )
	{
		EveSOFDataGenericDecalShaderPtr shaderData;
		for( const auto& candidate : generic.m_decalShaders )
		{
			if( candidate->m_shader == BlueSharedString( effectName ) )
			{
				shaderData = candidate;
				break;
			}
		}
		if( !shaderData )
		{
			error = std::string( "Missing generic Astero decal shader descriptor: " ) + effectName;
			return false;
		}

		const std::string effectPath = generic.m_decalShaderLocation + "/" + generic.m_shaderPrefix + effectName;
		effect.CreateInstance();
		effect->StartUpdate();
		effect->SetEffectPathName( effectPath.c_str() );
		auto addTexture = [&]( const auto& name, const std::string& path, const char* role ) {
			if( path.empty() )
			{
				error = std::string( "Empty Astero decal texture path for " ) + name.c_str();
				return false;
			}
			if( !PrepareTextureResourceWithoutYield( path, role, error ) )
			{
				return false;
			}
			effect->AddResourceTexture2D( BlueSharedString( name.c_str() ), path.c_str() );
			return true;
		};

		for( const auto& parameter : item.m_parameters )
		{
			effect->AddParameterVector4( parameter->m_name, &parameter->m_value );
		}
		for( const auto& texture : item.m_textures )
		{
			std::fprintf(
				stderr,
				"  decal texture %s=%s\n",
				texture->m_name.c_str(),
				texture->m_resFilePath.c_str() );
			if( !addTexture( texture->m_name, texture->m_resFilePath, "Astero authored decal" ) )
			{
				effect->EndUpdate();
				return false;
			}
		}
		for( const auto& texture : shaderData->m_defaultTextures )
		{
			if( !addTexture( texture->m_name, texture->m_resFilePath, "Astero generic decal" ) )
			{
				effect->EndUpdate();
				return false;
			}
		}
		for( const auto& parentName : shaderData->m_parentTextures )
		{
			std::string parentPath;
			if( !FindDecalParentTexture( hull, item.m_meshIndex, parentName->m_str, parentPath ) )
			{
				if( parentName->m_str == "AoMap" )
				{
					if( !m_reportedMissingDecalAoMap )
					{
						std::fprintf( stderr, "Astero decal optional parent AoMap is absent; retaining the client shader default\n" );
						m_reportedMissingDecalAoMap = true;
					}
					continue;
				}
				error = "Missing required Astero decal parent texture " + parentName->m_str;
				effect->EndUpdate();
				return false;
			}
			if( !addTexture( parentName->m_str, parentPath, "Astero decal parent hull" ) )
			{
				effect->EndUpdate();
				return false;
			}
		}

		if( item.m_usage == EveSOFDataHullDecalSetItem::USAGE_LOGO )
		{
			const int logoIndex = static_cast<int>( item.m_logoType );
			if( !faction.m_logoSet || logoIndex < 0 || logoIndex >= EveSOFDataLogoSet::TYPE_MAX || !faction.m_logoSet->m_logos[logoIndex] )
			{
				error = "Active Astero logo decal has no faction logo set";
				effect->EndUpdate();
				return false;
			}
			for( const auto& texture : faction.m_logoSet->m_logos[logoIndex]->m_textures )
			{
				if( !addTexture( texture->m_name, texture->m_resFilePath, "Astero faction logo decal" ) )
				{
					effect->EndUpdate();
					return false;
				}
			}
		}
		else if( item.m_usage != EveSOFDataHullDecalSetItem::USAGE_STANDARD )
		{
			const int colorIndex = static_cast<int>( item.m_glowColorType );
			if( colorIndex < 0 || colorIndex >= SOFDataFactionColorChooser::TYPE_MAX )
			{
				error = "Astero killmark decal has an invalid faction glow-color index";
				effect->EndUpdate();
				return false;
			}
			Color glowColor = faction.m_colorSet->m_colors[colorIndex];
			std::fprintf(
				stderr,
				"Astero killmark glow: color-index=%d rgba=(%.6f, %.6f, %.6f, %.6f)\n",
				colorIndex,
				glowColor.r,
				glowColor.g,
				glowColor.b,
				glowColor.a );
			effect->AddParameterColor( BlueSharedString( "DecalGlowColor" ), &glowColor );
		}

		effect->EndUpdate();
		if( !ValidateEffect( *effect, effectName, false ) )
		{
			error = "Failed to validate Astero decal effect " + effectPath;
			return false;
		}
		std::fprintf(
			stderr,
			"Astero decal effect ready: item=%s effect=%s batch=%s\n",
			item.m_name.c_str(),
			effectPath.c_str(),
			"decal" );
		return true;
	}

	enum AttachmentFamily
	{
		ATTACHMENT_FAMILY_SPRITE,
		ATTACHMENT_FAMILY_SPRITE_LINE,
		ATTACHMENT_FAMILY_SPOTLIGHT,
		ATTACHMENT_FAMILY_PLANE,
		ATTACHMENT_FAMILY_HAZE,
		ATTACHMENT_FAMILY_BANNER,
		ATTACHMENT_FAMILY_COUNT,
	};

	struct AttachmentStats
	{
		uint32_t declared = 0;
		uint32_t excluded = 0;
		uint32_t active = 0;
	};

	static const char* AttachmentFamilyName( AttachmentFamily family )
	{
		static const char* names[] = { "sprites", "sprite-lines", "spotlights", "planes", "hazes", "banners" };
		return names[family];
	}

	void CountAttachmentSet( AttachmentFamily family, size_t count, bool active )
	{
		m_attachmentStats[family].declared += static_cast<uint32_t>( count );
		if( active )
			m_attachmentStats[family].active += static_cast<uint32_t>( count );
		else
			m_attachmentStats[family].excluded += static_cast<uint32_t>( count );
	}

	void ReportAttachmentBone( AttachmentFamily family, int32_t boneIndex ) const
	{
		if( boneIndex >= 0 )
			std::fprintf( stderr, "Astero visible attachment uses static identity bone delta: family=%s bone=%d\n", AttachmentFamilyName( family ), boneIndex );
	}

	void ReportAttachmentStats() const
	{
		std::fprintf( stderr, "Astero visible-attachment inventory:\n" );
		for( int family = 0; family < ATTACHMENT_FAMILY_COUNT; ++family )
		{
			const AttachmentStats& stats = m_attachmentStats[family];
			std::fprintf( stderr, "  %-12s declared=%u visibility-excluded=%u active=%u\n", AttachmentFamilyName( static_cast<AttachmentFamily>( family ) ), stats.declared, stats.excluded, stats.active );
		}
	}

	float NormalizeAuthoredLength( float value ) const
	{
		return value;
	}

	Vector3 NormalizeAuthoredPosition( const Vector3& value ) const
	{
		return value;
	}

	static Color ModifyAttachmentColor( const Color& color, float saturation, float brightness )
	{
		CTriColor converted;
		const float maximum = std::max( { color.r, color.g, color.b, 1.0f } );
		converted.SetRGB( color.r / maximum, color.g / maximum, color.b / maximum, 1.0f );
		float hue, sourceSaturation, value;
		converted.GetHSV( &hue, &sourceSaturation, &value );
		sourceSaturation *= saturation;
		value *= brightness;
		converted.SetHSV( hue, sourceSaturation, value );
		return Color( converted.r, converted.g, converted.b, color.a );
	}

	void ForwardAttachmentBatches( ITriRenderBatchAccumulator * batches, TriBatchType batchType, const Tr2PerObjectData* perObjectData, Tr2RenderReason reason )
	{
		const Tr2PerObjectData* attachmentData = perObjectData ? perObjectData : &m_eveV5PerObjectData;
		for( const auto& set : m_hazeAttachmentSets )
			set->GetBatches( batches, batchType, attachmentData, reason );
		for( const auto& set : m_bannerAttachmentSets )
			set->GetBatches( batches, batchType, attachmentData, reason );
	}

	bool CreateAttachmentEffect( Tr2EffectPtr & effect, const char* path, const char* label, std::string& error )
	{
		effect.CreateInstance();
		effect->StartUpdate();
		effect->SetEffectPathName( path );
		effect->EndUpdate();
		if( !ValidateEffect( *effect, label, false ) )
		{
			error = std::string( "Failed to validate Astero " ) + label + " effect " + path;
			return false;
		}
		return true;
	}

	TriTextureParameterPtr AddAttachmentTexture( Tr2Effect & effect, const char* name, const std::string& path )
	{
		TriTextureParameterPtr parameter;
		parameter.CreateInstance();
		parameter->SetParameterName( BlueSharedString( name ) );
		parameter->SetResourcePath( path.c_str() );
		effect.AddResource( parameter );
		return parameter;
	}

	bool CreateTexturedAttachmentEffect( Tr2EffectPtr & effect, const char* path, const std::string& texture, const char* label, std::string& error, float zOffset = 0.0f )
	{
		if( !PrepareTextureResourceWithoutYield( texture, label, error ) )
			return false;
		effect.CreateInstance();
		effect->StartUpdate();
		effect->SetEffectPathName( path );
		AddAttachmentTexture( *effect, "TextureMap", texture );
		if( zOffset != 0.0f )
			effect->AddParameterFloat( BlueSharedString( "zOffset" ), zOffset );
		effect->EndUpdate();
		if( !ValidateEffect( *effect, label, false ) )
		{
			error = std::string( "Failed to validate Astero " ) + label + " effect " + path;
			return false;
		}
		return true;
	}

	bool CreatePlaneAttachmentEffect( Tr2EffectPtr & effect, const EveSOFDataHullPlaneSet& source, EvePlaneSet& set, std::string& error )
	{
		const std::string textures[] = { source.m_layer1MapResPath, source.m_layer2MapResPath, source.m_maskMapResPath };
		for( const std::string& texture : textures )
		{
			if( !PrepareTextureResourceWithoutYield( texture, "Astero plane attachment", error ) )
				return false;
		}
		effect.CreateInstance();
		effect->StartUpdate();
		effect->SetEffectPathName( "res:/graphics/effect/managed/space/spaceobject/fx/planeglow.fx" );
		TriTextureParameterPtr layer1 = AddAttachmentTexture( *effect, "Layer1Map", source.m_layer1MapResPath );
		TriTextureParameterPtr layer2 = AddAttachmentTexture( *effect, "Layer2Map", source.m_layer2MapResPath );
		TriTextureParameterPtr mask = AddAttachmentTexture( *effect, "MaskMap", source.m_maskMapResPath );
		const Vector4 planeData( source.m_usage == EveSOFDataHullPlaneSet::USAGE_HAZE ? 1.0f : 0.0f,
								 static_cast<float>( source.m_atlasSize ),
								 std::floor( source.m_atlasAspectRatio.x ),
								 std::floor( source.m_atlasAspectRatio.y ) );
		effect->AddParameterVector4( BlueSharedString( "PlaneData" ), &planeData );
		effect->EndUpdate();
		set.SetLayerMap1Parameter( layer1 );
		set.SetLayerMap2Parameter( layer2 );
		set.SetMaskMapParameter( mask );
		if( !ValidateEffect( *effect, "plane", false ) )
		{
			error = "Failed to validate Astero plane attachment effect";
			return false;
		}
		return true;
	}

	bool ConfigureBannerAttachments( const EveSOFDataHull& hull, const EveSOFDataGeneric& generic, const std::set<std::string>& visibilityGroups, std::string& error )
	{
		const char* bannerPath = "res:/dx9/model/shared/faction_logos/soe_logo.dds";
		if( !PrepareTextureResourceWithoutYield( bannerPath, "Astero Sisters of EVE square logo", error ) )
			return false;
		for( const auto& setData : hull.m_bannerSets )
		{
			const bool active = visibilityGroups.count( setData->m_visibilityGroup.c_str() ) != 0;
			CountAttachmentSet( ATTACHMENT_FAMILY_BANNER, setData->m_banners.size(), active );
			if( !active )
				continue;
			std::map<int, EveBannerSetPtr> sets;
			for( const auto& item : setData->m_banners )
			{
				const int usage = static_cast<int>( item->m_usage );
				EveBannerSetPtr& set = sets[usage];
				if( !set )
				{
					set.CreateInstance();
					Tr2EffectPtr effect;
					effect.CreateInstance();
					effect->StartUpdate();
					effect->SetEffectPathName( generic.m_bannerShader.m_shader.c_str() );
					for( const auto& parameter : generic.m_bannerShader.m_defaultParameters )
						effect->AddParameterVector4( parameter->m_name, &parameter->m_value );
					for( const auto& texture : generic.m_bannerShader.m_defaultTextures )
					{
						if( !PrepareTextureResourceWithoutYield( texture->m_resFilePath, "Astero generic banner", error ) )
							return false;
						AddAttachmentTexture( *effect, texture->m_name.c_str(), texture->m_resFilePath );
					}
					TriTextureParameterPtr image = AddAttachmentTexture( *effect, "ImageMap", bannerPath );
					effect->EndUpdate();
					if( !ValidateEffect( *effect, "banner", false ) )
					{
						error = "Failed to validate Astero banner attachment effect";
						return false;
					}
					set->SetEffect( effect );
					set->SetPrimaryTextureParameter( image );
					set->SetKey( usage );
				}
				EveBannerItem runtime;
				runtime.bone = item->m_boneIndex;
				runtime.position = NormalizeAuthoredPosition( item->m_position );
				runtime.rotation = item->m_rotation;
				runtime.scaling = item->m_scaling * NormalizeAuthoredLength( 1.0f );
				runtime.angleX = item->m_angleX;
				runtime.angleY = item->m_angleY;
				runtime.reference = usage;
				ReportAttachmentBone( ATTACHMENT_FAMILY_BANNER, item->m_boneIndex );
				set->AddBanner( runtime );
			}
			for( auto& entry : sets )
			{
				entry.second->Rebuild();
				if( !entry.second->Initialize() )
				{
					error = "Failed to initialize Astero banner attachment set";
					return false;
				}
				m_bannerAttachmentSets.push_back( entry.second );
			}
		}
		return true;
	}

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
		uint32_t shadowEligible = 0;
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
		uint32_t declared = 0, excluded = 0, constructed = 0, shadowEligible = 0, accepted = 0;
		std::fprintf( stderr, "Astero local-light counts (%s):\n", phase );
		for( int family = 0; family < LIGHT_FAMILY_COUNT; ++family )
		{
			const LightStats& stats = m_lightStats[family];
			declared += stats.declared;
			excluded += stats.visibilityExcluded;
			constructed += stats.constructed;
			shadowEligible += stats.shadowEligible;
			accepted += stats.frustumAccepted;
			std::fprintf( stderr, "  %-11s declared=%u visibility-excluded=%u constructed=%u shadow-eligible=%u frustum-accepted=%u\n", LightFamilyName( static_cast<LightFamily>( family ) ), stats.declared, stats.visibilityExcluded, stats.constructed, stats.shadowEligible, stats.frustumAccepted );
		}
		std::fprintf( stderr, "  total       declared=%u visibility-excluded=%u constructed=%u shadow-eligible=%u frustum-accepted=%u\n", declared, excluded, constructed, shadowEligible, accepted );
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

	void NormalizeAuthoredLight( LightData & data, LightFamily family )
	{
		data.castsShadows = m_localShadowMode == STANDALONE_LOCAL_SHADOWS_AUTHORED ?
			PerLightShadowSetting::ENABLED_ONLY_ON_HIGH_QUALITY :
			PerLightShadowSetting::DISABLED;
		if( data.castsShadows != PerLightShadowSetting::DISABLED )
		{
			++m_lightStats[family].shadowEligible;
		}
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
		if( !GetComponentRegistry() )
		{
			return;
		}
		if( TotalConstructedLights() > 0 )
		{
			GetComponentRegistry()->RegisterComponent<ITr2LightOwner>( this );
		}
		if( m_shadowCastingEnabled )
		{
			GetComponentRegistry()->RegisterComponent<IEveShadowCaster>( this );
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
		uint32_t shadowTechnique = 0;
		uint32_t dynamicLightShadowTechnique = 0;
		if( !shader || !shader->GetTechniqueIndex( BlueSharedString( "Main" ), mainTechnique ) ||
			( requireDepth && !shader->GetTechniqueIndex( BlueSharedString( "Depth" ), depthTechnique ) ) ||
			( requireDepth && m_shadowCastingEnabled && !shader->GetTechniqueIndex( BlueSharedString( "Shadow" ), shadowTechnique ) ) ||
			( requireDepth && m_localShadowMode != STANDALONE_LOCAL_SHADOWS_OFF &&
			  !shader->GetTechniqueIndex( BlueSharedString( "DynamicLightShadow" ), dynamicLightShadowTechnique ) ) )
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
		std::fprintf(
			stderr,
			"Astero %s effect ready: Main=%u Depth=%s Shadow=%s DynamicLightShadow=%s\n",
			label,
			mainTechnique,
			requireDepth ? std::to_string( depthTechnique ).c_str() : "n/a",
			requireDepth && m_shadowCastingEnabled ? std::to_string( shadowTechnique ).c_str() : "n/a",
			requireDepth && m_localShadowMode != STANDALONE_LOCAL_SHADOWS_OFF ?
				std::to_string( dynamicLightShadowTechnique ).c_str() :
				"n/a" );
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

	bool CommitShadowAreaBatch( ITriRenderBatchAccumulator * batches, uint32_t sourceGroup, Tr2Effect* effect, const Tr2PerObjectData* perObjectData ) const
	{
		if( sourceGroup >= m_sectionsByGroup.size() || !m_sectionsByGroup[sourceGroup] || !effect ||
			!effect->GetShaderStateInterface() || ( m_areaView != 0 && m_areaView != static_cast<int>( sourceGroup + 1 ) ) )
		{
			return false;
		}
		const TrinityStandaloneCmfSection& section = *m_sectionsByGroup[sourceGroup];
		Tr2RenderBatch batch;
		batch.SetMaterial( effect );
		batch.SetGeometry( m_vertexDeclaration, m_vertexBuffer, sizeof( TrinityStandaloneEveV5Vertex ), m_indexBuffer, sizeof( uint32_t ) );
		batch.SetPerObjectData( perObjectData );
		batch.SetDrawIndexedInstanced( section.indexCount, 1, section.firstIndex, 0, 0 );
		batch.SetRenderingMode( Tr2EffectStateManager::RM_OPAQUE );
		batches->Commit( batch );
		return true;
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
		const float yaw = seconds * 0.38f - 0.8f + m_modelYawOffset;
		const float pitch = -0.28f;
		if( m_useEveV5Material )
		{
			const Matrix rotation = RotationYMatrix( yaw ) * RotationXMatrix( pitch );
			float centerScale[4];
			m_model.GetCenterAndScale( centerScale );
			m_decalWorldTransform = TranslationMatrix(
										Vector3( -centerScale[0], -centerScale[1], -centerScale[2] ) ) *
				rotation;
			m_worldTransform = m_decalWorldTransform;
			const Matrix world = Transpose( m_worldTransform );
			const Matrix inverseWorld = Transpose( Inverse( m_worldTransform ) );
			m_eveV5PerObjectData.m_vsData.worldTransform = world;
			m_eveV5PerObjectData.m_vsData.worldTransformLast = world;
			m_eveV5PerObjectData.m_vsData.invWorldTransform = inverseWorld;
			m_eveV5PerObjectData.m_vsData.shipData = Vector4(
				1.0f,
				1.0f,
				0.0f,
				m_shadowBoundingRadius );
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
	bool m_shadowCastingEnabled = false;
	bool m_shLightingEnabled = true;
	bool m_reportedShLighting = false;
	mutable bool m_reportedAcceptedLights = false;
	int m_localLightMode = 0;
	int m_localShadowMode = STANDALONE_LOCAL_SHADOWS_OFF;
	int m_attachmentMode = 0;
	int m_attachmentView = 0;
	int m_decalMode = 0;
	int m_decalView = 0;
	int m_normalMapMode = 0;
	uint32_t m_shUpdateCount = 0;
	uint32_t m_killCount = 0;
	float m_modelYawOffset = 0.0f;
	uint32_t m_decalWarmupFrames = 0;
	uint32_t m_decalDeclaredSetCount = 0;
	uint32_t m_decalDeclaredItemCount = 0;
	uint32_t m_decalSourceVertexCount = 0;
	int m_areaView = 0;
	Matrix m_worldTransform = IdentityMatrix();
	Matrix m_decalWorldTransform = IdentityMatrix();
	float m_shadowBoundingRadius = 0.0f;
	float m_authoredWorldScale = 1.0f;
	bool m_reportedMissingDecalAoMap = false;
	bool m_reportedDecalSubmission = false;
	bool m_reportedDecalFailure = false;
	mutable std::atomic<uint32_t> m_shadowCullTests = 0;
	mutable std::atomic<uint32_t> m_shadowAcceptedCascades = 0;
	mutable std::atomic<uint32_t> m_shadowCommittedBatches = 0;
	TrinityStandaloneCmfModel m_model;
	TriGeometryResPtr m_decalGeometry;
	std::array<const TrinityStandaloneCmfSection*, 3> m_sectionsByGroup = {};
	std::array<bool, 3> m_reportedAreaBatch = {};
	std::array<DecalStats, DECAL_FAMILY_COUNT> m_decalStats = {};
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
	std::array<AttachmentStats, ATTACHMENT_FAMILY_COUNT> m_attachmentStats = {};
	std::vector<EveSpriteSetPtr> m_spriteLightSets;
	std::vector<EveSpriteLineSetPtr> m_spriteLineLightSets;
	std::vector<EveSpotlightSetPtr> m_spotlightLightSets;
	std::vector<EvePlaneSetPtr> m_planeLightSets;
	std::vector<EveHazeSetPtr> m_hazeLightSets;
	std::vector<EveBannerSetPtr> m_bannerLightSets;
	std::vector<EveSpriteSetPtr> m_spriteAttachmentSets;
	std::vector<EveSpriteLineSetPtr> m_spriteLineAttachmentSets;
	std::vector<EveSpotlightSetPtr> m_spotlightAttachmentSets;
	std::vector<EvePlaneSetPtr> m_planeAttachmentSets;
	std::vector<EveHazeSetPtr> m_hazeAttachmentSets;
	std::vector<EveBannerSetPtr> m_bannerAttachmentSets;
	std::vector<EveSpaceObjectDecalPtr> m_decals;
	std::vector<DecalEntry> m_decalEntries;
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
						MAP_INTERFACE( IEveShadowCaster )
							MAP_INTERFACE( IEveSpaceObjectDecalOwner )
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
	STANDALONE_LIGHTING_ENVIRONMENT = 4,
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

enum StandaloneReflectionSource
{
	STANDALONE_REFLECTION_SOURCE_OFF = 0,
	STANDALONE_REFLECTION_SOURCE_STATIC = 1,
	STANDALONE_REFLECTION_SOURCE_DYNAMIC = 2,
};

const char* ReflectionSourceName( int source )
{
	switch( source )
	{
	case STANDALONE_REFLECTION_SOURCE_OFF:
		return "off";
	case STANDALONE_REFLECTION_SOURCE_STATIC:
		return "static";
	case STANDALONE_REFLECTION_SOURCE_DYNAMIC:
		return "dynamic";
	default:
		return "invalid";
	}
}

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
	STANDALONE_CAPTURE_SHADOW = 1 << 3,
	STANDALONE_CAPTURE_SHADOW_ATLAS = 1 << 4,
	STANDALONE_CAPTURE_AO = 1 << 5,
	STANDALONE_CAPTURE_BENT_NORMAL = 1 << 6,
	STANDALONE_CAPTURE_REFLECTION = 1 << 7,
	STANDALONE_CAPTURE_LOCAL_SHADOW_ATLAS = 1 << 8,
	STANDALONE_CAPTURE_FREEZE_SCENE = 1 << 9,
	STANDALONE_CAPTURE_HDR_COMPOSITE = 1 << 10,
	STANDALONE_CAPTURE_POST_TONEMAP = 1 << 11,
	STANDALONE_CAPTURE_TONE_CONTRACT = 1 << 12,
};

enum StandaloneDynamicExposureMode
{
	STANDALONE_DYNAMIC_EXPOSURE_OFF = 0,
	STANDALONE_DYNAMIC_EXPOSURE_CLIENT = 1,
};

enum StandaloneShadowMode
{
	STANDALONE_SHADOWS_OFF = 0,
	STANDALONE_SHADOWS_LOW = 1,
	STANDALONE_SHADOWS_HIGH = 2,
};

enum StandaloneAmbientOcclusionMode
{
	STANDALONE_AO_OFF = 0,
	STANDALONE_AO_LOW = 1,
	STANDALONE_AO_MEDIUM = 2,
	STANDALONE_AO_HIGH = 3,
};

enum StandaloneAoMethod
{
	STANDALONE_AO_CORTAO = 0,
	STANDALONE_AO_CACAO = 1,
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

enum StandaloneDecals
{
	STANDALONE_DECALS_AUTO = 0,
	STANDALONE_DECALS_OFF = 1,
	STANDALONE_DECALS_AUTHORED = 2,
};

enum StandaloneDecalView
{
	STANDALONE_DECAL_VIEW_ALL = 0,
	STANDALONE_DECAL_VIEW_STANDARD = 1,
	STANDALONE_DECAL_VIEW_LOGOS = 2,
	STANDALONE_DECAL_VIEW_KILLMARKS = 3,
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
	struct HdrCompositeDiagnostics
	{
		bool valid = false;
		uint32_t format = 0;
		uint32_t width = 0;
		uint32_t height = 0;
		uint64_t rawHash = 0;
		uint64_t finiteComponents = 0;
		uint64_t nanComponents = 0;
		uint64_t infComponents = 0;
		uint64_t negativeComponents = 0;
		uint64_t saturatedComponents = 0;
		uint64_t pixelsAboveOne = 0;
		double minimumLuminance = 0.0;
		double meanLuminance = 0.0;
		double maximumLuminance = 0.0;
		double p50Luminance = 0.0;
		double p95Luminance = 0.0;
		double p99Luminance = 0.0;
	};

	~StandaloneProbe()
	{
		if( scene )
		{
			scene->SetShLightingManager( nullptr );
		}
		renderProductReadback = Tr2TextureAL{};
		hdrCompositeReadback = Tr2TextureAL{};
		postTonemapReadback = Tr2TextureAL{};
		renderProductVisualizer.Unlock();
		reflectionProductVisualizer.Unlock();
		dynamicLightShadowResolveEffect.Unlock();
		driver.Unlock();
		ssao.Unlock();
		scene.Unlock();
		renderable.Unlock();
		secondaryLights.clear();
		shLightingManager.Unlock();
		g_eveSpaceSceneDynamicLighting = false;
		Tr2LightManager::SetDynamicLightShadowsEnabled( false );
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
	Tr2SSAOPtr ssao;
	BluePtr<TrinityStandaloneRenderable> renderable;
	std::vector<BluePtr<TrinityStandaloneSecondaryLight>> secondaryLights;
	Tr2ShLightingManagerPtr shLightingManager;
	TriViewPtr view;
	TriProjectionPtr projection;
	Tr2EffectPtr renderProductVisualizer;
	Tr2EffectPtr reflectionProductVisualizer;
	Tr2EffectPtr dynamicLightShadowResolveEffect;
	Tr2TextureAL renderProductReadback;
	Tr2TextureAL hdrCompositeReadback;
	Tr2TextureAL postTonemapReadback;
	HdrCompositeDiagnostics hdrCompositeDiagnostics;
	TrinityStandaloneToneValidation toneValidation;
	TrinityStandalonePostProcessDiagnostics postProcessDiagnostics;
	Tr2PPDynamicExposureEffectPtr clientDynamicExposure;
	Tr2PPTonemappingEffectPtr clientTonemapping;
	Tr2PostProcess2Ptr clientPostProcess;
	int dynamicExposureMode = STANDALONE_DYNAMIC_EXPOSURE_CLIENT;
	int qualityRung = STANDALONE_PROBE_RUNG_SHELL;
	bool postProcessDiagnosticsEnabled = false;
	int64_t lastRealTime = 0;
	int64_t lastSimTime = 0;
	uint32_t renderWidth = 0;
	uint32_t renderHeight = 0;
	int localLights = STANDALONE_LOCAL_LIGHTS_OFF;
	int localShadows = STANDALONE_LOCAL_SHADOWS_OFF;
	int shadows = STANDALONE_SHADOWS_OFF;
	int ambientOcclusion = STANDALONE_AO_OFF;
	int aoMethod = STANDALONE_AO_CORTAO;
	int reflectionSource = STANDALONE_REFLECTION_SOURCE_OFF;
	bool reportedShadowStats = false;
	bool reportedReflectionStatus = false;
	std::vector<uint8_t> capturedProductPixels;
	uint32_t capturedProductWidth = 0;
	uint32_t capturedProductHeight = 0;
	bool reportedResolvedLights = false;
	bool reportedLocalShadowStats = false;
	bool aoProductValidated = false;
	bool backgroundPrepared = false;
	bool newEdenSystemPrepared = false;
	bool planetSurfacePrepared = false;
	bool planetAtmospherePrepared = false;
	bool planetCloudsPrepared = false;
	bool sunGeometryPrepared = false;
	bool authoredSunFlarePrepared = false;
	bool lensFlarePrepared = false;
	bool godRaysPrepared = false;
	std::string compositionValidationSummary;
	uint64_t renderedFrameCount = 0;
	bool reportedCameraOrbit = false;
	bool exposureSequenceActive = false;
	int cameraView = STANDALONE_CAMERA_MODEL;
	EvePlanet* celestialInspectionTarget = nullptr;
	const char* celestialInspectionName = nullptr;
	float celestialExpectedPixelDiameter = 0.0f;
	float celestialInspectionFovRadians = 0.0f;
	float modelWorldScale = 1.0f;
	float celestialInspectionScale = 1.0f;
	Vector3 celestialInspectionDirection;
	Vector3 cinematicSunDirection;
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
	std::fprintf( stderr, "%s failed with HRESULT 0x%08x\n", operation, static_cast<uint32_t>( result ) );
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

bool ConfigureReflectionSourceWithoutYield(
	StandaloneProbe& probe,
	EveSpaceScene& scene,
	int reflectionSource,
	std::string& error )
{
	if( reflectionSource < STANDALONE_REFLECTION_SOURCE_OFF ||
		reflectionSource > STANDALONE_REFLECTION_SOURCE_DYNAMIC )
	{
		error = "Invalid standalone reflection source";
		return false;
	}

	probe.reflectionSource = reflectionSource;
	if( reflectionSource == STANDALONE_REFLECTION_SOURCE_DYNAMIC )
	{
		scene.SetReflectionSetting( EntityComponents::REFLECTION_SETTING_ULTRA );
		Tr2ReflectionProbePtr reflectionProbe = scene.GetReflectionProbe();
		if( !reflectionProbe || !reflectionProbe->IsValid() )
		{
			error = "Dynamic reflection probe failed to create its render targets";
			return false;
		}
		reflectionProbe->SetRenderFrequency( Tr2ReflectionProbe::ALL_SIDES_PER_FRAME );
		const std::pair<Tr2EffectPtr, const char*> effects[] = {
			{ reflectionProbe->GetPreFilterEffect(), "reflection prefilter" },
			{ reflectionProbe->GetFilterEffect(), "reflection main filter" },
			{ reflectionProbe->GetCopyMipEffect(), "reflection cube copy" },
		};
		for( const auto& effect : effects )
		{
			if( !effect.first || !PrepareEffectResourcesWithoutYield( *effect.first, effect.second, error ) )
			{
				if( error.empty() )
				{
					error = std::string( effect.second ) + " effect is unavailable";
				}
				return false;
			}
		}
		if( reflectionProbe->GetReflectionWidth() != 256 ||
			reflectionProbe->GetReflectionHeight() != 256 ||
			reflectionProbe->GetReflectionMipCount() != 8 )
		{
			std::ostringstream message;
			message << "Dynamic reflection target has invalid dimensions "
					<< reflectionProbe->GetReflectionWidth() << "x"
					<< reflectionProbe->GetReflectionHeight() << " mips="
					<< reflectionProbe->GetReflectionMipCount();
			error = message.str();
			return false;
		}
		std::fprintf(
			stderr,
			"EVE reflection source ready: mode=dynamic quality=ultra frequency=all-sides-per-frame "
			"cube=%ux%u mips=%u\n",
			reflectionProbe->GetReflectionWidth(),
			reflectionProbe->GetReflectionHeight(),
			reflectionProbe->GetReflectionMipCount() );
		return true;
	}

	scene.SetReflectionSetting( EntityComponents::REFLECTION_SETTING_OFF );
	scene.SetReflectionProbe( nullptr );
	std::string environmentPath = scene.GetLightingSetup().environmentMapPath;
	if( reflectionSource == STANDALONE_REFLECTION_SOURCE_OFF )
	{
		environmentPath = "res:/dx9/scene/cinematic/black_cube_refl.dds";
		scene.SetEnvironmentMapResourcePath( environmentPath );
	}
	if( environmentPath.empty() )
	{
		error = std::string( ReflectionSourceName( reflectionSource ) ) + " reflection source has no environment cube";
		return false;
	}
	if( !PrepareTextureResourceWithoutYield(
			environmentPath,
			reflectionSource == STANDALONE_REFLECTION_SOURCE_OFF ? "disabled reflection cube" : "authored reflection cube",
			error ) )
	{
		return false;
	}
	std::fprintf(
		stderr,
		"EVE reflection source ready: mode=%s quality=off path=%s\n",
		ReflectionSourceName( reflectionSource ),
		environmentPath.c_str() );
	return true;
}

bool PrepareManagedEffectWithoutYield(
	const char* logicalPath,
	const char* role,
	std::initializer_list<const char*> requiredTechniques,
	std::string& error )
{
	Tr2EffectPtr effect;
	if( !effect.CreateInstance() )
	{
		error = std::string( role ) + " failed to create effect: " + logicalPath;
		return false;
	}
	effect->StartUpdate();
	effect->SetEffectPathName( logicalPath );
	effect->EndUpdate();
	Tr2EffectRes* resource = effect->GetEffectRes();
	if( resource )
	{
		resource->ForceSynchronousLoad();
		resource->Reload();
	}
	Tr2Shader* shader = effect->GetShaderStateInterface();
	if( !resource || !resource->IsGood() || !shader )
	{
		error = std::string( role ) + " failed to prepare: " + logicalPath;
		return false;
	}
	for( const char* techniqueName : requiredTechniques )
	{
		uint32_t technique = 0;
		if( !shader->GetTechniqueIndex( BlueSharedString( techniqueName ), technique ) )
		{
			error = std::string( role ) + " is missing technique " + techniqueName + ": " + logicalPath;
			return false;
		}
	}
	std::fprintf( stderr, "EVE %s effect ready: %s\n", role, logicalPath );
	return true;
}

bool PrepareSceneBackgroundWithoutYield( EveSpaceScene& scene, const char* scenePath, std::string& error )
{
	const EveSpaceScene::LightingSetup lighting = scene.GetLightingSetup();
	std::fprintf(
		stderr,
		"EVE scene lighting: scene=%s background=%s sunDirection=(%.4f, %.4f, %.4f) "
		"sunColor=(%.4f, %.4f, %.4f) ambient=(%.4f, %.4f, %.4f) nebula=%.4f "
		"reflection=%.4f backgroundReflection=%.4f diffuseRoughness=%.4f backlightContrast=%.4f "
		"dynamicObjectReflection=%s envRotation=(%.4f, %.4f, %.4f, %.4f) env=%s env1=%s env2=%s sh=%s\n",
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
		lighting.backgroundReflectionIntensity,
		lighting.defaultDiffuseRoughness,
		lighting.reflectionBackLightingContrast,
		lighting.dynamicObjectReflectionEnabled ? "enabled" : "disabled",
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
	float celestialRadiusScale = 1.0f;
	if( composition == STANDALONE_COMPOSITION_CINEMATIC )
	{
		scene.SetPlanetShadowsEnabled( false );
		celestialRadiusScale = probe.modelWorldScale;
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
			static_cast<float>( cinematicSunCenterDistance ) * celestialRadiusScale;
		planetPosition = Normalize( Vector3( 0.55f, -0.12f, 1.0f ) ) *
			static_cast<float>( warpCenterDistance ) * celestialRadiusScale;
		sunDirection = -Normalize( sunPosition );
		std::fprintf(
			stderr,
			"New Eden cinematic composition: authored bodies retained; exact-system planet eclipse disabled; standard planet warp-in "
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
	probe.cinematicSunDirection = Normalize( sunPosition );
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
		static_cast<float>( starRadius ) * celestialRadiusScale,
		Color( 0.0f, 0.0f, 0.0f, 1.0f ),
		Color( emissiveR, emissiveG, emissiveB, 1.0f ) );
	planet->SetStandalonePlacement(
		planetPosition,
		static_cast<float>( planetRadius ) * celestialRadiusScale,
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
	probe.planetSurfacePrepared = true;
	probe.planetAtmospherePrepared =
		planetLayers == STANDALONE_PLANET_ATMOSPHERE || planetLayers == STANDALONE_PLANET_ALL;
	probe.planetCloudsPrepared =
		planetLayers == STANDALONE_PLANET_CLOUDS || planetLayers == STANDALONE_PLANET_ALL;
	probe.sunGeometryPrepared = true;
	probe.authoredSunFlarePrepared =
		sunEffects == STANDALONE_SUN_EFFECTS_FLARE || sunEffects == STANDALONE_SUN_EFFECTS_ALL;
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
		probe.lensFlarePrepared = true;
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
	probe.newEdenSystemPrepared = true;
	return true;
}

bool ConfigureShLighting(
	StandaloneProbe& probe,
	int lightingView,
	int shSource,
	int sceneFixture,
	std::string& error )
{
	if( lightingView < STANDALONE_LIGHTING_COMBINED || lightingView > STANDALONE_LIGHTING_ENVIRONMENT )
	{
		error = "Invalid standalone lighting view";
		return false;
	}
	if( shSource < STANDALONE_SH_SOURCE_NEW_EDEN_CELESTIALS || shSource > STANDALONE_SH_SOURCE_NONE )
	{
		error = "Invalid standalone SH source";
		return false;
	}

	if( lightingView == STANDALONE_LIGHTING_SH || lightingView == STANDALONE_LIGHTING_LOCAL ||
		lightingView == STANDALONE_LIGHTING_ENVIRONMENT )
	{
		const EveSpaceScene::LightingSetup lighting = probe.scene->GetLightingSetup();
		probe.scene->SetSunLighting( lighting.sunDirection, Color( 0.0f, 0.0f, 0.0f, 1.0f ) );
		const char* isolatedName = lightingView == STANDALONE_LIGHTING_SH ? "SH" :
																			( lightingView == STANDALONE_LIGHTING_LOCAL ? "local" : "environment" );
		std::fprintf( stderr, "Lighting isolation: %s only; direct scene sun color is zero\n", isolatedName );
	}
	else
	{
		std::fprintf( stderr, "Lighting isolation: %s\n", lightingView == STANDALONE_LIGHTING_DIRECT ? "direct only" : "combined direct plus SH, local, and environment" );
	}

	if( lightingView == STANDALONE_LIGHTING_DIRECT || lightingView == STANDALONE_LIGHTING_LOCAL ||
		lightingView == STANDALONE_LIGHTING_ENVIRONMENT || shSource == STANDALONE_SH_SOURCE_NONE )
	{
		std::fprintf( stderr, "Trinity SH manager disabled for this comparison\n" );
		return true;
	}

	if( !probe.shLightingManager.CreateInstance() )
	{
		error = "Failed to create Tr2ShLightingManager";
		return false;
	}
	probe.shLightingManager->SetPrimaryIntensity( 3.14f );
	probe.shLightingManager->SetSecondaryIntensity( 3.14f );
	probe.scene->SetShLightingManager( probe.shLightingManager );
	std::fprintf(
		stderr,
		"Trinity SH manager: primaryIntensity=%.2f secondaryIntensity=%.2f source=%s\n",
		probe.shLightingManager->GetPrimaryIntensity(),
		probe.shLightingManager->GetSecondaryIntensity(),
		shSource == STANDALONE_SH_SOURCE_NEW_EDEN_CELESTIALS ? "new-eden-celestials" : "validation" );

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
			  "The standalone bridge reconstructs their active light descriptors through `ITr2LightOwner` and Trinity's tiled light manager. "
			  "Visible sprites, cones, planes, haze, and banners submit through the native attachment and quad-renderer paths under independent controls.\n\n";
	output << "## RC-06 Dynamic-Light Contract\n\n";
	output << "The current `soebase` payload enables `primary` and `soe`; the Capsuleer Day explicit light strip is outside those groups and is excluded. "
			  "Active point and spotlight attachments retain Trinity blink, fade, noise, profile, and cone conversion through the native attachment-light wrappers. "
			  "The static CMF bridge uses identity rest-pose bone deltas. V5 hull vertices, indexed decals, attachments, and lights retain raw GR2/SOF coordinates and share the same center-and-rotation object transform. "
			  "Banner lights use native `EveBannerSet` average-color sampling from the staged Sisters of EVE faction banner fixture. The installed client `LogoLoader` would normally replace these alliance/corporation slots from its photo cache and otherwise uses `res:/texture/global/black.dds`. ";
	if( hasLightBuffer && hasLightIndexBuffer )
	{
		output << "The selected Metal V5 payload declares both tiled-light buffers, so opaque consumption is eligible for visual A/B acceptance. ";
	}
	else
	{
		output << "The selected Metal V5 payload does not declare both tiled-light buffers, so manager/list generation remains capability-only evidence. ";
	}
	output << "Volumetric lighting remains disabled. Visible attachments, CORTAO, directional shadows, and indexed decals have separate accepted runtime gates.\n\n";

	output << "## RC-08B Local-Shadow Contract\n\n";
	output << "All six active Astero lights are authored SOF point lights: two haze lights and four banner lights. "
			  "SOF has no `castsShadows` field, so `--local-shadows authored` labels all six `ENABLED_ONLY_ON_HIGH_QUALITY` under the explicit `probe-all-active` policy. "
			  "Trinity selects and packs them through the native 16384x16384 D32 raster atlas, executes six cube faces per point light, and submits only the opaque hull and booster through each accepted `DynamicLightShadow` pass. "
			  "Directional cascades remain independently controlled by `--shadows`; attachments, decals, planets, atmosphere, and sun effects are not local-shadow casters. "
			  "The sample-owned resolver can project that atlas into a full-resolution R16_UINT light mask, but all 2,224 unique current-client Metal libraries exposing `EveSpaceSceneDynamicShadowMap` compile it as AIR `readnone`. The engine feature flag defaults off and the installed client does not override it. `--local-shadows auto` therefore resolves off for client parity; explicit modes retain the native writer as CP-21 diagnostic capability.\n\n";

	output << "## Per-Object Field Contract\n\n";
	output << "| Field family | Probe value | Classification |\n";
	output << "|---|---|---|\n";
	output << "| World/inverse/previous transforms | Rotating root transform | Derived each frame; previous equals current until velocity/TAA work. |\n";
	output << "| `shipData` | `(1, 1, 0, rawRadius)` | Authored root scale, activation, default dirt, and raw-model bounding radius. |\n";
	output << "| Clip sphere and custom masks | Disabled/zero; custom-mask matrices identity | Neutral because clipping and PPT permutations are disabled. |\n";
	output << "| Shape ellipsoid | `(-1, -1, -1, 0)` | Matches the unconfigured `EveSpaceObject2` sentinel. |\n";
	output << "| Bone and morph offsets | Zero | Static bind-pose bridge; no runtime animation or morph targets. |\n";
	output << "| SH coefficients | Scene manager or zero by isolation mode | High-tier opaque V5 consumes all seven packed SH vectors; the client sets primary and secondary intensity to `3.14`. |\n";
	output << "| Environment reflection | Authored static cube, ultra dynamic probe, or checksummed black control | High-tier opaque V5 samples the environment cube and reads reflection intensity from the packed ambient/reflection vector. Its selected permutation does not use authored ambient RGB as diffuse fill. |\n";
	output << "| Tiled local lights | Authored, validation, or disabled by isolation mode | Real `Tr2LightManager` buffers and `ComputeLightLists`; "
		   << ( hasLightBuffer && hasLightIndexBuffer ? "selected Metal V5 bindings present; visual A/B required for acceptance." : "selected Metal V5 bindings incomplete; opaque consumption unaccepted." )
		   << " |\n";
	output << "| Local-light shadows | Auto off; explicit authored or synthetic validation | Current macOS client parity is off: the feature flag defaults false and every shipped receiving candidate compiles the binding `readnone`. Explicit modes retain the native raster atlas and `DynamicLightShadow` caster pass as diagnostics. |\n";
	output << "| Screen/custom/impact data | Zero | Neutral for the selected opaque permutations and absent impact/custom systems. |\n";
	output << "\nThe selected client Metal payload requires an 1888-byte per-frame buffer and a 464-byte per-object pixel buffer. Both layouts are compile-time assertions in the standalone bridge.\n";

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
	probe.renderProductVisualizer->SetParameter( BlueSharedString( "AoUsesAlpha" ), 1.0f );
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

bool PrepareReflectionProductVisualizer( StandaloneProbe& probe )
{
	if( probe.reflectionProductVisualizer )
	{
		return true;
	}
	probe.reflectionProductVisualizer.CreateInstance();
	probe.reflectionProductVisualizer->StartUpdate();
	probe.reflectionProductVisualizer->SetEffectPathName(
		"res:/graphics/effect/managed/space/spaceobject/probe/reflectionproductvisualizer.fx" );
	probe.reflectionProductVisualizer->SetParameter( BlueSharedString( "ReflectionMip" ), 0.0f );
	probe.reflectionProductVisualizer->EndUpdate();
	Tr2EffectRes* resource = probe.reflectionProductVisualizer->GetEffectRes();
	if( resource )
	{
		resource->ForceSynchronousLoad();
		resource->Reload();
	}
	Tr2Shader* shader = probe.reflectionProductVisualizer->GetShaderStateInterface();
	uint32_t technique = 0;
	if( !resource || !resource->IsGood() || !shader ||
		!shader->GetTechniqueIndex( BlueSharedString( "Main" ), technique ) )
	{
		std::fprintf( stderr, "EVE reflection-product visualization effect is unavailable\n" );
		probe.reflectionProductVisualizer.Unlock();
		return false;
	}
	std::fprintf( stderr, "EVE reflection-product visualization effect ready: Main=%u layout=3x2\n", technique );
	return true;
}

bool EnsureRenderProductReadback( StandaloneProbe& probe, Tr2RenderContext& renderContext )
{
	if( probe.renderProductReadback.IsValid() &&
		probe.renderProductReadback.GetWidth() == probe.renderWidth &&
		probe.renderProductReadback.GetHeight() == probe.renderHeight )
	{
		return true;
	}

	probe.renderProductReadback = Tr2TextureAL{};
	const HRESULT result = probe.renderProductReadback.Create(
		Tr2BitmapDimensions(
			probe.renderWidth,
			probe.renderHeight,
			1,
			Tr2RenderContextEnum::PIXEL_FORMAT_R8G8B8A8_UNORM ),
		Tr2GpuUsage::RENDER_TARGET,
		Tr2CpuUsage::READ,
		renderContext.GetPrimaryRenderContext() );
	if( FAILED( result ) )
	{
		std::fprintf( stderr, "Failed to create CPU-readable EVE render-product target (HRESULT=0x%08x)\n", result );
		return false;
	}
	probe.renderProductReadback.SetName( "StandaloneProbeRenderProductReadback" );
	return true;
}

bool EnsureHdrCompositeReadback( StandaloneProbe& probe, Tr2RenderContext& renderContext )
{
	if( probe.hdrCompositeReadback.IsValid() &&
		probe.hdrCompositeReadback.GetWidth() == probe.renderWidth &&
		probe.hdrCompositeReadback.GetHeight() == probe.renderHeight )
	{
		return true;
	}

	probe.hdrCompositeReadback = Tr2TextureAL{};
	const HRESULT result = probe.hdrCompositeReadback.Create(
		Tr2BitmapDimensions(
			probe.renderWidth,
			probe.renderHeight,
			1,
			Tr2RenderContextEnum::PIXEL_FORMAT_R16G16B16A16_FLOAT ),
		Tr2GpuUsage::COPY_DESTINATION,
		Tr2CpuUsage::READ,
		renderContext.GetPrimaryRenderContext() );
	if( FAILED( result ) )
	{
		std::fprintf( stderr, "Failed to create CPU-readable FP16 composite target (HRESULT=0x%08x)\n", result );
		return false;
	}
	probe.hdrCompositeReadback.SetName( "StandaloneProbeHdrCompositeReadback" );
	return true;
}

bool EnsurePostTonemapReadback(
	StandaloneProbe& probe,
	Tr2RenderContext& renderContext,
	Tr2RenderContextEnum::PixelFormat format )
{
	if( probe.postTonemapReadback.IsValid() &&
		probe.postTonemapReadback.GetWidth() == probe.renderWidth &&
		probe.postTonemapReadback.GetHeight() == probe.renderHeight &&
		probe.postTonemapReadback.GetFormat() == format )
	{
		return true;
	}

	probe.postTonemapReadback = Tr2TextureAL{};
	const HRESULT result = probe.postTonemapReadback.Create(
		Tr2BitmapDimensions( probe.renderWidth, probe.renderHeight, 1, format ),
		Tr2GpuUsage::COPY_DESTINATION,
		Tr2CpuUsage::READ,
		renderContext.GetPrimaryRenderContext() );
	if( FAILED( result ) )
	{
		std::fprintf( stderr, "Failed to create CPU-readable post-tonemap target (HRESULT=0x%08x)\n", result );
		return false;
	}
	probe.postTonemapReadback.SetName( "StandaloneProbePostTonemapReadback" );
	return true;
}

bool ReadPostProcessDiagnostics( StandaloneProbe& probe, Tr2RenderContext& renderContext )
{
	Tr2PostProcessRenderer::Diagnostics source;
	if( !probe.driver || !probe.driver->ReadPostProcessDiagnostics( renderContext, source ) )
	{
		return false;
	}
	auto& destination = probe.postProcessDiagnostics;
	destination = {};
	destination.realTime = probe.lastRealTime;
	destination.simTime = probe.lastSimTime;
	destination.dynamicExposureActive = source.dynamicExposureActive;
	destination.histogramCreated = source.histogramCreated;
	destination.histogramMerged = source.histogramMerged;
	destination.exposureMeasured = source.exposureMeasured;
	destination.tonemappingSucceeded = source.tonemappingSucceeded;
	destination.sourceWidth = source.sourceWidth;
	destination.sourceHeight = source.sourceHeight;
	destination.sourceFormat = source.sourceFormat;
	destination.postTonemapWidth = source.postTonemapWidth;
	destination.postTonemapHeight = source.postTonemapHeight;
	destination.postTonemapFormat = source.postTonemapFormat;
	std::copy( source.histogram.begin(), source.histogram.end(), destination.histogram );
	std::copy( source.exposure.begin(), source.exposure.end(), destination.exposure );
	destination.minBrightness = source.minBrightness;
	destination.maxBrightness = source.maxBrightness;
	destination.increaseSpeed = source.increaseSpeed;
	destination.decreaseSpeed = source.decreaseSpeed;
	destination.minLuminance = source.minLuminance;
	destination.maxLuminance = source.maxLuminance;
	destination.exposureInfluence = source.exposureInfluence;
	destination.exposureMiddleValue = source.exposureMiddleValue;
	destination.exposureAdjustment = source.exposureAdjustment;
	destination.minExposure = source.minExposure;
	destination.maxExposure = source.maxExposure;
	destination.shoulderStrength = source.shoulderStrength;
	destination.linearStrength = source.linearStrength;
	destination.linearAngle = source.linearAngle;
	destination.toeStrength = source.toeStrength;
	destination.toeNumerator = source.toeNumerator;
	destination.toeDenominator = source.toeDenominator;
	destination.whiteScale = source.whiteScale;
	destination.outputGamma = source.outputGamma;
	destination.tonemappingMethod = source.tonemappingMethod;
	return true;
}

float DecodeSrgb( float value )
{
	return value <= 0.04045f ? value / 12.92f :
		std::pow( ( value + 0.055f ) / 1.055f, 2.4f );
}

float EncodeSrgb( float value )
{
	value = std::max( value, 0.0f );
	return value <= 0.0031308f ? value * 12.92f :
		1.055f * std::pow( value, 1.0f / 2.4f ) - 0.055f;
}

float Uncharted2Curve( float value, const TrinityStandalonePostProcessDiagnostics& diagnostics )
{
	const float a = diagnostics.shoulderStrength;
	const float b = diagnostics.linearStrength;
	const float c = diagnostics.linearAngle;
	const float d = diagnostics.toeStrength;
	const float e = diagnostics.toeNumerator;
	const float f = diagnostics.toeDenominator;
	return ( value * ( a * value + c * b ) + d * e ) /
			   ( value * ( a * value + b ) + d * f ) -
		e / f;
}

bool ReadToneMappingContract( StandaloneProbe& probe, Tr2RenderContext& renderContext )
{
	probe.toneValidation = {};
	if( !ReadPostProcessDiagnostics( probe, renderContext ) )
	{
		std::fprintf( stderr, "Failed to read RC-10 postprocess diagnostics\n" );
		return false;
	}
	const auto& diagnostics = probe.postProcessDiagnostics;
	if( !diagnostics.tonemappingSucceeded ||
		( diagnostics.dynamicExposureActive &&
		  ( !diagnostics.histogramCreated || !diagnostics.histogramMerged || !diagnostics.exposureMeasured ) ) )
	{
		std::fprintf( stderr, "RC-10 exposure or tone pass did not complete\n" );
		return false;
	}

	const void* preData = nullptr;
	const void* postData = nullptr;
	uint32_t prePitch = 0;
	uint32_t postPitch = 0;
	if( FAILED( probe.hdrCompositeReadback.MapForReading(
			Tr2TextureSubresource( 0 ), true, preData, prePitch, renderContext ) ) || !preData )
	{
		return false;
	}
	if( FAILED( probe.postTonemapReadback.MapForReading(
			Tr2TextureSubresource( 0 ), true, postData, postPitch, renderContext ) ) || !postData )
	{
		probe.hdrCompositeReadback.UnmapForReading( renderContext );
		return false;
	}

	constexpr uint64_t fnvOffset = 14695981039346656037ull;
	constexpr uint64_t fnvPrime = 1099511628211ull;
	uint64_t preHash = fnvOffset;
	uint64_t postHash = fnvOffset;
	std::vector<float> errors;
	std::vector<float> luminances;
	errors.reserve( static_cast<size_t>( probe.renderWidth ) * probe.renderHeight * 3 );
	luminances.reserve( static_cast<size_t>( probe.renderWidth ) * probe.renderHeight );
	double errorSum = 0.0;
	const bool bgra = probe.postTonemapReadback.GetFormat() == Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_UNORM ||
		probe.postTonemapReadback.GetFormat() == Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_UNORM_SRGB;
	const float adaptedLuminance = diagnostics.exposure[0];
	const float denominator = 0.5f +
		( adaptedLuminance - 0.5f ) * diagnostics.exposureInfluence;
	const float exposureMultiplier = diagnostics.dynamicExposureActive ?
		std::pow( 2.0f, diagnostics.exposureAdjustment ) * diagnostics.exposureMiddleValue /
			std::max( denominator, 0.000001f ) :
		std::pow( 2.0f, diagnostics.exposureAdjustment );
	const float white = Uncharted2Curve( diagnostics.whiteScale, diagnostics );

	for( uint32_t y = 0; y < probe.renderHeight; ++y )
	{
		const uint8_t* preRow = static_cast<const uint8_t*>( preData ) + static_cast<size_t>( y ) * prePitch;
		const uint8_t* postRow = static_cast<const uint8_t*>( postData ) + static_cast<size_t>( y ) * postPitch;
		for( size_t byte = 0; byte < static_cast<size_t>( probe.renderWidth ) * sizeof( Float_16 ) * 4; ++byte )
		{
			preHash = ( preHash ^ preRow[byte] ) * fnvPrime;
		}
		for( size_t byte = 0; byte < static_cast<size_t>( probe.renderWidth ) * 4; ++byte )
		{
			postHash = ( postHash ^ postRow[byte] ) * fnvPrime;
		}
		const Float_16* prePixels = reinterpret_cast<const Float_16*>( preRow );
		for( uint32_t x = 0; x < probe.renderWidth; ++x )
		{
			const uint8_t* output = postRow + x * 4;
			const uint8_t actual[3] = { output[bgra ? 2 : 0], output[1], output[bgra ? 0 : 2] };
			if( actual[0] == 0 && actual[1] == 0 && actual[2] == 0 )
				++probe.toneValidation.fullyBlackPixels;
			if( actual[0] == 255 && actual[1] == 255 && actual[2] == 255 )
				++probe.toneValidation.fullyWhitePixels;
			if( actual[0] == 255 || actual[1] == 255 || actual[2] == 255 )
				++probe.toneValidation.anySaturatedChannelPixels;
			luminances.push_back(
				0.2126f * actual[0] + 0.7152f * actual[1] + 0.0722f * actual[2] );
			for( uint32_t channel = 0; channel < 3; ++channel )
			{
				const float source = DecodeSrgb( static_cast<float>( prePixels[x * 4 + channel] ) );
				const float mapped = Uncharted2Curve( 2.0f * source * exposureMultiplier, diagnostics ) /
					std::max( white, 0.000001f );
				const float encoded = std::pow( std::max( EncodeSrgb( mapped ), 0.0f ), diagnostics.outputGamma );
				const float expected = std::min( std::max( encoded, 0.0f ), 1.0f ) * 255.0f;
				const float error = std::abs( static_cast<float>( actual[channel] ) - expected );
				errors.push_back( error );
				errorSum += error;
			}
		}
	}
	probe.postTonemapReadback.UnmapForReading( renderContext );
	probe.hdrCompositeReadback.UnmapForReading( renderContext );

	std::sort( errors.begin(), errors.end() );
	std::sort( luminances.begin(), luminances.end() );
	auto percentile = []( const std::vector<float>& values, double fraction ) {
		const size_t index = static_cast<size_t>( std::ceil( fraction * values.size() ) ) - 1;
		return values[std::min( index, values.size() - 1 )];
	};
	auto& result = probe.toneValidation;
	result.width = probe.renderWidth;
	result.height = probe.renderHeight;
	result.preTonemapFormat = probe.hdrCompositeReadback.GetFormat();
	result.postTonemapFormat = probe.postTonemapReadback.GetFormat();
	result.preTonemapHash = preHash;
	result.postTonemapHash = postHash;
	result.luminanceP01 = percentile( luminances, 0.01 );
	result.luminanceP50 = percentile( luminances, 0.50 );
	result.luminanceP99 = percentile( luminances, 0.99 );
	result.meanAbsoluteError = errorSum / errors.size();
	result.p999AbsoluteError = percentile( errors, 0.999 );
	result.maximumAbsoluteError = errors.back();
	result.valid = result.meanAbsoluteError <= 0.75 && result.p999AbsoluteError <= 2.0 &&
		result.maximumAbsoluteError <= 4.0;
	std::fprintf(
		stderr,
		"EVE RC-10 tone validation: pre=%016llx post=%016llx dimensions=%ux%u "
		"black=%llu white=%llu saturated=%llu luminance=(%.4f,%.4f,%.4f) "
		"error=(mean=%.4f p99.9=%.4f max=%.4f) validation=%s\n",
		static_cast<unsigned long long>( result.preTonemapHash ),
		static_cast<unsigned long long>( result.postTonemapHash ),
		result.width,
		result.height,
		static_cast<unsigned long long>( result.fullyBlackPixels ),
		static_cast<unsigned long long>( result.fullyWhitePixels ),
		static_cast<unsigned long long>( result.anySaturatedChannelPixels ),
		result.luminanceP01,
		result.luminanceP50,
		result.luminanceP99,
		result.meanAbsoluteError,
		result.p999AbsoluteError,
		result.maximumAbsoluteError,
		result.valid ? "pass" : "fail" );
	return result.valid;
}

double HdrPercentile( const std::vector<float>& sorted, double percentile )
{
	if( sorted.empty() )
	{
		return 0.0;
	}
	const size_t index = static_cast<size_t>( std::ceil( percentile * sorted.size() ) ) - 1;
	return sorted[std::min( index, sorted.size() - 1 )];
}

bool ReadRawHdrComposite( StandaloneProbe& probe, Tr2RenderContext& renderContext )
{
	auto& diagnostics = probe.hdrCompositeDiagnostics;
	diagnostics = {};
	diagnostics.format = static_cast<uint32_t>( probe.hdrCompositeReadback.GetFormat() );
	diagnostics.width = probe.hdrCompositeReadback.GetWidth();
	diagnostics.height = probe.hdrCompositeReadback.GetHeight();

	const void* sourceData = nullptr;
	uint32_t sourcePitch = 0;
	if( FAILED( probe.hdrCompositeReadback.MapForReading(
			Tr2TextureSubresource( 0 ), true, sourceData, sourcePitch, renderContext ) ) ||
		!sourceData )
	{
		std::fprintf( stderr, "Failed to read back the raw FP16 scene composite\n" );
		return false;
	}

	constexpr uint64_t fnvOffset = 14695981039346656037ull;
	constexpr uint64_t fnvPrime = 1099511628211ull;
	constexpr float halfMaximum = 65504.0f;
	const size_t rowBytes = static_cast<size_t>( diagnostics.width ) * sizeof( Float_16 ) * 4;
	std::vector<float> luminances;
	luminances.reserve( static_cast<size_t>( diagnostics.width ) * diagnostics.height );
	uint64_t rawHash = fnvOffset;
	double luminanceSum = 0.0;

	for( uint32_t y = 0; y < diagnostics.height; ++y )
	{
		const uint8_t* sourceRow = static_cast<const uint8_t*>( sourceData ) + static_cast<size_t>( y ) * sourcePitch;
		for( size_t byte = 0; byte < rowBytes; ++byte )
		{
			rawHash ^= sourceRow[byte];
			rawHash *= fnvPrime;
		}

		const Float_16* pixels = reinterpret_cast<const Float_16*>( sourceRow );
		for( uint32_t x = 0; x < diagnostics.width; ++x )
		{
			float channels[4];
			bool rgbFinite = true;
			for( uint32_t channel = 0; channel < 4; ++channel )
			{
				const float value = static_cast<float>( pixels[x * 4 + channel] );
				channels[channel] = value;
				if( std::isnan( value ) )
				{
					++diagnostics.nanComponents;
					rgbFinite = rgbFinite && channel >= 3;
				}
				else if( !std::isfinite( value ) )
				{
					++diagnostics.infComponents;
					rgbFinite = rgbFinite && channel >= 3;
				}
				else
				{
					++diagnostics.finiteComponents;
					if( value < -0.001f )
						++diagnostics.negativeComponents;
					if( std::abs( value ) >= halfMaximum )
						++diagnostics.saturatedComponents;
				}
			}
			if( rgbFinite )
			{
				const float luminance =
					0.2126f * channels[0] + 0.7152f * channels[1] + 0.0722f * channels[2];
				luminances.push_back( luminance );
				luminanceSum += luminance;
				if( luminance > 1.0f )
					++diagnostics.pixelsAboveOne;
			}
		}
	}
	probe.hdrCompositeReadback.UnmapForReading( renderContext );

	diagnostics.rawHash = rawHash;
	if( !luminances.empty() )
	{
		std::sort( luminances.begin(), luminances.end() );
		diagnostics.minimumLuminance = luminances.front();
		diagnostics.meanLuminance = luminanceSum / static_cast<double>( luminances.size() );
		diagnostics.maximumLuminance = luminances.back();
		diagnostics.p50Luminance = HdrPercentile( luminances, 0.50 );
		diagnostics.p95Luminance = HdrPercentile( luminances, 0.95 );
		diagnostics.p99Luminance = HdrPercentile( luminances, 0.99 );
	}

	const bool dimensionsValid = diagnostics.width == probe.renderWidth && diagnostics.height == probe.renderHeight;
	const bool formatValid = diagnostics.format == Tr2RenderContextEnum::PIXEL_FORMAT_R16G16B16A16_FLOAT;
	const bool finite = diagnostics.nanComponents == 0 && diagnostics.infComponents == 0;
	const bool rangeValid = diagnostics.negativeComponents == 0 && diagnostics.saturatedComponents == 0;
	const bool nonuniform = diagnostics.maximumLuminance - diagnostics.minimumLuminance > 0.000001;
	const bool nonblack = diagnostics.maximumLuminance > 0.000001;
	const bool hasHdrHeadroom = diagnostics.pixelsAboveOne != 0;
	diagnostics.valid = dimensionsValid && formatValid && finite && rangeValid && nonuniform && nonblack && hasHdrHeadroom;

	std::fprintf(
		stderr,
		"EVE pre-tonemap FP16 composite: format=%u dimensions=%ux%u hash=%016llx "
		"finite=%llu nan=%llu inf=%llu negative=%llu saturated=%llu above-one=%llu "
		"luminance=[min=%.8f mean=%.8f max=%.8f p50=%.8f p95=%.8f p99=%.8f] validation=%s\n",
		diagnostics.format,
		diagnostics.width,
		diagnostics.height,
		static_cast<unsigned long long>( diagnostics.rawHash ),
		static_cast<unsigned long long>( diagnostics.finiteComponents ),
		static_cast<unsigned long long>( diagnostics.nanComponents ),
		static_cast<unsigned long long>( diagnostics.infComponents ),
		static_cast<unsigned long long>( diagnostics.negativeComponents ),
		static_cast<unsigned long long>( diagnostics.saturatedComponents ),
		static_cast<unsigned long long>( diagnostics.pixelsAboveOne ),
		diagnostics.minimumLuminance,
		diagnostics.meanLuminance,
		diagnostics.maximumLuminance,
		diagnostics.p50Luminance,
		diagnostics.p95Luminance,
		diagnostics.p99Luminance,
		diagnostics.valid ? "pass" : "fail" );
	if( !diagnostics.valid )
	{
		std::fprintf(
			stderr,
			"FP16 composition contract failed: format=%s dimensions=%s finite=%s range=%s nonuniform=%s nonblack=%s hdr-headroom=%s\n",
			formatValid ? "valid" : "invalid",
			dimensionsValid ? "valid" : "invalid",
			finite ? "yes" : "no",
			rangeValid ? "valid" : "invalid",
			nonuniform ? "yes" : "no",
			nonblack ? "yes" : "no",
			hasHdrHeadroom ? "yes" : "no" );
	}
	return diagnostics.valid;
}

bool ReadVisualizedRenderProduct(
	StandaloneProbe& probe,
	int captureProduct,
	Tr2RenderContext& renderContext )
{
	probe.capturedProductWidth = probe.renderProductReadback.GetWidth();
	probe.capturedProductHeight = probe.renderProductReadback.GetHeight();
	probe.capturedProductPixels.resize(
		static_cast<size_t>( probe.capturedProductWidth ) * probe.capturedProductHeight * 4 );

	const void* sourceData = nullptr;
	uint32_t sourcePitch = 0;
	if( FAILED( probe.renderProductReadback.MapForReading(
			Tr2TextureSubresource( 0 ), true, sourceData, sourcePitch, renderContext ) ) ||
		!sourceData )
	{
		std::fprintf( stderr, "Failed to read back visualized EVE render product\n" );
		probe.capturedProductPixels.clear();
		return false;
	}

	uint8_t minimum = 255;
	uint8_t maximum = 0;
	for( uint32_t y = 0; y < probe.capturedProductHeight; ++y )
	{
		const uint8_t* sourceRow = static_cast<const uint8_t*>( sourceData ) +
			static_cast<size_t>( y ) * sourcePitch;
		uint8_t* destinationRow = probe.capturedProductPixels.data() +
			static_cast<size_t>( y ) * probe.capturedProductWidth * 4;
		std::memcpy( destinationRow, sourceRow, static_cast<size_t>( probe.capturedProductWidth ) * 4 );
		for( uint32_t x = 0; x < probe.capturedProductWidth; ++x )
		{
			const uint8_t* destination = destinationRow + x * 4;
			const uint8_t red = destination[0];
			const uint8_t green = destination[1];
			const uint8_t blue = destination[2];
			minimum = std::min( minimum, std::min( red, std::min( green, blue ) ) );
			maximum = std::max( maximum, std::max( red, std::max( green, blue ) ) );
		}
	}
	probe.renderProductReadback.UnmapForReading( renderContext );

	const bool requireVariation = captureProduct == STANDALONE_CAPTURE_SHADOW ||
		captureProduct == STANDALONE_CAPTURE_SHADOW_ATLAS ||
		captureProduct == STANDALONE_CAPTURE_LOCAL_SHADOW_ATLAS || captureProduct == STANDALONE_CAPTURE_AO ||
		captureProduct == STANDALONE_CAPTURE_BENT_NORMAL ||
		captureProduct == STANDALONE_CAPTURE_HDR_COMPOSITE ||
		( captureProduct == STANDALONE_CAPTURE_REFLECTION &&
		  probe.reflectionSource == STANDALONE_REFLECTION_SOURCE_DYNAMIC );
	std::fprintf(
		stderr,
		"EVE render-product readback: output=%ux%u range=[%u,%u]\n",
		probe.capturedProductWidth,
		probe.capturedProductHeight,
		minimum,
		maximum );
	if( requireVariation && minimum == maximum )
	{
		std::fprintf( stderr, "Enabled EVE render product is uniform and cannot satisfy its runtime contract\n" );
		probe.capturedProductPixels.clear();
		return false;
	}
	return true;
}

bool DrawDriverFrame( StandaloneProbe& probe, Be::Time realTime, Be::Time simTime, int captureProducts )
{
	Tr2RenderContext& renderContext = *probe.renderContext;
	EveSpaceSceneRenderDriver& driver = *probe.driver;
	const Tr2TextureAL& destination = renderContext.GetDefaultBackBuffer();
	const Tr2BitmapDimensions destinationDimensions = destination.GetDesc();
	std::array<BlueSharedString, 8> requestedNames;
	std::array<ITr2RenderNode::TempOutput, 8> outputStorage;
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
	if( captureProducts & STANDALONE_CAPTURE_SHADOW )
	{
		requestedNames[requestedCount] = BlueSharedString( "ShadowMap" );
		outputStorage[requestedCount].name = requestedNames[requestedCount];
		++requestedCount;
	}
	if( captureProducts & STANDALONE_CAPTURE_SHADOW_ATLAS )
	{
		requestedNames[requestedCount] = BlueSharedString( "CascadedShadowDepth" );
		outputStorage[requestedCount].name = requestedNames[requestedCount];
		++requestedCount;
	}
	if( captureProducts & STANDALONE_CAPTURE_LOCAL_SHADOW_ATLAS )
	{
		requestedNames[requestedCount] = BlueSharedString( "DynamicLightShadowDepth" );
		outputStorage[requestedCount].name = requestedNames[requestedCount];
		++requestedCount;
	}
	if( captureProducts & ( STANDALONE_CAPTURE_AO | STANDALONE_CAPTURE_BENT_NORMAL ) )
	{
		requestedNames[requestedCount] = BlueSharedString( "SSAOMap" );
		outputStorage[requestedCount].name = requestedNames[requestedCount];
		++requestedCount;
	}
	if( captureProducts & ( STANDALONE_CAPTURE_HDR_COMPOSITE | STANDALONE_CAPTURE_TONE_CONTRACT ) )
	{
		requestedNames[requestedCount] = BlueSharedString( "PreTonemapColor" );
		outputStorage[requestedCount].name = requestedNames[requestedCount];
		++requestedCount;
	}
	if( captureProducts & ( STANDALONE_CAPTURE_POST_TONEMAP | STANDALONE_CAPTURE_TONE_CONTRACT ) )
	{
		requestedNames[requestedCount] = BlueSharedString( "PostTonemapColor" );
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
	bool renderContextOpen = ok;
	if( ok )
	{
		const Tr2TextureAL* destinationTexture = &destination;
		ITr2RenderNode::Span<const Tr2TextureAL> destinationSpan = { destinationTexture, 1 };
		ITr2RenderNode::Span<ITr2RenderNode::TempOutput> outputSpan = { outputStorage.data(), requestedCount };
		Tr2ProfileTimer rootTimer;
		driver.Execute( destinationSpan, outputSpan, realTime, simTime, rootTimer, renderContext );
		if( !driver.GetLastPostProcessExecutionSucceeded() )
		{
			CCP_LOGERR( "EVE postprocess execution failed" );
			ok = false;
		}

		const bool colorProduct = captureProducts == STANDALONE_CAPTURE_COLOR;
		const bool reflectionProduct = captureProducts == STANDALONE_CAPTURE_REFLECTION;
		const bool hdrCompositeProduct = captureProducts == STANDALONE_CAPTURE_HDR_COMPOSITE;
		const bool postTonemapProduct = captureProducts == STANDALONE_CAPTURE_POST_TONEMAP;
		const bool toneContractProduct = captureProducts == STANDALONE_CAPTURE_TONE_CONTRACT;
		if( ok && toneContractProduct )
		{
			const Tr2TextureAL* preTonemap = nullptr;
			const Tr2TextureAL* postTonemap = nullptr;
			for( size_t outputIndex = 0; outputIndex < requestedCount; ++outputIndex )
			{
				if( outputStorage[outputIndex].name == BlueSharedString( "PreTonemapColor" ) &&
					outputStorage[outputIndex].texture.IsValid() )
					preTonemap = &outputStorage[outputIndex].texture.Get();
				if( outputStorage[outputIndex].name == BlueSharedString( "PostTonemapColor" ) &&
					outputStorage[outputIndex].texture.IsValid() )
					postTonemap = &outputStorage[outputIndex].texture.Get();
			}
			if( !preTonemap || !postTonemap || preTonemap->GetWidth() != probe.renderWidth ||
				preTonemap->GetHeight() != probe.renderHeight || postTonemap->GetWidth() != probe.renderWidth ||
				postTonemap->GetHeight() != probe.renderHeight ||
				preTonemap->GetFormat() != Tr2RenderContextEnum::PIXEL_FORMAT_R16G16B16A16_FLOAT )
			{
				std::fprintf( stderr, "RC-10 atomic tone outputs are missing or invalid\n" );
				ok = false;
			}
			if( ok )
			{
				ok = CheckHresult( Tr2Renderer::EndRenderContext(), "End RC-10 tone source" );
				renderContextOpen = false;
			}
			if( ok )
			{
				ok = CheckHresult( Tr2Renderer::BeginRenderContext(), "Begin RC-10 tone readback" );
				renderContextOpen = ok;
			}
			if( ok && EnsureHdrCompositeReadback( probe, renderContext ) &&
				EnsurePostTonemapReadback( probe, renderContext, postTonemap->GetFormat() ) )
			{
				ok = CheckHresult(
						 preTonemap->Resolve( probe.hdrCompositeReadback, renderContext ),
						 "Resolve RC-10 pre-tonemap color" ) &&
					CheckHresult(
						 postTonemap->Resolve( probe.postTonemapReadback, renderContext ),
						 "Resolve RC-10 post-tonemap color" );
			}
			else if( ok )
			{
				ok = false;
			}
			if( renderContextOpen )
			{
				ok = CheckHresult( Tr2Renderer::EndRenderContext(), "End RC-10 tone readback" ) && ok;
				renderContextOpen = false;
			}
			if( ok )
			{
				ok = ReadToneMappingContract( probe, renderContext );
			}
		}
		const char* selectedName = nullptr;
		if( colorProduct )
			selectedName = "Color";
		else if( reflectionProduct )
			selectedName = "ReflectionCube";
		else if( captureProducts == STANDALONE_CAPTURE_DEPTH )
			selectedName = "DepthMap";
		else if( captureProducts == STANDALONE_CAPTURE_NORMAL )
			selectedName = "NormalMap";
		else if( captureProducts == STANDALONE_CAPTURE_SHADOW )
			selectedName = "ShadowMap";
		else if( captureProducts == STANDALONE_CAPTURE_SHADOW_ATLAS )
			selectedName = "CascadedShadowDepth";
		else if( captureProducts == STANDALONE_CAPTURE_LOCAL_SHADOW_ATLAS )
			selectedName = "DynamicLightShadowDepth";
		else if( captureProducts == STANDALONE_CAPTURE_AO || captureProducts == STANDALONE_CAPTURE_BENT_NORMAL )
			selectedName = "SSAOMap";
		else if( hdrCompositeProduct )
			selectedName = "PreTonemapColor";
		else if( postTonemapProduct )
			selectedName = "PostTonemapColor";
		if( selectedName )
		{
			ok = CheckHresult( Tr2Renderer::EndRenderContext(), "End scene render-product source" ) && ok;
			renderContextOpen = false;
			if( ok )
			{
				ok = CheckHresult( Tr2Renderer::BeginRenderContext(), "Begin render-product visualization" );
				renderContextOpen = ok;
			}
			const Tr2TextureAL* selectedTexture = nullptr;
			if( colorProduct )
			{
				selectedTexture = &destination;
			}
			else if( reflectionProduct )
			{
				ITr2TextureProviderPtr environment = probe.scene->GetResolvedEnvironmentMap();
				if( environment )
				{
					selectedTexture = environment->GetTexture();
				}
			}
			else
			{
				for( size_t outputIndex = 0; outputIndex < requestedCount; ++outputIndex )
				{
					if( outputStorage[outputIndex].name == BlueSharedString( selectedName ) && outputStorage[outputIndex].texture.IsValid() )
					{
						selectedTexture = &outputStorage[outputIndex].texture.Get();
						break;
					}
				}
			}
			if( !selectedTexture )
			{
				std::fprintf( stderr, "Requested EVE render product '%s' was not produced\n", selectedName );
				ok = false;
			}
			else
			{
				if( reflectionProduct &&
					( selectedTexture->GetType() != Tr2RenderContextEnum::TEX_TYPE_CUBE ||
					  selectedTexture->GetWidth() <= 1 || selectedTexture->GetHeight() <= 1 ||
					  selectedTexture->GetMipCount() == 0 ) )
				{
					std::fprintf(
						stderr,
						"Reflection cube is invalid: type=%d dimensions=%ux%u mips=%u\n",
						static_cast<int>( selectedTexture->GetType() ),
						selectedTexture->GetWidth(),
						selectedTexture->GetHeight(),
						selectedTexture->GetMipCount() );
					ok = false;
				}
				const bool fullResolutionProduct = captureProducts == STANDALONE_CAPTURE_SHADOW ||
					captureProducts == STANDALONE_CAPTURE_AO || captureProducts == STANDALONE_CAPTURE_BENT_NORMAL ||
					hdrCompositeProduct || postTonemapProduct;
				if( fullResolutionProduct &&
					( selectedTexture->GetWidth() != probe.renderWidth || selectedTexture->GetHeight() != probe.renderHeight ) )
				{
					std::fprintf(
						stderr,
						"Requested EVE render product '%s' has invalid dimensions %ux%u; expected %ux%u\n",
						selectedName,
						selectedTexture->GetWidth(),
						selectedTexture->GetHeight(),
						probe.renderWidth,
						probe.renderHeight );
					ok = false;
				}
				if( captureProducts == STANDALONE_CAPTURE_SHADOW_ATLAS &&
					( selectedTexture->GetWidth() != 16384 || selectedTexture->GetHeight() != 4096 ) )
				{
					std::fprintf(
						stderr,
						"Cascaded shadow atlas has invalid dimensions %ux%u; expected 16384x4096\n",
						selectedTexture->GetWidth(),
						selectedTexture->GetHeight() );
					ok = false;
				}
				if( captureProducts == STANDALONE_CAPTURE_LOCAL_SHADOW_ATLAS &&
					( selectedTexture->GetWidth() != 16384 || selectedTexture->GetHeight() != 16384 ||
					  selectedTexture->GetFormat() != Tr2RenderContextEnum::PIXEL_FORMAT_D32_FLOAT ) )
				{
					std::fprintf(
						stderr,
						"Dynamic-light shadow atlas is invalid: format=%u dimensions=%ux%u; expected D32 16384x16384\n",
						static_cast<uint32_t>( selectedTexture->GetFormat() ),
						selectedTexture->GetWidth(),
						selectedTexture->GetHeight() );
					ok = false;
				}
				if( captureProducts == STANDALONE_CAPTURE_AO || captureProducts == STANDALONE_CAPTURE_BENT_NORMAL )
				{
					const auto expectedFormat = probe.aoMethod == STANDALONE_AO_CORTAO ?
						Tr2RenderContextEnum::PIXEL_FORMAT_R8G8B8A8_SNORM :
						Tr2RenderContextEnum::PIXEL_FORMAT_R8_UNORM;
					if( selectedTexture->GetFormat() != expectedFormat )
					{
						std::fprintf(
							stderr,
							"SSAO product has invalid format=%u; expected=%u\n",
							static_cast<uint32_t>( selectedTexture->GetFormat() ),
							static_cast<uint32_t>( expectedFormat ) );
						ok = false;
					}
				}
				if( hdrCompositeProduct &&
					selectedTexture->GetFormat() != Tr2RenderContextEnum::PIXEL_FORMAT_R16G16B16A16_FLOAT )
				{
					std::fprintf(
						stderr,
						"Pre-tonemap composite has invalid format=%u; expected R16G16B16A16_FLOAT\n",
						static_cast<uint32_t>( selectedTexture->GetFormat() ) );
					ok = false;
				}
				if( postTonemapProduct &&
					selectedTexture->GetFormat() != Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_UNORM &&
					selectedTexture->GetFormat() != Tr2RenderContextEnum::PIXEL_FORMAT_R8G8B8A8_UNORM )
				{
					std::fprintf(
						stderr,
						"Post-tonemap color has invalid format=%u; expected an 8-bit UNORM target\n",
						static_cast<uint32_t>( selectedTexture->GetFormat() ) );
					ok = false;
				}
				const bool visualizerReady = reflectionProduct ?
					PrepareReflectionProductVisualizer( probe ) :
					PrepareRenderProductVisualizer( probe );
				Tr2EffectPtr visualizer = reflectionProduct ?
					probe.reflectionProductVisualizer :
					probe.renderProductVisualizer;
				if( ok && visualizerReady &&
					EnsureRenderProductReadback( probe, renderContext ) &&
					( !hdrCompositeProduct || EnsureHdrCompositeReadback( probe, renderContext ) ) )
				{
					if( hdrCompositeProduct )
					{
						ok = CheckHresult(
								 selectedTexture->Resolve( probe.hdrCompositeReadback, renderContext ),
								 "Resolve raw FP16 composition" ) &&
							ok;
					}
					if( !reflectionProduct )
					{
						const bool depthAtlasProduct = captureProducts == STANDALONE_CAPTURE_SHADOW_ATLAS ||
							captureProducts == STANDALONE_CAPTURE_LOCAL_SHADOW_ATLAS;
						const float visualizationMode = hdrCompositeProduct ? 7.0f :
							postTonemapProduct                                ? 2.0f :
							colorProduct                                    ? 2.0f :
																			  ( captureProducts == STANDALONE_CAPTURE_DEPTH ? 1.0f :
																															  ( captureProducts == STANDALONE_CAPTURE_NORMAL ? 2.0f :
																																											   ( captureProducts == STANDALONE_CAPTURE_SHADOW ? 3.0f :
																																																								( depthAtlasProduct ? 4.0f :
																																																													  ( captureProducts == STANDALONE_CAPTURE_AO ? 5.0f : 6.0f ) ) ) ) );
						probe.renderProductVisualizer->SetParameter(
							BlueSharedString( "RenderProductMode" ),
							visualizationMode );
						probe.renderProductVisualizer->SetParameter(
							BlueSharedString( "AoUsesAlpha" ),
							probe.aoMethod == STANDALONE_AO_CORTAO ? 1.0f : 0.0f );
					}
					renderContext.RenderPassHint( { Tr2LoadAction::DONT_CARE, Tr2StoreAction::STORE }, {} );
					renderContext.m_esm.SetRenderTarget( 0, probe.renderProductReadback );
					renderContext.m_esm.SetRenderTarget( 1, Tr2TextureAL{} );
					renderContext.m_esm.SetRenderTarget( 2, Tr2TextureAL{} );
					renderContext.m_esm.SetRenderTarget( 3, Tr2TextureAL{} );
					renderContext.m_esm.SetDepthStencilBuffer( {} );
					renderContext.m_esm.SetFullScreenViewport();
					ok = CheckHresult(
							 renderContext.Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0xff000000, 1.0f ),
							 "Clear render-product readback target" ) &&
						ok;
					renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_FULLSCREEN );
					ok = Tr2Renderer::DrawTexture( renderContext, visualizer, *selectedTexture ) && ok;
					ok = CheckHresult( Tr2Renderer::EndRenderContext(), "End render-product readback" ) && ok;
					renderContextOpen = false;
					if( ok )
					{
						if( hdrCompositeProduct )
						{
							ok = ReadRawHdrComposite( probe, renderContext );
						}
						ok = ReadVisualizedRenderProduct( probe, captureProducts, renderContext ) && ok;
						if( ok && captureProducts == STANDALONE_CAPTURE_AO )
						{
							probe.aoProductValidated = true;
						}
						if( ok && reflectionProduct && probe.reflectionSource == STANDALONE_REFLECTION_SOURCE_DYNAMIC )
						{
							probe.reportedReflectionStatus = true;
						}
					}
					if( ok )
					{
						ok = CheckHresult( Tr2Renderer::BeginRenderContext(), "Begin render-product presentation" );
						renderContextOpen = ok;
					}
					if( ok )
					{
						renderContext.RenderPassHint( { Tr2LoadAction::DONT_CARE, Tr2StoreAction::STORE }, {} );
						renderContext.m_esm.SetRenderTarget( 0, destination );
						renderContext.m_esm.SetRenderTarget( 1, Tr2TextureAL{} );
						renderContext.m_esm.SetRenderTarget( 2, Tr2TextureAL{} );
						renderContext.m_esm.SetRenderTarget( 3, Tr2TextureAL{} );
						renderContext.m_esm.SetDepthStencilBuffer( {} );
						renderContext.m_esm.SetFullScreenViewport();
						renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_FULLSCREEN );
						ok = Tr2Renderer::DrawTexture(
								 renderContext,
								 visualizer,
								 *selectedTexture ) &&
							ok;
					}
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
		const bool validateDynamicReflection =
			( captureProducts == 0 || captureProducts == STANDALONE_CAPTURE_COLOR ) &&
			probe.reflectionSource == STANDALONE_REFLECTION_SOURCE_DYNAMIC &&
			!probe.reportedReflectionStatus && probe.renderedFrameCount >= 1;
		if( ok && renderContextOpen && validateDynamicReflection )
		{
			Tr2ReflectionProbePtr reflectionProbe = probe.scene->GetReflectionProbe();
			ITr2TextureProviderPtr environment = probe.scene->GetResolvedEnvironmentMap();
			Tr2TextureAL* reflectionTexture = environment ? environment->GetTexture() : nullptr;
			if( !reflectionProbe || !reflectionProbe->LastFilterSucceeded() || !reflectionProbe->HasData() ||
				!reflectionTexture || !reflectionTexture->IsValid() ||
				reflectionTexture->GetType() != Tr2RenderContextEnum::TEX_TYPE_CUBE ||
				reflectionTexture->GetWidth() <= 1 || reflectionTexture->GetHeight() <= 1 ||
				reflectionTexture->GetMipCount() <= 1 )
			{
				std::fprintf(
					stderr,
					"Dynamic reflection contract failed after two warm-up frames: filter=%s data=%s texture=%s\n",
					reflectionProbe && reflectionProbe->LastFilterSucceeded() ? "success" : "failed",
					reflectionProbe && reflectionProbe->HasData() ? "yes" : "no",
					reflectionTexture && reflectionTexture->IsValid() ? "valid" : "invalid" );
				ok = false;
			}
			if( ok && PrepareReflectionProductVisualizer( probe ) &&
				EnsureRenderProductReadback( probe, renderContext ) )
			{
				renderContext.RenderPassHint( { Tr2LoadAction::DONT_CARE, Tr2StoreAction::STORE }, {} );
				renderContext.m_esm.SetRenderTarget( 0, probe.renderProductReadback );
				renderContext.m_esm.SetRenderTarget( 1, Tr2TextureAL{} );
				renderContext.m_esm.SetRenderTarget( 2, Tr2TextureAL{} );
				renderContext.m_esm.SetRenderTarget( 3, Tr2TextureAL{} );
				renderContext.m_esm.SetDepthStencilBuffer( {} );
				renderContext.m_esm.SetFullScreenViewport();
				renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_FULLSCREEN );
				ok = Tr2Renderer::DrawTexture(
					renderContext,
					probe.reflectionProductVisualizer,
					*reflectionTexture );
				ok = CheckHresult( Tr2Renderer::EndRenderContext(), "End dynamic reflection validation" ) && ok;
				renderContextOpen = false;
				if( ok )
				{
					ok = ReadVisualizedRenderProduct( probe, STANDALONE_CAPTURE_REFLECTION, renderContext );
				}
				if( ok )
				{
					std::fprintf(
						stderr,
						"Dynamic reflection contract accepted: format=%d cube=%ux%u mips=%u filter=success nonuniform=yes\n",
						static_cast<int>( reflectionTexture->GetFormat() ),
						reflectionTexture->GetWidth(),
						reflectionTexture->GetHeight(),
						reflectionTexture->GetMipCount() );
					probe.reportedReflectionStatus = true;
					probe.capturedProductPixels.clear();
				}
			}
			else if( ok )
			{
				ok = false;
			}
		}
		if( renderContextOpen )
		{
			ok = CheckHresult( Tr2Renderer::EndRenderContext(), "Tr2Renderer::EndRenderContext" ) && ok;
			renderContextOpen = false;
		}
	}
	Tr2Renderer::EndFrame();
	if( !ok )
	{
		return false;
	}
	return ok && CheckHresult( renderContext.Present(), "Present" );
}

bool ValidateRc09Composition( StandaloneProbe& probe )
{
	if( !probe.scene || !probe.driver || !probe.renderable )
	{
		CCP_LOGERR( "RC-09 composition validation requires a configured EVE model scene" );
		return false;
	}

	std::vector<std::string> failures;
	auto require = [&]( bool condition, const char* contract ) {
		if( !condition )
		{
			failures.emplace_back( contract );
		}
	};

	const uint32_t opaqueBatches = probe.renderable->GetCommittedOpaqueBatchCount();
	std::array<uint32_t, 6> attachmentCounts = {};
	for( uint32_t family = 0; family < attachmentCounts.size(); ++family )
	{
		attachmentCounts[family] = probe.renderable->GetAttachmentActiveCount( family );
	}
	const uint32_t decalCount = probe.renderable->GetSelectedDecalCount();
	const uint32_t decalTriangles = probe.renderable->GetSelectedDecalTriangleCount();
	Tr2LightManager* lightManager = Tr2LightManager::GetInstance();
	const size_t resolvedLights = lightManager ? lightManager->GetResolvedLightCount() : 0;
	const auto* directionalShadows = probe.scene->FindDirectionalShadowCasterDiagnostics(
		static_cast<const IEveShadowCaster*>( probe.renderable.p ) );
	const uint32_t acceptedCascades = directionalShadows ? directionalShadows->acceptedCascades : 0;
	const uint32_t directionalShadowBatches = directionalShadows ? directionalShadows->committedBatches : 0;

	require( probe.renderedFrameCount >= 2, "at least two scene warm-up frames" );
	require( !probe.renderable->DrawFailed(), "renderable draw status" );
	require( opaqueBatches == 2, "two committed opaque hull/booster batches" );
	require( attachmentCounts[0] == 83, "83 active sprites" );
	require( attachmentCounts[1] == 0, "zero active sprite lines" );
	require( attachmentCounts[2] == 4, "four active spotlights" );
	require( attachmentCounts[3] == 16, "16 active planes" );
	require( attachmentCounts[4] == 2, "two active hazes" );
	require( attachmentCounts[5] == 4, "four active banners" );
	require( decalCount == 11 && decalTriangles == 28 && probe.renderable->HaveDecalsCommitted(),
			 "11 committed decals containing 28 LOD0 triangles" );
	require( probe.renderable->GetConstructedLightCount() == 6 && resolvedLights == 6,
			 "six authored and resolved local lights" );
	require( probe.localShadows == STANDALONE_LOCAL_SHADOWS_OFF,
			 "client-parity local-light shadows disabled" );
	require( acceptedCascades == 3 && directionalShadowBatches == 6,
			 "three accepted directional cascades and six shadow batches" );
	require( probe.aoProductValidated, "full-resolution high CORTAO product" );
	require( probe.ssao && probe.ssao->GetCortaoEnabled() && probe.ssao->GetCortaoBentNormal(),
			 "CORTAO bent-normal configuration" );
	require( probe.hdrCompositeDiagnostics.valid, "finite nonuniform FP16 composite with HDR headroom" );

	Tr2ReflectionProbePtr reflectionProbe = probe.scene->GetReflectionProbe();
	require( probe.reflectionSource == STANDALONE_REFLECTION_SOURCE_DYNAMIC && reflectionProbe,
			 "dynamic reflection probe" );
	require( reflectionProbe && reflectionProbe->HasData() && reflectionProbe->LastFilterSucceeded(),
			 "successful filtered reflection data" );
	require( reflectionProbe && reflectionProbe->GetReflectionWidth() == 256 &&
				 reflectionProbe->GetReflectionHeight() == 256 && reflectionProbe->GetReflectionMipCount() == 8,
			 "256x256 eight-mip reflection cube" );
	require( probe.scene->GetReflectionSetting() == EntityComponents::REFLECTION_SETTING_ULTRA,
			 "ultra reflection quality" );
	require( probe.driver->GetReflectionCorrectionEnabled(), "client reflection correction" );
	require( probe.shLightingManager &&
				 std::abs( probe.shLightingManager->GetPrimaryIntensity() - 3.14f ) < 0.001f &&
				 std::abs( probe.shLightingManager->GetSecondaryIntensity() - 3.14f ) < 0.001f,
			 "primary and secondary SH intensity 3.14" );

	Tr2EffectPtr background = probe.scene->GetBackgroundEffect();
	Tr2EffectRes* backgroundResource = background ? background->GetEffectRes() : nullptr;
	require( probe.backgroundPrepared && backgroundResource && backgroundResource->IsGood(),
			 "prepared authored New Eden background" );
	require( probe.newEdenSystemPrepared && probe.scene->Planets().size() == 2,
			 "prepared New Eden sun and planet" );
	require( probe.planetSurfacePrepared && probe.planetAtmospherePrepared && probe.planetCloudsPrepared,
			 "prepared planet surface, atmosphere, and clouds" );
	require( probe.sunGeometryPrepared && probe.authoredSunFlarePrepared,
			 "prepared sun geometry and authored flare geometry" );
	require( probe.lensFlarePrepared && probe.scene->Lensflares().size() == 1,
			 "prepared system lens flare with occlusion" );
	Tr2PostProcess2Ptr postProcess = probe.scene->GetPostProcess();
	require( probe.godRaysPrepared && postProcess && postProcess->GetGodRaysIfAvailable( PostProcess::HIGH ),
			 "prepared depth-aware god rays" );
	const bool expectsDynamicExposure = probe.qualityRung >= STANDALONE_PROBE_RUNG_HDR_EXPOSURE &&
		probe.dynamicExposureMode == STANDALONE_DYNAMIC_EXPOSURE_CLIENT;
	require(
		postProcess && static_cast<bool>( postProcess->GetDynamicExposureIfAvailable( PostProcess::HIGH ) ) ==
			expectsDynamicExposure,
		expectsDynamicExposure ? "client dynamic exposure enabled" : "dynamic exposure disabled" );
	require( postProcess && !postProcess->GetBloomIfAvailable( PostProcess::HIGH ), "bloom disabled" );
	require( postProcess && !postProcess->GetFilmGrainIfAvailable( PostProcess::HIGH ), "film grain disabled" );
	require( postProcess && !postProcess->GetTaaIfAvailable( PostProcess::HIGH ), "TAA disabled" );
	require( postProcess && !postProcess->GetDepthOfFieldIfAvailable( PostProcess::HIGH ),
			 "depth of field disabled" );
	require( postProcess && !postProcess->GetFogIfAvailable( PostProcess::HIGH ), "postprocess fog disabled" );
	const size_t volumetricRenderableCount = probe.scene->m_componentRegistry ?
		probe.scene->m_componentRegistry->ComponentCount<ITr2VolumetricRenderable>() :
		0;
	const bool froxelFogActive = probe.scene->m_volumetricsRenderer && probe.scene->m_volumetricsRenderer->HasFog();
	require( volumetricRenderableCount == 0 && !froxelFogActive,
			 "volumetric renderables and froxel fog disabled" );

	const auto& settings = probe.driver->GetSettings();
	require( !settings.bypassPostProcessing && !settings.postProcessBlitOnly,
			 "full HDR postprocess path" );
	require( settings.postProcessingQuality == PostProcess::HIGH, "high postprocess quality" );
	require( settings.shadowQuality == ShadowQuality::SHADOW_HIGH && settings.enableDirectionalShadows,
			 "high directional cascades" );
	require( settings.aoQuality == EveSpaceSceneRenderDriver::AmbientOcclusionQuality::High,
			 "high ambient occlusion" );
	require( !settings.enableDistortion && !settings.enableUpscaling &&
				 settings.antiAliasingQuality == EveSpaceSceneRenderDriver::AntiAliasingQuality::Disabled &&
				 !settings.forceVelocityMap,
			 "distortion, upscaling, TAA, and velocity disabled" );
	require( settings.forceOpaqueBuffer && settings.forceNormalMap,
			 "opaque and normal products retained" );

	std::fprintf(
		stderr,
		"EVE RC-09 composition inventory: opaque=%u attachments=[sprites=%u sprite-lines=%u spotlights=%u "
		"planes=%u hazes=%u banners=%u] decals=%u triangles=%u lights=%zu cascades=%u shadowBatches=%u "
		"ao=%s reflection=%ux%u/%u hdr=%s failures=%zu\n",
		opaqueBatches,
		attachmentCounts[0],
		attachmentCounts[1],
		attachmentCounts[2],
		attachmentCounts[3],
		attachmentCounts[4],
		attachmentCounts[5],
		decalCount,
		decalTriangles,
		resolvedLights,
		acceptedCascades,
		directionalShadowBatches,
		probe.aoProductValidated ? "valid" : "invalid",
		reflectionProbe ? reflectionProbe->GetReflectionWidth() : 0,
		reflectionProbe ? reflectionProbe->GetReflectionHeight() : 0,
		reflectionProbe ? reflectionProbe->GetReflectionMipCount() : 0,
		probe.hdrCompositeDiagnostics.valid ? "valid" : "invalid",
		failures.size() );
	std::ostringstream summary;
	summary << "profile=RC-09 validation=" << ( failures.empty() ? "pass" : "fail" )
			<< " frame=" << probe.renderedFrameCount << " opaque=" << opaqueBatches
			<< " sprites=" << attachmentCounts[0] << " spriteLines=" << attachmentCounts[1]
			<< " spotlights=" << attachmentCounts[2] << " planes=" << attachmentCounts[3]
			<< " hazes=" << attachmentCounts[4] << " banners=" << attachmentCounts[5]
			<< " decals=" << decalCount << " decalTriangles=" << decalTriangles
			<< " localLights=" << resolvedLights << " acceptedCascades=" << acceptedCascades
			<< " shadowBatches=" << directionalShadowBatches
			<< " ao=" << ( probe.aoProductValidated ? "valid" : "invalid" )
			<< " reflection=" << ( reflectionProbe ? reflectionProbe->GetReflectionWidth() : 0 ) << "x"
			<< ( reflectionProbe ? reflectionProbe->GetReflectionHeight() : 0 ) << "/"
			<< ( reflectionProbe ? reflectionProbe->GetReflectionMipCount() : 0 )
			<< " hdr=" << ( probe.hdrCompositeDiagnostics.valid ? "valid" : "invalid" );
	probe.compositionValidationSummary = summary.str();
	for( const std::string& failure : failures )
	{
		std::fprintf( stderr, "  RC-09 FAILED: %s\n", failure.c_str() );
	}
	if( failures.empty() )
	{
		std::fprintf( stderr, "EVE RC-09 complete HDR composition validation accepted\n" );
	}
	return failures.empty();
}

bool ValidateClientPostProcessContract( StandaloneProbe& probe, Tr2PostProcess2& postProcess, std::string& error )
{
	auto exposure = postProcess.GetDynamicExposureIfAvailable( PostProcess::HIGH );
	auto tonemapping = postProcess.GetTonemappingIfAvailable( PostProcess::HIGH );
	if( !exposure || !tonemapping )
	{
		error = "The client postprocess does not contain dynamic exposure and tone mapping";
		return false;
	}
	auto near = []( float actual, float expected ) { return std::abs( actual - expected ) <= 0.000001f; };
	auto require = [&]( bool condition, const char* field ) {
		if( !condition && error.empty() )
		{
			error = std::string( "Client postprocess contract mismatch: " ) + field;
		}
	};
	require( tonemapping->m_method == Tr2PPTonemappingEffect::Uncharted2, "tone method is not Uncharted2" );
	require( near( tonemapping->m_uncharted2.m_shoulderStrength, 0.125f ), "shoulderStrength" );
	require( near( tonemapping->m_uncharted2.m_linearStrength, 0.25f ), "linearStrength" );
	require( near( tonemapping->m_uncharted2.m_linearAngle, 0.1f ), "linearAngle" );
	require( near( tonemapping->m_uncharted2.m_toeStrength, 0.15f ), "toeStrength" );
	require( near( tonemapping->m_uncharted2.m_toeNumerator, 0.021f ), "toeNumerator" );
	require( near( tonemapping->m_uncharted2.m_toeDenominator, 0.3f ), "toeDenominator" );
	require( near( tonemapping->m_uncharted2.m_whiteScale, 2.5f ), "whiteScale" );
	require( near( exposure->m_minBrightness, 0.9f ), "minBrightness" );
	require( near( exposure->m_maxBrightness, 0.98f ), "maxBrightness" );
	require( near( exposure->m_increaseSpeed, 2.0f ), "increaseSpeed" );
	require( near( exposure->m_decreaseSpeed, 1.5f ), "decreaseSpeed" );
	require( near( exposure->m_minLuminance, 0.4649f ), "minLuminance" );
	require( near( exposure->m_maxLuminance, 10.0f ), "maxLuminance" );
	require( near( exposure->m_influence, 1.0f ), "influence" );
	require( near( exposure->m_middleValue, 0.55f ), "middleValue" );
	require( near( exposure->m_adjustment, 0.0f ), "adjustment" );
	require( near( exposure->m_minExposure, -3.7f ), "minExposure" );
	require( near( exposure->m_maxExposure, 10.0f ), "maxExposure" );
	require( near( postProcess.m_exposureAdjustment, 0.0f ), "postprocess exposureAdjustment" );
	if( !error.empty() )
	{
		return false;
	}
	probe.clientDynamicExposure = exposure;
	probe.clientTonemapping = tonemapping;
	std::fprintf(
		stderr,
		"EVE RC-10 postprocess contract: method=Uncharted2 containerMethod=baked gamma=1.000000 "
		"curve=(%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f) "
		"percentiles=(%.6f,%.6f) luminance=(%.6f,%.6f) speeds=(%.6f,%.6f) "
		"middle=%.6f influence=%.6f adjustment=%.6f stops=(%.6f,%.6f)\n",
		tonemapping->m_uncharted2.m_shoulderStrength,
		tonemapping->m_uncharted2.m_linearStrength,
		tonemapping->m_uncharted2.m_linearAngle,
		tonemapping->m_uncharted2.m_toeStrength,
		tonemapping->m_uncharted2.m_toeNumerator,
		tonemapping->m_uncharted2.m_toeDenominator,
		tonemapping->m_uncharted2.m_whiteScale,
		exposure->m_minBrightness,
		exposure->m_maxBrightness,
		exposure->m_minLuminance,
		exposure->m_maxLuminance,
		exposure->m_increaseSpeed,
		exposure->m_decreaseSpeed,
		exposure->m_middleValue,
		exposure->m_influence,
		exposure->m_adjustment,
		exposure->m_minExposure,
		exposure->m_maxExposure );
	return true;
}

void UpdateProbeCamera( StandaloneProbe& probe )
{
	constexpr uint64_t kStaticCameraFrames = 180;
	constexpr float kCameraRadius = 5.2f;
	constexpr float kOrbitFrames = 900.0f;
	if( probe.exposureSequenceActive || !probe.view || !probe.renderable ||
		probe.cameraView != STANDALONE_CAMERA_MODEL || probe.renderedFrameCount < kStaticCameraFrames )
	{
		return;
	}

	const float orbitFrame = static_cast<float>( probe.renderedFrameCount - kStaticCameraFrames + 1 );
	const float angle = orbitFrame * ( 2.0f * 3.1415926535f / kOrbitFrames );
	const Vector3 eye(
		kCameraRadius * probe.modelWorldScale * std::sin( angle ),
		0.0f,
		-kCameraRadius * probe.modelWorldScale * std::cos( angle ) );
	probe.view->SetLookAtPosition( eye, Vector3( 0.0f, 0.0f, 0.0f ), Vector3( 0.0f, 1.0f, 0.0f ) );
	if( !probe.reportedCameraOrbit )
	{
		std::fprintf( stderr, "EVE probe camera orbit active after %llu static frames: radius=%.1f period=%.1f seconds\n", static_cast<unsigned long long>( kStaticCameraFrames ), kCameraRadius * probe.modelWorldScale, kOrbitFrames / 60.0f );
		probe.reportedCameraOrbit = true;
	}
}

bool ConfigureDriverScene( StandaloneProbe& probe, int qualityRung, const char* assetPath, int materialView, int materialMode, int areaView, const char* sceneResourcePath, int sceneFixture, int lightingView, int shSource, int localLights, int localShadows, int reflectionSource, int reflectionCorrection, int normalMapMode, int cameraView, int composition, int planetLayers, int cloudYear, int cloudMonth, int cloudDay, int sunEffects, int attachments, int attachmentView, int decals, int decalView, uint32_t killCount, float modelYawDegrees, int shadows, int ambientOcclusion, int aoMethod )
{
	probe.qualityRung = qualityRung;
	if( !std::isfinite( modelYawDegrees ) )
	{
		CCP_LOGERR( "Invalid standalone model yaw" );
		return false;
	}
	if( localLights < STANDALONE_LOCAL_LIGHTS_OFF || localLights > STANDALONE_LOCAL_LIGHTS_VALIDATION )
	{
		CCP_LOGERR( "Invalid standalone local-light mode" );
		return false;
	}
	if( localShadows < STANDALONE_LOCAL_SHADOWS_OFF || localShadows > STANDALONE_LOCAL_SHADOWS_VALIDATION )
	{
		CCP_LOGERR( "Invalid standalone local-shadow mode" );
		return false;
	}
	if( reflectionCorrection < STANDALONE_REFLECTION_CORRECTION_OFF || reflectionCorrection > STANDALONE_REFLECTION_CORRECTION_CLIENT )
	{
		CCP_LOGERR( "Invalid standalone reflection-correction mode" );
		return false;
	}
	if( reflectionSource < STANDALONE_REFLECTION_SOURCE_OFF || reflectionSource > STANDALONE_REFLECTION_SOURCE_DYNAMIC )
	{
		CCP_LOGERR( "Invalid standalone reflection-source mode" );
		return false;
	}
	if( normalMapMode < STANDALONE_NORMAL_MAP_AUTHORED || normalMapMode > STANDALONE_NORMAL_MAP_FLAT )
	{
		CCP_LOGERR( "Invalid standalone normal-map mode" );
		return false;
	}
	if( decals < STANDALONE_DECALS_OFF || decals > STANDALONE_DECALS_AUTHORED ||
		decalView < STANDALONE_DECAL_VIEW_ALL || decalView > STANDALONE_DECAL_VIEW_KILLMARKS )
	{
		CCP_LOGERR( "Invalid standalone decal mode or view" );
		return false;
	}
	if( shadows < STANDALONE_SHADOWS_OFF || shadows > STANDALONE_SHADOWS_HIGH ||
		ambientOcclusion < STANDALONE_AO_OFF || ambientOcclusion > STANDALONE_AO_HIGH ||
		aoMethod < STANDALONE_AO_CORTAO || aoMethod > STANDALONE_AO_CACAO )
	{
		CCP_LOGERR( "Invalid standalone shadow or ambient-occlusion mode" );
		return false;
	}
	if( localShadows != STANDALONE_LOCAL_SHADOWS_OFF && shadows == STANDALONE_SHADOWS_LOW )
	{
		CCP_LOGERR( "Local-light shadows require high native shadow quality and cannot be combined with low directional shadows" );
		return false;
	}
	probe.shadows = shadows;
	probe.localShadows = localShadows;
	probe.ambientOcclusion = ambientOcclusion;
	probe.aoMethod = aoMethod;
	const float aspect = probe.renderHeight > 0 ? static_cast<float>( probe.renderWidth ) / static_cast<float>( probe.renderHeight ) : 1.0f;
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
		probe.modelWorldScale = probe.renderable->GetAuthoredWorldScale();
		std::fprintf(
			stderr,
			"Standalone authored-scale contract: worldScale=%.8f cameraDistance=%.8f nativeShadowSplits=yes\n",
			probe.modelWorldScale,
			5.2f * probe.modelWorldScale );
	}
	std::string qualityResourceError;
	if( shadows != STANDALONE_SHADOWS_OFF )
	{
		if( !PrepareManagedEffectWithoutYield(
				"res:/graphics/effect/managed/space/system/ShadowDepth.fx",
				"cascaded shadow resolve",
				{ "Main" },
				qualityResourceError ) )
		{
			std::fprintf( stderr, "Failed to prepare directional shadows: %s\n", qualityResourceError.c_str() );
			return false;
		}
		if( shadows == STANDALONE_SHADOWS_HIGH )
		{
			const std::pair<const char*, const char*> denoisers[] = {
				{ "res:/graphics/effect/managed/space/system/EstimateNoise.fx", "shadow noise estimation" },
				{ "res:/graphics/effect/managed/space/system/DenoiseEstimate.fx", "shadow denoise estimation" },
				{ "res:/graphics/effect/managed/space/system/Denoise1D.fx", "shadow one-dimensional denoise" },
			};
			for( const auto& denoiser : denoisers )
			{
				if( !PrepareManagedEffectWithoutYield( denoiser.first, denoiser.second, { "Main" }, qualityResourceError ) )
				{
					std::fprintf( stderr, "Failed to prepare high-quality directional shadows: %s\n", qualityResourceError.c_str() );
					return false;
				}
			}
		}
	}
	if( ambientOcclusion != STANDALONE_AO_OFF )
	{
		if( aoMethod == STANDALONE_AO_CORTAO )
		{
			if( !PrepareManagedEffectWithoutYield(
					"res:/graphics/effect/managed/space/system/CORTAO/CORTAO.fx",
					"CORTAO",
					{ "Pack", "MainPass" },
					qualityResourceError ) ||
				!PrepareManagedEffectWithoutYield(
					"res:/graphics/effect/managed/space/system/CORTAO/Blur.fx",
					"CORTAO blur",
					{ "Blur" },
					qualityResourceError ) ||
				!PrepareTextureResourceWithoutYield(
					"res:/texture/ssao/24x24x16x16.dds",
					"CORTAO lookup table",
					qualityResourceError ) )
			{
				std::fprintf( stderr, "Failed to prepare CORTAO: %s\n", qualityResourceError.c_str() );
				return false;
			}
		}
		else if( !PrepareManagedEffectWithoutYield(
					 "res:/graphics/effect/managed/space/system/SSAO/SSAO.fx",
					 "FidelityFX CACAO",
					 {},
					 qualityResourceError ) )
		{
			std::fprintf( stderr, "Failed to prepare CACAO: %s\n", qualityResourceError.c_str() );
			return false;
		}
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
	if( lightingView == STANDALONE_LIGHTING_DIRECT || lightingView == STANDALONE_LIGHTING_SH ||
		lightingView == STANDALONE_LIGHTING_ENVIRONMENT )
	{
		localLights = STANDALONE_LOCAL_LIGHTS_OFF;
	}
	if( localShadows == STANDALONE_LOCAL_SHADOWS_AUTHORED && localLights != STANDALONE_LOCAL_LIGHTS_AUTHORED )
	{
		CCP_LOGERR( "Authored local shadows require authored local lights" );
		return false;
	}
	if( localShadows == STANDALONE_LOCAL_SHADOWS_VALIDATION && localLights != STANDALONE_LOCAL_LIGHTS_VALIDATION )
	{
		CCP_LOGERR( "Validation local shadows require the validation local light" );
		return false;
	}
	if( localShadows != STANDALONE_LOCAL_SHADOWS_OFF &&
		( qualityRung < STANDALONE_PROBE_RUNG_MODEL || materialMode == 0 || areaView != 0 ) )
	{
		CCP_LOGERR( "Local shadows require an all-area eve-v5 model at the model rung or higher" );
		return false;
	}
	if( lightingView == STANDALONE_LIGHTING_DIRECT || lightingView == STANDALONE_LIGHTING_SH ||
		lightingView == STANDALONE_LIGHTING_LOCAL )
	{
		reflectionSource = STANDALONE_REFLECTION_SOURCE_OFF;
	}
	probe.localLights = localLights;
	probe.localShadows = localShadows;
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
		Tr2LightManager::SetDynamicLightShadowsEnabled( localShadows != STANDALONE_LOCAL_SHADOWS_OFF );
		Tr2LightManager::DeleteInstance();
		if( !Tr2LightManager::GetOrCreateInstance( effectPath ) )
		{
			CCP_LOGERR( "Failed to create Tr2LightManager" );
			return false;
		}
		std::fprintf(
			stderr,
			"Trinity tiled local-light path ready: effect=%s passes=%u localShadows=%s eligibility=%s\n",
			effectPath,
			shader->GetPassCount( 0 ),
			localShadows == STANDALONE_LOCAL_SHADOWS_OFF ? "off" :
														   ( localShadows == STANDALONE_LOCAL_SHADOWS_AUTHORED ? "authored" : "validation" ),
			localShadows == STANDALONE_LOCAL_SHADOWS_AUTHORED ? "probe-all-active" :
																( localShadows == STANDALONE_LOCAL_SHADOWS_VALIDATION ? "probe-validation" : "none" ) );
		if( localShadows != STANDALONE_LOCAL_SHADOWS_OFF )
		{
			const char* resolvePath = "res:/graphics/effect/managed/space/system/rasterdynamicshadowresolve.fx";
			if( !probe.dynamicLightShadowResolveEffect.CreateInstance() )
			{
				CCP_LOGERR( "Failed to create the raster dynamic-light shadow receiver resolve effect" );
				return false;
			}
			probe.dynamicLightShadowResolveEffect->StartUpdate();
			probe.dynamicLightShadowResolveEffect->SetEffectPathName( resolvePath );
			probe.dynamicLightShadowResolveEffect->EndUpdate();
			if( !PrepareEffectResourcesWithoutYield(
					*probe.dynamicLightShadowResolveEffect,
					"raster dynamic-light shadow receiver resolve",
					qualityResourceError ) )
			{
				std::fprintf( stderr, "Failed to prepare local-shadow receiver resolve: %s\n", qualityResourceError.c_str() );
				return false;
			}
			uint32_t techniqueIndex = 0;
			Tr2Shader* shader = probe.dynamicLightShadowResolveEffect->GetShaderStateInterface();
			if( !shader || !shader->GetTechniqueIndex( BlueSharedString( "Main" ), techniqueIndex ) )
			{
				CCP_LOGERR( "Raster dynamic-light shadow receiver resolve is missing the Main technique" );
				return false;
			}
			std::fprintf(
				stderr,
				"Raster local-shadow receiver adapter ready: effect=%s technique=Main output=R16_UINT\n",
				resolvePath );
		}
	}
	else
	{
		g_eveSpaceSceneDynamicLighting = false;
		Tr2LightManager::SetDynamicLightShadowsEnabled( false );
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
		probe.backgroundPrepared = true;

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
			postProcess->SetColorCorrection( nullptr );
			postProcess->ClearLuts();
			postProcess->SetVignette( nullptr );
			postProcess->SetDesaturate( nullptr );
			postProcess->SetFade( nullptr );
			postProcess->SetDepthOfField( nullptr );
			postProcess->SetFog( nullptr );
			postProcess->SetGenericEffect( nullptr );
			postProcess->SetSignalLoss( nullptr );
			auto exposure = postProcess->GetDynamicExposureIfAvailable();
			if( !exposure || !postProcess->GetTonemappingIfAvailable() )
			{
				CCP_LOGERR( "EVE default postprocess does not provide dynamic exposure and tone mapping" );
				return false;
			}
			if( !ValidateClientPostProcessContract( probe, *postProcess, postProcessError ) )
			{
				std::fprintf( stderr, "EVE client postprocess contract failed: %s\n", postProcessError.c_str() );
				return false;
			}
			probe.clientPostProcess = postProcess;
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
			probe.godRaysPrepared = true;
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
	probe.scene->SetDynamicLightShadowResolveEffect( probe.dynamicLightShadowResolveEffect );
	std::string reflectionError;
	if( !ConfigureReflectionSourceWithoutYield( probe, *probe.scene, reflectionSource, reflectionError ) )
	{
		std::fprintf(
			stderr,
			"Failed to configure %s EVE reflection source: %s\n",
			ReflectionSourceName( reflectionSource ),
			reflectionError.c_str() );
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
	if( ambientOcclusion != STANDALONE_AO_OFF )
	{
		if( !probe.ssao.CreateInstance() )
		{
			CCP_LOGERR( "Failed to create Tr2SSAO" );
			return false;
		}
		probe.ssao->SetCortaoEnabled( aoMethod == STANDALONE_AO_CORTAO );
		probe.ssao->SetCortaoBentNormal( aoMethod == STANDALONE_AO_CORTAO );
		probe.driver->SetSSAO( probe.ssao );
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

	constexpr float newEdenPlanetWarpCenterDistance = 31245000.0f;
	const Vector3 planetPosition = composition == STANDALONE_COMPOSITION_CINEMATIC ? Normalize( Vector3( 0.55f, -0.12f, 1.0f ) ) * newEdenPlanetWarpCenterDistance * probe.modelWorldScale : Vector3( 1083758787326.0f, -205372890997.0f, -787280443197.0f );
	const Vector3 eye( 0.0f, 0.0f, -5.2f * probe.modelWorldScale );
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
	settings.enableDirectionalShadows = shadows != STANDALONE_SHADOWS_OFF;
	settings.shadowQuality = localShadows != STANDALONE_LOCAL_SHADOWS_OFF ? ShadowQuality::SHADOW_HIGH :
																			( shadows == STANDALONE_SHADOWS_HIGH ? ShadowQuality::SHADOW_HIGH :
																												   ( shadows == STANDALONE_SHADOWS_LOW ? ShadowQuality::SHADOW_LOW : ShadowQuality::SHADOW_DISABLED ) );
	settings.antiAliasingQuality = EveSpaceSceneRenderDriver::AntiAliasingQuality::Disabled;
	settings.aoQuality = ambientOcclusion == STANDALONE_AO_HIGH ? EveSpaceSceneRenderDriver::AmbientOcclusionQuality::High :
																  ( ambientOcclusion == STANDALONE_AO_MEDIUM ? EveSpaceSceneRenderDriver::AmbientOcclusionQuality::Medium :
																											   ( ambientOcclusion == STANDALONE_AO_LOW ? EveSpaceSceneRenderDriver::AmbientOcclusionQuality::Low :
																																						 EveSpaceSceneRenderDriver::AmbientOcclusionQuality::Disabled ) );
	settings.volumetricQuality = Tr2VolumerticQuality::Low;
	settings.postProcessingQuality = qualityRung >= STANDALONE_PROBE_RUNG_HDR_POST ? PostProcess::HIGH : PostProcess::LOW;
	settings.forceNormalMap = qualityRung >= STANDALONE_PROBE_RUNG_MODEL;
	settings.forceOpaqueBuffer = true;
	settings.forceVelocityMap = false;
	std::fprintf(
		stderr,
		"EVE RC-08B quality: directionalShadows=%s nativeShadowQuality=%s denoiser=%s "
		"localShadows=%s eligibility=%s ao=%s method=%s bentNormals=%s "
		"reflection=%s forceOpaqueBuffer=true v5PerFrame=%zu v5PerObject=%zu\n",
		shadows == STANDALONE_SHADOWS_HIGH ? "high" : ( shadows == STANDALONE_SHADOWS_LOW ? "low" : "off" ),
		settings.shadowQuality == ShadowQuality::SHADOW_HIGH ? "high" :
															   ( settings.shadowQuality == ShadowQuality::SHADOW_LOW ? "low" : "off" ),
		shadows == STANDALONE_SHADOWS_HIGH ? "enabled" : "disabled",
		localShadows == STANDALONE_LOCAL_SHADOWS_OFF ? "off" :
													   ( localShadows == STANDALONE_LOCAL_SHADOWS_AUTHORED ? "authored" : "validation" ),
		localShadows == STANDALONE_LOCAL_SHADOWS_AUTHORED ? "probe-all-active" :
															( localShadows == STANDALONE_LOCAL_SHADOWS_VALIDATION ? "probe-validation" : "none" ),
		ambientOcclusion == STANDALONE_AO_HIGH ? "high" :
												 ( ambientOcclusion == STANDALONE_AO_MEDIUM ? "medium" : ( ambientOcclusion == STANDALONE_AO_LOW ? "low" : "off" ) ),
		aoMethod == STANDALONE_AO_CORTAO ? "cortao" : "cacao",
		ambientOcclusion != STANDALONE_AO_OFF && aoMethod == STANDALONE_AO_CORTAO ? "enabled" : "disabled",
		ReflectionSourceName( reflectionSource ),
		sizeof( EveSpaceScene::PerFramePSData ),
		sizeof( EveSpaceObjectPSData ) );
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
		std::string loadError;
		probe.renderable->SetModelYawDegrees( modelYawDegrees );
		probe.renderable->SetShadowCastingEnabled(
			shadows != STANDALONE_SHADOWS_OFF || localShadows != STANDALONE_LOCAL_SHADOWS_OFF );
		if( localLights != STANDALONE_LOCAL_LIGHTS_OFF || attachments == 2 || decals == STANDALONE_DECALS_AUTHORED )
		{
			auto hull = LoadBlackObjectWithoutYield<EveSOFDataHull>(
				"res:/dx9/model/spaceobjectfactory/hulls/soef1_t1.black", loadError );
			auto faction = LoadBlackObjectWithoutYield<EveSOFDataFaction>(
				"res:/dx9/model/spaceobjectfactory/factions/soebase.black", loadError );
			if( !hull || !faction )
			{
				std::fprintf( stderr, "Failed to load Astero SOF attachment descriptors: %s\n", loadError.c_str() );
				return false;
			}
			if( localLights != STANDALONE_LOCAL_LIGHTS_OFF &&
				!probe.renderable->ConfigureLocalLights( localLights, localShadows, *hull, *faction, loadError ) )
			{
				std::fprintf( stderr, "Failed to configure Astero local lights: %s\n", loadError.c_str() );
				return false;
			}
			EveSOFDataGenericPtr generic;
			if( attachments == 2 || decals == STANDALONE_DECALS_AUTHORED )
			{
				generic = LoadBlackObjectWithoutYield<EveSOFDataGeneric>(
					"res:/dx9/model/spaceobjectfactory/generic.black", loadError );
				if( !generic )
				{
					std::fprintf( stderr, "Failed to load Astero generic SOF descriptor: %s\n", loadError.c_str() );
					return false;
				}
			}
			if( attachments == 2 && !probe.renderable->ConfigureAttachments( attachments, attachmentView, *hull, *faction, *generic, loadError ) )
			{
				std::fprintf( stderr, "Failed to configure Astero visible attachments: %s\n", loadError.c_str() );
				return false;
			}
			if( decals == STANDALONE_DECALS_AUTHORED &&
				!probe.renderable->ConfigureDecals( decals, decalView, killCount, *hull, *faction, *generic, loadError ) )
			{
				std::fprintf( stderr, "Failed to configure Astero indexed decals: %s\n", loadError.c_str() );
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

TRINITY_STANDALONE_EXPORT bool TrinityStandaloneProbeGetCapturedProduct(
	void* opaqueProbe,
	const uint8_t** pixels,
	uint32_t* width,
	uint32_t* height,
	uint32_t* pitch )
{
	auto* probe = static_cast<StandaloneProbe*>( opaqueProbe );
	if( !probe || !pixels || !width || !height || !pitch || probe->capturedProductPixels.empty() )
	{
		return false;
	}
	*pixels = probe->capturedProductPixels.data();
	*width = probe->capturedProductWidth;
	*height = probe->capturedProductHeight;
	*pitch = probe->capturedProductWidth * 4;
	return true;
}

TRINITY_STANDALONE_EXPORT bool TrinityStandaloneProbeGetHdrCompositeDiagnostics(
	void* opaqueProbe,
	uint32_t* format,
	uint32_t* width,
	uint32_t* height,
	uint64_t* rawHash,
	uint64_t* finiteComponents,
	uint64_t* nanComponents,
	uint64_t* infComponents,
	uint64_t* negativeComponents,
	uint64_t* saturatedComponents,
	uint64_t* pixelsAboveOne,
	double* minimumLuminance,
	double* meanLuminance,
	double* maximumLuminance,
	double* p50Luminance,
	double* p95Luminance,
	double* p99Luminance,
	bool* validationPassed )
{
	auto* probe = static_cast<StandaloneProbe*>( opaqueProbe );
	if( !probe || !format || !width || !height || !rawHash || !finiteComponents || !nanComponents ||
		!infComponents || !negativeComponents || !saturatedComponents || !pixelsAboveOne ||
		!minimumLuminance || !meanLuminance || !maximumLuminance || !p50Luminance ||
		!p95Luminance || !p99Luminance || !validationPassed || probe->hdrCompositeDiagnostics.width == 0 )
	{
		return false;
	}
	const auto& diagnostics = probe->hdrCompositeDiagnostics;
	*format = diagnostics.format;
	*width = diagnostics.width;
	*height = diagnostics.height;
	*rawHash = diagnostics.rawHash;
	*finiteComponents = diagnostics.finiteComponents;
	*nanComponents = diagnostics.nanComponents;
	*infComponents = diagnostics.infComponents;
	*negativeComponents = diagnostics.negativeComponents;
	*saturatedComponents = diagnostics.saturatedComponents;
	*pixelsAboveOne = diagnostics.pixelsAboveOne;
	*minimumLuminance = diagnostics.minimumLuminance;
	*meanLuminance = diagnostics.meanLuminance;
	*maximumLuminance = diagnostics.maximumLuminance;
	*p50Luminance = diagnostics.p50Luminance;
	*p95Luminance = diagnostics.p95Luminance;
	*p99Luminance = diagnostics.p99Luminance;
	*validationPassed = diagnostics.valid;
	return true;
}

TRINITY_STANDALONE_EXPORT bool TrinityStandaloneProbeValidateComposition( void* opaqueProbe )
{
	auto* probe = static_cast<StandaloneProbe*>( opaqueProbe );
	return probe && ValidateRc09Composition( *probe );
}

TRINITY_STANDALONE_EXPORT const char* TrinityStandaloneProbeGetCompositionValidationSummary( void* opaqueProbe )
{
	auto* probe = static_cast<StandaloneProbe*>( opaqueProbe );
	return probe && !probe->compositionValidationSummary.empty() ?
		probe->compositionValidationSummary.c_str() :
		nullptr;
}

TRINITY_STANDALONE_EXPORT bool TrinityStandaloneProbeGetShadowDiagnostics(
	void* opaqueProbe,
	uint32_t* casterTests,
	uint32_t* acceptedCascades,
	uint32_t* committedBatches )
{
	auto* probe = static_cast<StandaloneProbe*>( opaqueProbe );
	if( !probe || !probe->scene || !casterTests || !acceptedCascades || !committedBatches )
	{
		return false;
	}
	const auto* diagnostics = probe->renderable ?
		probe->scene->FindDirectionalShadowCasterDiagnostics(
			static_cast<const IEveShadowCaster*>( probe->renderable.p ) ) :
		nullptr;
	if( !diagnostics )
	{
		return false;
	}
	*casterTests = diagnostics->tests;
	*acceptedCascades = diagnostics->acceptedCascades;
	*committedBatches = diagnostics->committedBatches;
	return true;
}

TRINITY_STANDALONE_EXPORT bool TrinityStandaloneProbeGetLocalShadowDiagnostics(
	void* opaqueProbe,
	uint32_t* eligibleLights,
	uint32_t* selectedLights,
	uint32_t* droppedLights,
	uint32_t* packedEntries,
	uint32_t* hazeEligible,
	uint32_t* bannerEligible,
	uint32_t* validationEligible,
	uint32_t* pointLights,
	uint32_t* spotLights,
	uint32_t* facePasses,
	uint32_t* casterTests,
	uint32_t* acceptedCasterPasses,
	uint32_t* committedBatches,
	uint32_t* atlasFormat,
	uint32_t* atlasWidth,
	uint32_t* atlasHeight,
	bool* allocationSucceeded,
	bool* atlasValid )
{
	auto* probe = static_cast<StandaloneProbe*>( opaqueProbe );
	Tr2LightManager* manager = Tr2LightManager::GetInstance();
	if( !probe || !probe->renderable || !probe->scene || !manager || !eligibleLights || !selectedLights ||
		!droppedLights || !packedEntries || !hazeEligible || !bannerEligible || !validationEligible ||
		!pointLights || !spotLights || !facePasses || !casterTests || !acceptedCasterPasses ||
		!committedBatches || !atlasFormat || !atlasWidth || !atlasHeight || !allocationSucceeded || !atlasValid )
	{
		return false;
	}
	const auto& managerStats = manager->GetDynamicShadowDiagnostics();
	const auto& sceneStats = probe->scene->GetDynamicLightShadowDiagnostics();
	*eligibleLights = managerStats.eligibleLightCount;
	*selectedLights = managerStats.selectedLightCount;
	*droppedLights = managerStats.droppedLightCount;
	*packedEntries = static_cast<uint32_t>( managerStats.entries.size() );
	*hazeEligible = probe->renderable->GetHazeShadowEligibleCount();
	*bannerEligible = probe->renderable->GetBannerShadowEligibleCount();
	*validationEligible = probe->renderable->GetValidationShadowEligibleCount();
	*pointLights = sceneStats.pointLights;
	*spotLights = sceneStats.spotLights;
	*facePasses = sceneStats.facePasses;
	*casterTests = sceneStats.casterTests;
	*acceptedCasterPasses = sceneStats.acceptedCasterPasses;
	*committedBatches = sceneStats.committedBatches;
	*atlasFormat = sceneStats.atlasFormat;
	*atlasWidth = sceneStats.atlasWidth;
	*atlasHeight = sceneStats.atlasHeight;
	*allocationSucceeded = managerStats.allocationSucceeded;
	*atlasValid = sceneStats.atlasValid;
	return true;
}

TRINITY_STANDALONE_EXPORT bool TrinityStandaloneProbeGetReflectionDiagnostics(
	void* opaqueProbe,
	int* source,
	uint32_t* format,
	uint32_t* width,
	uint32_t* height,
	uint32_t* mipCount,
	bool* hasData,
	bool* filterSucceeded,
	float* reflectionIntensity,
	float* shPrimaryIntensity,
	float* shSecondaryIntensity )
{
	auto* probe = static_cast<StandaloneProbe*>( opaqueProbe );
	if( !probe || !probe->scene || !source || !format || !width || !height || !mipCount ||
		!hasData || !filterSucceeded || !reflectionIntensity || !shPrimaryIntensity || !shSecondaryIntensity )
	{
		return false;
	}
	ITr2TextureProviderPtr environment = probe->scene->GetResolvedEnvironmentMap();
	Tr2TextureAL* texture = environment ? environment->GetTexture() : nullptr;
	Tr2ReflectionProbePtr reflectionProbe = probe->scene->GetReflectionProbe();
	*source = probe->reflectionSource;
	*format = texture && texture->IsValid() ? static_cast<uint32_t>( texture->GetFormat() ) : 0;
	*width = texture && texture->IsValid() ? texture->GetWidth() : 0;
	*height = texture && texture->IsValid() ? texture->GetHeight() : 0;
	*mipCount = texture && texture->IsValid() ? texture->GetMipCount() : 0;
	*hasData = probe->reflectionSource == STANDALONE_REFLECTION_SOURCE_DYNAMIC ?
		( reflectionProbe && reflectionProbe->HasData() ) :
		texture && texture->IsValid();
	*filterSucceeded = probe->reflectionSource != STANDALONE_REFLECTION_SOURCE_DYNAMIC ||
		( reflectionProbe && reflectionProbe->LastFilterSucceeded() );
	*reflectionIntensity = probe->scene->GetLightingSetup().reflectionIntensity;
	*shPrimaryIntensity = probe->shLightingManager ? probe->shLightingManager->GetPrimaryIntensity() : 0.0f;
	*shSecondaryIntensity = probe->shLightingManager ? probe->shLightingManager->GetSecondaryIntensity() : 0.0f;
	return true;
}

TRINITY_STANDALONE_EXPORT bool TrinityStandaloneProbeConfigurePostProcess(
	void* opaqueProbe,
	int dynamicExposureMode,
	bool diagnosticsEnabled )
{
	auto* probe = static_cast<StandaloneProbe*>( opaqueProbe );
	if( !probe || !probe->scene || !probe->driver ||
		( dynamicExposureMode != STANDALONE_DYNAMIC_EXPOSURE_OFF &&
		  dynamicExposureMode != STANDALONE_DYNAMIC_EXPOSURE_CLIENT ) )
	{
		return false;
	}
	Tr2PostProcess2Ptr postProcess = probe->clientPostProcess;
	if( !postProcess || !probe->clientTonemapping )
	{
		return false;
	}
	if( dynamicExposureMode == STANDALONE_DYNAMIC_EXPOSURE_CLIENT && !probe->clientDynamicExposure )
	{
		return false;
	}
	postProcess->SetDynamicExposure(
		dynamicExposureMode == STANDALONE_DYNAMIC_EXPOSURE_CLIENT ?
			probe->clientDynamicExposure :
			Tr2PPDynamicExposureEffectPtr{} );
	g_eveSpaceSceneGammaBrightness = 1.0f;
	probe->dynamicExposureMode = dynamicExposureMode;
	probe->postProcessDiagnosticsEnabled = diagnosticsEnabled;
	probe->driver->SetPostProcessDiagnosticsEnabled( diagnosticsEnabled );
	std::fprintf(
		stderr,
		"EVE RC-10 postprocess mode: dynamicExposure=%s diagnostics=%s outputGamma=1.000000\n",
		dynamicExposureMode == STANDALONE_DYNAMIC_EXPOSURE_CLIENT ? "client" : "off",
		diagnosticsEnabled ? "enabled" : "disabled" );
	return true;
}

TRINITY_STANDALONE_EXPORT bool TrinityStandaloneProbeSetExposureCameraPhase(
	void* opaqueProbe,
	bool sequenceActive,
	bool sunFacing )
{
	auto* probe = static_cast<StandaloneProbe*>( opaqueProbe );
	if( !probe || !probe->view || !probe->projection )
	{
		return false;
	}
	probe->exposureSequenceActive = sequenceActive;
	if( !sequenceActive )
	{
		return true;
	}
	const Vector3 eye( 0.0f, 0.0f, -5.2f * probe->modelWorldScale );
	const Vector3 target = sunFacing ? eye + probe->cinematicSunDirection : Vector3( 0.0f, 0.0f, 0.0f );
	probe->view->SetLookAtPosition( eye, target, Vector3( 0.0f, 1.0f, 0.0f ) );
	probe->projection->PerspectiveFov(
		( sunFacing ? 20.0f : 60.0f ) * 3.1415926535f / 180.0f,
		static_cast<float>( probe->renderWidth ) / std::max( 1.0f, static_cast<float>( probe->renderHeight ) ),
		1.0f,
		10000.0f );
	return true;
}

TRINITY_STANDALONE_EXPORT bool TrinityStandaloneProbeGetPostProcessDiagnostics(
	void* opaqueProbe,
	TrinityStandalonePostProcessDiagnostics* diagnostics )
{
	auto* probe = static_cast<StandaloneProbe*>( opaqueProbe );
	if( !probe || !probe->renderContext || !diagnostics || !probe->postProcessDiagnosticsEnabled ||
		!ReadPostProcessDiagnostics( *probe, *probe->renderContext ) )
	{
		return false;
	}
	*diagnostics = probe->postProcessDiagnostics;
	return true;
}

TRINITY_STANDALONE_EXPORT bool TrinityStandaloneProbeGetToneValidation(
	void* opaqueProbe,
	TrinityStandaloneToneValidation* validation )
{
	auto* probe = static_cast<StandaloneProbe*>( opaqueProbe );
	if( !probe || !validation || probe->toneValidation.width == 0 )
	{
		return false;
	}
	*validation = probe->toneValidation;
	return true;
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

TRINITY_STANDALONE_EXPORT bool TrinityStandaloneProbeCreateEveScene( void* opaqueProbe, int qualityRung, const char* assetPath, int materialView, int materialMode, int areaView, const char* sceneResourcePath, int sceneFixture, int lightingView, int shSource, int localLights, int localShadows, int reflectionSource, int reflectionCorrection, int normalMapMode, int cameraView, int composition, int planetLayers, int cloudYear, int cloudMonth, int cloudDay, int sunEffects, int attachments, int attachmentView, int decals, int decalView, uint32_t killCount, float modelYawDegrees, int shadows, int ambientOcclusion, int aoMethod )
{
	auto* probe = static_cast<StandaloneProbe*>( opaqueProbe );
	if( !probe )
	{
		return false;
	}
	return ConfigureDriverScene( *probe, qualityRung, assetPath, materialView, materialMode, areaView, sceneResourcePath, sceneFixture, lightingView, shSource, localLights, localShadows, reflectionSource, reflectionCorrection, normalMapMode, cameraView, composition, planetLayers, cloudYear, cloudMonth, cloudDay, sunEffects, attachments, attachmentView, decals, decalView, killCount, modelYawDegrees, shadows, ambientOcclusion, aoMethod );
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
		captureProduct != STANDALONE_CAPTURE_DEPTH && captureProduct != STANDALONE_CAPTURE_NORMAL &&
		captureProduct != STANDALONE_CAPTURE_SHADOW && captureProduct != STANDALONE_CAPTURE_SHADOW_ATLAS &&
		captureProduct != STANDALONE_CAPTURE_LOCAL_SHADOW_ATLAS &&
		captureProduct != STANDALONE_CAPTURE_AO && captureProduct != STANDALONE_CAPTURE_BENT_NORMAL &&
		captureProduct != STANDALONE_CAPTURE_REFLECTION &&
		captureProduct != STANDALONE_CAPTURE_HDR_COMPOSITE &&
		captureProduct != STANDALONE_CAPTURE_POST_TONEMAP &&
		captureProduct != STANDALONE_CAPTURE_TONE_CONTRACT )
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
	probe->lastRealTime = realTime;
	probe->lastSimTime = simTime;
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
	if( rendered && !freezeScene && probe->localShadows != STANDALONE_LOCAL_SHADOWS_OFF &&
		probe->renderedFrameCount >= 2 )
	{
		Tr2LightManager* manager = Tr2LightManager::GetInstance();
		if( !manager || !probe->scene || !probe->renderable )
		{
			CCP_LOGERR( "Dynamic-light shadow diagnostics are unavailable" );
			return false;
		}
		const Tr2LightManager::DynamicShadowDiagnostics& managerStats = manager->GetDynamicShadowDiagnostics();
		const EveSpaceScene::DynamicLightShadowDiagnostics& sceneStats =
			probe->scene->GetDynamicLightShadowDiagnostics();
		const uint32_t expectedLights = probe->localShadows == STANDALONE_LOCAL_SHADOWS_AUTHORED ? 6u : 1u;
		bool entriesValid = managerStats.entries.size() == expectedLights;
		for( size_t i = 0; i < managerStats.entries.size() && entriesValid; ++i )
		{
			const auto& entry = managerStats.entries[i];
			entriesValid = !entry.isSpotLight && entry.faceSize > 0 &&
				entry.offsetX + entry.width <= managerStats.atlasSettings.size &&
				entry.offsetY + entry.height <= managerStats.atlasSettings.size;
			for( size_t j = i + 1; j < managerStats.entries.size() && entriesValid; ++j )
			{
				const auto& other = managerStats.entries[j];
				const bool overlaps = entry.offsetX < other.offsetX + other.width &&
					entry.offsetX + entry.width > other.offsetX &&
					entry.offsetY < other.offsetY + other.height &&
					entry.offsetY + entry.height > other.offsetY;
				entriesValid = !overlaps;
			}
		}
		const uint32_t expectedHaze = probe->localShadows == STANDALONE_LOCAL_SHADOWS_AUTHORED ? 2u : 0u;
		const uint32_t expectedBanner = probe->localShadows == STANDALONE_LOCAL_SHADOWS_AUTHORED ? 4u : 0u;
		const uint32_t expectedValidation = probe->localShadows == STANDALONE_LOCAL_SHADOWS_VALIDATION ? 1u : 0u;
		if( !managerStats.enabled || !managerStats.allocationSucceeded ||
			managerStats.eligibleLightCount != expectedLights || managerStats.selectedLightCount != expectedLights ||
			managerStats.droppedLightCount != 0 || managerStats.atlasSettings.actualTextureSize != 16384 ||
			managerStats.atlasSettings.size != 16384 || !entriesValid || !sceneStats.atlasValid ||
			sceneStats.atlasFormat != Tr2RenderContextEnum::PIXEL_FORMAT_D32_FLOAT ||
			sceneStats.atlasWidth != 16384 || sceneStats.atlasHeight != 16384 ||
			sceneStats.selectedLights != expectedLights || sceneStats.pointLights != expectedLights ||
			sceneStats.spotLights != 0 || sceneStats.facePasses != expectedLights * 6 ||
			sceneStats.casterTests != sceneStats.facePasses || sceneStats.acceptedCasterPasses == 0 ||
			sceneStats.committedBatches != sceneStats.acceptedCasterPasses * 2 ||
			probe->renderable->GetHazeShadowEligibleCount() != expectedHaze ||
			probe->renderable->GetBannerShadowEligibleCount() != expectedBanner ||
			probe->renderable->GetValidationShadowEligibleCount() != expectedValidation ||
			!sceneStats.receiverMaskResolved ||
			sceneStats.receiverMaskFormat != Tr2RenderContextEnum::PIXEL_FORMAT_R16_UINT ||
			sceneStats.receiverMaskWidth != probe->renderWidth ||
			sceneStats.receiverMaskHeight != probe->renderHeight )
		{
			std::fprintf(
				stderr,
				"Astero local-shadow contract failed: manager(enabled=%u allocation=%u eligible=%u selected=%u dropped=%u packed=%zu) atlas(format=%u size=%ux%u valid=%u) receiver(format=%u size=%ux%u resolved=%u) faces=%u accepted=%u batches=%u\n",
				managerStats.enabled ? 1u : 0u,
				managerStats.allocationSucceeded ? 1u : 0u,
				managerStats.eligibleLightCount,
				managerStats.selectedLightCount,
				managerStats.droppedLightCount,
				managerStats.entries.size(),
				sceneStats.atlasFormat,
				sceneStats.atlasWidth,
				sceneStats.atlasHeight,
				sceneStats.atlasValid ? 1u : 0u,
				sceneStats.receiverMaskFormat,
				sceneStats.receiverMaskWidth,
				sceneStats.receiverMaskHeight,
				sceneStats.receiverMaskResolved ? 1u : 0u,
				sceneStats.facePasses,
				sceneStats.acceptedCasterPasses,
				sceneStats.committedBatches );
			CCP_LOGERR(
				"Astero local-shadow contract failed: eligible=%u selected=%u dropped=%u packed=%zu atlas=%ux%u valid=%u point=%u spot=%u faces=%u tests=%u accepted=%u batches=%u haze=%u banner=%u validation=%u receiver=%ux%u format=%u resolved=%u",
				managerStats.eligibleLightCount,
				managerStats.selectedLightCount,
				managerStats.droppedLightCount,
				managerStats.entries.size(),
				sceneStats.atlasWidth,
				sceneStats.atlasHeight,
				sceneStats.atlasValid ? 1u : 0u,
				sceneStats.pointLights,
				sceneStats.spotLights,
				sceneStats.facePasses,
				sceneStats.casterTests,
				sceneStats.acceptedCasterPasses,
				sceneStats.committedBatches,
				probe->renderable->GetHazeShadowEligibleCount(),
				probe->renderable->GetBannerShadowEligibleCount(),
				probe->renderable->GetValidationShadowEligibleCount(),
				sceneStats.receiverMaskWidth,
				sceneStats.receiverMaskHeight,
				sceneStats.receiverMaskFormat,
				sceneStats.receiverMaskResolved ? 1u : 0u );
			return false;
		}
		if( !probe->reportedLocalShadowStats )
		{
			std::fprintf(
				stderr,
				"Astero local-shadow result: mode=%s eligibility=%s eligible=%u selected=%u point=%u spot=%u faces=%u accepted=%u batches=%u atlas=D32 %ux%u entries=%zu receiver=R16_UINT %ux%u\n",
				probe->localShadows == STANDALONE_LOCAL_SHADOWS_AUTHORED ? "authored" : "validation",
				probe->localShadows == STANDALONE_LOCAL_SHADOWS_AUTHORED ? "probe-all-active" : "probe-validation",
				managerStats.eligibleLightCount,
				managerStats.selectedLightCount,
				sceneStats.pointLights,
				sceneStats.spotLights,
				sceneStats.facePasses,
				sceneStats.acceptedCasterPasses,
				sceneStats.committedBatches,
				sceneStats.atlasWidth,
				sceneStats.atlasHeight,
				managerStats.entries.size(),
				sceneStats.receiverMaskWidth,
				sceneStats.receiverMaskHeight );
			for( const auto& entry : managerStats.entries )
			{
				std::fprintf(
					stderr,
					"  local-shadow entry: light=%u type=%s offset=(%u,%u) face=%u block=%ux%u\n",
					entry.lightIndex,
					entry.isSpotLight ? "spot" : "point",
					entry.offsetX,
					entry.offsetY,
					entry.faceSize,
					entry.width,
					entry.height );
			}
			probe->reportedLocalShadowStats = true;
		}
	}
	if( rendered && !freezeScene && probe->shadows != STANDALONE_SHADOWS_OFF && probe->scene )
	{
		const auto* diagnostics = probe->renderable ?
			probe->scene->FindDirectionalShadowCasterDiagnostics(
				static_cast<const IEveShadowCaster*>( probe->renderable.p ) ) :
			nullptr;
		if( !diagnostics )
		{
			CCP_LOGERR( "Astero directional shadow diagnostics are unavailable" );
			return false;
		}
		const uint32_t cullTests = diagnostics->tests;
		const uint32_t acceptedCascades = diagnostics->acceptedCascades;
		const uint32_t committedBatches = diagnostics->committedBatches;
		if( acceptedCascades == 0 || committedBatches != acceptedCascades * 2 )
		{
			CCP_LOGERR(
				"Astero directional shadow contract failed: tests=%u accepted=%u batches=%u expected=%u",
				cullTests,
				acceptedCascades,
				committedBatches,
				acceptedCascades * 2 );
			return false;
		}
		if( !probe->reportedShadowStats )
		{
			std::fprintf(
				stderr,
				"Astero directional shadow result: caster=1 tests=%u accepted-cascades=%u batches=%u atlas=16384x4096\n",
				cullTests,
				acceptedCascades,
				committedBatches );
			probe->reportedShadowStats = true;
		}
	}
	return rendered && ( !probe->renderable || !probe->renderable->DrawFailed() );
}
