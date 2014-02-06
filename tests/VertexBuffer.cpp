#include "StdAfx.h"
#include "WithValidRenderContextFixture.h"
#include "WithRenderContextFixture.h"

using namespace Tr2RenderContextEnum;

TEST_F( WithValidRenderContext, VertexBufferIsInvalidBeforeCreation )
{
	Tr2VertexBufferAL vb;
	EXPECT_FALSE( vb.IsValid() );
}

TEST_F( WithRenderContext, CreatingVertexBufferWithoutRenderContextFails )
{
	Tr2VertexBufferAL vb;
	ASSERT_HRESULT_FAILED( vb.Create( 128, *renderContext ) );
}

TEST_F( WithValidRenderContext, CreatingImmutableVertexBufferWithoutInitialDataFails )
{
	Tr2VertexBufferAL vb;
	ASSERT_HRESULT_FAILED( vb.Create( 128, USAGE_IMMUTABLE, nullptr, *renderContext ) );
}

TEST_F( WithValidRenderContext, VertexBufferIsValidAfterCreation )
{
	Tr2VertexBufferAL vb;
	ASSERT_HRESULT_SUCCEEDED( vb.Create( 128, *renderContext ) );
	EXPECT_TRUE( vb.IsValid() );
}

TEST_F( WithValidRenderContext, VertexBufferIsInvalidAfterDestruction )
{
	Tr2VertexBufferAL vb;
	ASSERT_HRESULT_SUCCEEDED( vb.Create( 128, *renderContext ) );
	vb.Destroy();
	EXPECT_FALSE( vb.IsValid() );
}

TEST_F( WithValidRenderContext, VertexBufferReportsCorrectSize )
{
	Tr2VertexBufferAL vb;
	ASSERT_HRESULT_SUCCEEDED( vb.Create( 128, *renderContext ) );
	EXPECT_EQ( 128, vb.GetTotalSizeInBytes() );
}

TEST_F( WithValidRenderContext, VertexBufferEqualsItself )
{
	Tr2VertexBufferAL vb;
	ASSERT_HRESULT_SUCCEEDED( vb.Create( 128, *renderContext ) );
	EXPECT_TRUE( vb == vb );
}

TEST_F( WithValidRenderContext, DifferentVertexBuffersAreNotEqual )
{
	Tr2VertexBufferAL vb1;
	ASSERT_HRESULT_SUCCEEDED( vb1.Create( 128, *renderContext ) );
	Tr2VertexBufferAL vb2;
	ASSERT_HRESULT_SUCCEEDED( vb2.Create( 128, *renderContext ) );
	EXPECT_FALSE( vb1 == vb2 );
}

TEST_F( WithValidRenderContext, LockingInvalidBufferFails )
{
	Tr2VertexBufferAL vb;
	void* data;
	ASSERT_HRESULT_FAILED( vb.Lock( 0, 16, &data, LOCK_READONLY, *renderContext ) );
	ASSERT_HRESULT_FAILED( vb.Lock( 0, 16, &data, LOCK_WRITEONLY, *renderContext ) );
}

TEST_F( WithValidRenderContext, CanLockVertexBufferForReading )
{
	Tr2VertexBufferAL vb;
	ASSERT_HRESULT_SUCCEEDED( vb.Create( 128, *renderContext ) );
	void* data;
	ASSERT_HRESULT_SUCCEEDED( vb.Lock( 0, 16, &data, LOCK_READONLY, *renderContext ) );
	ASSERT_HRESULT_SUCCEEDED( vb.Unlock( *renderContext ) );
}

TEST_F( WithValidRenderContext, CanLockVertexBufferForWriting )
{
	Tr2VertexBufferAL vb;
	ASSERT_HRESULT_SUCCEEDED( vb.Create( 128, *renderContext ) );
	void* data;
	ASSERT_HRESULT_SUCCEEDED( vb.Lock( 0, 16, &data, LOCK_WRITEONLY, *renderContext ) );
	ASSERT_HRESULT_SUCCEEDED( vb.Unlock( *renderContext ) );
}

TEST_F( WithValidRenderContext, CanLockDynamicVertexBufferWithNoOverwrite )
{
	Tr2VertexBufferAL vb;
	ASSERT_HRESULT_SUCCEEDED( vb.Create( 128, USAGE_CPU_WRITE | USAGE_LOCK_FREQUENTLY, nullptr, *renderContext ) );
	void* data;
	if( renderContext->GetCaps().SupportsNoOverwriteLock() )
	{
		ASSERT_HRESULT_SUCCEEDED( vb.Lock( 0, 16, &data, LOCK_NO_OVERWRITE, *renderContext ) );
		ASSERT_HRESULT_SUCCEEDED( vb.Unlock( *renderContext ) );
	}
	else
	{
		ASSERT_HRESULT_FAILED( vb.Lock( 0, 16, &data, LOCK_NO_OVERWRITE, *renderContext ) );
	}
}

TEST_F( WithValidRenderContext, CanNotLockVertexBufferWithNoOverwrite )
{
	Tr2VertexBufferAL vb;
	ASSERT_HRESULT_SUCCEEDED( vb.Create( 128, USAGE_CPU_WRITE, nullptr, *renderContext ) );
	void* data;
	ASSERT_HRESULT_FAILED( vb.Lock( 0, 16, &data, LOCK_NO_OVERWRITE, *renderContext ) );
}

TEST_F( WithValidRenderContext, VertexBufferHasMemoryClass )
{
	Tr2VertexBufferAL vb;
	ASSERT_HRESULT_SUCCEEDED( vb.Create( 128, *renderContext ) );
	auto memoryClass = vb.GetMemoryClass();
	EXPECT_TRUE( memoryClass == AL_MEMORY_VIDEO || memoryClass == AL_MEMORY_MANAGED );
}