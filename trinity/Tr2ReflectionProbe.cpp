// Copyright © 2019 CCP ehf.

#include "StdAfx.h"
#include "Tr2ReflectionProbe.h"
#include "Tr2Renderer.h"
#include "Tr2TextureReference.h"
#include "Tr2RenderTarget.h"
#include "Shader/Tr2Effect.h"
#include "Resources/TriTextureRes.h"
#include "Shader/Parameter/Tr2RuntimeTextureParameter.h"
#include <algorithm>
#include <sstream>

using namespace Tr2RenderContextEnum;

const unsigned MIP_COUNT = 7;
const unsigned FILTER_SIZE = 128;
const unsigned FILTER_GROUP_DIM = 342; // ceil(sum(2**(x+x) for x in range(1, MIP_COUNT + 1)) / GROUP_SIZE)

namespace
{

const auto s_backLightColorName = BlueSharedString( "BackLightColor" );
const auto s_backLightContrastName = BlueSharedString( "BackLightContrast" );
const auto s_viewDirectionName = BlueSharedString( "ViewDirection" );

}

Tr2ReflectionProbe::Tr2ReflectionProbe( IRoot* lockobj ) :
	m_initialized( false ),
	m_hasData( false ),
	m_lastFilterSucceeded( false ),
	m_lastSamplingCopySucceeded( false ),
	m_position( 0, 0, 0 ),
	m_intermediateSize( FILTER_SIZE * 4 ),
	m_prevCullInversion( false ),
	m_customSourceTexture(),
	m_renderFrequency( ONE_SIDE_PER_FRAME ),
	m_currentFrame( 0 ),
	m_onePassDone( false ),
	m_hdrOutput( true ),
	m_hollywoodMode( true ),
	m_lockPosition( false ),
	m_backlightColor( 1, 1, 1, 1 ),
	m_backlightContrast( 16 )
{
	for( unsigned i = 0; i < 6; i++ )
	{
		m_renderTargets[i].CreateInstance();
	}

	m_renderTargetCube.CreateInstance();
	m_preFilterEffect.CreateInstance();
	m_copyMipEffect.CreateInstance();
	m_filterEffect.CreateInstance();
	m_preFilterTarget.CreateInstance();
	m_postFilterTarget.CreateInstance();
	m_samplingTarget.CreateInstance();
	PrepareResources();
}

Tr2ReflectionProbe::~Tr2ReflectionProbe()
{
	ReleaseResources( TRISTORAGE_ALL );
}

bool Tr2ReflectionProbe::IsValid()
{
	return PrepareResources();
}

bool Tr2ReflectionProbe::HasData() const
{
	return m_hasData && m_samplingTarget->IsValid();
}

bool Tr2ReflectionProbe::LastFilterSucceeded() const
{
	return m_lastFilterSucceeded;
}

bool Tr2ReflectionProbe::LastSamplingCopySucceeded() const
{
	return m_lastSamplingCopySucceeded;
}

TriFrustum Tr2ReflectionProbe::GetFrustum( unsigned face, Tr2RenderContext& renderContext )
{
	TriFrustum frustum = TriFrustum();
	frustum.DeriveFrustum( &Tr2Renderer::GetViewTransform(), &m_position, &Tr2Renderer::GetProjectionTransform(), renderContext.m_esm.GetViewport() );
	return frustum;
}

uint8_t Tr2ReflectionProbe::GetStartFace() const
{
	if( m_renderFrequency == ALL_SIDES_PER_FRAME || !m_onePassDone )
	{
		return 0;
	}
	return m_currentFrame;
}

uint8_t Tr2ReflectionProbe::GetEndFace() const
{
	if( m_renderFrequency == ALL_SIDES_PER_FRAME || !m_onePassDone )
	{
		return 6;
	}
	return m_currentFrame + 1;
}


