// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "EveSpaceSceneRenderDriver.h"
#include "Tr2VolumetricsRenderer.h"
#include "EveSpaceScene.h"
#include "../Tr2Renderer.h"
#include "../TriProjection.h"
#include "../TriView.h"
#include "../Shader/Tr2Effect.h"
#include "../RenderJob/TriStepRenderFps.h"
#include "../Tr2SSAO.h"
#include "Eve/EveCamera.h"
#include "../Particle/Tr2GpuParticleSystem.h"
#include "../Tr2TextureReference.h"
#include "Resources/TriTextureRes.h"


CCP_STATS_DECLARE( updateDynamicLightLists, "Trinity/EveSpaceScene/updateDynamicLights", true, CST_TIME, "Time took to gather dynamic lights for EveSpaceScene" );

namespace
{

float HalfToFloat( uint16_t value )
{
	const uint32_t sign = static_cast<uint32_t>( value & 0x8000u ) << 16;
	int32_t exponent = ( value >> 10 ) & 0x1fu;
	uint32_t mantissa = value & 0x03ffu;
	uint32_t result = 0;
	if( exponent == 0 )
	{
		if( mantissa != 0 )
		{
			exponent = 1;
			while( ( mantissa & 0x0400u ) == 0 )
			{
				mantissa <<= 1;
				--exponent;
			}
			mantissa &= 0x03ffu;
			result = sign | ( static_cast<uint32_t>( exponent + 112 ) << 23 ) | ( mantissa << 13 );
		}
		else
		{
			result = sign;
		}
	}
	else if( exponent == 31 )
	{
		result = sign | 0x7f800000u | ( mantissa << 13 );
	}
	else
	{
		result = sign | ( static_cast<uint32_t>( exponent + 112 ) << 23 ) | ( mantissa << 13 );
	}
	float converted = 0.0f;
	std::memcpy( &converted, &result, sizeof( converted ) );
	return converted;
}

bool EnsureReadbackTexture(
	Tr2TextureAL& texture,
	const Tr2TextureAL& source,
	const char* name,
	Tr2RenderContext& renderContext )
{
	if( texture.IsValid() && texture.GetWidth() == source.GetWidth() &&
		texture.GetHeight() == source.GetHeight() && texture.GetFormat() == source.GetFormat() )
	{
		return true;
	}
	texture = Tr2TextureAL{};
	if( FAILED( texture.Create(
			Tr2BitmapDimensions( source.GetWidth(), source.GetHeight(), 1, source.GetFormat() ),
			Tr2GpuUsage::COPY_DESTINATION,
			Tr2CpuUsage::READ,
			renderContext.GetPrimaryRenderContext() ) ) )
	{
		return false;
	}
	texture.SetName( name );
	return true;
}

void BeginRenderPass( Tr2RenderContext& renderContext, const std::initializer_list<Tr2TextureAL>& colorAttachments, const Tr2TextureAL& depthAttachment )
{
	uint32_t index = 0;
	for( auto& colorAttachment : colorAttachments )
	{
		renderContext.m_esm.SetRenderTarget( index++, colorAttachment );
	}
	// clear out remaining slots, generously assuming max 4 render targets
	const uint32_t maxRenderTargets = 4;
	for( ; index < maxRenderTargets; ++index )
	{
		renderContext.m_esm.SetRenderTarget( index++, Tr2TextureAL{} );
	}
	renderContext.m_esm.SetDepthStencilBuffer( depthAttachment );
	renderContext.m_esm.SetFullScreenViewport();
}

Tr2GpuResourcePool::Texture GetEmptySSAO( Tr2GpuResourcePool& gpuResourcePool )
{
	const uint8_t white[] = { 255, 255, 255, 255 };
	Tr2SubresourceData initData = { white, 4, 4 };

	return gpuResourcePool.GetPersistentTexture( "EmptySSAO", 1, 1, ImageIO::PIXEL_FORMAT_R8G8B8A8_UNORM, Tr2GpuUsage::SHADER_RESOURCE, &initData );
}

void DeleteUpscalingContext( Tr2UpscalingContextAL* context )
{
	if( context )
	{
		USE_MAIN_THREAD_RENDER_CONTEXT();
		context->SetHudLessTexture( nullptr );
		renderContext.DeleteUpscalingContext( context->GetID() );
	}
}

bool ApplyDistortion(
	const Tr2TextureAL& destination,
	const Tr2TextureAL& distortion,
	Tr2GpuResourcePool& gpuResourcePool,
	const Tr2EffectPtr& effect,
	Tr2RenderContext& renderContext,
	bool& copySucceeded )
{
	auto backBufferCopy = gpuResourcePool.GetTempTexture( "backBufferCopy", destination.GetWidth(), destination.GetHeight(), destination.GetFormat(), Tr2GpuUsage::COPY_DESTINATION | Tr2GpuUsage::SHADER_RESOURCE );
	copySucceeded = SUCCEEDED( backBufferCopy->CopySubresourceRegion( {}, destination, {}, renderContext ) );
	if( !copySucceeded || !effect || !effect->GetShaderStateInterface() )
	{
		return false;
	}

	BeginRenderPass( renderContext, { destination }, {} );
	renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_FULLSCREEN );
	effect->SetParameter( BlueSharedString( "BlitCurrent" ), backBufferCopy );
	effect->SetParameter( BlueSharedString( "TexDistortion" ), distortion );
	const bool compositeSucceeded = Tr2Renderer::DrawFullScreenWithShader( renderContext, effect );
	effect->SetParameter( BlueSharedString( "BlitCurrent" ), Tr2TextureAL{} );
	effect->SetParameter( BlueSharedString( "TexDistortion" ), Tr2TextureAL{} );
	return compositeSucceeded;
}

struct TimeSection
{
	TimeSection( Tr2ProfileTimer& timer, const char* name, const Tr2ProfileTimer& rootTimer, Tr2RenderContext& renderContext ) :
		m_timer( timer )
	{
		if( rootTimer.GetCaptureCpuTime() || rootTimer.GetCaptureGpuTime() )
		{
			m_timer.SetStatName( ( rootTimer.GetStatName() + std::string( "/" ) + name ).c_str() );
			m_timer.SetCaptureCpuTime( rootTimer.GetCaptureCpuTime() );
			m_timer.SetCaptureGpuTime( rootTimer.GetCaptureGpuTime() );
			m_timer.Begin( renderContext );
			m_renderContext = &renderContext;
		}
		else
		{
			m_timer.SetCaptureCpuTime( false );
			m_timer.SetCaptureGpuTime( false );
		}
	}

	TimeSection( const TimeSection& ) = delete;
	TimeSection& operator=( const TimeSection& ) = delete;

