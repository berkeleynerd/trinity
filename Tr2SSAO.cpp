////////////////////////////////////////////////////////////////////////////////
//
// Created:   March 2022
// Copyright: CCP 2022
//

#include "StdAfx.h"

#include "ffx_cacao_defines.h"
#include "ffx_cacao.cpp"
#include "CACAOCommon/Common.h"

#include "Tr2SSAO.h"
#include "Tr2Renderer.h"
#include "Tr2TextureReference.h"
#include "Tr2DepthStencil.h"
#include "Tr2RenderTarget.h"
#include "Tr2GpuBuffer.h"
#include "Shader/Tr2Effect.h"
#include "Shader/Tr2Shader.h"
#include "Resources/TriTextureRes.h"

using namespace Tr2RenderContextEnum;

Tr2SSAO::Tr2SSAO( IRoot* lockobj )
{
	m_settings = FFX_CACAO_PRESETS[0].settings;
	m_settings.radius = 6;
	m_settings.shadowMultiplier = 1.0f;
	m_settings.shadowPower = 2.6f;
	m_settings.shadowClamp = 0.98f;
	m_settings.sharpness = 0.5f;
	m_settings.detailShadowStrength = 5;

	m_settingsLarge = FFX_CACAO_PRESETS[0].settings;
	m_settingsLarge.radius = 2.5f;
	m_settingsLarge.shadowMultiplier = 1.0f;
	m_settingsLarge.shadowPower = 2.6f;
	m_settingsLarge.shadowClamp = 0.98f;
	m_settingsLarge.sharpness = 0.5f;
	m_settingsLarge.detailShadowStrength = 5;

	m_ssaoEffect.CreateInstance();
	m_ssaoLargeEffect.CreateInstance();
	m_deinterleavedDepthTarget.CreateInstance();
	m_deinterleavedNormalTarget.CreateInstance();
	m_ssaoWorkerTargetA.CreateInstance();
	m_ssaoWorkerTargetB.CreateInstance();
	m_loadCounterBuffer.CreateInstance();
	m_importanceTargetA.CreateInstance();
	m_importanceTargetB.CreateInstance();
	m_blankOutputTexture.CreateInstance();
	m_outputTarget.CreateInstance();

	m_ssaoLargeEffect->SetOption( BlueSharedString( "SSAO_APPLICATION" ), BlueSharedString( "SSAO_APPLY_MIX" ) );

	PrepareResources();
}

Tr2SSAO::~Tr2SSAO()
{
	ReleaseResources( TRISTORAGE_ALL );
}

bool Tr2SSAO::IsValid()
{
	return PrepareResources();
}

void Tr2SSAO::ReleaseResources( TriStorage s )
{
	m_initialized = false;
	m_ssaoEffect->ClearAllResources();
	m_ssaoEffect->ClearAllParameters();
	m_ssaoLargeEffect->ClearAllResources();
	m_ssaoLargeEffect->ClearAllParameters();
	m_deinterleavedDepthTexture = {};
	m_deinterleavedNormalTexture = {};
	m_ssaoWorkerTextureA = {};
	m_ssaoWorkerTextureB = {};
	m_importanceTargetA->Destroy();
	m_importanceTargetB->Destroy();
	m_outputTarget->Destroy();
}

void Tr2SSAO::SetInputBuffers( Tr2DepthStencilPtr depthBuffer, Tr2RenderTargetPtr normalBuffer )
{
	if( m_inputDepthBuffer != depthBuffer || m_inputNormalBuffer != normalBuffer )
	{
		m_inputDepthBuffer = depthBuffer;
		m_inputNormalBuffer = normalBuffer;
		m_initialized = false;
		ReleaseResources( {} );
		OnPrepareResources();
	}
	if( m_inputDepthBuffer )
	{
		m_ssaoEffect->SetOption( BlueSharedString( "SSAO_INPUT_2D_TEXTURE_TYPE" ), BlueSharedString( m_inputDepthBuffer->GetMsaaSamples() > 1 ? "TEXTURE_2DMS" : "TEXTURE_2D" ) );
		m_ssaoLargeEffect->SetOption( BlueSharedString( "SSAO_INPUT_2D_TEXTURE_TYPE" ), BlueSharedString( m_inputDepthBuffer->GetMsaaSamples() > 1 ? "TEXTURE_2DMS" : "TEXTURE_2D" ) );
	}
}

