#include "StdAfx.h"
#include "WithValidRenderContextFixture.h"
#include "WithRenderContextFixture.h"

#if TRACK_AL_RESOURCES

using namespace Tr2RenderContextEnum;

namespace
{
    class LiveResourceCount
    {
    public:
        LiveResourceCount()
        :   m_count( 0 )
        {
        }
        
        void operator ()( Tr2RenderContextEnum::ObjectType, const char*, const void*, const std::map<std::string, uint32_t>& )
        {
            ++m_count;
        }
        
        uint32_t GetCount() const
        {
            return m_count;
        }
    private:
        uint32_t m_count;
    };
    
    uint32_t GetTotalLiveCount()
    {
        LiveResourceCount count;
        Tr2TrackedALObjectBase::GetAllObjectDescriptions( AL_MEMORY_MANAGED | AL_MEMORY_VIDEO, count );
        return count.GetCount();
    }
    
    template <typename T>
    void CheckInvalidObjectDoesNotCountAsLive()
    {
        uint32_t initialCount = GetTotalLiveCount();
        T obj;
        uint32_t newCount = GetTotalLiveCount();
        
        EXPECT_EQ( initialCount, newCount );
    }
}

TEST_F( WithRenderContext, NoValidResourcesWithoutRenderContext )
{
    uint32_t count = GetTotalLiveCount();
	EXPECT_EQ( 0, count );
}

TEST_F( WithValidRenderContext, InvalidConstantBufferDoestNotCountAsLive )
{
    CheckInvalidObjectDoesNotCountAsLive<Tr2ConstantBufferAL>();
}

TEST_F( WithValidRenderContext, ValidConstantBufferCountsAsLive )
{
    uint32_t initialCount = GetTotalLiveCount();
	Tr2ConstantBufferAL cb;
	ASSERT_HRESULT_SUCCEEDED( cb.Create( 128, *renderContext ) );
    uint32_t newCount = GetTotalLiveCount();
    
	EXPECT_LT( initialCount, newCount );
}

TEST_F( WithValidRenderContext, InvalidDepthStencilDoestNotCountAsLive )
{
    CheckInvalidObjectDoesNotCountAsLive<Tr2DepthStencilAL>();
}

TEST_F( WithValidRenderContext, ValidDepthStencilCountsAsLive )
{
    uint32_t initialCount = GetTotalLiveCount();
	Tr2DepthStencilAL ds;
	ASSERT_HRESULT_SUCCEEDED( ds.Create( 128, 64, DSFMT_D24S8, 1, 0, *renderContext ) );
    uint32_t newCount = GetTotalLiveCount();
    
	EXPECT_LT( initialCount, newCount );
}

TEST_F( WithValidRenderContext, InvalidIndexBufferDoestNotCountAsLive )
{
    CheckInvalidObjectDoesNotCountAsLive<Tr2IndexBufferAL>();
}

TEST_F( WithValidRenderContext, ValidIndexBufferCountsAsLive )
{
    uint32_t initialCount = GetTotalLiveCount();
	Tr2IndexBufferAL ib;
	ASSERT_HRESULT_SUCCEEDED( ib.Create( 128, 0, Tr2RenderContextEnum::IB_16BIT, nullptr, *renderContext ) );
    uint32_t newCount = GetTotalLiveCount();
    
	EXPECT_LT( initialCount, newCount );
}

TEST_F( WithValidRenderContext, InvalidRenderTargetDoestNotCountAsLive )
{
    CheckInvalidObjectDoesNotCountAsLive<Tr2RenderTargetAL>();
}

TEST_F( WithValidRenderContext, ValidRenderTargetCountsAsLive )
{
    uint32_t initialCount = GetTotalLiveCount();
	Tr2RenderTargetAL rt;
	ASSERT_HRESULT_SUCCEEDED( rt.Create( 128, 64, 1, PIXEL_FORMAT_B8G8R8A8_UNORM, 1, 0, *renderContext ) );
    uint32_t newCount = GetTotalLiveCount();
    
	EXPECT_LT( initialCount, newCount );
}

TEST_F( WithValidRenderContext, InvalidSamplerStateDoestNotCountAsLive )
{
    CheckInvalidObjectDoesNotCountAsLive<Tr2SamplerStateAL>();
}