	void Stop()
	{
		if( m_renderContext )
		{
			m_timer.End( *m_renderContext );
			m_renderContext = nullptr;
		}
	}

	~TimeSection()
	{
		if( m_renderContext )
		{
			m_timer.End( *m_renderContext );
		}
	}

	Tr2ProfileTimer& m_timer;
	Tr2RenderContext* m_renderContext = nullptr;
};

void SetNamedOutput( const ITr2RenderNode::Span<ITr2RenderNode::TempOutput>& outputs, const char* name, const Tr2GpuResourcePool::Texture& texture )
{
	auto sname = BlueSharedString( name );
	for( auto& output : outputs )
	{
		if( output.name == sname )
		{
			output.texture = texture;
			return;
		}
	}
}

ITr2RenderNode::TempOutput* FindNamedOutput( const ITr2RenderNode::Span<ITr2RenderNode::TempOutput>& outputs, const char* name )
{
	auto sname = BlueSharedString( name );
	auto found = std::find_if( outputs.begin(), outputs.end(), [&]( const auto& output ) { return output.name == sname; } );
	if( found != outputs.end() )
	{
		return found;
	}
	return nullptr;
}

}


EveSpaceSceneRenderDriver::EveSpaceSceneRenderDriver( IRoot* lockobj ) :
	PARENTLOCK( m_toolsScenes ),
	m_gpuResourcePool( &GetGlobalGpuResourcePool() ),
	m_reflectionCorrectionEnabled( true )
{
	m_distortionEffect.CreateInstance();
	m_distortionEffect->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/PostProcess/Distortion.fx" );

	m_postProcess.CreateInstance();
	m_fpsRenderer.CreateInstance();


	//We have a few different resolution lookup tables to choose from. Use the highest quality one for now.
	//BeResMan->GetResource( "res:/texture/reflectioncorrection/32x32.dds", "", m_reflectionCorrectionMap );
	BeResMan->GetResource( "res:/texture/reflectioncorrection/128x128.dds", "", m_reflectionCorrectionMap );

	BeResMan->GetResource( "res:/texture/global/black.dds", "", m_blackReflectionCorrectionMap );
}

EveSpaceSceneRenderDriver::~EveSpaceSceneRenderDriver()
{
	DeleteUpscalingContext( m_upscalingContext );
}

void EveSpaceSceneRenderDriver::ReleaseResources( TriStorage s )
{
	if( s == TriStorageFlags::TRISTORAGE_ALL )
	{
		DeleteUpscalingContext( m_upscalingContext );
		m_upscalingContext = nullptr;
		m_preLensFlareReadback = Tr2TextureAL{};
		m_postLensFlareReadback = Tr2TextureAL{};
		m_lastLensFlareDiagnostics = {};
	}
}

bool EveSpaceSceneRenderDriver::OnPrepareResources()
{
	return true;
}

void EveSpaceSceneRenderDriver::SetupUpscaling( const TextureSize2D& displaySize )
{
	USE_MAIN_THREAD_RENDER_CONTEXT();

	if( m_settings.enableUpscaling )
	{
		uint32_t width = 0;
		uint32_t height = 0;
		if( m_upscalingContext )
		{
			m_upscalingContext->GetDisplayDimensions( width, height );
		}
		if( width != displaySize.width || height != displaySize.height )
		{
			Tr2UpscalingAL::UpscalingContextParams params = Tr2UpscalingAL::UpscalingContextParams( renderContext );
			params.allowFramegen = true;
			params.displayWidth = displaySize.width;
			params.displayHeight = displaySize.height;
			params.sourceFormat = m_internalPixelFormat;
			params.depthFormat = Tr2RenderContextEnum::DSFMT_D32F;

			m_upscalingContext = renderContext.CreateUpscalingContext( params, m_upscalingContext ? m_upscalingContext->GetID() : Tr2UpscalingAL::INVALID_CONTEXT_ID );
		}
	}
	else if( m_upscalingContext )
	{
		DeleteUpscalingContext( m_upscalingContext );
		m_upscalingContext = nullptr;
	}
}

void EveSpaceSceneRenderDriver::PropagateSettings()
{
	// AA setting
	if( m_scene->m_sceneDefaultPostProcess )
	{
		if( m_settings.antiAliasingQuality == AntiAliasingQuality::Disabled )
		{
			m_scene->m_sceneDefaultPostProcess->SetTaa( nullptr );
		}
		else
		{
			if( !m_scene->m_sceneDefaultPostProcess->GetTaaIfAvailable() )
			{
				Tr2PPTaaEffectPtr taa;
				taa.CreateInstance();
				m_scene->m_sceneDefaultPostProcess->SetTaa( taa );
			}
			m_scene->m_sceneDefaultPostProcess->GetTaaIfAvailable()->m_quality = (Tr2PPTaaEffect::Quality)m_settings.antiAliasingQuality;
			m_scene->m_sceneDefaultPostProcess->GetTaaIfAvailable()->SetDebugMode( m_settings.taaDebugMode );
		}
	}

	// Shadow quality setting
	if( m_settings.shadowQuality == ShadowQuality::SHADOW_DISABLED || !m_settings.enableDirectionalShadows )
	{
		m_scene->m_cascadedShadowMap = nullptr;
		m_scene->m_rtManager = nullptr;
	}
	else if( m_settings.shadowQuality == ShadowQuality::SHADOW_RAYTRACED )
	{
		m_scene->m_cascadedShadowMap = nullptr;
		if( !m_scene->m_rtManager )
		{
			m_scene->m_rtManager.CreateInstance();
		}
	}
	else
	{
		m_scene->m_rtManager = nullptr;
		if( !m_scene->m_cascadedShadowMap )
		{
			m_scene->m_cascadedShadowMap.CreateInstance();
		}
		if( m_scene->m_cascadedShadowMap )
		{
			m_scene->m_cascadedShadowMap->ShouldUseDenoiser( m_settings.shadowQuality == ShadowQuality::SHADOW_HIGH );
		}
	}
	m_scene->m_shadowQuality = m_settings.shadowQuality;

	// AO quality setting
	if( m_ssao )
	{
		if( m_settings.aoQuality == AmbientOcclusionQuality::Disabled )
		{
			m_ssao->Enable( false );
		}
		else
		{
			m_ssao->Enable( true );

			switch( m_settings.aoQuality )
			{
			case AmbientOcclusionQuality::Low:
				m_ssao->SetQuality( SSAOQuality::LOW, true );
				break;
			case AmbientOcclusionQuality::Medium:
				m_ssao->SetQuality( SSAOQuality::MEDIUM, false );
				break;
			default:
				m_ssao->SetQuality( SSAOQuality::HIGHEST, false );
				break;
			}
		}
	}

	if( m_settings.visualizeMethod != EveSpaceScene::VM_NONE )
	{
		m_postProcess->SetPostProcessingQuality( PostProcess::LOW );
	}
	else
	{
		m_postProcess->SetPostProcessingQuality( m_settings.postProcessingQuality );
	}

	if( m_scene->m_volumetricsRenderer )
	{
		m_scene->m_volumetricsRenderer->SetQuality( m_settings.volumetricQuality );
	}
}


