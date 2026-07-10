// Copyright © 2026 CCP ehf.

#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <array>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <string>

#import <Cocoa/Cocoa.h>
#import <QuartzCore/QuartzCore.h>
#import <objc/objc-runtime.h>

typedef id Tr2WindowHandle;

#include <TrinityAL.h>

const char* g_moduleName = "TrinityALHelloTriangle_metal";

namespace
{
constexpr uint32_t kWindowWidth = 640;
constexpr uint32_t kWindowHeight = 480;
constexpr uint32_t kPyramidPointCount = 5;
constexpr uint32_t kPyramidTriangleCount = 6;
constexpr uint32_t kPyramidVertexCount = kPyramidTriangleCount * 3;

bool g_shouldQuit = false;

const uint8_t kPositionOnlyVS[] = {
#include "PositionOnly.vs.h"
};

const uint8_t kConstantColorPS[] = {
#include "ConstantColor.ps.h"
};

void PrintUsage( const char* executable )
{
	std::cerr << "Usage: " << executable << " [--frames N]\n";
}

bool ParseFrameCount( int argc, char** argv, int& frameCount )
{
	frameCount = -1;
	for( int i = 1; i < argc; ++i )
	{
		if( std::strcmp( argv[i], "--frames" ) == 0 )
		{
			if( i + 1 >= argc )
			{
				return false;
			}
			char* end = nullptr;
			long value = std::strtol( argv[++i], &end, 10 );
			if( end == argv[i] || *end != '\0' || value <= 0 )
			{
				return false;
			}
			frameCount = int( value );
			continue;
		}
		if( std::strcmp( argv[i], "--help" ) == 0 || std::strcmp( argv[i], "-h" ) == 0 )
		{
			PrintUsage( argv[0] );
			std::exit( 0 );
		}
		return false;
	}
	return true;
}

bool Failed( ALResult result, const char* expression )
{
	HRESULT hr = result.GetResult();
	if( SUCCEEDED( hr ) )
	{
		return false;
	}

	std::cerr << expression << " failed with HRESULT 0x" << std::hex << uint32_t( hr ) << std::dec << "\n";
	return true;
}

#define RETURN_FALSE_IF_FAILED( expression ) \
	do                                      \
	{                                       \
		if( Failed( ( expression ), #expression ) ) \
		{                                   \
			return false;                   \
		}                                   \
	} while( false )

struct DemoVertex
{
	float position[3];
	float color[4];
};

struct PyramidPoint
{
	float x;
	float y;
	float z;
};

struct PyramidFace
{
	int indices[3];
	float color[4];
	float sortDepth = 0.0f;
};

struct DemoResources
{
	Tr2ShaderAL vertexShader;
	Tr2ShaderAL pixelShader;
	Tr2ShaderProgramAL shaderProgram;
	Tr2BufferAL vertexBuffer;
	Tr2VertexLayoutAL vertexLayout;
	uint32_t vertexStride = 0;
};

PyramidPoint RotatePoint( const PyramidPoint& point, float angle )
{
	const float cosY = std::cos( angle );
	const float sinY = std::sin( angle );
	const float cosX = std::cos( 0.45f );
	const float sinX = std::sin( 0.45f );

	PyramidPoint rotated = {
		point.x * cosY + point.z * sinY,
		point.y,
		-point.x * sinY + point.z * cosY,
	};

	return {
		rotated.x,
		rotated.y * cosX - rotated.z * sinX,
		rotated.y * sinX + rotated.z * cosX,
	};
}

DemoVertex ProjectPoint( const PyramidPoint& point, const float color[4] )
{
	const float cameraZ = point.z + 2.5f;
	const float projectionScale = 1.25f / cameraZ;

	return {
		{ point.x * projectionScale, point.y * projectionScale, 0.5f },
		{ color[0], color[1], color[2], color[3] },
	};
}

void BuildPyramidVertices( float angle, std::array<DemoVertex, kPyramidVertexCount>& vertices )
{
	const std::array<PyramidPoint, kPyramidPointCount> modelPoints = { {
		{ 0.0f, 0.75f, 0.0f },
		{ -0.7f, -0.55f, -0.7f },
		{ 0.7f, -0.55f, -0.7f },
		{ 0.7f, -0.55f, 0.7f },
		{ -0.7f, -0.55f, 0.7f },
	} };

	std::array<PyramidPoint, kPyramidPointCount> transformedPoints;
	for( size_t i = 0; i < modelPoints.size(); ++i )
	{
		transformedPoints[i] = RotatePoint( modelPoints[i], angle );
	}

	std::array<PyramidFace, kPyramidTriangleCount> faces = { {
		{ { 0, 1, 2 }, { 1.0f, 0.15f, 0.12f, 1.0f } },
		{ { 0, 2, 3 }, { 0.1f, 0.65f, 1.0f, 1.0f } },
		{ { 0, 3, 4 }, { 0.95f, 0.75f, 0.12f, 1.0f } },
		{ { 0, 4, 1 }, { 0.45f, 0.9f, 0.25f, 1.0f } },
		{ { 1, 4, 3 }, { 0.45f, 0.4f, 0.5f, 1.0f } },
		{ { 1, 3, 2 }, { 0.35f, 0.3f, 0.42f, 1.0f } },
	} };

	for( auto& face : faces )
	{
		face.sortDepth = 0.0f;
		for( int index : face.indices )
		{
			face.sortDepth += transformedPoints[index].z;
		}
	}

	std::sort( faces.begin(), faces.end(), []( const PyramidFace& a, const PyramidFace& b ) {
		return a.sortDepth > b.sortDepth;
	} );

	size_t vertexIndex = 0;
	for( const auto& face : faces )
	{
		for( int index : face.indices )
		{
			vertices[vertexIndex++] = ProjectPoint( transformedPoints[index], face.color );
		}
	}
}

bool CreateResources( Tr2PrimaryRenderContextAL& renderContext, DemoResources& resources )
{
	using namespace Tr2RenderContextEnum;

	auto vertexShaderInput =
		Tr2ShaderSignatureAL()
			.Add( Tr2VertexDefinition::POSITION, 0, 0, Tr2ShaderPipelineInputAL::FLOAT, 3 )
			.Add( Tr2VertexDefinition::COLOR, 0, 1, Tr2ShaderPipelineInputAL::FLOAT, 4 );
	RETURN_FALSE_IF_FAILED(
		resources.vertexShader.Create( VERTEX_SHADER, kPositionOnlyVS, vertexShaderInput, "", renderContext ) );
	RETURN_FALSE_IF_FAILED(
		resources.pixelShader.Create( PIXEL_SHADER, kConstantColorPS, Tr2ShaderSignatureAL(), "", renderContext ) );

	Tr2ShaderAL shaders[] = { resources.vertexShader, resources.pixelShader };
	RETURN_FALSE_IF_FAILED( resources.shaderProgram.Create( shaders, 2, renderContext ) );

	std::array<DemoVertex, kPyramidVertexCount> vertices;
	BuildPyramidVertices( 0.0f, vertices );
	resources.vertexStride = sizeof( DemoVertex );
	RETURN_FALSE_IF_FAILED( resources.vertexBuffer.Create(
		resources.vertexStride,
		uint32_t( vertices.size() ),
		Tr2GpuUsage::VERTEX_BUFFER,
		Tr2CpuUsage::WRITE_OFTEN,
		vertices.data(),
		renderContext ) );

	Tr2VertexDefinition vertexDefinition;
	vertexDefinition.Add( Tr2VertexDefinition::FLOAT32_3, Tr2VertexDefinition::POSITION );
	vertexDefinition.Add( Tr2VertexDefinition::FLOAT32_4, Tr2VertexDefinition::COLOR );
	RETURN_FALSE_IF_FAILED( resources.vertexLayout.Create( vertexDefinition, renderContext ) );

	return true;
}

bool DrawFrame( Tr2PrimaryRenderContextAL& renderContext, DemoResources& resources, int frameIndex )
{
	using namespace Tr2RenderContextEnum;

	std::array<DemoVertex, kPyramidVertexCount> vertices;
	BuildPyramidVertices( frameIndex * 0.035f, vertices );
	RETURN_FALSE_IF_FAILED( resources.vertexBuffer.UpdateBuffer(
		0, uint32_t( vertices.size() * sizeof( DemoVertex ) ), vertices.data(), renderContext ) );

	RETURN_FALSE_IF_FAILED( renderContext.BeginScene() );
	RETURN_FALSE_IF_FAILED( renderContext.Clear( CLEARFLAGS_TARGET, 0xff101010, 1.0f ) );
	RETURN_FALSE_IF_FAILED( renderContext.SetStreamSource( 0, resources.vertexBuffer, 0, resources.vertexStride ) );
	RETURN_FALSE_IF_FAILED( renderContext.SetVertexLayout( resources.vertexLayout ) );
	RETURN_FALSE_IF_FAILED( renderContext.SetShaderProgram( resources.shaderProgram ) );
	RETURN_FALSE_IF_FAILED( renderContext.SetTopology( TOP_TRIANGLES ) );
	RETURN_FALSE_IF_FAILED( renderContext.SetRenderState( RS_ZENABLE, 0 ) );
	RETURN_FALSE_IF_FAILED( renderContext.SetRenderState( RS_CULLMODE, CULLMODE_NONE ) );
	RETURN_FALSE_IF_FAILED( renderContext.DrawPrimitive( 0, kPyramidTriangleCount ) );
	RETURN_FALSE_IF_FAILED( renderContext.EndScene() );
	RETURN_FALSE_IF_FAILED( renderContext.Present() );

	return true;
}

bool CreatePresentParameters( Tr2WindowHandle windowHandle, Tr2PresentParametersAL& presentParameters )
{
	presentParameters = {};
	RETURN_FALSE_IF_FAILED( Tr2VideoAdapterInfo::GetAdapterDisplayMode( Tr2VideoAdapterInfo::DEFAULT_ADAPTER, presentParameters.mode ) );
	presentParameters.mode.width = kWindowWidth;
	presentParameters.mode.height = kWindowHeight;
	presentParameters.mode.format = Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_UNORM;
	presentParameters.backBufferCount = 1;
	presentParameters.msaaType = 0;
	presentParameters.msaaQuality = 0;
	presentParameters.swapEffect = Tr2RenderContextEnum::SWAP_EFFECT_DISCARD;
	presentParameters.outputWindow = windowHandle;
	presentParameters.windowed = true;
	presentParameters.software = false;
	presentParameters.presentInterval = Tr2RenderContextEnum::PRESENT_INTERVAL_ONE;
	presentParameters.variableRefreshRateSupported = false;
	return true;
}

void ProcessEvents( NSWindow* window )
{
	while( true )
	{
		NSEvent* event = [NSApp nextEventMatchingMask:NSEventMaskAny
											untilDate:[NSDate distantPast]
											   inMode:NSDefaultRunLoopMode
											  dequeue:YES];
		if( event == nil )
		{
			break;
		}

		if( event.type == NSEventTypeKeyDown && event.keyCode == 53 )
		{
			g_shouldQuit = true;
			[window close];
			continue;
		}

		[NSApp sendEvent:event];
	}
}
}

@interface HelloTriangleWindowDelegate : NSObject <NSWindowDelegate>
@end

@implementation HelloTriangleWindowDelegate
- (BOOL)windowShouldClose:(id)sender
{
	(void)sender;
	g_shouldQuit = true;
	return YES;
}
@end

@interface HelloTriangleContentView : NSView
@end

@implementation HelloTriangleContentView
- (CALayer*)makeBackingLayer
{
	return [CAMetalLayer layer];
}

- (BOOL)wantsLayer
{
	return YES;
}

- (BOOL)canBecomeKeyView
{
	return YES;
}

- (BOOL)acceptsFirstResponder
{
	return YES;
}
@end

int main( int argc, char** argv )
{
	@autoreleasepool
	{
		int maxFrames = -1;
		if( !ParseFrameCount( argc, argv, maxFrames ) )
		{
			PrintUsage( argv[0] );
			return 2;
		}

		[NSApplication sharedApplication];
		[NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

		NSRect frame = NSMakeRect( 0, 0, kWindowWidth, kWindowHeight );
		NSWindow* window = [[NSWindow alloc] initWithContentRect:frame
													   styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable
														 backing:NSBackingStoreBuffered
														   defer:NO];
		[window setTitle:@"TrinityAL Hello Pyramid"];
		[window center];

		HelloTriangleContentView* view = [[HelloTriangleContentView alloc] initWithFrame:frame];
		view.wantsLayer = YES;
		[window setContentView:view];
		[window makeFirstResponder:view];

		HelloTriangleWindowDelegate* delegate = [[HelloTriangleWindowDelegate alloc] init];
		[window setDelegate:delegate];
		[window makeKeyAndOrderFront:nil];
		[NSApp activateIgnoringOtherApps:YES];

		Tr2PrimaryRenderContextAL renderContext;
		Tr2PrimaryRenderContextAL::SetPrimaryRenderContext( &renderContext );

		Tr2WindowHandle windowHandle = [window contentView];
		Tr2PresentParametersAL presentParameters;
		if( !CreatePresentParameters( windowHandle, presentParameters ) )
		{
			Tr2PrimaryRenderContextAL::SetPrimaryRenderContext( nullptr );
			return 1;
		}
		if( Failed( renderContext.CreateDevice( 0, windowHandle, presentParameters ), "renderContext.CreateDevice" ) )
		{
			Tr2PrimaryRenderContextAL::SetPrimaryRenderContext( nullptr );
			return 1;
		}

		DemoResources resources;
		if( !CreateResources( renderContext, resources ) )
		{
			Tr2PrimaryRenderContextAL::SetPrimaryRenderContext( nullptr );
			return 1;
		}

		int renderedFrames = 0;
		while( !g_shouldQuit && ( maxFrames < 0 || renderedFrames < maxFrames ) )
		{
			@autoreleasepool
			{
				ProcessEvents( window );
				if( g_shouldQuit )
				{
					break;
				}
				if( !DrawFrame( renderContext, resources, renderedFrames ) )
				{
					Tr2PrimaryRenderContextAL::SetPrimaryRenderContext( nullptr );
					return 1;
				}
				++renderedFrames;
			}
		}

		Failed( renderContext.SetStreamSource( 0, Tr2BufferAL(), 0, 0 ), "renderContext.SetStreamSource" );
		Failed( renderContext.SetShaderProgram( Tr2ShaderProgramAL() ), "renderContext.SetShaderProgram" );
		Tr2PrimaryRenderContextAL::SetPrimaryRenderContext( nullptr );
		[window close];
	}

	return 0;
}