inline static uint32_t DispatchSize( uint32_t tileSize, uint32_t totalSize )
{
	return ( totalSize + tileSize - 1 ) / tileSize;
}

FFX_CACAO_Settings GetSettings( bool useDownsampledSSAO, bool generateNormals, SSAOQuality quality )
{
	unsigned settingsIndex = static_cast<int>( quality ) + useDownsampledSSAO * ( static_cast<int>( SSAOQuality::LOWEST ) + 1 );
	FFX_CACAO_Settings settings = FFX_CACAO_PRESETS[settingsIndex].settings;
	
	settings.generateNormals = generateNormals;

	// Set fade out to something large to disable it
	settings.fadeOutFrom = 10;
	settings.fadeOutTo = 1;

	// Set to maximum to compensate for our large scene sizes
	settings.radius = 5;
	settings.sharpness = 0.5f;
	settings.detailShadowStrength = 5;
	settings.shadowPower = 2;

	return settings;
}

bool Tr2SSAO::OnPrepareResources()
{
	if( !m_blankOutputTexture->IsValid() )
	{
		BeResMan->GetResource( "res:/texture/global/white.dds", "", m_blankOutputTexture );
	}

	if( !SHADER_TYPE_EXISTS( COMPUTE_SHADER ) )
	{
		return false;
	}

	if( !m_enabled )
	{
		return false;
	}

	if( !m_inputDepthBuffer || !m_inputDepthBuffer->IsValid() )
	{
		return false;
	}

	bool hasNormals = m_inputNormalBuffer && m_inputNormalBuffer->IsValid();

	FFX_CACAO_BufferSizeInfo size{};
	FFX_CACAO_UpdateBufferSizeInfo( m_inputDepthBuffer->GetWidth(), m_inputDepthBuffer->GetHeight(), m_downsampledSSAO, &size );
	FFX_CACAO_Settings settings = GetSettings( m_downsampledSSAO, !hasNormals, m_ssaoQuality );

	// Check if the smallest map is still large enough
	if( !size.importanceMapWidth || !size.importanceMapHeight )
	{
		return false;
	}

	USE_MAIN_THREAD_RENDER_CONTEXT();

	for( unsigned i = 0; i <= SSAO_PASS_COUNT; i++ )
	{
		if( !m_constBuffers[i].IsValid() )
		{
			CR_RETURN_VAL( m_constBuffers[i].Create( sizeof( FFX_CACAO_Constants ), renderContext ), false );
		}
	}

	if( !m_deinterleavedDepthTexture.IsValid() )
	{
		unsigned mipCount = settings.qualityLevel >= FFX_CACAO_QUALITY_MEDIUM ? 4 : 1;
		Tr2BitmapDimensions dims( TEX_TYPE_2D, PIXEL_FORMAT_R16_FLOAT, size.deinterleavedDepthBufferWidth, size.deinterleavedDepthBufferHeight, 1, mipCount, SSAO_PASS_COUNT );
		CR_RETURN_VAL( m_deinterleavedDepthTexture.Create( dims, Tr2MsaaDesc( 0, 0 ), Tr2GpuUsage::UNORDERED_ACCESS | Tr2GpuUsage::RENDER_TARGET | Tr2GpuUsage::SHADER_RESOURCE, renderContext ), false );
		m_deinterleavedDepthTarget->Attach( m_deinterleavedDepthTexture, this );
	}

	if( !m_deinterleavedNormalTexture.IsValid() )
	{
		Tr2BitmapDimensions dims( TEX_TYPE_2D, PIXEL_FORMAT_R8G8B8A8_SNORM, size.deinterleavedDepthBufferWidth, size.deinterleavedDepthBufferHeight, 1, 1, SSAO_PASS_COUNT );
		CR_RETURN_VAL( m_deinterleavedNormalTexture.Create( dims, Tr2MsaaDesc( 0, 0 ), Tr2GpuUsage::UNORDERED_ACCESS | Tr2GpuUsage::RENDER_TARGET | Tr2GpuUsage::SHADER_RESOURCE, renderContext ), false );
		m_deinterleavedNormalTarget->Attach( m_deinterleavedNormalTexture, this );
	}

	if( !m_ssaoWorkerTextureA.IsValid() )
	{
		Tr2BitmapDimensions dims( TEX_TYPE_2D, PIXEL_FORMAT_R8G8_SNORM, size.ssaoBufferWidth, size.ssaoBufferHeight, 1, 1, SSAO_PASS_COUNT );
		CR_RETURN_VAL( m_ssaoWorkerTextureA.Create( dims, Tr2MsaaDesc( 0, 0 ), Tr2GpuUsage::UNORDERED_ACCESS | Tr2GpuUsage::RENDER_TARGET | Tr2GpuUsage::SHADER_RESOURCE, renderContext ), false );
		m_ssaoWorkerTargetA->Attach( m_ssaoWorkerTextureA, this );
	}

	if( !m_ssaoWorkerTextureB.IsValid() )
	{
		Tr2BitmapDimensions dims( TEX_TYPE_2D, PIXEL_FORMAT_R8G8_SNORM, size.ssaoBufferWidth, size.ssaoBufferHeight, 1, 1, SSAO_PASS_COUNT );
		CR_RETURN_VAL( m_ssaoWorkerTextureB.Create( dims, Tr2MsaaDesc( 0, 0 ), Tr2GpuUsage::UNORDERED_ACCESS | Tr2GpuUsage::RENDER_TARGET | Tr2GpuUsage::SHADER_RESOURCE, renderContext ), false );
		m_ssaoWorkerTargetB->Attach( m_ssaoWorkerTextureB, this );
	}

	if( settings.qualityLevel == FFX_CACAO_QUALITY_HIGHEST )
	{
		if( !m_loadCounterBuffer->IsValid() )
		{
			CR_RETURN_VAL( m_loadCounterBuffer->Create( 1, PIXEL_FORMAT_R32_UINT, Tr2GpuBuffer::GPU_WRITABLE ), false);
		}

		if( !m_importanceTargetA->IsValid() )
		{
			CR_RETURN_VAL( m_importanceTargetA->Create( size.importanceMapWidth, size.importanceMapHeight, 1, PIXEL_FORMAT_R8_UNORM, 1, 0, EX_BIND_UNORDERED_ACCESS ), false );
		}

		if( !m_importanceTargetB->IsValid() )
		{
			CR_RETURN_VAL( m_importanceTargetB->Create( size.importanceMapWidth, size.importanceMapHeight, 1, PIXEL_FORMAT_R8_UNORM, 1, 0, EX_BIND_UNORDERED_ACCESS ), false );
		}
	}

	if( !m_outputTarget->IsValid() || m_outputTarget->IsAttached() )
	{
		m_outputTarget->Detach();
		CR_RETURN_VAL( m_outputTarget->Create( size.inputOutputBufferWidth, size.inputOutputBufferHeight, 1, PIXEL_FORMAT_R16_FLOAT, 1, 0, EX_BIND_UNORDERED_ACCESS ), false );
	}

	if( !m_initialized )
	{
		UpdateEffect( m_ssaoEffect, settings.blurPassCount );
		UpdateEffect( m_ssaoLargeEffect, settings.blurPassCount );
		m_initialized = true;
	}

	return true;
}