void Tr2ReflectionProbe::InitRenderPass( Tr2RenderContext& renderContext )
{
	if( !m_lockPosition )
	{
		m_position = Tr2Renderer::GetViewPosition();
	}

	renderContext.m_esm.PushViewport();
	Tr2Renderer::PushViewTransform();
	Tr2Renderer::PushProjection();
	renderContext.m_esm.PushRenderTarget();
	renderContext.m_esm.PushDepthStencilBuffer();

	renderContext.m_esm.SetViewport( m_intermediateSize, m_intermediateSize, 0, 0, 0, 1 );

	// Square projection matrix, with near and far clip planes from the current projection
	Matrix newProjection = IdentityMatrix();
	const Matrix& currentProjection = Tr2Renderer::GetProjectionTransform();
	newProjection.m[2][2] = currentProjection.m[2][2];
	newProjection.m[2][3] = -1.0f;
	newProjection.m[3][2] = currentProjection.m[3][2];
	newProjection.m[3][3] = 0.0f;

	Tr2Renderer::SetProjectionTransform( newProjection );

	// We need to invert cull-mode because of the left-handed coordinates of the cube rendertarget
	m_prevCullInversion = renderContext.m_esm.IsCullModeInverted();
	renderContext.m_esm.SetInvertedCullMode( true );

#if !TRINITY_PLATFORM_SUPPORTS_RENDER_PASS_HINTS
	{
		GPU_REGION( renderContext, "Reflection Depth Clear" );

		for( int i = GetStartFace(); i < GetEndFace(); i++ )
		{
			renderContext.SetRenderTarget( *m_renderTargets[i], 0 );
			renderContext.m_esm.SetDepthStencilBuffer( m_stencilMaps[i] );
			CR( renderContext.Clear( CLEARFLAGS_ZBUFFER | CLEARFLAGS_TARGET, 0, 0, 0 ) );
		}
	}
#endif
}

Tr2TextureAL Tr2ReflectionProbe::GetDepthBuffer( unsigned face )
{
	return m_stencilMaps[face];
}

void Tr2ReflectionProbe::StartRenderFace( unsigned face, Tr2RenderContext& renderContext )
{
	renderContext.RenderPassHint( { Tr2LoadAction::CLEAR, Tr2StoreAction::STORE, 0 }, { Tr2LoadAction::CLEAR, Tr2StoreAction::DONT_CARE, 0.F } );

	renderContext.m_esm.SetRenderTarget( 0, *m_renderTargets[face] );
	renderContext.m_esm.SetDepthStencilBuffer( m_stencilMaps[face] );

	static const Vector3 faceDirections[] = {
		Vector3( 0, 1, 0 ),
		Vector3( 1, 0, 0 ), //+X
		Vector3( 0, 1, 0 ),
		Vector3( -1, 0, 0 ), //-X
		Vector3( 0, 0, -1 ),
		Vector3( 0, 1, 0 ), //+Y
		Vector3( 0, 0, 1 ),
		Vector3( 0, -1, 0 ), //-Y
		Vector3( 0, 1, 0 ),
		Vector3( 0, 0, 1 ), //+Z
		Vector3( 0, 1, 0 ),
		Vector3( 0, 0, -1 ), //-Z
	};

	// We need to invert the X-axis because of the left-handed coordinates of the cube rendertarget
	static const Matrix xInverter = Matrix(
		-1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 );

	const Vector3& up = faceDirections[face * 2];
	const Vector3& at = faceDirections[face * 2 + 1];
	Tr2Renderer::SetViewTransform( LookAtMatrix( m_position, m_position + at, up ) * xInverter );
}

void Tr2ReflectionProbe::EndRenderPass( Tr2RenderContext& renderContext )
{
	bool copiesSucceeded = true;
	{
		GPU_REGION( renderContext, "Reflection RT Copy" );

		for( int i = GetStartFace(); i < GetEndFace(); i++ )
		{
			const ALResult result = m_renderTargetCube->GetRenderTarget().CopySubresourceRegion( Tr2TextureSubresource( i, 0 ), *m_renderTargets[i], Tr2TextureSubresource( 0, 0 ), renderContext );
			CR( result );
			copiesSucceeded = copiesSucceeded && SUCCEEDED( result );
		}
	}

	renderContext.m_esm.SetInvertedCullMode( m_prevCullInversion );

	renderContext.m_esm.PopDepthStencilBuffer();
	renderContext.m_esm.PopRenderTarget();
	Tr2Renderer::PopProjection();
	Tr2Renderer::PopViewTransform();
	renderContext.m_esm.PopViewport();

	m_lastSamplingCopySucceeded = false;
	m_lastFilterSucceeded = copiesSucceeded && Filter( renderContext );
	if( m_lastFilterSucceeded )
	{
		m_lastSamplingCopySucceeded = CopyFilteredOutput( renderContext );
		m_lastFilterSucceeded = m_lastSamplingCopySucceeded;
	}
	m_hasData = m_lastFilterSucceeded;
	m_onePassDone = m_lastFilterSucceeded;

	if( m_lastFilterSucceeded && m_renderFrequency != ALL_SIDES_PER_FRAME )
	{
		m_currentFrame = ( m_currentFrame + 1 ) % 6;
	}
}

