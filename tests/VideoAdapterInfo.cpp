#include "StdAfx.h"

TEST( VideoAdapterInfo, HasAtLeastOneAdapter )
{
	unsigned count = 0;
	ASSERT_HRESULT_SUCCEEDED( Tr2VideoAdapterInfo::GetAdapterCount( count ) );
	EXPECT_GT( count, 0u );
}

TEST( VideoAdapterInfo, CanGetDefaultAdapterInfo )
{
	Tr2AdapterInfo info;
	ASSERT_HRESULT_SUCCEEDED( Tr2VideoAdapterInfo::GetAdapterInfo( Tr2VideoAdapterInfo::DEFAULT_ADAPTER, info ) );
}

TEST( VideoAdapterInfo, CanGetDefaultAdapterMonitor )
{
	void* monitor;
	ASSERT_HRESULT_SUCCEEDED( Tr2VideoAdapterInfo::GetAdapterMonitor( Tr2VideoAdapterInfo::DEFAULT_ADAPTER, monitor ) );
}

TEST( VideoAdapterInfo, CanGetDefaultAdapterDisplayMode )
{
	Tr2DisplayModeInfo mode;
	memset( &mode, 0, sizeof( mode ) );
	ASSERT_HRESULT_SUCCEEDED( Tr2VideoAdapterInfo::GetAdapterDisplayMode( Tr2VideoAdapterInfo::DEFAULT_ADAPTER, mode ) );
	EXPECT_GT( mode.width, 0u );
	EXPECT_GT( mode.height, 0u );
	EXPECT_GT( mode.format, 0 );
}

TEST( VideoAdapterInfo, CanEnumerateModesForDefaultAdapter )
{
	Tr2DisplayModeInfo mode;
	memset( &mode, 0, sizeof( mode ) );
	ASSERT_HRESULT_SUCCEEDED( Tr2VideoAdapterInfo::GetAdapterDisplayMode( Tr2VideoAdapterInfo::DEFAULT_ADAPTER, mode ) );
	
	Tr2RenderContextEnum::PixelFormat backBufferFormat = mode.format;
	unsigned count = 0;
	ASSERT_HRESULT_SUCCEEDED( Tr2VideoAdapterInfo::GetAdapterModeCount( Tr2VideoAdapterInfo::DEFAULT_ADAPTER, backBufferFormat, count ) );

	EXPECT_GT( count, 0u );

	for( unsigned i = 0; i < count; ++i )
	{
		memset( &mode, 0, sizeof( mode ) );
		ASSERT_HRESULT_SUCCEEDED( Tr2VideoAdapterInfo::GetAdapterMode( Tr2VideoAdapterInfo::DEFAULT_ADAPTER, backBufferFormat, i, mode ) );
		EXPECT_GT( mode.width, 0u );
		EXPECT_GT( mode.height, 0u );
		EXPECT_GT( mode.format, 0 );
	}
}

TEST( VideoAdapterInfo, CanGetShaderVersionForDefaultAdapter )
{
	unsigned version = 0xDeadBeef;
	ASSERT_HRESULT_SUCCEEDED( Tr2VideoAdapterInfo::GetAdapterShaderVersion( Tr2VideoAdapterInfo::DEFAULT_ADAPTER, version ) );
	ASSERT_NE( version, 0xDeadBeef );
}

TEST( VideoAdapterInfo, DefaultAdapterSupportsItsCurrentBackBufferFormat )
{
	Tr2DisplayModeInfo mode;
	memset( &mode, 0, sizeof( mode ) );
	ASSERT_HRESULT_SUCCEEDED( Tr2VideoAdapterInfo::GetAdapterDisplayMode( Tr2VideoAdapterInfo::DEFAULT_ADAPTER, mode ) );
	
	EXPECT_TRUE( 
		Tr2VideoAdapterInfo::SupportsBackBufferFormat( Tr2VideoAdapterInfo::DEFAULT_ADAPTER, mode.format, false ) ||
		Tr2VideoAdapterInfo::SupportsBackBufferFormat( Tr2VideoAdapterInfo::DEFAULT_ADAPTER, mode.format, true ) );
}

TEST( VideoAdapterInfo, SameAdaptersAreNotDifferent )
{
	EXPECT_FALSE( Tr2VideoAdapterInfo::AreAdaptersDifferent( Tr2VideoAdapterInfo::DEFAULT_ADAPTER, Tr2VideoAdapterInfo::DEFAULT_ADAPTER ) );
}