void Tr2SSAO::UpdateEffect( Tr2Effect* ssaoEffect, uint32_t blurPassCount )
{
	ssaoEffect->SetEffectPathName( "res:/graphics/effect/managed/space/System/SSAO/SSAO.fx" );

	// Clear load counter pass
	ssaoEffect->SetParameter( BlueSharedString( "g_ClearLoadCounter_LoadCounter" ), m_loadCounterBuffer );

	// Prepare pass
	ssaoEffect->SetParameter( BlueSharedString( "g_DepthIn" ), m_inputDepthBuffer );
	ssaoEffect->SetParameter( BlueSharedString( "g_PrepareNormalsFromNormalsInput" ), m_inputNormalBuffer );
	ssaoEffect->SetParameter( BlueSharedString( "g_PrepareDepthsOut" ), m_deinterleavedDepthTarget );
	ssaoEffect->SetParameter( BlueSharedString( "g_PrepareNormals_NormalOut" ), m_deinterleavedNormalTarget );

	// SSAO pass
	ssaoEffect->SetParameter( BlueSharedString( "g_ViewspaceDepthSource" ), m_deinterleavedDepthTarget );
	ssaoEffect->SetParameter( BlueSharedString( "g_DeinterleavedNormals" ), m_deinterleavedNormalTarget );
	ssaoEffect->SetParameter( BlueSharedString( "g_LoadCounter" ), m_loadCounterBuffer );
	ssaoEffect->SetParameter( BlueSharedString( "g_ImportanceMap" ), m_importanceTargetA );
	ssaoEffect->SetParameter( BlueSharedString( "g_FinalSSAO" ), m_ssaoWorkerTargetA );

	// Importance pass
	ssaoEffect->SetParameter( BlueSharedString( "g_ImportanceFinalSSAO" ), m_ssaoWorkerTargetA );
	ssaoEffect->SetParameter( BlueSharedString( "g_ImportanceOut" ), m_importanceTargetA );
	ssaoEffect->SetParameter( BlueSharedString( "g_ImportanceAIn" ), m_importanceTargetA );
	ssaoEffect->SetParameter( BlueSharedString( "g_ImportanceAOut" ), m_importanceTargetB );
	ssaoEffect->SetParameter( BlueSharedString( "g_ImportanceBIn" ), m_importanceTargetB );
	ssaoEffect->SetParameter( BlueSharedString( "g_ImportanceBOut" ), m_importanceTargetA );
	ssaoEffect->SetParameter( BlueSharedString( "g_ImportanceBLoadCounter" ), m_loadCounterBuffer );

	// Edge sensitive blur pass
	ssaoEffect->SetParameter( BlueSharedString( "g_EdgeSensitiveBlur_Input" ), m_ssaoWorkerTargetB );
	ssaoEffect->SetParameter( BlueSharedString( "g_EdgeSensitiveBlur_Output" ), m_ssaoWorkerTargetA );

	// Bilateral upsample pass
	ssaoEffect->SetParameter( BlueSharedString( "g_BilateralUpscaleInput" ), blurPassCount ? m_ssaoWorkerTargetA : m_ssaoWorkerTargetB );
	ssaoEffect->SetParameter( BlueSharedString( "g_BilateralUpscaleDepth" ), m_inputDepthBuffer );
	ssaoEffect->SetParameter( BlueSharedString( "g_BilateralUpscaleDownscaledDepth" ), m_deinterleavedDepthTarget );
	ssaoEffect->SetParameter( BlueSharedString( "g_BilateralUpscaleOutput" ), m_outputTarget );

	// Apply pass
	ssaoEffect->SetParameter( BlueSharedString( "g_ApplyFinalSSAO" ), blurPassCount ? m_ssaoWorkerTargetA : m_ssaoWorkerTargetB );
	ssaoEffect->SetParameter( BlueSharedString( "g_ApplyOutput" ), m_outputTarget );
}