Tr2RenderTargetPtr Tr2ReflectionProbe::GetReflection()
{
	return m_samplingTarget;
}

Tr2EffectPtr Tr2ReflectionProbe::GetPreFilterEffect() const
{
	return m_preFilterEffect;
}

Tr2EffectPtr Tr2ReflectionProbe::GetFilterEffect() const
{
	return m_filterEffect;
}

Tr2EffectPtr Tr2ReflectionProbe::GetCopyMipEffect() const
{
	return m_copyMipEffect;
}

Tr2ReflectionProbe::ReflectionProbeRenderFrequency Tr2ReflectionProbe::GetRenderFrequency() const
{
	return m_renderFrequency;
}

void Tr2ReflectionProbe::SetRenderFrequency( ReflectionProbeRenderFrequency frequency )
{
	m_renderFrequency = frequency;
}

uint32_t Tr2ReflectionProbe::GetReflectionWidth() const
{
	return m_postFilterTarget && m_postFilterTarget->IsValid() ? m_postFilterTarget->GetWidth() : 0;
}

uint32_t Tr2ReflectionProbe::GetReflectionHeight() const
{
	return m_postFilterTarget && m_postFilterTarget->IsValid() ? m_postFilterTarget->GetHeight() : 0;
}

uint32_t Tr2ReflectionProbe::GetReflectionMipCount() const
{
	return m_postFilterTarget && m_postFilterTarget->IsValid() ? m_postFilterTarget->GetMipCount() : 0;
}

bool Tr2ReflectionProbe::CollectRawDiagnosticsForTesting(
	Tr2RenderContext& renderContext,
	RawDiagnosticsForTesting& diagnostics ) const
{
	diagnostics = {};
	auto collect = [&]( const Tr2RenderTargetPtr& target,
						std::vector<RawTextureHashForTesting>& hashes ) {
		if( !target || !target->IsValid() )
		{
			return false;
		}
		const Tr2TextureAL& source = target->GetRenderTarget();
		const uint32_t bytesPerPixel = GetBytesPerPixel( source.GetFormat() );
		if( source.GetType() != TEX_TYPE_CUBE || bytesPerPixel == 0 )
		{
			return false;
		}
		for( uint32_t face = 0; face < 6; ++face )
		{
			for( uint32_t mip = 0; mip < source.GetMipCount(); ++mip )
			{
				const uint32_t width = std::max( 1u, source.GetWidth() >> mip );
				const uint32_t height = std::max( 1u, source.GetHeight() >> mip );
				Tr2TextureAL readback;
				if( FAILED( readback.Create(
						Tr2BitmapDimensions( width, height, 1, source.GetFormat() ),
						Tr2GpuUsage::COPY_DESTINATION,
						Tr2CpuUsage::READ,
						renderContext.GetPrimaryRenderContext() ) ) ||
					FAILED( readback.CopySubresourceRegion(
						Tr2TextureSubresource( 0, 0 ),
						source,
						Tr2TextureSubresource( face, mip ),
						renderContext ) ) )
				{
					return false;
				}
				const void* mapped = nullptr;
				uint32_t pitch = 0;
				if( FAILED( readback.MapForReading(
						Tr2TextureSubresource( 0 ), true, mapped, pitch, renderContext ) ) ||
					!mapped )
				{
					return false;
				}
				constexpr uint64_t fnvOffset = 14695981039346656037ull;
				constexpr uint64_t fnvPrime = 1099511628211ull;
				uint64_t hash = fnvOffset;
				const size_t rowBytes = static_cast<size_t>( width ) * bytesPerPixel;
				for( uint32_t y = 0; y < height; ++y )
				{
					const uint8_t* row = static_cast<const uint8_t*>( mapped ) +
						static_cast<size_t>( y ) * pitch;
					for( size_t byte = 0; byte < rowBytes; ++byte )
					{
						hash = ( hash ^ row[byte] ) * fnvPrime;
					}
				}
				readback.UnmapForReading( renderContext );
				hashes.push_back( { face, mip, width, height, source.GetFormat(), hash } );
			}
		}
		return true;
	};

	return collect( m_renderTargetCube, diagnostics.source ) &&
		collect( m_preFilterTarget, diagnostics.prefilter ) &&
		collect( m_postFilterTarget, diagnostics.filtered ) &&
		collect( m_samplingTarget, diagnostics.sampled );
}

