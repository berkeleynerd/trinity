/* 
	*************************************************************************

	TriTextureRes.h

	Created:   Nov. 2000
	OS:        Win32
	Project:   Yep

	Description:   

		Impl of textureres


	Dependencies:

		Blue, Trinity

	(c) CCP 2000

	*************************************************************************
*/

#if !defined(_TriTextureRes_H_)
#define _TriTextureRes_H_

#include "include/ITriTextureRes.h"
#include "Tr2DeviceResource.h"
#include "ITr2TextureProvider.h"
#include "Tr2AsyncSave.h"
#include "Tr2DepthStencil.h"

class Tr2RenderTarget;
class Tr2ImageHandler;

BLUE_DECLARE( Tr2RenderTarget );
BLUE_DECLARE( Tr2HostBitmap );
BLUE_DECLARE( Tr2TexturePipeline );
BLUE_DECLARE( Tr2ImageRes );


/////////////////////////////////////////////////////////////////////////////////////
// TriTextureRes
/////////////////////////////////////////////////////////////////////////////////////
BLUE_CLASS( TriTextureRes ) :
	public BlueAsyncRes,
	public ITr2TextureProvider,
	public ICacheable,
	public ITriTextureRes,
	public Tr2DeviceResource,
	public Tr2BitmapDimensions,
	public Tr2AsyncSave
{
public:
	using BlueAsyncRes::Lock;
	using BlueAsyncRes::Unlock;

	TriTextureRes();
	~TriTextureRes();

	unsigned m_resourceRebuiltCounter;

	void RebuildCachedData();

	/////////////////////////////////////////////////////////////////////////////////////
	// ITriTextureRes
	Tr2TextureAL* GetTexture();
	unsigned GetWidth()				const { return Tr2BitmapDimensions::GetWidth();    }
	unsigned GetHeight()			const { return Tr2BitmapDimensions::GetHeight();   }
	unsigned GetMipLevelCount()		const { return Tr2BitmapDimensions::GetMipCount(); }
	uint32_t GetMsaaType() const;
	uint32_t GetMsaaQuality() const;

	Color GetAverageColor()			const{ return m_averageColor; }
	bool IsValid() const { return IsGood(); }
	Tr2RenderContextEnum::TextureType GetType() const { return Tr2BitmapDimensions::GetType(); }
	long UpdateSubresource( unsigned left, unsigned top, unsigned right, unsigned bottom, const void* source, unsigned sourcePitch );
	void SetAverageColor( float red, float green, float blue, float alpha ) override;
	const Tr2TextureAL* GetTexture() const;
	
	bool SetTextureFromRT( Tr2RenderTarget* renderTarget );
	
	// Make a TriTextureRes that takes a copy of the renderTarget contents and sticks with it (not a live view).
	bool CreateFromRT( Tr2RenderTarget* renderTarget, unsigned width = 0, unsigned height = 0 );

	// Make an immutable TriTextureRes that takes a copy of the hostBitmap contents and sticks with it (not a live view).
	bool CreateFromHostBitmap( Tr2HostBitmap* bitmap );
	bool CreateEmptyTexture( uint32_t width, uint32_t height, uint32_t mipCount, Tr2RenderContextEnum::PixelFormat format );
	BlueStdResult CreateFromTexture( TriTextureRes* texture );

	// Create a new TriTextureRes with undefined contents.  It's implied that the usage is dynamic.
	bool Create(	uint32_t width, 
					uint32_t height, 
					uint32_t mipCount, 
					Tr2RenderContextEnum::PixelFormat format, 
					Tr2RenderContextEnum::BufferUsageFlags usage,
					Tr2PrimaryRenderContext& renderContext );

	ALResult OpenShared( uintptr_t handle );

	bool SaveAsync( const wchar_t* filename );
	bool Save( const wchar_t* filename );	// synchronous


	Tr2TexturePipeline* GetPipeline() const;
	
	//////////////////////////////////////////////////////////////////////////
	// IBlueResource
	//
	// Initialize is implemented by the BlueAsyncRes base class, but we
	// need to intercept it to capture mip-skip settings
	void Initialize( const wchar_t* name, const wchar_t* ext );
	void OnShutdown();

	/////////////////////////////////////////////////////////////////////////////////////
	// ITriDeviceResource
private:
	bool OnPrepareResources();
public:				
	void ReleaseResources( TriStorage s );

#if TRINITYDEV
	virtual void GetDescription( std::string& desc );
#endif

	//////////////////////////////////////////////////////////////////////////
	// ICacheable
	bool IsMemoryUsageKnown();
	size_t GetMemoryUsage();

	// Handy name for debugging and memory overviews, otherwise unused.
	std::string m_name;

	virtual void SetName( const char* name )
	{
		m_name = name;
	}
	virtual const char* GetName() const
	{
		return m_name.c_str();
	}

private:
	Tr2TextureAL *m_texture;
	Tr2TextureAL m_ownTexture;
	Tr2RenderTargetPtr	m_wrappedRenderTarget;

	unsigned ComputeMipSkipCount();

	unsigned int m_memoryUse;

	float	m_cutoutX;
	float	m_cutoutY;
	float	m_cutoutWidth;
	float	m_cutoutHeight;

	ImageIO::Metadata m_metadata;

	std::unique_ptr<ImageIO::HostBitmap> m_loadedBitmap;

    unsigned	m_mipLevelMaxCount;
	bool m_isTextureLoadDisabled		: 1;
	bool m_isTextureResizable			: 1;	// do we listen to the global mipskip setting?
	Color m_averageColor; 
	
private:
	bool SetTexture( Tr2TextureAL& texture );
	bool Generate( const char *	);
	bool HasALObject( int type, size_t object );

protected:
	// Provide the functions that do the actual work of loading and preparing.
	// The async management itself is done in BlueAsyncRes.
	virtual LoadingResult DoLoad();
	virtual bool DoPrepare();
	virtual void CleanupLoadData();

private:
	// These member variables and functions exist to support async texture saving;
	// from Tr2AsyncSave
	virtual bool DoPrepareAsyncSave();
	virtual bool DoExecuteAsyncSave();
	virtual void DoCleanupAsyncSave();

	friend Tr2HostBitmap;

	Tr2TexturePipelinePtr m_pipeline;
	std::unordered_map<std::wstring, Tr2ImageResPtr> m_pipelineInputs;
	CcpAtomic<uint32_t> m_resourceLoadCbId;
	CcpAtomic<uint32_t> m_resourcePrepCbId;

	static void StaticResourceLoadFinished( void* pContext );
	static void StaticResourcePrepFinished( void* pContext );
	void ResourcePrepFinished();

	Tr2HostBitmapPtr	m_asyncSaveBitmap;
	std::shared_ptr<Tr2ImageHandler> m_asyncSaveImage;

	bool CreateAndCopyFromRenderTargetPython( Tr2RenderTarget* renderTarget ) { return CreateFromRT( renderTarget ); }
	bool CreateAndCopyFromRenderTargetWithSizePython( Tr2RenderTarget* renderTarget, unsigned width, unsigned height )
	{ return CreateFromRT( renderTarget, width, height ); }

	// for python
	bool CreateAndCopyFromRenderTarget( Tr2RenderTarget* renderTarget ) { return CreateFromRT( renderTarget ); }
	bool CreateAndCopyFromRenderTargetWithSize( Tr2RenderTarget* renderTarget, unsigned width, unsigned height ) { return CreateFromRT( renderTarget, width, height ); }

public:
	EXPOSE_TO_BLUE();

	/////////////////////////////////////////
	// Python thunkers
	typedef TriTextureRes _Class;

};

TYPEDEF_BLUECLASS_WR_SHUTDOWN(TriTextureRes);

#endif