bool EveSpaceSceneRenderDriver::Validate( const Span<const Tr2BitmapDimensions>& destDimensions, const Span<const BlueSharedString>& outputs, Be::Time realTime, Be::Time simTime )
{
	if( destDimensions.size == 0 )
	{
		CCP_ASSERT_M( false, "EveSpaceSceneRenderDriver requires at least one destination texture" );
		return false;
	}
	if( !m_enableRendering )
	{
		return true;
	}
	if( !m_scene )
	{
		CCP_LOG( "EveSpaceSceneRenderDriver::Validate failed because the render driver does not have a valid scene" );
		return false;
	}
	bool hasCamera = m_camera || ( m_view && m_projection );
	if( !hasCamera )
	{
		CCP_LOG( "EveSpaceSceneRenderDriver::Validate failed because the render driver does not have a valid camera" );
		return false;
	}
	const TextureSize2D displaySize = destDimensions[0];

	SetupUpscaling( displaySize );
	const auto renderSize = GetRenderSize( displaySize );
	if( renderSize != m_temporalRenderSize )
	{
		m_temporalRenderSize = renderSize;
		m_temporalResetPending = true;
	}

	if( m_background )
	{
		auto bgDimensions = Tr2BitmapDimensions( renderSize.width, renderSize.height, 1, m_internalPixelFormat );
		if( !m_background->Validate( { &bgDimensions, 1 }, {}, realTime, simTime ) )
		{
			return false;
		}
	}
	if( m_sceneOverlay )
	{
		Tr2BitmapDimensions overlayDimensions[] = {
			Tr2BitmapDimensions( renderSize.width, renderSize.height, 1, m_internalPixelFormat ),
			Tr2BitmapDimensions( renderSize.width, renderSize.height, 1, ImageIO::PIXEL_FORMAT_R16G16_FLOAT )
		};
		if( !m_sceneOverlay->Validate( { overlayDimensions, 2 }, {}, realTime, simTime ) )
		{
			return false;
		}
	}

	for( auto& output : outputs )
	{
		if( strcmp( output.c_str(), "DepthMap" ) != 0 &&
			strcmp( output.c_str(), "VelocityMap" ) != 0 &&
			strcmp( output.c_str(), "NormalMap" ) != 0 &&
			strcmp( output.c_str(), "CustomStencilMap" ) != 0 &&
			strcmp( output.c_str(), "ShadowMap" ) != 0 &&
			strcmp( output.c_str(), "CascadedShadowDepth" ) != 0 &&
			strcmp( output.c_str(), "DynamicLightShadowDepth" ) != 0 &&
			strcmp( output.c_str(), "SSAOMap" ) != 0 &&
			strcmp( output.c_str(), "OpaqueColorMap" ) != 0 &&
			strcmp( output.c_str(), "PreTaaColor" ) != 0 &&
			strcmp( output.c_str(), "PostTaaColor" ) != 0 &&
			strcmp( output.c_str(), "TaaCooldownMap" ) != 0 &&
			strcmp( output.c_str(), "PreTonemapColor" ) != 0 &&
			strcmp( output.c_str(), "BloomMap" ) != 0 &&
			strcmp( output.c_str(), "PostTonemapColor" ) != 0 &&
			strcmp( output.c_str(), "FinalPostProcessColor" ) != 0 &&
			strcmp( output.c_str(), "DistortionMap" ) != 0 &&
			strcmp( output.c_str(), "VolumetricSlices" ) != 0 &&
			strcmp( output.c_str(), "FroxelFog" ) != 0 &&
			strcmp( output.c_str(), "MieEnvironmentMap" ) != 0 )
		{
			CCP_LOGERR( "EveSpaceSceneRenderDriver does not support the output '%s'", output.c_str() );
			return false;
		}
	}
	return true;
}

Tr2GpuResourcePool::Texture EveSpaceSceneRenderDriver::RenderSSAO( const Tr2TextureAL& depthMap, const Tr2TextureAL& normalMap, Tr2RenderContext& renderContext )
{
	Tr2GpuResourcePool::Texture ssao;
	if( m_scene->m_display && m_ssao )
	{
		renderContext.SetReadOnlyDepth( true );

		auto upscalingInfo = renderContext.GetPrimaryRenderContext().GetUpscalingInfo( m_upscalingContext ? m_upscalingContext->GetID() : Tr2UpscalingAL::INVALID_CONTEXT_ID );
		auto scenePostProcess = m_scene->m_sceneDefaultPostProcess;
		bool temporal = upscalingInfo.temporal || ( scenePostProcess != nullptr && scenePostProcess->GetTaaIfAvailable() != nullptr );

		ssao = m_ssao->Filter( depthMap, normalMap, m_gpuResourcePool, renderContext, temporal );
		renderContext.SetReadOnlyDepth( false );
	}
	if( ssao.IsValid() )
	{
		return ssao;
	}
	else
	{
		return GetEmptySSAO( m_gpuResourcePool );
	}
}

void EveSpaceSceneRenderDriver::SetCameraToRenderer( Tr2RenderContext& renderContext ) const
{
	auto projection = m_camera ? m_camera->GetProjection() : m_projection;
	auto view = m_camera ? m_camera->GetViewMatrix() : m_view;

	projection->SetProjection( renderContext );
	Tr2Renderer::SetViewTransform( view->GetTransform() );
}

TextureSize2D EveSpaceSceneRenderDriver::GetRenderSize( const TextureSize2D& displaySize ) const
{
	if( m_upscalingContext )
	{
		TextureSize2D result;
		m_upscalingContext->GetRenderDimensions( result.width, result.height );
		return result;
	}
	return displaySize;
}

