#pragma once
#ifndef Tr2TextureALOrbis_h_
#define Tr2TextureALOrbis_h_

struct Tr2SubresourceData;

#if( TRINITY_PLATFORM==TRINITY_ORBIS )

#include <gnm/texture.h>
#include <memory>

// -------------------------------------------------------------
// Description:
//   A low level wrapper around the calls needed to set up a GPU
// texture.
// SeeAlso:
//   Tr2SubresourceData
// -------------------------------------------------------------
class Tr2TextureAL :
	public Tr2BitmapDimensions,
	public Tr2TrackedALObject<Tr2RenderContextEnum::OT_TEXTURE>
{
public:
	Tr2TextureAL();
	~Tr2TextureAL();

	Tr2TextureAL& operator=( Tr2TextureAL&& );
	Tr2TextureAL& operator=( const Tr2TextureAL& other );

	ALResult Create2D( uint32_t width,
					   uint32_t height,
					   uint32_t mipLevelCount,
					   Tr2RenderContextEnum::PixelFormat format,
					   Tr2RenderContextEnum::BufferUsage usage,
					   Tr2SubresourceData* initialData,
					   Tr2RenderContextAL& renderContext );

	ALResult CreateCube( uint32_t width,
						 uint32_t height,
						 uint32_t mipLevelCount,
						 Tr2RenderContextEnum::PixelFormat format,
						 Tr2RenderContextEnum::BufferUsage usage,
						 Tr2SubresourceData* initialData,
						 Tr2RenderContextAL& renderContext );

	ALResult CreateVolume( uint32_t width,
						   uint32_t height,
						   uint32_t depth,
						   uint32_t mipLevelCount,
						   Tr2RenderContextEnum::PixelFormat format,
						   Tr2RenderContextEnum::BufferUsage usage,
						   Tr2SubresourceData* initialData,
						   Tr2RenderContextAL& renderContext );

	bool IsValid() const;
	bool IsAlias() const
	{
		return m_isAlias;
	}
	void Destroy();

	Tr2RenderContextEnum::BufferUsage GetUsage()		const
	{
		return m_usage;
	}

	ALResult UpdateSubresource( uint32_t left,
								uint32_t top,
								uint32_t right,
								uint32_t bottom,
								const void* source,
								uint32_t sourcePitch,
								Tr2RenderContextAL& renderContext );
	ALResult CopySubresourceRegion( const Tr2TextureSubresource& destSubresource,
									Tr2TextureAL& source,
									const Tr2TextureSubresource& sourceSubresource,
									Tr2RenderContextAL& renderContext );

	ALResult CopySubresourceRegion( const Tr2TextureSubresource& destSubresource,
									Tr2RenderTargetAL& source,
									const Tr2TextureSubresource& sourceSubresource,
									Tr2RenderContextAL& renderContext );

	ALResult Lock( uint32_t mipLevel,
				   void*& data,
				   uint32_t& pitch,
				   Tr2RenderContextEnum::LockType,
				   Tr2RenderContextAL& renderContext );
	ALResult Lock( uint32_t mipLevel,
				   uint32_t* ltrb,
				   void*& data,
				   uint32_t& pitch,
				   Tr2RenderContextEnum::LockType,
				   Tr2RenderContextAL& renderContext );
	ALResult Lock( Tr2RenderContextEnum::CubemapFace face,
				   uint32_t mipLevel,
				   uint32_t* ltrb,
				   void*& data,
				   uint32_t& pitch,
				   Tr2RenderContextEnum::LockType,
				   Tr2RenderContextAL& renderContext );
	ALResult Unlock( Tr2RenderContextAL& renderContext );

	bool operator==( const Tr2TextureAL& other ) const;

	Tr2ALMemoryType GetMemoryClass() const
	{
		return AL_MEMORY_MANAGED;
	}

	ALResult CreateUAV( Tr2PrimaryRenderContextAL& renderContext )
	{
		return E_FAIL;
	}

	ALResult InternalGetTextureForGpu( Tr2RenderContextEnum::ColorSpace colorSpace, sce::Gnm::Texture& texture, Tr2RenderContextAL& renderContext ) const;
	ALResult InternalCreateTexture2D( 
		uint32_t width,
		uint32_t height,
		uint32_t mipLevelCount,
		Tr2RenderContextEnum::PixelFormat format,
		sce::Gnm::TileMode tileMode,
		sce::Gnm::NumSamples numSamples,
		Tr2BufferImplAL::Access cpuAccess,
		Tr2BufferImplAL::Access gpuAccess,
		Tr2RenderContextAL& renderContext );
	ALResult InternalAttachToDepthBuffer(
		const sce::Gnm::DepthRenderTarget& depthBuffer,
		const std::shared_ptr<Tr2BufferImplAL>& memory,
		Tr2RenderContextAL& renderContext );
private:
	Tr2RenderContextEnum::BufferUsage m_usage;
	// Texture is owned by other AL object (render target, depth stencil)
	bool m_isAlias;


	std::shared_ptr<Tr2BufferImplAL> m_memory;

	friend Tr2RenderTargetAL;

	mutable sce::Gnm::Texture m_texture;
	mutable sce::Gnm::Texture m_textureSrgb;
};

#endif

#endif
