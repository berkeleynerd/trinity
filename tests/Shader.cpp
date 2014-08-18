#include "StdAfx.h"
#include "WithValidRenderContextFixture.h"
#include "WithRenderContextFixture.h"

using namespace Tr2RenderContextEnum;

TEST_F( WithValidRenderContext, ShaderIsInvalidBeforeCreation )
{
	Tr2ShaderAL s;
	EXPECT_FALSE( s.IsValid() );
}

TEST_F( WithValidRenderContext, CanCreateShader )
{
	uint32_t vsBytecode[] = {
#include INCLUDE_SHADER_CODE( PositionOnly.vs )
	};

	Tr2ShaderInputDefinition vsInput;
	vsInput.elements.resize( 1 );
	vsInput.elements[0].usage = Tr2VertexDefinition::POSITION;
	vsInput.elements[0].usageIndex = 0;
	vsInput.ComputeHash();

	Tr2ShaderAL vs;
	ASSERT_HRESULT_SUCCEEDED( vs.Create( 
		*renderContext, 
		Tr2RenderContextEnum::VERTEX_SHADER,
		vsBytecode,
		sizeof( vsBytecode ),
		nullptr,
		0,
		vsInput ) );

	EXPECT_TRUE( vs.IsValid() );
	EXPECT_EQ( VERTEX_SHADER, vs.GetType() );

	const void* bytecode;
	uint32_t size;
	ASSERT_HRESULT_SUCCEEDED( vs.GetBytecode( bytecode, size ) );
	EXPECT_EQ( sizeof( vsBytecode ), size );
	EXPECT_EQ( 0, memcmp( bytecode, vsBytecode, size ) );

	auto memoryClass = vs.GetMemoryClass();
	EXPECT_TRUE( memoryClass == AL_MEMORY_VIDEO || memoryClass == AL_MEMORY_MANAGED );
}

TEST_F( WithValidRenderContext, ShaderEqualsItself )
{
	uint32_t vsBytecode[] = {
#include INCLUDE_SHADER_CODE( PositionOnly.vs )
	};

	Tr2ShaderInputDefinition vsInput;
	vsInput.elements.resize( 1 );
	vsInput.elements[0].usage = Tr2VertexDefinition::POSITION;
	vsInput.elements[0].usageIndex = 0;
	vsInput.ComputeHash();

	Tr2ShaderAL vs;
	ASSERT_HRESULT_SUCCEEDED( vs.Create( 
		*renderContext, 
		Tr2RenderContextEnum::VERTEX_SHADER,
		vsBytecode,
		sizeof( vsBytecode ),
		nullptr,
		0,
		vsInput ) );

	EXPECT_TRUE( vs == vs );
}

TEST_F( WithValidRenderContext, DifferentShadersAreNotEqual )
{
	uint32_t vsBytecode1[] = {
#include INCLUDE_SHADER_CODE( PositionOnly.vs )
	};

	Tr2ShaderInputDefinition vsInput;
	vsInput.elements.resize( 1 );
	vsInput.elements[0].usage = Tr2VertexDefinition::POSITION;
	vsInput.elements[0].usageIndex = 0;
	vsInput.ComputeHash();

	Tr2ShaderAL vs1;
	ASSERT_HRESULT_SUCCEEDED( vs1.Create( 
		*renderContext, 
		Tr2RenderContextEnum::VERTEX_SHADER,
		vsBytecode1,
		sizeof( vsBytecode1 ),
		nullptr,
		0,
		vsInput ) );

	uint32_t vsBytecode2[] = {
#include INCLUDE_SHADER_CODE( PositionOnlyWithPerObjectData.vs )
	};

	Tr2ShaderAL vs2;
	ASSERT_HRESULT_SUCCEEDED( vs2.Create( 
		*renderContext, 
		Tr2RenderContextEnum::VERTEX_SHADER,
		vsBytecode2,
		sizeof( vsBytecode2 ),
		nullptr,
		0,
		vsInput ) );

	EXPECT_FALSE( vs1 == vs2 );
}

#if( TRINITYPLATFORM == TRINITY_STUB )
TEST_F( WithValidRenderContext, ShaderStoresType )
{
	uint32_t vsBytecode[] = {
#include INCLUDE_SHADER_CODE( PositionOnly.vs )
	};

	Tr2ShaderInputDefinition vsInput;
	vsInput.elements.resize( 1 );
	vsInput.elements[0].usage = Tr2VertexDefinition::POSITION;
	vsInput.elements[0].usageIndex = 0;
	vsInput.ComputeHash();

	Tr2ShaderAL vs;
	ASSERT_HRESULT_SUCCEEDED( vs.Create( 
		*renderContext, 
		Tr2RenderContextEnum::HULL_SHADER,
		vsBytecode,
		sizeof( vsBytecode ),
		nullptr,
		0,
		vsInput ) );

	Tr2RenderContextEnum::ShaderType expected = Tr2RenderContextEnum::HULL_SHADER;
	int actual = vs.GetType();
	EXPECT_EQ( expected, actual  );
}
#endif