TEST_F( WithValidRenderContext, ValidSamplerStateCountsAsLive )
{
    uint32_t initialCount = GetTotalLiveCount();
	Tr2SamplerStateAL ss;
	float borderColor[] = { 0.1f, 0.2f, 0.3f, 0.4f };
	Tr2SamplerDescription desc(
                               TF_ANISOTROPIC,
                               TF_LINEAR,
                               TF_POINT,
                               false,
                               TA_WRAP,
                               TA_CLAMP,
                               TA_BORDER,
                               0.5f,
                               4,
                               CMP_ALWAYS,
                               borderColor,
                               0.1f,
                               3.2f,
                               false );
    ASSERT_HRESULT_SUCCEEDED( ss.Create( *renderContext, desc ) );
    uint32_t newCount = GetTotalLiveCount();
    
	EXPECT_LT( initialCount, newCount );
}

TEST_F( WithValidRenderContext, InvalidTextureDoestNotCountAsLive )
{
    CheckInvalidObjectDoesNotCountAsLive<Tr2TextureAL>();
}

TEST_F( WithValidRenderContext, ValidTextureCountsAsLive )
{
    uint32_t initialCount = GetTotalLiveCount();
    Tr2TextureAL texture;
	ASSERT_HRESULT_SUCCEEDED( texture.Create2D( 128, 128, 1, PIXEL_FORMAT_B8G8R8A8_UNORM, 0, nullptr, *renderContext ) );
    uint32_t newCount = GetTotalLiveCount();
    
	EXPECT_LT( initialCount, newCount );
}

TEST_F( WithValidRenderContext, InvalidVertexBufferDoestNotCountAsLive )
{
    CheckInvalidObjectDoesNotCountAsLive<Tr2VertexBufferAL>();
}

TEST_F( WithValidRenderContext, ValidVertexBufferCountsAsLive )
{
    uint32_t initialCount = GetTotalLiveCount();
	Tr2VertexBufferAL vb;
	ASSERT_HRESULT_SUCCEEDED( vb.Create( 128, *renderContext ) );
    uint32_t newCount = GetTotalLiveCount();
    
	EXPECT_LT( initialCount, newCount );
}

TEST_F( WithValidRenderContext, InvalidVertexLayoutDoestNotCountAsLive )
{
    CheckInvalidObjectDoesNotCountAsLive<Tr2VertexLayoutAL>();
}

TEST_F( WithValidRenderContext, ValidVertexLayoutCountsAsLive )
{
    uint32_t initialCount = GetTotalLiveCount();
	Tr2VertexLayoutAL layout;
	Tr2VertexDefinition def;
	def.Add( Tr2VertexDefinition::FLOAT32_3, Tr2VertexDefinition::POSITION );
	ASSERT_HRESULT_SUCCEEDED( layout.Create( def, *renderContext ) );
    uint32_t newCount = GetTotalLiveCount();
    
	EXPECT_LT( initialCount, newCount );
}

TEST_F( WithValidRenderContext, InvalidOcclusionQueryDoestNotCountAsLive )
{
    CheckInvalidObjectDoesNotCountAsLive<Tr2OcclusionQueryAL>();
}

TEST_F( WithValidRenderContext, ValidOcclusionQueryCountsAsLive )
{
    uint32_t initialCount = GetTotalLiveCount();
	Tr2OcclusionQueryAL query;
	if( SUCCEEDED( query.Create( *renderContext ) ) )
    {
        uint32_t newCount = GetTotalLiveCount();
        EXPECT_LT( initialCount, newCount );
    }
}

TEST_F( WithValidRenderContext, InvalidSwapChainDoestNotCountAsLive )
{
    CheckInvalidObjectDoesNotCountAsLive<Tr2SwapChainAL>();
}

TEST_F( WithValidRenderContext, InvalidGpuBufferDoestNotCountAsLive )
{
    CheckInvalidObjectDoesNotCountAsLive<Tr2GpuBufferAL>();
}

TEST_F( WithValidRenderContext, ValidGpuBufferCountsAsLive )
{
    uint32_t initialCount = GetTotalLiveCount();
	Tr2GpuBufferAL buffer;
	if( SUCCEEDED( buffer.Create( 64, PIXEL_FORMAT_R32_FLOAT, 0, nullptr, *renderContext ) ) )
    {
        uint32_t newCount = GetTotalLiveCount();
        EXPECT_LT( initialCount, newCount );
    }
}

TEST_F( WithValidRenderContext, InvalidFenceDoestNotCountAsLive )
{
    CheckInvalidObjectDoesNotCountAsLive<Tr2FenceAL>();
}

TEST_F( WithValidRenderContext, ValidFenceCountsAsLive )
{
    uint32_t initialCount = GetTotalLiveCount();
	Tr2FenceAL fence;
	if( SUCCEEDED( fence.Create( *renderContext ) ) )
    {
        uint32_t newCount = GetTotalLiveCount();
        EXPECT_LT( initialCount, newCount );
    }
}


#endif