void Tr2ReflectionProbe::SetBackLightColor( Color color )
{
	m_backlightColor = color;
}

void Tr2ReflectionProbe::SetBackLightContrast( float contrast )
{
	m_backlightContrast = contrast;
}

void Tr2ReflectionProbe::ReleaseResources( TriStorage s )
{
	m_initialized = false;
	m_hasData = false;
	m_lastFilterSucceeded = false;
	m_lastSamplingCopySucceeded = false;
}

bool Tr2ReflectionProbe::OnPrepareResources()
{
	if( !SHADER_TYPE_EXISTS( COMPUTE_SHADER ) )
	{
		return false;
	}

	USE_MAIN_THREAD_RENDER_CONTEXT();

#if TRINITY_PLATFORM == TRINITY_DIRECTX12
	auto rtFormat = renderContext.FormatIsUAVCompatibleDx12( DXGI_FORMAT_R11G11B10_FLOAT ) ? PIXEL_FORMAT_R11G11B10_FLOAT : PIXEL_FORMAT_R16G16B16A16_FLOAT;
#else
	auto rtFormat = PIXEL_FORMAT_R11G11B10_FLOAT;
#endif
	return DoPrepareResources( rtFormat, renderContext );
}

bool Tr2ReflectionProbe::DoPrepareResources( ImageIO::PixelFormat rtFormat, Tr2PrimaryRenderContext& renderContext )
{

	for( int i = 0; i < 6; i++ )
	{
		if( !m_renderTargets[i]->IsValid() )
		{
			CR_RETURN_VAL( m_renderTargets[i]->CreateManual( m_intermediateSize, m_intermediateSize, 1, rtFormat, 0, 0, EX_BIND_UNORDERED_ACCESS, TEX_TYPE_2D, Tr2CpuUsage::NONE, Tr2GpuUsage::RENDER_TARGET ), false );
		}

		if( !m_stencilMaps[i].IsValid() )
		{
			int stencilSize = ( m_customSourceTexture && m_customSourceTexture->IsValid() ) ? m_customSourceTexture->GetWidth() : m_intermediateSize;
			CR_RETURN_VAL( m_stencilMaps[i].Create( Tr2BitmapDimensions( stencilSize, stencilSize, 1, PIXEL_FORMAT_D32_FLOAT ), Tr2GpuUsage::DEPTH_STENCIL | Tr2GpuUsage::SHADER_RESOURCE, renderContext ), false );
		}
	}

	if( !m_renderTargetCube->IsValid() )
	{
		CR_RETURN_VAL( m_renderTargetCube->Create( m_intermediateSize, m_intermediateSize, 1, rtFormat, 0, 0, EX_BIND_UNORDERED_ACCESS, TEX_TYPE_CUBE ), false );
	}

	if( !m_preFilterTarget->IsValid() )
	{
		CR_RETURN_VAL( m_preFilterTarget->Create( FILTER_SIZE, FILTER_SIZE, 8, rtFormat, 0, 0, EX_BIND_UNORDERED_ACCESS, TEX_TYPE_CUBE ), false );
	}

	if( !m_postFilterTarget->IsValid() )
	{
		CR_RETURN_VAL( m_postFilterTarget->Create( FILTER_SIZE * 2, FILTER_SIZE * 2, MIP_COUNT + 1, m_hdrOutput ? rtFormat : PIXEL_FORMAT_R8G8B8A8_UNORM, 0, 0, EX_BIND_UNORDERED_ACCESS, TEX_TYPE_CUBE ), false );
		m_hasData = false;
	}

	if( !m_samplingTarget->IsValid() )
	{
		CR_RETURN_VAL( m_samplingTarget->Create( FILTER_SIZE * 2, FILTER_SIZE * 2, MIP_COUNT + 1, m_hdrOutput ? rtFormat : PIXEL_FORMAT_R8G8B8A8_UNORM, 0, 0, EX_NONE, TEX_TYPE_CUBE ), false );
		m_hasData = false;
	}

	if( !m_initialized )
	{
		m_filterEffect->SetEffectPathName( "res:/graphics/effect/managed/space/System/Reflection/ReflectionFilterActivision128.fx" );
		m_preFilterEffect->SetEffectPathName( "res:/graphics/effect/managed/space/System/Reflection/ReflectionFilterActivisionPre.fx" );
		auto source = dynamic_cast<ITr2TextureProvider*>( m_customSourceTexture.p );
		m_preFilterEffect->SetParameter( BlueSharedString( "tex_hi_res" ), source ? source : static_cast<ITr2TextureProvider*>( m_renderTargetCube ) );
		m_preFilterEffect->SetParameter( BlueSharedString( "tex_lo_res" ), m_preFilterTarget );

		m_preFilterEffect->SetOption( BlueSharedString( "HOLLYWOOD_MODE" ), BlueSharedString( m_hollywoodMode ? "HOLLYWOOD_ON" : "HOLLYWOOD_OFF" ) );
		if( m_hollywoodMode )
		{
			m_preFilterEffect->SetParameter( s_backLightColorName, Vector4( m_backlightColor ) );
			m_preFilterEffect->SetParameter( s_backLightContrastName, m_backlightContrast );
			m_preFilterEffect->SetParameter( s_viewDirectionName, Vector3( 0, 1, 0 ) );
		}

		m_filterEffect->SetParameter( BlueSharedString( "tex_in" ), m_preFilterTarget );

		for( int i = 0; i < MIP_COUNT; i++ )
		{
			std::stringstream str;
			str << "tex_out" << i;
			Tr2RuntimeTextureParameterPtr param;
			param.CreateInstance();
			param->Create( BlueSharedString( str.str() ), m_postFilterTarget, i + 1 );
			m_filterEffect->AddResource( param );
		}

		m_filterEffect->SetParameter( BlueSharedString( "output_srgb" ), m_hdrOutput ? 0.f : 1.f );

		m_copyMipEffect->SetEffectPathName( "res:/graphics/effect/managed/space/System/Reflection/CopyCube.fx" );
		m_copyMipEffect->SetParameter( BlueSharedString( "tex_hi_res" ), source ? source : static_cast<ITr2TextureProvider*>( m_renderTargetCube ) );
		m_copyMipEffect->SetParameter( BlueSharedString( "tex_lo_res" ), m_postFilterTarget );
		m_copyMipEffect->SetOption( BlueSharedString( "HOLLYWOOD_MODE" ), BlueSharedString( m_hollywoodMode ? "HOLLYWOOD_ON" : "HOLLYWOOD_OFF" ) );
		if( m_hollywoodMode )
		{
			m_copyMipEffect->SetParameter( s_backLightColorName, Vector4( m_backlightColor ) );
			m_copyMipEffect->SetParameter( s_backLightContrastName, m_backlightContrast );
			m_copyMipEffect->SetParameter( s_viewDirectionName, Vector3( 0, 1, 0 ) );
		}

		m_initialized = true;
	}

	return true;
}