void EveSpaceSceneRenderDriver::Execute( const Span<const Tr2TextureAL>& destinations, const Span<TempOutput>& outputs, Be::Time realTime, Be::Time simTime, const Tr2ProfileTimer& rootTimer, Tr2RenderContext& renderContext )
{
	m_lastDistortionDiagnostics = {};
	m_lastDistortionDiagnostics.enabled = m_settings.enableDistortion;
	const bool hasCamera = m_camera || ( m_view && m_projection );

	if( !m_enableRendering )
	{
		if( !m_scene || !hasCamera )
		{
			return;
		}

		SetCameraToRenderer( renderContext );
		m_scene->Update( realTime, simTime );
		UpdateGpuParticleSystem( renderContext );
		return;
	}

	if( !m_scene || !hasCamera )
	{
		CCP_LOGWARN( "EveSpaceSceneRenderDriver::Execute called without a valid scene or a camera" );
		return;
	}


	GlobalStore().RegisterVariable( "EveSpaceSceneReflectionCorrectionLookupTable", m_reflectionCorrectionEnabled ? m_reflectionCorrectionMap : m_blackReflectionCorrectionMap );


	m_scene->m_viewLast = m_viewLast;
	m_scene->m_projectionLast = m_projectionLast;

	if( m_camera )
	{
		m_camera->Update( simTime );
	}

	const TextureSize2D displaySize = destinations.data->GetDesc();
	const TextureSize2D renderSize = GetRenderSize( displaySize );

	PropagateSettings();

	GlobalStore().RegisterVariable( "SSAOMap", GetEmptySSAO( m_gpuResourcePool ) );

	renderContext.m_esm.SetInvertedDepthTest( true );
	ON_BLOCK_EXIT( [&] { renderContext.m_esm.SetInvertedDepthTest( false ); } );


	renderContext.m_esm.PushRenderTarget( 0 );
	renderContext.m_esm.PushRenderTarget( 1 );
	renderContext.m_esm.PushDepthStencilBuffer();
	ON_BLOCK_EXIT( [&] {
		renderContext.m_esm.PopDepthStencilBuffer();
		renderContext.m_esm.PopRenderTarget( 1 );
		renderContext.m_esm.PopRenderTarget( 0 );
	} );

	Tr2Renderer::SetUpscalingContextID( m_upscalingContext ? m_upscalingContext->GetID() : Tr2UpscalingAL::INVALID_CONTEXT_ID );

	auto customBackBuffer = m_gpuResourcePool.GetTempTexture( "customBackBuffer", renderSize, m_internalPixelFormat, Tr2GpuUsage::RENDER_TARGET | Tr2GpuUsage::SHADER_RESOURCE );
	const Tr2TextureAL& sceneColorTarget = m_settings.bypassPostProcessing ? *destinations.data : customBackBuffer.Get();

	if( m_background )
	{
		BeginRenderPass( renderContext, { sceneColorTarget }, {} );
		renderContext.Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, m_settings.clearColor, 0 );

		m_background->Execute( { &sceneColorTarget, 1 }, {}, realTime, simTime, rootTimer, renderContext );
	}

	auto depthBuffer = m_gpuResourcePool.GetTempTexture( "depthBuffer", renderSize, ImageIO::PIXEL_FORMAT_D32_FLOAT, Tr2GpuUsage::DEPTH_STENCIL | Tr2GpuUsage::SHADER_RESOURCE );

	BeginRenderPass( renderContext, { sceneColorTarget }, depthBuffer );
	renderContext.Clear( m_background ? Tr2RenderContextEnum::CLEARFLAGS_ZBUFFER : ( Tr2RenderContextEnum::CLEARFLAGS_TARGET | Tr2RenderContextEnum::CLEARFLAGS_ZBUFFER ), m_settings.clearColor, 0 );

	SetCameraToRenderer( renderContext );
	if( m_temporalResetPending )
	{
		m_viewLast = Tr2Renderer::GetViewTransform();
		m_projectionLast = Tr2Renderer::GetProjectionTransform();
		m_scene->ResetTemporalHistory( m_viewLast, m_projectionLast );
		if( m_postProcess )
			m_postProcess->ResetTaaHistory();
		m_temporalResetPending = false;
	}
	{
		TimeSection updateSection( m_timers.update, "Update", rootTimer, renderContext );
		m_scene->Update( realTime, simTime );
	}
	UpdateGpuParticleSystem( renderContext );

	auto prevVisualizeMethod = m_scene->m_visualizeMethod;
	m_scene->m_visualizeMethod = m_settings.visualizeMethod;
	ON_BLOCK_EXIT( [&] { m_scene->m_visualizeMethod = prevVisualizeMethod; } );

	{
		TimeSection beginRenderSection( m_timers.beginRender, "BeginRender", rootTimer, renderContext );
		m_scene->BeginRender( m_settings.enableDistortion, renderContext );
	}
	m_temporalFrameSnapshot.currentView = Tr2Renderer::GetViewTransform();
	m_temporalFrameSnapshot.currentProjection = Tr2Renderer::GetReversedDepthProjectionTransform();
	m_temporalFrameSnapshot.currentViewProjection =
		m_temporalFrameSnapshot.currentView * m_temporalFrameSnapshot.currentProjection;
	m_temporalFrameSnapshot.previousView = m_scene->GetPreviousView();
	m_temporalFrameSnapshot.previousProjection = m_scene->GetPreviousJitteredProjection();
	m_temporalFrameSnapshot.previousViewProjection =
		m_temporalFrameSnapshot.previousView * m_temporalFrameSnapshot.previousProjection;
	m_temporalFrameSnapshot.jitter = m_scene->GetJitter();
	m_temporalFrameSnapshot.valid = true;
	{
		TimeSection reflectionsSection( m_timers.reflections, "RenderReflections", rootTimer, renderContext );
		m_scene->RenderReflectionPass( m_gpuResourcePool, renderContext );
	}
	GlobalStore().RegisterVariable( "DepthMap", depthBuffer );

	SetNamedOutput( outputs, "DepthMap", depthBuffer );

	Tr2GpuResourcePool::Texture distortionMap = GetDistortionMapIfNeeded( renderSize );
	if( distortionMap.IsValid() )
	{
		m_lastDistortionDiagnostics.mapCreated = true;
		m_lastDistortionDiagnostics.mapWidth = distortionMap->GetWidth();
		m_lastDistortionDiagnostics.mapHeight = distortionMap->GetHeight();
		m_lastDistortionDiagnostics.mapFormat = static_cast<uint32_t>( distortionMap->GetFormat() );
	}
	Tr2GpuResourcePool::Texture velocityMap = GetVelocityMapIfNeeded( renderSize, outputs );

	bool hasBackgroundDistortionBatches = false;
	{
		TimeSection backgroundSection( m_timers.background, "BackgroundPass", rootTimer, renderContext );
		hasBackgroundDistortionBatches = m_scene->RenderBackgroundPass( depthBuffer, distortionMap, velocityMap, renderContext );
	}
	if( m_settings.enableDistortion && hasBackgroundDistortionBatches )
	{
		m_lastDistortionDiagnostics.backgroundBatches = true;
		bool copySucceeded = false;
		m_lastDistortionDiagnostics.compositeSucceeded =
			ApplyDistortion( customBackBuffer, distortionMap, m_gpuResourcePool, m_distortionEffect, renderContext, copySucceeded );
		m_lastDistortionDiagnostics.copySucceeded = copySucceeded;
		m_lastDistortionDiagnostics.backgroundApplications = 1;
		SetNamedOutput( outputs, "DistortionMap", distortionMap );

		renderContext.m_esm.SetDepthStencilBuffer( depthBuffer );
	}
	distortionMap = {};

	Tr2GpuResourcePool::Texture normalMap = GetNormalMapIfNeeded( renderSize, outputs );
	Tr2GpuResourcePool::Texture customStencil = GetCustomStencilMapIfNeeded( renderSize, outputs );
	{
		TimeSection depthPassSection( m_timers.depthPass, "DepthPass", rootTimer, renderContext );
		m_scene->RenderDepthPass( depthBuffer, normalMap, customStencil, renderContext, m_depthPassTechnique );
	}
	GlobalStore().RegisterVariable( "SpaceSceneNormalMap", normalMap );
	GlobalStore().RegisterVariable( "SpaceSceneCustomStencil", customStencil );
	ON_BLOCK_EXIT( [&] { GlobalStore().RegisterVariable( "SpaceSceneCustomStencil", Tr2TextureAL{} ); } );

	Tr2GpuResourcePool::Texture opaqueBackBuffer;
	Tr2GpuResourcePool::Texture volumetricSlices;
	EveSpaceScene::ShadowResources shadowResources;
	if( m_scene->m_display && m_mainPassRenderingEnabled )
	{
		{
			TimeSection shadowsSection( m_timers.shadows, "Shadows", rootTimer, renderContext );
			shadowResources = m_scene->RenderShadows( depthBuffer, normalMap, m_gpuResourcePool, renderContext );
		}
		SetNamedOutput( outputs, "ShadowMap", shadowResources.shadowMap );
		SetNamedOutput( outputs, "CascadedShadowDepth", shadowResources.cascadedShadowDepth );
		SetNamedOutput( outputs, "DynamicLightShadowDepth", shadowResources.pointLightShadowDepth );
		TimeSection ssaoSection( m_timers.ssao, "SSAO", rootTimer, renderContext );
		auto ssao = RenderSSAO( depthBuffer, normalMap, renderContext );
		ssaoSection.Stop();
		SetNamedOutput( outputs, "SSAOMap", ssao );

		GlobalStore().RegisterVariable( "SSAOMap", ssao );

		if( auto lightManager = Tr2LightManager::GetInstance() )
		{
			GPU_REGION( renderContext, "Lighting" );
			CCP_STATS_SCOPED_TIME( updateDynamicLightLists );
			renderContext.SetReadOnlyDepth( true );
			lightManager->UpdateLists( depthBuffer, renderContext );
			renderContext.SetReadOnlyDepth( false );
		}

		opaqueBackBuffer = GetOpaqueColorMapIfNeeded( renderSize, outputs );

		RegisterWithVariableStore( shadowResources, m_gpuResourcePool );
		bool hasDistortionBatches = false;
		Tr2GpuResourcePool::Texture froxelFog;
		distortionMap = GetDistortionMapIfNeeded( renderSize );
		{
			TimeSection mainPassSection( m_timers.mainPass, "MainPass", rootTimer, renderContext );
			hasDistortionBatches = m_scene->RenderMainPass(
				sceneColorTarget,
				depthBuffer,
				distortionMap,
				velocityMap,
				opaqueBackBuffer,
				m_gpuResourcePool,
				renderContext,
				&froxelFog,
				&volumetricSlices );
		}
		SetNamedOutput( outputs, "FroxelFog", froxelFog );
		if( Tr2VolumetricsRendererPtr volumetrics = m_scene->GetVolumetricsRenderer() )
		{
			volumetrics->SetLocalOutputCopySucceeded( false );
			auto volumeOutput = FindNamedOutput( outputs, "VolumetricSlices" );
			if( volumeOutput && volumetricSlices.IsValid() )
			{
				volumeOutput->texture = m_gpuResourcePool.GetTempTexture(
					"VolumetricSlicesOutput",
					volumetricSlices->GetDesc(),
					Tr2GpuUsage::COPY_DESTINATION | Tr2GpuUsage::SHADER_RESOURCE );
				const bool copied = volumeOutput->texture.IsValid() &&
					SUCCEEDED( volumetricSlices->Resolve( volumeOutput->texture, renderContext ) );
				volumetrics->SetLocalOutputCopySucceeded( copied );
				if( !copied )
				{
					volumeOutput->texture = {};
				}
			}
		}
		if( Tr2VolumetricsRendererPtr volumetrics = m_scene->GetVolumetricsRenderer() )
		{
			const Tr2TextureAL* mie = volumetrics->GetMieEnvironmentMap();
			if( mie && mie->IsValid() )
			{
				auto mieOutput = FindNamedOutput( outputs, "MieEnvironmentMap" );
				if( mieOutput )
				{
					mieOutput->texture = m_gpuResourcePool.GetTempTexture( "MieEnvironmentMapOutput", mie->GetDesc(), Tr2GpuUsage::SHADER_RESOURCE );
					if( FAILED( mie->Resolve( mieOutput->texture, renderContext ) ) )
					{
						mieOutput->texture = {};
					}
				}
			}
		}
		RegisterWithVariableStore( {}, m_gpuResourcePool );

		if( m_settings.enableDistortion && hasDistortionBatches )
		{
			m_lastDistortionDiagnostics.foregroundBatches = true;
			bool copySucceeded = false;
			const bool compositeSucceeded =
				ApplyDistortion( customBackBuffer, distortionMap, m_gpuResourcePool, m_distortionEffect, renderContext, copySucceeded );
			m_lastDistortionDiagnostics.copySucceeded =
				m_lastDistortionDiagnostics.backgroundApplications == 0 ? copySucceeded :
																		  m_lastDistortionDiagnostics.copySucceeded && copySucceeded;
			m_lastDistortionDiagnostics.compositeSucceeded =
				m_lastDistortionDiagnostics.backgroundApplications == 0 ? compositeSucceeded :
																		  m_lastDistortionDiagnostics.compositeSucceeded && compositeSucceeded;
			m_lastDistortionDiagnostics.foregroundApplications = 1;
			SetNamedOutput( outputs, "DistortionMap", distortionMap );
			renderContext.m_esm.SetDepthStencilBuffer( depthBuffer );
		}
		distortionMap = {};
		GlobalStore().RegisterVariable( "SSAOMap", Tr2TextureAL{} );
	}
	else
	{
		// Even if we skip main pass rendering, we might need opaqueBackBuffer for upscaling or TAA
		opaqueBackBuffer = GetOpaqueColorMapIfNeeded( renderSize, outputs );
		if( opaqueBackBuffer.IsValid() )
		{
			renderContext.RenderPassHint( { Tr2LoadAction::DONT_CARE, Tr2StoreAction::STORE }, {} );
			BeginRenderPass( renderContext, { opaqueBackBuffer }, {} );
			renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_FULLSCREEN );
			Tr2Renderer::DrawTexture( renderContext, customBackBuffer );
		}
	}
	if( m_sceneOverlay )
	{
		Tr2TextureAL targets[] = { customBackBuffer.Get(), velocityMap.Get() };
		m_sceneOverlay->Execute( { targets, 2 }, {}, realTime, simTime, rootTimer, renderContext );
	}
	GlobalStore().RegisterVariable( "SpaceSceneNormalMap", Tr2TextureAL{} );
	normalMap = {};

	BeginRenderPass( renderContext, { sceneColorTarget }, depthBuffer );
	{
		// LensflareOccluder samples the local volumetric slices to attenuate
		// foreground visibility. RenderMainPass clears its temporary variable-store
		// binding after transparent rendering, so restore the exact texture produced
		// for this view while the native occlusion queries execute. Leaving the
		// resource unbound selects Metal's opaque diagnostic texture and falsely
		// clamps an empty-fog query to thirty percent visibility.
		GlobalStore().RegisterVariable( "EveSceneFogVolumeMap", volumetricSlices );
		ON_BLOCK_EXIT( [&] { GlobalStore().RegisterVariable( "EveSceneFogVolumeMap", Tr2TextureAL{} ); } );
		renderContext.SetReadOnlyDepth( true );
		m_scene->RunLensflareOcclusionQueries( depthBuffer, renderContext );
		renderContext.SetReadOnlyDepth( false );
	}
	{
		TimeSection endRenderSection( m_timers.endRender, "EndRender", rootTimer, renderContext );
		m_lastLensFlareDiagnostics = {};
		m_lastLensFlareDiagnostics.enabled = m_lensFlareDiagnosticsEnabled;
		if( m_lensFlareDiagnosticsEnabled )
		{
			const Tr2TextureAL& source = sceneColorTarget;
			const bool prepared = source.GetFormat() == Tr2RenderContextEnum::PIXEL_FORMAT_R16G16B16A16_FLOAT &&
				EnsureReadbackTexture(
					m_preLensFlareReadback, source, "EvePreLensFlareReadback", renderContext ) &&
				EnsureReadbackTexture(
					m_postLensFlareReadback, source, "EvePostLensFlareReadback", renderContext );
			m_lastLensFlareDiagnostics.captured = prepared &&
				SUCCEEDED( source.Resolve( m_preLensFlareReadback, renderContext ) );
			if( m_lastLensFlareDiagnostics.captured )
			{
				// Resolving closes the active pass.  The diagnostic rerun must retain
				// the scene color and depth that the native lens-flare pass consumes.
				renderContext.RenderPassHint(
					{ Tr2LoadAction::LOAD, Tr2StoreAction::STORE },
					{ Tr2LoadAction::LOAD, Tr2StoreAction::STORE } );
			}
		}
		m_scene->EndRender( renderContext );
		if( m_lensFlareDiagnosticsEnabled && m_lastLensFlareDiagnostics.captured )
		{
			m_lastLensFlareDiagnostics.captured =
				SUCCEEDED( sceneColorTarget.Resolve( m_postLensFlareReadback, renderContext ) );
		}
	}
	m_viewLast = m_scene->m_viewLast;
	m_projectionLast = m_scene->m_projectionLast;
	m_scene->PopulateAndApplyPerFrameData( renderContext );

	renderContext.m_esm.SetRenderTarget( 0, *destinations.data );
	renderContext.m_esm.SetDepthStencilBuffer( {} );

	GlobalStore().RegisterVariable( "EveSpaceSceneOpaqueMap", Tr2TextureAL{} );

	{
		TimeSection postProcessSection( m_timers.postProcess, "PostProcess", rootTimer, renderContext );
		if( m_settings.bypassPostProcessing )
		{
			// The scene rendered directly to the destination for asset-free probes.
		}
		else if( m_settings.postProcessBlitOnly )
		{
			auto resolvedBackBuffer = m_gpuResourcePool.GetTempTexture(
				"postProcessBlitOnlyResolved",
				renderSize,
				m_internalPixelFormat,
				Tr2GpuUsage::RENDER_TARGET | Tr2GpuUsage::SHADER_RESOURCE );
			customBackBuffer->Resolve( resolvedBackBuffer, renderContext );
			renderContext.RenderPassHint( { Tr2LoadAction::DONT_CARE, Tr2StoreAction::STORE }, {} );
			BeginRenderPass( renderContext, { *destinations.data }, {} );
			renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_FULLSCREEN );
			Tr2Renderer::DrawTexture( renderContext, resolvedBackBuffer );
		}
		else
		{
			auto preTonemapOutput = FindNamedOutput( outputs, "PreTonemapColor" );
			auto preTaaOutput = FindNamedOutput( outputs, "PreTaaColor" );
			auto postTaaOutput = FindNamedOutput( outputs, "PostTaaColor" );
			auto taaCooldownOutput = FindNamedOutput( outputs, "TaaCooldownMap" );
			auto bloomOutput = FindNamedOutput( outputs, "BloomMap" );
			auto postTonemapOutput = FindNamedOutput( outputs, "PostTonemapColor" );
			auto finalPostProcessOutput = FindNamedOutput( outputs, "FinalPostProcessColor" );
			m_postProcess->Execute(
				*destinations.data,
				std::move( customBackBuffer ),
				depthBuffer,
				std::move( velocityMap ),
				std::move( opaqueBackBuffer ),
				m_scene,
				m_upscalingContext,
				m_gpuResourcePool,
				renderContext,
				preTaaOutput ? &preTaaOutput->texture : nullptr,
				postTaaOutput ? &postTaaOutput->texture : nullptr,
				taaCooldownOutput ? &taaCooldownOutput->texture : nullptr,
				preTonemapOutput ? &preTonemapOutput->texture : nullptr,
				bloomOutput ? &bloomOutput->texture : nullptr,
				postTonemapOutput ? &postTonemapOutput->texture : nullptr,
				finalPostProcessOutput ? &finalPostProcessOutput->texture : nullptr );
		}
	}
	SetCameraToRenderer( renderContext );
	{
		TimeSection ui3dSection( m_timers.ui3d, "Render3dUI", rootTimer, renderContext );
		m_scene->Render3DUI( renderContext );
	}

	for( auto& toolScene : m_toolsScenes )
	{
		if( ITr2UpdateablePtr updateable = BlueCastPtr( toolScene ) )
		{
			updateable->Update( realTime, simTime );
		}
		toolScene->Render( renderContext );
	}

	GlobalStore().RegisterVariable( "SSAOMap", GetEmptySSAO( m_gpuResourcePool ) );
	GlobalStore().RegisterVariable( "DepthMap", Tr2TextureAL{} );

	if( m_settings.showFPS )
	{
		m_fpsRenderer->Execute( realTime, simTime, renderContext );
	}
}