bool Tr2SSAO::OnModified( Be::Var* value )
{
	ReleaseResources( {} );
	OnPrepareResources();

	return true;
}

Tr2RenderTargetPtr Tr2SSAO::GetOutput() const
{
	return m_outputTarget;
}

HRESULT Tr2SSAO::ApplyConstBuffer( unsigned pass, Tr2RenderContext& renderContext )
{
	return renderContext.SetConstants( m_constBuffers[std::min( pass, SSAO_PASS_COUNT )], COMPUTE_SHADER, Tr2Renderer::GetPerFramePSStartRegister() );
}

void Tr2SSAO::Filter( Tr2RenderContext& renderContext )
{
	if( !IsValid() )
	{
		if( !m_outputTarget->IsAttached() )
		{
			m_outputTarget->Attach( *m_blankOutputTexture->GetTexture(), this );
		}
		return;
	}

	GPU_REGION( renderContext, "SSAO" );
	{
		GPU_REGION( renderContext, "Detail" );
		PerformPass( m_ssaoQuality, m_settings, m_zoomLevel, m_ssaoEffect, renderContext );
	}
	if( m_largeEffect )
	{
		GPU_REGION( renderContext, "Large" );
		PerformPass( SSAOQuality::LOW, m_settingsLarge, m_zoomLevelLarge, m_ssaoLargeEffect, renderContext );
	}
}