bool Tr2ReflectionProbe::OnModified( Be::Var* value )
{
	DestroyRenderTargets();
	PrepareResources();

	return true;
}

void Tr2ReflectionProbe::DestroyRenderTargets()
{
	for( unsigned i = 0; i < 6; i++ )
	{
		m_renderTargets[i]->Destroy();
		m_stencilMaps[i] = Tr2TextureAL();
	}

	m_renderTargetCube->Destroy();
	m_preFilterTarget->Destroy();
	m_postFilterTarget->Destroy();
	m_samplingTarget->Destroy();
	m_initialized = false;
	m_hasData = false;
	m_lastFilterSucceeded = false;
	m_lastSamplingCopySucceeded = false;
	m_onePassDone = false;
	m_currentFrame = 0;
}

bool Tr2ReflectionProbe::Filter( Tr2RenderContext& renderContext )
{
	if( !IsValid() )
	{
		return false;
	}

	{
		GPU_REGION( renderContext, "Reflection Pre Filter" );

		if( m_hollywoodMode )
		{
			m_preFilterEffect->SetParameter( s_backLightColorName, Vector4( m_backlightColor ) );
			m_preFilterEffect->SetParameter( s_backLightContrastName, m_backlightContrast );
			m_preFilterEffect->SetParameter( s_viewDirectionName, Tr2Renderer::GetInverseViewTransform().GetZ() );

			m_copyMipEffect->SetParameter( s_backLightColorName, Vector4( m_backlightColor ) );
			m_copyMipEffect->SetParameter( s_backLightContrastName, m_backlightContrast );
			m_copyMipEffect->SetParameter( s_viewDirectionName, Tr2Renderer::GetInverseViewTransform().GetZ() );
		}

		if( !Tr2Renderer::RunComputeShader( m_preFilterEffect, FILTER_SIZE * 2 / 8, FILTER_SIZE * 2 / 8, 6, renderContext ) )
		{
			return false;
		}
	}

	{
		GPU_REGION( renderContext, "Reflection Filter Mip Generation" );
		if( FAILED( m_preFilterTarget->GenerateMipMaps() ) )
		{
			return false;
		}
	}
	{
		GPU_REGION( renderContext, "Reflection Main Filter" );
		if( !Tr2Renderer::RunComputeShader( m_filterEffect, FILTER_GROUP_DIM, 6, 1, renderContext ) )
		{
			return false;
		}
	}
	{
		GPU_REGION( renderContext, "Copy Mip" );
		if( !Tr2Renderer::RunComputeShader( m_copyMipEffect, FILTER_SIZE * 2 / 8, FILTER_SIZE * 2 / 8, 6, renderContext ) )
		{
			return false;
		}
	}
	return true;
}