void EveSpaceSceneRenderDriver::SetPostProcessDiagnosticsEnabled( bool enabled )
{
	if( m_postProcess )
	{
		m_postProcess->SetDiagnosticsEnabled( enabled );
	}
}

void EveSpaceSceneRenderDriver::SetDynamicExposureApplicationEnabledForTesting( bool enabled )
{
	if( m_postProcess )
	{
		m_postProcess->SetDynamicExposureApplicationEnabledForTesting( enabled );
	}
}

void EveSpaceSceneRenderDriver::SetDeterministicTaaExposureForTesting(
	bool enabled,
	float exposure )
{
	if( m_postProcess )
	{
		m_postProcess->SetDeterministicTaaExposureForTesting( enabled, exposure );
	}
}

bool EveSpaceSceneRenderDriver::ReadPostProcessDiagnostics(
	Tr2RenderContext& renderContext,
	Tr2PostProcessRenderer::Diagnostics& diagnostics ) const
{
	return m_postProcess && m_postProcess->ReadDiagnostics( const_cast<Tr2GpuResourcePool&>( m_gpuResourcePool ), renderContext, diagnostics );
}

bool EveSpaceSceneRenderDriver::GetLastPostProcessExecutionSucceeded() const
{
	return m_postProcess && m_postProcess->GetLastExecutionSucceeded();
}

