#include "StdAfx.h"
#include "WithValidRenderContextFixture.h"
#include "WithRenderContextFixture.h"

using namespace Tr2RenderContextEnum;

TEST_F( WithValidRenderContext, FenceIsInvalidBeforeCreation )
{
	Tr2FenceAL fence;
	EXPECT_FALSE( fence.IsValid() );
}

TEST_F( WithRenderContext, CreatingFenceWithoutRenderContextFails )
{
	Tr2FenceAL fence;
	ASSERT_HRESULT_FAILED( fence.Create( *renderContext ) );
	EXPECT_FALSE( fence.IsValid() );
}

TEST_F( WithValidRenderContext, FenceIsValidAfterCreation )
{
	Tr2FenceAL fence;
	ASSERT_HRESULT_SUCCEEDED( fence.Create( *renderContext ) );
	EXPECT_TRUE( fence.IsValid() );
}

TEST_F( WithValidRenderContext, FenceIsInvalidAfterDestruction )
{
	Tr2FenceAL fence;
	ASSERT_HRESULT_SUCCEEDED( fence.Create( *renderContext ) );
	EXPECT_TRUE( fence.IsValid() );
	fence.Destroy();
	EXPECT_FALSE( fence.IsValid() );
}

TEST_F( WithValidRenderContext, UsingInvalidFenceFails )
{
	Tr2FenceAL fence;

	ASSERT_HRESULT_FAILED( fence.PutFence( *renderContext ) );

	bool reached = false;
	ASSERT_HRESULT_FAILED( fence.IsReached( reached, *renderContext ) );
}

TEST_F( WithValidRenderContext, CanUseFence )
{
	Tr2FenceAL fence;
	ASSERT_HRESULT_SUCCEEDED( fence.Create( *renderContext ) );
	ASSERT_TRUE( fence.IsValid() );

	ASSERT_HRESULT_SUCCEEDED( fence.PutFence( *renderContext ) );
	bool reached = false;
	ASSERT_HRESULT_SUCCEEDED( fence.IsReached( reached, *renderContext ) );
}

TEST_F( WithValidRenderContext, FenceEqualsItself )
{
	Tr2FenceAL fence;
	ASSERT_HRESULT_SUCCEEDED( fence.Create( *renderContext ) );
	EXPECT_TRUE( fence == fence );
}

TEST_F( WithValidRenderContext, DifferentFencesAreNotEqual )
{
	Tr2FenceAL fence1;
	ASSERT_HRESULT_SUCCEEDED( fence1.Create( *renderContext ) );
	Tr2FenceAL fence2;
	ASSERT_HRESULT_SUCCEEDED( fence2.Create( *renderContext ) );
	EXPECT_FALSE( fence1 == fence2 );
}

TEST_F( WithValidRenderContext, FenceHasMemoryClass )
{
	Tr2FenceAL fence;
	ASSERT_HRESULT_SUCCEEDED( fence.Create( *renderContext ) );
	auto memoryClass = fence.GetMemoryClass();
	EXPECT_TRUE( memoryClass == AL_MEMORY_VIDEO || memoryClass == AL_MEMORY_MANAGED );
}