bool Tr2ReflectionProbe::CopyFilteredOutput( Tr2RenderContext& renderContext )
{
	if( !m_postFilterTarget || !m_postFilterTarget->IsValid() ||
		!m_samplingTarget || !m_samplingTarget->IsValid() )
	{
		return false;
	}
	const Tr2TextureAL& filtered = m_postFilterTarget->GetRenderTarget();
	Tr2TextureAL& sampled = m_samplingTarget->GetRenderTarget();
	if( filtered.GetMipCount() != sampled.GetMipCount() )
	{
		return false;
	}
	for( uint32_t face = 0; face < 6; ++face )
	{
		for( uint32_t mip = 0; mip < filtered.GetMipCount(); ++mip )
		{
			if( FAILED( sampled.CopySubresourceRegion(
					Tr2TextureSubresource( face, mip ),
					filtered,
					Tr2TextureSubresource( face, mip ),
					renderContext ) ) )
			{
				return false;
			}
		}
	}
	return true;
}

void Tr2ReflectionProbe::RunFilter()
{
	USE_MAIN_THREAD_RENDER_CONTEXT();
	DestroyRenderTargets();
	m_lastSamplingCopySucceeded = false;
	m_lastFilterSucceeded = DoPrepareResources( PIXEL_FORMAT_R16G16B16A16_FLOAT, renderContext ) &&
		Filter( renderContext );
	if( m_lastFilterSucceeded )
	{
		m_lastSamplingCopySucceeded = CopyFilteredOutput( renderContext );
		m_lastFilterSucceeded = m_lastSamplingCopySucceeded;
	}
	m_hasData = m_lastFilterSucceeded;
	m_onePassDone = m_lastFilterSucceeded;
}

bool Tr2ReflectionProbe::IsHollyWoodModeOn() const
{
	return m_hollywoodMode;
}

bool Tr2ReflectionProbe::ReadyForDynamicObjectReflections() const
{
	// We need to have initialized the cubemap, before we start using it for rendering reflections (otherwise we just get garbage!)
	return m_onePassDone;
}