void EveSpaceSceneRenderDriver::ResetTemporalHistory()
{
	m_temporalResetPending = true;
}

void EveSpaceSceneRenderDriver::SetTemporalHistoryFrozen( bool frozen )
{
	if( m_scene )
	{
		m_scene->SetTemporalHistoryFrozen( frozen );
	}
	if( m_postProcess )
	{
		m_postProcess->SetTaaHistoryFrozen( frozen );
		m_postProcess->SetDynamicExposureHistoryFrozen( frozen );
	}
}

void EveSpaceSceneRenderDriver::SetUseNewBloom( bool enabled )
{
	if( m_postProcess )
	{
		m_postProcess->SetUseNewBloom( enabled );
	}
}

bool EveSpaceSceneRenderDriver::GetUseNewBloom() const
{
	return m_postProcess && m_postProcess->GetUseNewBloom();
}

bool EveSpaceSceneRenderDriver::GetLastDistortionExecutionSucceeded() const
{
	if( !m_lastDistortionDiagnostics.enabled ||
		( !m_lastDistortionDiagnostics.backgroundBatches && !m_lastDistortionDiagnostics.foregroundBatches ) )
	{
		return true;
	}
	return m_lastDistortionDiagnostics.mapCreated && m_lastDistortionDiagnostics.copySucceeded &&
		m_lastDistortionDiagnostics.compositeSucceeded;
}

