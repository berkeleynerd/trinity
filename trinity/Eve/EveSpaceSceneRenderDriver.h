// Copyright © 2026 CCP ehf.

#pragma once

#include "EveSpaceScene.h"
#include "../Tr2LightManager.h"
#include "../PostProcess/Tr2PostProcessRenderer.h"
#include "../Tr2GpuResourcePool.h"
#include "../ITr2RenderNode.h"
#include "../Tr2ProfileTimer.h"


BLUE_DECLARE( Tr2RenderTarget );
BLUE_DECLARE( TriProjection );
BLUE_DECLARE( TriView );
BLUE_DECLARE( EveSpaceScene );
BLUE_DECLARE( TriCurveSet );
BLUE_DECLARE( Tr2Effect );
BLUE_DECLARE( TriStepRenderFps );
BLUE_DECLARE_IVECTOR( ITr2Scene );
BLUE_DECLARE( EveCamera );
BLUE_DECLARE( Tr2Sprite2dScene );




BLUE_CLASS( EveSpaceSceneRenderDriver ) :
	public ITr2RenderNode,
	public Tr2DeviceResource
{
public:
	struct TemporalFrameSnapshot
	{
		Matrix currentView = IdentityMatrix();
		Matrix currentProjection = IdentityMatrix();
		Matrix currentViewProjection = IdentityMatrix();
		Matrix previousView = IdentityMatrix();
		Matrix previousProjection = IdentityMatrix();
		Matrix previousViewProjection = IdentityMatrix();
		Vector4 jitter = Vector4( 0, 0, 0, 0 );
		bool valid = false;
	};

	enum class AntiAliasingQuality
	{
		Disabled,
		Low,
		Medium,
		High,
	};
	enum class AmbientOcclusionQuality
	{
		Disabled,
		Low,
		Medium,
		High,
	};
	enum class ReflectionQuality
	{
		Disabled,
		Low,
		Medium,
		High,
		Ultra,
	};
	struct Settings
	{
		ShadowQuality shadowQuality = ShadowQuality::SHADOW_HIGH;
		AntiAliasingQuality antiAliasingQuality = AntiAliasingQuality::High;
		Tr2PPTaaEffect::Debug taaDebugMode = Tr2PPTaaEffect::TAA_DEBUG_OFF;
		AmbientOcclusionQuality aoQuality = AmbientOcclusionQuality::High;
		PostProcess::Quality postProcessingQuality = PostProcess::Quality::HIGH;
		Tr2VolumerticQuality volumetricQuality = Tr2VolumerticQuality::High;

		Color clearColor = Color( 0.0f, 0.0f, 0.0f, 1.0f );

		bool forceOpaqueBuffer = false;
		bool forceNormalMap = false;
		bool forceVelocityMap = false;
		bool enableDirectionalShadows = true;
		bool enableUpscaling = true;
		bool bypassPostProcessing = false;
		bool postProcessBlitOnly = false;
		bool enableDistortion = true; // should be tied to shader quality
		bool showFPS = false;
		EveSpaceScene::EveVisualizeMethod visualizeMethod = EveSpaceScene::VM_NONE;
	};
	struct DistortionDiagnostics
	{
		bool enabled = false;
		bool mapCreated = false;
		bool backgroundBatches = false;
		bool foregroundBatches = false;
		bool copySucceeded = false;
		bool compositeSucceeded = false;
		uint32_t backgroundApplications = 0;
		uint32_t foregroundApplications = 0;
		uint32_t mapWidth = 0;
		uint32_t mapHeight = 0;
		uint32_t mapFormat = 0;
	};
	struct LensFlareDiagnostics
	{
		bool enabled = false;
		bool captured = false;
		bool readbackSucceeded = false;
		uint32_t width = 0;
		uint32_t height = 0;
		uint32_t format = 0;
		uint64_t changedPixels = 0;
		double positiveRgbEnergy = 0.0;
		float maximumPositiveChannelDelta = 0.0f;
	};

	EveSpaceSceneRenderDriver( IRoot* lockobj = nullptr );
	~EveSpaceSceneRenderDriver();


	bool Validate( const Span<const Tr2BitmapDimensions>& destDimensions, const Span<const BlueSharedString>& outputs, Be::Time realTime, Be::Time simTime ) override;
	void Execute( const Span<const Tr2TextureAL>& destinations, const Span<TempOutput>& outputs, Be::Time realTime, Be::Time simTime, const Tr2ProfileTimer& rootTimer, Tr2RenderContext& renderContext ) override;


	void SetSettings( const Settings& settings )
	{
		if( m_settings.antiAliasingQuality != settings.antiAliasingQuality )
			m_temporalResetPending = true;
		m_settings = settings;
	}
	const Settings& GetSettings() const
	{
		return m_settings;
	}

	void SetDebugMode( bool enable );
	bool GetDebugMode() const;
	void SetRenderingEnabled( bool enabled )
	{
		m_enableRendering = enabled;
	}
	bool GetRenderingEnabled() const
	{
		return m_enableRendering;
	}

	std::vector<Tr2TextureReferencePtr> GetAllTempTextures() const;

	EveSpaceScene* GetScene() const;
	void SetScene( EveSpaceScene * scene );
	Tr2SSAO* GetSSAO() const
	{
		return m_ssao;
	}
	void SetSSAO( Tr2SSAO * ssao )
	{
		m_ssao = ssao;
	}
	void SetView( TriView * view );
	void SetProjection( TriProjection * projection );
	void SetReflectionCorrectionEnabled( bool enabled )
	{
		m_reflectionCorrectionEnabled = enabled;
	}
	bool GetReflectionCorrectionEnabled() const
	{
		return m_reflectionCorrectionEnabled;
	}
	void SetPostProcessDiagnosticsEnabled( bool enabled );
	void SetDynamicExposureApplicationEnabledForTesting( bool enabled );
	void SetDeterministicTaaExposureForTesting( bool enabled, float exposure );
	bool ReadPostProcessDiagnostics( Tr2RenderContext & renderContext, Tr2PostProcessRenderer::Diagnostics & diagnostics ) const;
	bool GetLastPostProcessExecutionSucceeded() const;
	bool GetPostProcessHistoryDiagnostics(
		Tr2PostProcessRenderer::HistoryDiagnostics& diagnostics ) const
	{
		if( !m_postProcess )
		{
			return false;
		}
		diagnostics = m_postProcess->GetHistoryDiagnostics();
		return true;
	}
	void ResetTemporalHistory();
	void SetTemporalHistoryFrozen( bool frozen );
	const TemporalFrameSnapshot& GetTemporalFrameSnapshot() const
	{
		return m_temporalFrameSnapshot;
	}
	void SetUseNewBloom( bool enabled );
	bool GetUseNewBloom() const;
	Tr2Effect* GetDistortionEffect() const
	{
		return m_distortionEffect;
	}
	const DistortionDiagnostics& GetDistortionDiagnostics() const
	{
		return m_lastDistortionDiagnostics;
	}
	bool GetLastDistortionExecutionSucceeded() const;
	void SetLensFlareDiagnosticsEnabled( bool enabled );
	bool ReadLensFlareDiagnostics( Tr2RenderContext& renderContext, LensFlareDiagnostics& diagnostics );

	EXPOSE_TO_BLUE();

private:
	void SetupUpscaling( const TextureSize2D& displaySize );
	void PropagateSettings();
	void SetCameraToRenderer( Tr2RenderContext & renderContext ) const;
	TextureSize2D GetRenderSize( const TextureSize2D& displaySize ) const;

	Tr2GpuResourcePool::Texture GetDistortionMapIfNeeded( const TextureSize2D& size );
	Tr2GpuResourcePool::Texture GetVelocityMapIfNeeded( const TextureSize2D& size, const Span<TempOutput>& outputs );
	Tr2GpuResourcePool::Texture GetNormalMapIfNeeded( const TextureSize2D& size, const Span<TempOutput>& outputs );
	Tr2GpuResourcePool::Texture GetCustomStencilMapIfNeeded( const TextureSize2D& size, const Span<TempOutput>& outputs );
	Tr2GpuResourcePool::Texture GetOpaqueColorMapIfNeeded( const TextureSize2D& size, const Span<TempOutput>& outputs );

	void UpdateGpuParticleSystem( Tr2RenderContext & renderContext );

	Tr2GpuResourcePool::Texture RenderSSAO( const Tr2TextureAL& depthMap, const Tr2TextureAL& normalMap, Tr2RenderContext& renderContext );

	void ReleaseResources( TriStorage s ) override;
	bool OnPrepareResources() override;

	EveSpaceScenePtr m_scene;

	Tr2SSAOPtr m_ssao;


	Tr2GpuResourcePool m_gpuResourcePool;

	TriProjectionPtr m_projection;
	TriViewPtr m_view;
	EveCameraPtr m_camera;

	ImageIO::PixelFormat m_internalPixelFormat = ImageIO::PIXEL_FORMAT_R16G16B16A16_FLOAT;
	Tr2UpscalingContextAL* m_upscalingContext = nullptr;

	Settings m_settings;

	Tr2EffectPtr m_distortionEffect;
	DistortionDiagnostics m_lastDistortionDiagnostics;
	bool m_lensFlareDiagnosticsEnabled = false;
	mutable LensFlareDiagnostics m_lastLensFlareDiagnostics;
	Tr2TextureAL m_preLensFlareReadback;
	Tr2TextureAL m_postLensFlareReadback;

	PITr2SceneVector m_toolsScenes;

	Tr2PostProcessRendererPtr m_postProcess;
	TriStepRenderFpsPtr m_fpsRenderer;

	ITr2RenderNodePtr m_background;
	ITr2RenderNodePtr m_sceneOverlay;

	std::string m_name;
	bool m_enableRendering = true;
	bool m_mainPassRenderingEnabled = true;
	ImageIO::PixelFormat m_customStencilFormat = ImageIO::PIXEL_FORMAT_UNKNOWN;
	BlueSharedString m_depthPassTechnique = BlueSharedString( "Depth" );

	// View and projection matrices from last frame, for velocity calculations
	Matrix m_viewLast = IdentityMatrix();
	Matrix m_projectionLast = IdentityMatrix();
	bool m_temporalResetPending = true;
	TextureSize2D m_temporalRenderSize;
	TemporalFrameSnapshot m_temporalFrameSnapshot;

	bool m_reflectionCorrectionEnabled;
	TriTextureResPtr m_reflectionCorrectionMap;
	TriTextureResPtr m_blackReflectionCorrectionMap;

	struct
	{
		Tr2ProfileTimer update;
		Tr2ProfileTimer beginRender;
		Tr2ProfileTimer endRender;
		Tr2ProfileTimer postProcess;
		Tr2ProfileTimer ui3d;
		Tr2ProfileTimer background;
		Tr2ProfileTimer depthPass;
		Tr2ProfileTimer mainPass;
		Tr2ProfileTimer reflections;
		Tr2ProfileTimer shadows;
		Tr2ProfileTimer ssao;
	} m_timers;
};

TYPEDEF_BLUECLASS( EveSpaceSceneRenderDriver );