void Tr2SSAO::PerformPass( SSAOQuality quality, const FFX_CACAO_Settings& settingsTemplate, float zoomLevel, Tr2Effect* ssaoEffect, Tr2RenderContext& renderContext )
{
	FFX_CACAO_BufferSizeInfo size{};
	FFX_CACAO_UpdateBufferSizeInfo( m_inputDepthBuffer->GetWidth(), m_inputDepthBuffer->GetHeight(), m_downsampledSSAO, &size );

	// From CACAO sample project
	FFX_CACAO_Matrix4x4 proj{}, normalsWorldToView{};
	{
		XMFLOAT4X4 p;
		XMMATRIX xProj = Tr2Renderer::GetReversedDepthProjectionTransform();
		XMStoreFloat4x4(&p, xProj);
		proj.elements[0][0] = p._11; proj.elements[0][1] = p._12; proj.elements[0][2] = p._13; proj.elements[0][3] = p._14;
		proj.elements[1][0] = p._21; proj.elements[1][1] = p._22; proj.elements[1][2] = p._23; proj.elements[1][3] = p._24;
		proj.elements[2][0] = p._31; proj.elements[2][1] = p._32; proj.elements[2][2] = p._33; proj.elements[2][3] = p._34;
		proj.elements[3][0] = p._41; proj.elements[3][1] = p._42; proj.elements[3][2] = p._43; proj.elements[3][3] = p._44;
		XMMATRIX xNormalsWorldToView = XMMATRIX(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 1) * Tr2Renderer::GetInverseViewTransform(); // should be transpose(inverse(view)), but XMM is row-major and HLSL is column-major
		XMStoreFloat4x4(&p, xNormalsWorldToView);
		normalsWorldToView.elements[0][0] = p._11; normalsWorldToView.elements[0][1] = p._12; normalsWorldToView.elements[0][2] = p._13; normalsWorldToView.elements[0][3] = p._14;
		normalsWorldToView.elements[1][0] = p._21; normalsWorldToView.elements[1][1] = p._22; normalsWorldToView.elements[1][2] = p._23; normalsWorldToView.elements[1][3] = p._24;
		normalsWorldToView.elements[2][0] = p._31; normalsWorldToView.elements[2][1] = p._32; normalsWorldToView.elements[2][2] = p._33; normalsWorldToView.elements[2][3] = p._34;
		normalsWorldToView.elements[3][0] = p._41; normalsWorldToView.elements[3][1] = p._42; normalsWorldToView.elements[3][2] = p._43; normalsWorldToView.elements[3][3] = p._44;
	}

	proj.elements[3][2] /= zoomLevel;

	bool hasNormals = m_inputNormalBuffer && m_inputNormalBuffer->IsValid();

	unsigned settingsIndex = static_cast<int>( quality ) + m_downsampledSSAO * ( static_cast<int>( SSAOQuality::HIGHEST ) + 1 );
	FFX_CACAO_Settings settings = GetSettings( m_downsampledSSAO, !hasNormals, quality );

	settings.radius = settingsTemplate.radius;
	settings.shadowMultiplier = settingsTemplate.shadowMultiplier;
	settings.shadowPower = settingsTemplate.shadowPower;
	settings.shadowClamp = settingsTemplate.shadowClamp;
	settings.sharpness = settingsTemplate.sharpness;
	settings.detailShadowStrength = settingsTemplate.detailShadowStrength;

	{
		GPU_REGION( renderContext, "Upload const buffers" );

		FFX_CACAO_Constants consts{};
		FFX_CACAO_UpdateConstants( &consts, &settings, &size, &proj, &normalsWorldToView );
		FFX_CACAO_Constants constsPP[SSAO_PASS_COUNT + 1]{};

		for( unsigned i = 0; i <= SSAO_PASS_COUNT; i++ )
		{
			constsPP[i] = consts;

			if( i < SSAO_PASS_COUNT )
			{
				FFX_CACAO_UpdatePerPassConstants( constsPP + i, &settings, &size, i );
			}

			void* data;
			CR_RETURN( m_constBuffers[i].Lock( &data, renderContext ) );
			memmove( data, constsPP + i, sizeof( consts ) );
			CR_RETURN( m_constBuffers[i].Unlock( renderContext ) );
		}
	}

	if( settings.qualityLevel == FFX_CACAO_QUALITY_HIGHEST )
	{
		GPU_REGION( renderContext, "Clear load counter" );
		Tr2Renderer::RunComputeShader( ssaoEffect, BlueSharedString( "ClearLoadCounter" ), 1, 1, 1, renderContext );
	}

	{
		GPU_REGION( renderContext, "Prepare" );

		CR_RETURN( ApplyConstBuffer( SSAO_PASS_COUNT, renderContext ) );

		if( settings.qualityLevel == FFX_CACAO_QUALITY_LOWEST)
		{
			std::string shader = m_downsampledSSAO ? "PrepareDownsampledDepthsHalf" : "PrepareNativeDepthsHalf";

			unsigned dimX = DispatchSize( FFX_CACAO_PREPARE_DEPTHS_HALF_WIDTH, size.deinterleavedDepthBufferWidth );
			unsigned dimY = DispatchSize( FFX_CACAO_PREPARE_DEPTHS_HALF_HEIGHT, size.deinterleavedDepthBufferHeight );
			Tr2Renderer::RunComputeShader( ssaoEffect, BlueSharedString( shader.c_str() ), dimX, dimY, 1, renderContext );
		}
		else
		{
			std::string shader = m_downsampledSSAO ? "PrepareDownsampledDepths" : "PrepareNativeDepths";

			unsigned dimX = DispatchSize( FFX_CACAO_PREPARE_DEPTHS_WIDTH, size.deinterleavedDepthBufferWidth );
			unsigned dimY = DispatchSize( FFX_CACAO_PREPARE_DEPTHS_HEIGHT, size.deinterleavedDepthBufferHeight );
			Tr2Renderer::RunComputeShader( ssaoEffect, BlueSharedString( shader.c_str() ), dimX, dimY, 1, renderContext );
		}

		if( settings.generateNormals )
		{
			std::string shader = m_downsampledSSAO ? "PrepareDownsampledNormals" : "PrepareNativeNormals";

			unsigned dimX = DispatchSize( FFX_CACAO_PREPARE_NORMALS_WIDTH, size.deinterleavedDepthBufferWidth );
			unsigned dimY = DispatchSize( FFX_CACAO_PREPARE_NORMALS_HEIGHT, size.deinterleavedDepthBufferHeight );
			Tr2Renderer::RunComputeShader( ssaoEffect, BlueSharedString( shader.c_str() ), dimX, dimY, 1, renderContext );
		}
		else
		{
			std::string shader = m_downsampledSSAO ? "PrepareDownsampledNormalsFromInputNormals" : "PrepareNativeNormalsFromInputNormals";

			unsigned dimX = DispatchSize( PREPARE_NORMALS_FROM_INPUT_NORMALS_WIDTH, size.deinterleavedDepthBufferWidth );
			unsigned dimY = DispatchSize( PREPARE_NORMALS_FROM_INPUT_NORMALS_HEIGHT, size.deinterleavedDepthBufferHeight );
			Tr2Renderer::RunComputeShader( ssaoEffect, BlueSharedString( shader.c_str() ), dimX, dimY, 1, renderContext );
		}
	}

	if( m_deinterleavedDepthTarget->GetMipCount() > 1 )
	{
		GPU_REGION( renderContext, "Prepare mips" );
		m_deinterleavedDepthTarget->GenerateMipMaps();
	}

	if( settings.qualityLevel == FFX_CACAO_QUALITY_HIGHEST )
	{
		{
			GPU_REGION( renderContext, "Interactive base pass" );

			ssaoEffect->SetParameter( BlueSharedString( "g_SSAOOutput" ), m_ssaoWorkerTargetA );

			unsigned dimX = DispatchSize( FFX_CACAO_GENERATE_WIDTH, size.ssaoBufferWidth );
			unsigned dimY = DispatchSize( FFX_CACAO_GENERATE_HEIGHT, size.ssaoBufferHeight );

			for( unsigned i = 0; i < SSAO_PASS_COUNT; i++ )
			{
				CR_RETURN( ApplyConstBuffer( i, renderContext ) );
				Tr2Renderer::RunComputeShader( ssaoEffect, BlueSharedString( "GenerateQ3Base" ), dimX, dimY, 1, renderContext );
			}
		}

		{
			GPU_REGION( renderContext, "Importance map" );

			CR_RETURN( ApplyConstBuffer( SSAO_PASS_COUNT, renderContext ) );

			unsigned dimX = DispatchSize( IMPORTANCE_MAP_WIDTH, size.importanceMapWidth );
			unsigned dimY = DispatchSize( IMPORTANCE_MAP_HEIGHT, size.importanceMapHeight );
			Tr2Renderer::RunComputeShader( ssaoEffect, BlueSharedString( "PostprocessImportanceMap" ), dimX, dimY, 1, renderContext );
		}
	}

	{
		GPU_REGION( renderContext, "Main pass" );

		ssaoEffect->SetParameter( BlueSharedString( "g_SSAOOutput" ), m_ssaoWorkerTargetB );

		unsigned dimX, dimY, dimZ;
		if( settings.qualityLevel <= FFX_CACAO_QUALITY_MEDIUM )
		{
			dimX = DispatchSize( FFX_CACAO_GENERATE_SPARSE_WIDTH, size.ssaoBufferWidth );
			dimX = ( dimX + 4 ) / 5;
			dimY = DispatchSize( FFX_CACAO_GENERATE_SPARSE_HEIGHT, size.ssaoBufferHeight );
			dimZ = 5;
		}
		else
		{
			dimX = DispatchSize( FFX_CACAO_GENERATE_WIDTH, size.ssaoBufferWidth );
			dimY = DispatchSize( FFX_CACAO_GENERATE_HEIGHT, size.ssaoBufferHeight );
			dimZ = 1;
		}

		unsigned quality = std::max( 0, settings.qualityLevel - 1 );
		std::string shader = "GenerateQ" + std::to_string( quality );

		for( unsigned i = 0; i < SSAO_PASS_COUNT; i++ )
		{
			CR_RETURN( ApplyConstBuffer( i, renderContext ) );
			Tr2Renderer::RunComputeShader( ssaoEffect, BlueSharedString( shader.c_str() ), dimX, dimY, dimZ, renderContext );
		}
	}

	if( settings.blurPassCount )
	{
		GPU_REGION( renderContext, "Edge sensitive blur" );
		
		unsigned dimX = DispatchSize( 4 * FFX_CACAO_BLUR_WIDTH  - 2 * settings.blurPassCount, size.ssaoBufferWidth );
		unsigned dimY = DispatchSize( 3 * FFX_CACAO_BLUR_HEIGHT - 2 * settings.blurPassCount, size.ssaoBufferHeight );

		for( unsigned i = 0; i < SSAO_PASS_COUNT; i++ )
		{
			if( settings.qualityLevel == FFX_CACAO_QUALITY_LOWEST && ( i == 1 || i == 2 ) )
			{
				continue;
			}

			CR_RETURN( ApplyConstBuffer( i, renderContext ) );

			std::string passName = "EdgeSensitiveBlur" + std::to_string( settings.blurPassCount );
			Tr2Renderer::RunComputeShader( ssaoEffect, BlueSharedString( passName.c_str() ), dimX, dimY, 1, renderContext );
		}
	}

	if( m_downsampledSSAO )
	{
		GPU_REGION( renderContext, "Bilateral upsample" );

		CR_RETURN( ApplyConstBuffer( SSAO_PASS_COUNT, renderContext ) );

		unsigned dimX = DispatchSize( 2 * FFX_CACAO_BILATERAL_UPSCALE_WIDTH, size.inputOutputBufferWidth );
		unsigned dimY = DispatchSize( 2 * FFX_CACAO_BILATERAL_UPSCALE_HEIGHT, size.inputOutputBufferHeight );

		std::string shader;
		switch( settings.qualityLevel )
		{
		case FFX_CACAO_QUALITY_LOWEST:
			shader = "UpscaleBilateral5x5Half";
			break;
		case FFX_CACAO_QUALITY_LOW:
		case FFX_CACAO_QUALITY_MEDIUM:
			shader = "UpscaleBilateral5x5NonSmart";
			break;
		default:
			shader = "UpscaleBilateral5x5Smart";
		}

		Tr2Renderer::RunComputeShader( ssaoEffect, BlueSharedString( shader.c_str() ), dimX, dimY, 1, renderContext );
	}
	else
	{
		GPU_REGION( renderContext, "Apply" );

		CR_RETURN( ApplyConstBuffer( SSAO_PASS_COUNT, renderContext ) );

		unsigned dimX = DispatchSize( FFX_CACAO_APPLY_WIDTH, size.inputOutputBufferWidth );
		unsigned dimY = DispatchSize( FFX_CACAO_APPLY_HEIGHT, size.inputOutputBufferHeight );

		std::string shader;
		switch( settings.qualityLevel )
		{
		case FFX_CACAO_QUALITY_LOWEST:
			shader = "NonSmartHalfApply";
			break;
		case FFX_CACAO_QUALITY_LOW:
			shader = "NonSmartApply";
			break;
		default:
			shader = "Apply";
		}

		Tr2Renderer::RunComputeShader( ssaoEffect, BlueSharedString( shader.c_str() ), dimX, dimY, 1, renderContext );
	}
}