void EveSpaceSceneRenderDriver::SetLensFlareDiagnosticsEnabled( bool enabled )
{
	m_lensFlareDiagnosticsEnabled = enabled;
	if( !enabled )
	{
		m_lastLensFlareDiagnostics = {};
	}
}

bool EveSpaceSceneRenderDriver::ReadLensFlareDiagnostics(
	Tr2RenderContext& renderContext,
	LensFlareDiagnostics& diagnostics )
{
	diagnostics = m_lastLensFlareDiagnostics;
	if( !diagnostics.enabled || !diagnostics.captured ||
		!m_preLensFlareReadback.IsValid() || !m_postLensFlareReadback.IsValid() )
	{
		return false;
	}
	const void* beforeData = nullptr;
	const void* afterData = nullptr;
	uint32_t beforePitch = 0;
	uint32_t afterPitch = 0;
	if( FAILED( m_preLensFlareReadback.MapForReading(
			Tr2TextureSubresource( 0 ), true, beforeData, beforePitch, renderContext ) ) || !beforeData )
	{
		return false;
	}
	if( FAILED( m_postLensFlareReadback.MapForReading(
			Tr2TextureSubresource( 0 ), true, afterData, afterPitch, renderContext ) ) || !afterData )
	{
		m_preLensFlareReadback.UnmapForReading( renderContext );
		return false;
	}
	diagnostics.width = m_preLensFlareReadback.GetWidth();
	diagnostics.height = m_preLensFlareReadback.GetHeight();
	diagnostics.format = static_cast<uint32_t>( m_preLensFlareReadback.GetFormat() );
	for( uint32_t y = 0; y < diagnostics.height; ++y )
	{
		const uint16_t* before = reinterpret_cast<const uint16_t*>(
			static_cast<const uint8_t*>( beforeData ) + static_cast<size_t>( y ) * beforePitch );
		const uint16_t* after = reinterpret_cast<const uint16_t*>(
			static_cast<const uint8_t*>( afterData ) + static_cast<size_t>( y ) * afterPitch );
		for( uint32_t x = 0; x < diagnostics.width; ++x )
		{
			bool changed = false;
			for( uint32_t channel = 0; channel < 3; ++channel )
			{
				const float delta = HalfToFloat( after[x * 4 + channel] ) -
					HalfToFloat( before[x * 4 + channel] );
				if( std::isfinite( delta ) && delta > 0.0f )
				{
					diagnostics.positiveRgbEnergy += delta;
					diagnostics.maximumPositiveChannelDelta =
						std::max( diagnostics.maximumPositiveChannelDelta, delta );
					changed = true;
				}
			}
			diagnostics.changedPixels += changed ? 1u : 0u;
		}
	}
	m_postLensFlareReadback.UnmapForReading( renderContext );
	m_preLensFlareReadback.UnmapForReading( renderContext );
	diagnostics.readbackSucceeded = true;
	m_lastLensFlareDiagnostics = diagnostics;
	return true;
}

