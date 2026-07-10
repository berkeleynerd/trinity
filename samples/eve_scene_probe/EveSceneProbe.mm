// Copyright (c) 2026 CCP Games

#import <Cocoa/Cocoa.h>
#import <ImageIO/ImageIO.h>
#import <QuartzCore/CAMetalLayer.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>

#include <algorithm>
#include <cctype>
#include <climits>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

extern "C" bool TrinityStandaloneProbeStartup( int argc, const char* const* argv, const char* executableDirectory );
extern "C" void* TrinityStandaloneProbeCreateDevice( void* windowHandle, uint32_t renderWidth, uint32_t renderHeight, int shaderTier );
extern "C" void TrinityStandaloneProbeDestroyDevice( void* opaqueProbe );
extern "C" bool TrinityStandaloneProbeInspectClientAssets( void* opaqueProbe, const char* reportPath );
extern "C" bool TrinityStandaloneProbeCreateEveScene( void* opaqueProbe, int qualityRung, const char* assetPath, int materialView, int materialMode, int areaView, const char* sceneResourcePath, int sceneFixture, int lightingView, int shSource, int localLights );
extern "C" bool TrinityStandaloneProbeRenderFrame( void* opaqueProbe, int qualityRung, int64_t realTime, int64_t simTime );

namespace
{

constexpr uint32_t kDefaultWindowWidth = 640;
constexpr uint32_t kDefaultWindowHeight = 480;
constexpr int64_t kFrameTime = 166667;

bool g_shouldQuit = false;

enum class QualityRung
{
	Shell,
	Scene,
	Model,
	HdrBlit,
	HdrPost,
	HdrExposure,
};

enum class MaterialView
{
	Lit,
	BaseColor,
	Normal,
	Roughness,
	Material,
	Glow,
	Dirt,
	Mask,
	Paint,
};

enum class MaterialMode
{
	Probe,
	EveV5,
};

enum class AreaView
{
	All,
	Distortion,
	Hull,
	Booster,
};

enum class SceneFixture
{
	Empty,
	Fitting,
	A01,
	NewEden,
};

enum class LightingView
{
	Combined,
	Direct,
	Sh,
	Local,
};

enum class LocalLights
{
	Off,
	Authored,
	Validation,
};

enum class ShaderTier
{
	Medium,
	High,
};

enum class ShSource
{
	NewEdenPlanet,
	Validation,
	None,
};

struct Options
{
	std::string asset = "astero";
	std::string inputPath;
	std::string capturePrefix;
	std::string inspectionReportPath;
	QualityRung qualityRung = QualityRung::Model;
	MaterialView materialView = MaterialView::Lit;
	MaterialMode materialMode = MaterialMode::Probe;
	AreaView areaView = AreaView::All;
	SceneFixture sceneFixture = SceneFixture::NewEden;
	LightingView lightingView = LightingView::Combined;
	ShSource shSource = ShSource::NewEdenPlanet;
	LocalLights localLights = LocalLights::Off;
	ShaderTier shaderTier = ShaderTier::High;
	int maxFrames = -1;
	uint32_t windowWidth = kDefaultWindowWidth;
	uint32_t windowHeight = kDefaultWindowHeight;
	bool windowed = false;
	bool materialModeExplicit = false;
	bool localLightsExplicit = false;
};

std::string ToLower( std::string value );

std::string AreaViewName( AreaView view )
{
	switch( view )
	{
	case AreaView::All:
		return "all";
	case AreaView::Distortion:
		return "distortion";
	case AreaView::Hull:
		return "hull";
	case AreaView::Booster:
		return "booster";
	}
	return "unknown";
}

bool ParseAreaView( const std::string& value, AreaView& view )
{
	const std::string normalized = ToLower( value );
	const AreaView values[] = { AreaView::All, AreaView::Distortion, AreaView::Hull, AreaView::Booster };
	for( AreaView candidate : values )
	{
		if( normalized == AreaViewName( candidate ) )
		{
			view = candidate;
			return true;
		}
	}
	return false;
}

bool ParseSceneFixture( const std::string& value, SceneFixture& fixture )
{
	const std::string lower = ToLower( value );
	if( lower == "empty" )
	{
		fixture = SceneFixture::Empty;
		return true;
	}
	if( lower == "fitting" )
	{
		fixture = SceneFixture::Fitting;
		return true;
	}
	if( lower == "a01" )
	{
		fixture = SceneFixture::A01;
		return true;
	}
	if( lower == "new-eden" || lower == "neweden" )
	{
		fixture = SceneFixture::NewEden;
		return true;
	}
	return false;
}

std::string LightingViewName( LightingView view )
{
	switch( view )
	{
	case LightingView::Combined:
		return "combined";
	case LightingView::Direct:
		return "direct";
	case LightingView::Sh:
		return "sh";
	case LightingView::Local:
		return "local";
	}
	return "unknown";
}

bool ParseLightingView( const std::string& value, LightingView& view )
{
	const std::string normalized = ToLower( value );
	const LightingView values[] = { LightingView::Combined, LightingView::Direct, LightingView::Sh, LightingView::Local };
	for( LightingView candidate : values )
	{
		if( normalized == LightingViewName( candidate ) )
		{
			view = candidate;
			return true;
		}
	}
	return false;
}

std::string LocalLightsName( LocalLights mode )
{
	switch( mode )
	{
	case LocalLights::Off:
		return "off";
	case LocalLights::Authored:
		return "authored";
	case LocalLights::Validation:
		return "validation";
	}
	return "unknown";
}

bool ParseLocalLights( const std::string& value, LocalLights& mode )
{
	const std::string normalized = ToLower( value );
	const LocalLights values[] = { LocalLights::Off, LocalLights::Authored, LocalLights::Validation };
	for( LocalLights candidate : values )
	{
		if( normalized == LocalLightsName( candidate ) )
		{
			mode = candidate;
			return true;
		}
	}
	return false;
}

std::string ShaderTierName( ShaderTier tier )
{
	return tier == ShaderTier::High ? "high" : "medium";
}

bool ParseShaderTier( const std::string& value, ShaderTier& tier )
{
	const std::string normalized = ToLower( value );
	if( normalized == "medium" || normalized == "hi" || normalized == "sm_3_0_hi" )
	{
		tier = ShaderTier::Medium;
		return true;
	}
	if( normalized == "high" || normalized == "depth" || normalized == "sm_3_0_depth" )
	{
		tier = ShaderTier::High;
		return true;
	}
	return false;
}

std::string ShSourceName( ShSource source )
{
	switch( source )
	{
	case ShSource::NewEdenPlanet:
		return "new-eden-planet";
	case ShSource::Validation:
		return "validation";
	case ShSource::None:
		return "none";
	}
	return "unknown";
}

bool ParseShSource( const std::string& value, ShSource& source )
{
	const std::string normalized = ToLower( value );
	const ShSource values[] = { ShSource::NewEdenPlanet, ShSource::Validation, ShSource::None };
	for( ShSource candidate : values )
	{
		if( normalized == ShSourceName( candidate ) )
		{
			source = candidate;
			return true;
		}
	}
	return false;
}

const char* SceneResourcePath( SceneFixture fixture )
{
	switch( fixture )
	{
	case SceneFixture::Empty:
		return "";
	case SceneFixture::Fitting:
		return "res:/dx9/scene/fitting/fitting.black";
	case SceneFixture::A01:
	case SceneFixture::NewEden:
		return "res:/dx9/scene/universe/a01_cube.black";
	}
	return "";
}

int SceneFixtureApiValue( SceneFixture fixture )
{
	return static_cast<int>( fixture );
}

std::string ToLower( std::string value )
{
	std::transform( value.begin(), value.end(), value.begin(), []( unsigned char c ) { return static_cast<char>( std::tolower( c ) ); } );
	return value;
}

std::string MaterialModeName( MaterialMode mode )
{
	return mode == MaterialMode::EveV5 ? "eve-v5" : "probe";
}

bool ParseMaterialMode( const std::string& value, MaterialMode& mode )
{
	const std::string normalized = ToLower( value );
	if( normalized == "probe" )
	{
		mode = MaterialMode::Probe;
		return true;
	}
	if( normalized == "eve-v5" || normalized == "v5" || normalized == "quadv5" )
	{
		mode = MaterialMode::EveV5;
		return true;
	}
	return false;
}

std::string MaterialViewName( MaterialView view )
{
	switch( view )
	{
	case MaterialView::Lit:
		return "lit";
	case MaterialView::BaseColor:
		return "basecolor";
	case MaterialView::Normal:
		return "normal";
	case MaterialView::Roughness:
		return "roughness";
	case MaterialView::Material:
		return "material";
	case MaterialView::Glow:
		return "glow";
	case MaterialView::Dirt:
		return "d";
	case MaterialView::Mask:
		return "mask";
	case MaterialView::Paint:
		return "p3";
	}
	return "unknown";
}

bool ParseMaterialView( const std::string& value, MaterialView& view )
{
	const std::string normalized = ToLower( value );
	const MaterialView values[] = {
		MaterialView::Lit,
		MaterialView::BaseColor,
		MaterialView::Normal,
		MaterialView::Roughness,
		MaterialView::Material,
		MaterialView::Glow,
		MaterialView::Dirt,
		MaterialView::Mask,
		MaterialView::Paint,
	};
	for( MaterialView candidate : values )
	{
		if( normalized == MaterialViewName( candidate ) )
		{
			view = candidate;
			return true;
		}
	}
	if( normalized == "dirt" )
	{
		view = MaterialView::Dirt;
		return true;
	}
	if( normalized == "paint" )
	{
		view = MaterialView::Paint;
		return true;
	}
	return false;
}
std::string QualityRungName( QualityRung rung )
{
	switch( rung )
	{
	case QualityRung::Shell:
		return "shell";
	case QualityRung::Scene:
		return "scene";
	case QualityRung::Model:
		return "model";
	case QualityRung::HdrBlit:
		return "hdr-blit";
	case QualityRung::HdrPost:
		return "hdr-post";
	case QualityRung::HdrExposure:
		return "hdr-exposure";
	}
	return "unknown";
}

int QualityRungApiValue( QualityRung rung )
{
	switch( rung )
	{
	case QualityRung::Shell:
		return 0;
	case QualityRung::Scene:
		return 2;
	case QualityRung::Model:
		return 3;
	case QualityRung::HdrBlit:
		return 4;
	case QualityRung::HdrPost:
		return 5;
	case QualityRung::HdrExposure:
		return 6;
	}
	return 0;
}

bool ParseQualityRung( const std::string& value, QualityRung& rung )
{
	const std::string normalized = ToLower( value );
	if( normalized == "shell" || normalized == "rung1" || normalized == "1" )
	{
		rung = QualityRung::Shell;
		return true;
	}
	if( normalized == "scene" || normalized == "empty-scene" || normalized == "rung2" || normalized == "2" )
	{
		rung = QualityRung::Scene;
		return true;
	}
	if( normalized == "model" || normalized == "synthetic-model" || normalized == "rung3" || normalized == "3" )
	{
		rung = QualityRung::Model;
		return true;
	}
	if( normalized == "hdr-blit" || normalized == "blit" )
	{
		rung = QualityRung::HdrBlit;
		return true;
	}
	if( normalized == "hdr" || normalized == "hdr-post" || normalized == "post" || normalized == "rung4" || normalized == "4" )
	{
		rung = QualityRung::HdrPost;
		return true;
	}
	if( normalized == "hdr-exposure" || normalized == "exposure" )
	{
		rung = QualityRung::HdrExposure;
		return true;
	}
	return false;
}

bool ParseUnsigned( const std::string& text, uint32_t& value )
{
	char* end = nullptr;
	unsigned long parsed = std::strtoul( text.c_str(), &end, 10 );
	if( !end || *end != '\0' || parsed == 0 || parsed > UINT32_MAX )
	{
		return false;
	}
	value = static_cast<uint32_t>( parsed );
	return true;
}

bool ParseWindowSize( const std::string& text, uint32_t& width, uint32_t& height )
{
	size_t separator = text.find( 'x' );
	if( separator == std::string::npos )
	{
		separator = text.find( 'X' );
	}
	if( separator == std::string::npos )
	{
		return false;
	}

	return ParseUnsigned( text.substr( 0, separator ), width ) &&
		ParseUnsigned( text.substr( separator + 1 ), height );
}

std::string ExecutableDirectory()
{
	NSString* executablePath = [[NSBundle mainBundle] executablePath];
	NSString* executableDirectory = [executablePath stringByDeletingLastPathComponent];
	return executableDirectory.UTF8String ? executableDirectory.UTF8String : ".";
}

std::string DefaultAssetPath( const Options& options )
{
	const std::string executableDirectory = ExecutableDirectory();
	if( options.asset == "fox" )
	{
		return executableDirectory + "/Assets/Fox.cmf";
	}
	if( options.asset == "astero" )
	{
		return executableDirectory + "/Assets/Astero.cmf";
	}
	return executableDirectory + "/Assets/11989_lite.cmf";
}

bool FileExists( const std::string& path )
{
	std::ifstream file( path, std::ios::binary );
	return file.good();
}

void PrintUsage( const char* executable )
{
	std::cerr
		<< "Usage: " << executable << " [--asset astero|ship|fox] [--input PATH] [--frames N]\n"
		<< "       [--windowed WxH] [--quality-rung shell|scene|model|hdr-blit|hdr-post|hdr-exposure]\n"
		<< "       [--material-mode probe|eve-v5]\n"
		<< "       [--area-view all|hull|booster|distortion]\n"
		<< "       [--scene-fixture empty|fitting|a01|new-eden]\n"
		<< "       [--lighting-view combined|direct|sh|local] [--sh-source new-eden-planet|validation|none]\n"
		<< "       [--local-lights off|authored|validation]\n"
		<< "       [--shader-tier medium|high]\n"
		<< "       [--material-view lit|basecolor|normal|roughness|material|glow|d|mask|p3]\n"
		<< "       [--capture-prefix PATH] [--inspect-client-assets REPORT.md]\n";
}

bool ParseArgs( int argc, char** argv, Options& options )
{
	for( int i = 1; i < argc; ++i )
	{
		const std::string arg = argv[i];
		if( arg == "--asset" )
		{
			if( ++i >= argc )
			{
				return false;
			}
			options.asset = ToLower( argv[i] );
			if( options.asset != "astero" && options.asset != "ship" && options.asset != "fox" )
			{
				return false;
			}
		}
		else if( arg == "--input" )
		{
			if( ++i >= argc )
			{
				return false;
			}
			options.inputPath = argv[i];
		}
		else if( arg == "--frames" )
		{
			if( ++i >= argc )
			{
				return false;
			}
			char* end = nullptr;
			long parsed = std::strtol( argv[i], &end, 10 );
			if( !end || *end != '\0' || parsed < 0 || parsed > INT32_MAX )
			{
				return false;
			}
			options.maxFrames = static_cast<int>( parsed );
		}
		else if( arg == "--windowed" )
		{
			if( ++i >= argc )
			{
				return false;
			}
			if( !ParseWindowSize( argv[i], options.windowWidth, options.windowHeight ) )
			{
				return false;
			}
			options.windowed = true;
		}
		else if( arg == "--quality-rung" )
		{
			if( ++i >= argc )
			{
				return false;
			}
			if( !ParseQualityRung( argv[i], options.qualityRung ) )
			{
				return false;
			}
		}
		else if( arg == "--capture-prefix" )
		{
			if( ++i >= argc )
			{
				return false;
			}
			options.capturePrefix = argv[i];
		}
		else if( arg == "--material-view" )
		{
			if( ++i >= argc || !ParseMaterialView( argv[i], options.materialView ) )
			{
				return false;
			}
		}
		else if( arg == "--material-mode" )
		{
			if( ++i >= argc || !ParseMaterialMode( argv[i], options.materialMode ) )
			{
				return false;
			}
			options.materialModeExplicit = true;
		}
		else if( arg == "--area-view" )
		{
			if( ++i >= argc || !ParseAreaView( argv[i], options.areaView ) )
			{
				return false;
			}
		}
		else if( arg == "--scene-fixture" )
		{
			if( ++i >= argc || !ParseSceneFixture( argv[i], options.sceneFixture ) )
			{
				return false;
			}
		}
		else if( arg == "--lighting-view" )
		{
			if( ++i >= argc || !ParseLightingView( argv[i], options.lightingView ) )
			{
				return false;
			}
		}
		else if( arg == "--sh-source" )
		{
			if( ++i >= argc || !ParseShSource( argv[i], options.shSource ) )
			{
				return false;
			}
		}
		else if( arg == "--local-lights" )
		{
			if( ++i >= argc || !ParseLocalLights( argv[i], options.localLights ) )
			{
				return false;
			}
			options.localLightsExplicit = true;
		}
		else if( arg == "--shader-tier" )
		{
			if( ++i >= argc || !ParseShaderTier( argv[i], options.shaderTier ) )
			{
				return false;
			}
		}
		else if( arg == "--inspect-client-assets" )
		{
			if( ++i >= argc )
			{
				return false;
			}
			options.inspectionReportPath = argv[i];
		}
		else
		{
			return false;
		}
	}

	if( options.inputPath.empty() )
	{
		options.inputPath = DefaultAssetPath( options );
	}
	if( !options.materialModeExplicit && options.asset == "astero" )
	{
		options.materialMode = MaterialMode::EveV5;
	}
	if( !options.localLightsExplicit && options.asset == "astero" && options.materialMode == MaterialMode::EveV5 )
	{
		options.localLights = LocalLights::Authored;
	}

	return true;
}

uint32_t GetBackingDimension( CGFloat points, CGFloat backingScale )
{
	return std::max( 1u, static_cast<uint32_t>( std::llround( points * backingScale ) ) );
}

}

@interface EveSceneProbeContentView : NSView
@end

@implementation EveSceneProbeContentView
- (BOOL)canBecomeKeyView
{
	return YES;
}

- (BOOL)acceptsFirstResponder
{
	return YES;
}
@end

namespace
{

NSWindow* CreateWindow( const Options& options, NSView** outView )
{
	NSRect frame = NSMakeRect( 0, 0, options.windowWidth, options.windowHeight );
	NSWindowStyleMask styleMask = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable;

	if( !options.windowed )
	{
		NSScreen* screen = [NSScreen mainScreen];
		frame = screen ? [screen frame] : frame;
		styleMask = NSWindowStyleMaskBorderless;
	}

	NSWindow* window = [[NSWindow alloc] initWithContentRect:frame
											   styleMask:styleMask
												 backing:NSBackingStoreBuffered
												   defer:NO];
	[window setReleasedWhenClosed:NO];
	[window setTitle:@"TrinityAL EVE Scene Probe"];
	[window setBackgroundColor:[NSColor blackColor]];
	[window setOpaque:YES];

	if( options.windowed )
	{
		[window center];
	}
	else
	{
		[window setFrame:frame display:YES];
		[window setLevel:NSMainMenuWindowLevel + 1];
		[window setCollectionBehavior:NSWindowCollectionBehaviorCanJoinAllSpaces | NSWindowCollectionBehaviorFullScreenPrimary];
	}

	EveSceneProbeContentView* view = [[EveSceneProbeContentView alloc] initWithFrame:frame];
	view.wantsLayer = YES;
	view.layer = [CAMetalLayer layer];
	[window setContentView:view];
	[window makeFirstResponder:view];
	*outView = view;
	return window;
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

bool EnsureParentDirectory( const std::string& path )
{
	NSString* nsPath = [NSString stringWithUTF8String:path.c_str()];
	NSString* parent = [nsPath stringByDeletingLastPathComponent];
	if( parent.length == 0 )
	{
		return true;
	}

	NSError* error = nil;
	if( ![[NSFileManager defaultManager] createDirectoryAtPath:parent withIntermediateDirectories:YES attributes:nil error:&error] )
	{
		std::cerr << "Failed to create capture directory: " << error.localizedDescription.UTF8String << "\n";
		return false;
	}
	return true;
}

bool CaptureWindowPng( NSWindow* window, const std::string& path )
{
	if( !EnsureParentDirectory( path ) )
	{
		return false;
	}

	CGImageRef image = CGWindowListCreateImage(
		CGRectNull,
		kCGWindowListOptionIncludingWindow,
		static_cast<CGWindowID>( window.windowNumber ),
		kCGWindowImageBoundsIgnoreFraming );
	if( !image )
	{
		std::cerr << "Failed to capture window image\n";
		return false;
	}

	NSString* nsPath = [NSString stringWithUTF8String:path.c_str()];
	NSURL* url = [NSURL fileURLWithPath:nsPath];
	CGImageDestinationRef destination = CGImageDestinationCreateWithURL(
		(__bridge CFURLRef)url,
		(__bridge CFStringRef)UTTypePNG.identifier,
		1,
		nullptr );
	if( !destination )
	{
		CGImageRelease( image );
		std::cerr << "Failed to create PNG destination\n";
		return false;
	}

	CGImageDestinationAddImage( destination, image, nullptr );
	const bool finalized = CGImageDestinationFinalize( destination );
	CFRelease( destination );
	CGImageRelease( image );

	if( !finalized )
	{
		std::cerr << "Failed to finalize PNG capture\n";
		return false;
	}
	return true;
}

bool WriteCaptureMetadata( const Options& options, uint32_t width, uint32_t height, int renderedFrames )
{
	if( options.capturePrefix.empty() )
	{
		return true;
	}

	const std::string materialSuffix = "_" + MaterialModeName( options.materialMode ) +
		( options.materialView == MaterialView::Lit ? "" : "_" + MaterialViewName( options.materialView ) ) +
		( options.areaView == AreaView::All ? "" : "_area-" + AreaViewName( options.areaView ) ) +
		"_lighting-" + LightingViewName( options.lightingView ) + "_sh-" + ShSourceName( options.shSource ) +
		"_local-" + LocalLightsName( options.localLights );
	const std::string metadataPath = options.capturePrefix + "_" + options.asset + "_" + QualityRungName( options.qualityRung ) + materialSuffix + ".txt";
	if( !EnsureParentDirectory( metadataPath ) )
	{
		return false;
	}

	std::ofstream metadata( metadataPath.c_str() );
	if( !metadata )
	{
		std::cerr << "Failed to write capture metadata: " << metadataPath << "\n";
		return false;
	}

	metadata << "asset=" << options.asset << "\n";
	metadata << "input=" << options.inputPath << "\n";
	metadata << "qualityRung=" << QualityRungName( options.qualityRung ) << "\n";
	metadata << "materialView=" << MaterialViewName( options.materialView ) << "\n";
	metadata << "materialMode=" << MaterialModeName( options.materialMode ) << "\n";
	metadata << "areaView=" << AreaViewName( options.areaView ) << "\n";
	metadata << "lightingView=" << LightingViewName( options.lightingView ) << "\n";
	metadata << "shSource=" << ShSourceName( options.shSource ) << "\n";
	metadata << "localLights=" << LocalLightsName( options.localLights ) << "\n";
	metadata << "shaderTier=" << ShaderTierName( options.shaderTier ) << "\n";
	metadata << "renderWidth=" << width << "\n";
	metadata << "renderHeight=" << height << "\n";
	metadata << "frames=" << renderedFrames << "\n";
	return true;
}

bool CaptureIfRequested( NSWindow* window, const Options& options, uint32_t width, uint32_t height, int renderedFrames )
{
	if( options.capturePrefix.empty() )
	{
		return true;
	}

	const std::string materialSuffix = "_" + MaterialModeName( options.materialMode ) +
		( options.materialView == MaterialView::Lit ? "" : "_" + MaterialViewName( options.materialView ) ) +
		( options.areaView == AreaView::All ? "" : "_area-" + AreaViewName( options.areaView ) ) +
		"_lighting-" + LightingViewName( options.lightingView ) + "_sh-" + ShSourceName( options.shSource ) +
		"_local-" + LocalLightsName( options.localLights );
	const std::string pngPath = options.capturePrefix + "_" + options.asset + "_" + QualityRungName( options.qualityRung ) + materialSuffix + ".png";
	return CaptureWindowPng( window, pngPath ) && WriteCaptureMetadata( options, width, height, renderedFrames );
}

}

@interface EveSceneProbeWindowDelegate : NSObject <NSWindowDelegate>
@end

@implementation EveSceneProbeWindowDelegate
- (BOOL)windowShouldClose:(id)sender
{
	(void)sender;
	g_shouldQuit = true;
	return YES;
}
@end

int main( int argc, char** argv )
{
	@autoreleasepool
	{
		Options options;
		if( !ParseArgs( argc, argv, options ) )
		{
			PrintUsage( argv[0] );
			return 2;
		}

		if( options.inspectionReportPath.empty() && options.qualityRung >= QualityRung::Model && !FileExists( options.inputPath ) )
		{
			std::cerr << "CMF asset file is not present: " << options.inputPath << "\n";
			return 1;
		}

		const std::string executableDirectory = ExecutableDirectory();
		if( !TrinityStandaloneProbeStartup( argc, const_cast<const char* const*>( argv ), executableDirectory.c_str() ) )
		{
			std::cerr << "Trinity standalone probe startup failed\n";
			return 1;
		}

		[NSApplication sharedApplication];
		[NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

		NSView* view = nil;
		NSWindow* window = CreateWindow( options, &view );
		EveSceneProbeWindowDelegate* delegate = [[EveSceneProbeWindowDelegate alloc] init];
		[window setDelegate:delegate];
		[window makeKeyAndOrderFront:nil];
		[NSApp activateIgnoringOtherApps:YES];

		const CGFloat backingScale = std::max( static_cast<CGFloat>( 1.0 ), [window backingScaleFactor] );
		const NSSize viewSize = view.bounds.size;
		const uint32_t renderWidth = GetBackingDimension( viewSize.width, backingScale );
		const uint32_t renderHeight = GetBackingDimension( viewSize.height, backingScale );
		if( [view.layer isKindOfClass:CAMetalLayer.class] )
		{
			CAMetalLayer* metalLayer = (CAMetalLayer*)view.layer;
			metalLayer.contentsScale = backingScale;
			metalLayer.drawableSize = CGSizeMake( renderWidth, renderHeight );
		}

		void* probe = TrinityStandaloneProbeCreateDevice( (__bridge void*)view, renderWidth, renderHeight, static_cast<int>( options.shaderTier ) );
		if( !probe )
		{
			std::cerr << "TrinityStandaloneProbeCreateDevice failed\n";
			[window close];
			return 1;
		}

		if( !options.inspectionReportPath.empty() )
		{
			bool inspectionSucceeded = false;
			@autoreleasepool
			{
				inspectionSucceeded = EnsureParentDirectory( options.inspectionReportPath ) &&
					TrinityStandaloneProbeInspectClientAssets( probe, options.inspectionReportPath.c_str() );
			}
			if( !inspectionSucceeded )
			{
				std::cerr << "TrinityStandaloneProbeInspectClientAssets failed\n";
				TrinityStandaloneProbeDestroyDevice( probe );
				[window close];
				return 1;
			}
			std::cout << "Client asset report: " << options.inspectionReportPath << "\n";
			TrinityStandaloneProbeDestroyDevice( probe );
			[window close];
			return 0;
		}

		const int qualityRung = QualityRungApiValue( options.qualityRung );
		if( options.qualityRung != QualityRung::Shell &&
			!TrinityStandaloneProbeCreateEveScene(
				probe,
				qualityRung,
					options.inputPath.c_str(),
					static_cast<int>( options.materialView ),
					static_cast<int>( options.materialMode ),
					static_cast<int>( options.areaView ),
					SceneResourcePath( options.sceneFixture ),
					SceneFixtureApiValue( options.sceneFixture ),
					static_cast<int>( options.lightingView ),
					static_cast<int>( options.shSource ),
					static_cast<int>( options.localLights ) ) )
		{
			std::cerr << "TrinityStandaloneProbeCreateEveScene failed\n";
			TrinityStandaloneProbeDestroyDevice( probe );
			[window close];
			return 1;
		}

		int renderedFrames = 0;
		while( !g_shouldQuit && ( options.maxFrames < 0 || renderedFrames < options.maxFrames ) )
		{
			@autoreleasepool
			{
				ProcessEvents( window );
				if( g_shouldQuit )
				{
					break;
				}

				const int64_t realTime = static_cast<int64_t>( renderedFrames ) * kFrameTime;
				const int64_t simTime = realTime;
				if( !TrinityStandaloneProbeRenderFrame( probe, qualityRung, realTime, simTime ) )
				{
					std::cerr << "TrinityStandaloneProbeRenderFrame failed\n";
					TrinityStandaloneProbeDestroyDevice( probe );
					[window close];
					return 1;
				}
				++renderedFrames;
			}
		}

		bool captureSucceeded = false;
		@autoreleasepool
		{
			captureSucceeded = CaptureIfRequested( window, options, renderWidth, renderHeight, renderedFrames );
		}
		if( !captureSucceeded )
		{
			TrinityStandaloneProbeDestroyDevice( probe );
			[window close];
			return 1;
		}

		TrinityStandaloneProbeDestroyDevice( probe );
		[window close];
	}

	return 0;
}