void EveSpaceSceneRenderDriver::UpdateGpuParticleSystem( Tr2RenderContext& renderContext )
{
	if( m_scene->GetGpuParticleSystem() )
	{
		GPU_REGION( renderContext, "Particles" );
		m_scene->PopulateAndApplyPerFrameData( renderContext );
		m_scene->GetGpuParticleSystem()->Update( m_scene->m_updateTime, m_scene->m_updateContext.GetOriginShift(), renderContext );
	}
}

Tr2GpuResourcePool::Texture EveSpaceSceneRenderDriver::GetDistortionMapIfNeeded( const TextureSize2D& size )
{
	if( m_settings.enableDistortion )
	{
		return m_gpuResourcePool.GetTempTexture( "distortionMap", size, ImageIO::PIXEL_FORMAT_B8G8R8A8_UNORM, Tr2GpuUsage::RENDER_TARGET | Tr2GpuUsage::SHADER_RESOURCE );
	}
	return {};
}

Tr2GpuResourcePool::Texture EveSpaceSceneRenderDriver::GetVelocityMapIfNeeded( const TextureSize2D& size, const Span<TempOutput>& outputs )
{
	USE_MAIN_THREAD_RENDER_CONTEXT();
	auto upscalingInfo = renderContext.GetUpscalingInfo( m_upscalingContext ? m_upscalingContext->GetID() : Tr2UpscalingAL::INVALID_CONTEXT_ID );
	bool needsVelocityMap = m_settings.antiAliasingQuality != AntiAliasingQuality::Disabled || ( upscalingInfo.technique != Tr2UpscalingAL::NONE && upscalingInfo.temporal );
	auto namedOutput = FindNamedOutput( outputs, "VelocityMap" );
	Tr2GpuResourcePool::Texture velocityMap;
	if( needsVelocityMap || m_settings.forceVelocityMap || namedOutput )
	{
		auto texture = m_gpuResourcePool.GetTempTexture( "velocityMap", size, ImageIO::PIXEL_FORMAT_R16G16_FLOAT, Tr2GpuUsage::RENDER_TARGET | Tr2GpuUsage::SHADER_RESOURCE );
		if( namedOutput )
		{
			namedOutput->texture = texture;
		}
		return texture;
	}
	return {};
}

Tr2GpuResourcePool::Texture EveSpaceSceneRenderDriver::GetNormalMapIfNeeded( const TextureSize2D& size, const Span<TempOutput>& outputs )
{
	auto namedOutput = FindNamedOutput( outputs, "NormalMap" );
	if( ( ( m_settings.aoQuality != AmbientOcclusionQuality::Disabled || m_settings.shadowQuality == ShadowQuality::SHADOW_RAYTRACED ) && Tr2Renderer::GetShaderModel() != TR2SM_3_0_LO ) || m_settings.forceNormalMap || namedOutput )
	{
		auto texture = m_gpuResourcePool.GetTempTexture( "normalMap", size, ImageIO::PIXEL_FORMAT_R10G10B10A2_UNORM, Tr2GpuUsage::RENDER_TARGET | Tr2GpuUsage::SHADER_RESOURCE );
		if( namedOutput )
		{
			namedOutput->texture = texture;
		}
		return texture;
	}
	return {};
}

Tr2GpuResourcePool::Texture EveSpaceSceneRenderDriver::GetCustomStencilMapIfNeeded( const TextureSize2D& size, const Span<TempOutput>& outputs )
{
	auto namedOutput = FindNamedOutput( outputs, "CustomStencilMap" );
	if( m_customStencilFormat != ImageIO::PIXEL_FORMAT_UNKNOWN || namedOutput )
	{
		auto texture = m_gpuResourcePool.GetTempTexture( "customStencil", size, m_customStencilFormat, Tr2GpuUsage::RENDER_TARGET | Tr2GpuUsage::SHADER_RESOURCE );
		if( namedOutput )
		{
			namedOutput->texture = texture;
		}
		return texture;
	}
	return {};
}

Tr2GpuResourcePool::Texture EveSpaceSceneRenderDriver::GetOpaqueColorMapIfNeeded( const TextureSize2D& size, const Span<TempOutput>& outputs )
{
	auto namedOutput = FindNamedOutput( outputs, "OpaqueColorMap" );
	bool needsOpaqueBackBuffer = m_upscalingContext != nullptr || m_settings.antiAliasingQuality >= AntiAliasingQuality::Medium || m_settings.forceOpaqueBuffer || namedOutput;
	if( !needsOpaqueBackBuffer )
	{
		// If there are SSSSS batches, we need an opaque back buffer
		auto batches = m_scene->m_primaryBatches[TRIBATCHTYPE_OPAQUE];
		static const auto SSSSS = BlueSharedString( "SSSSS" );
		needsOpaqueBackBuffer = Tr2RenderContext::TechniqueInBatch( batches->GetGdprBatches(), SSSSS ) || Tr2RenderContext::TechniqueInBatch( batches->GetBatches(), SSSSS );
	}
	if( needsOpaqueBackBuffer )
	{
		auto texture = m_gpuResourcePool.GetTempTexture( "opaqueBackBuffer", size, m_internalPixelFormat, Tr2GpuUsage::RENDER_TARGET | Tr2GpuUsage::SHADER_RESOURCE );
		if( namedOutput )
		{
			namedOutput->texture = texture;
		}
		return texture;
	}
	return {};
}

std::vector<Tr2TextureReferencePtr> EveSpaceSceneRenderDriver::GetAllTempTextures() const
{
	std::vector<Tr2TextureAL> textures;
	m_gpuResourcePool.DebugGetAllTempTextures( textures );

	std::vector<Tr2TextureReferencePtr> result;
	result.reserve( textures.size() );
	for( auto& tex : textures )
	{
		auto texRef = Tr2TextureReferencePtr();
		texRef.CreateInstance();
		*texRef->GetTexture() = tex;
		result.push_back( texRef );
	}
	return result;
}

void EveSpaceSceneRenderDriver::SetDebugMode( bool enable )
{
	if( enable != GetDebugMode() )
	{
		m_gpuResourcePool.SetDebugMode( enable );
	}
}

bool EveSpaceSceneRenderDriver::GetDebugMode() const
{
	return m_gpuResourcePool.GetDebugMode();
}

EveSpaceScene* EveSpaceSceneRenderDriver::GetScene() const
{
	return m_scene;
}

void EveSpaceSceneRenderDriver::SetScene( EveSpaceScene* scene )
{
	if( m_scene == scene )
	{
		return;
	}
	m_scene = scene;
}

void EveSpaceSceneRenderDriver::SetView( TriView* view )
{
	m_view = view;
}

void EveSpaceSceneRenderDriver::SetProjection( TriProjection* projection )
{
	m_projection = projection;
}
