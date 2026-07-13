// Copyright (c) 2026 CCP ehf.

#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <CommonCrypto/CommonDigest.h>

#include "StdAfx.h"
#include "ALResult.h"
#include "metal/MetalContext.h"
#include "metal/MetalWorkQueue.h"
#include "metal/Tr2RenderContextMetal.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <initializer_list>
#include <limits>
#include <map>
#include <string>
#include <utility>
#include <unistd.h>
#include <vector>

const char* g_moduleName = "TrinityALFroxelProbe_metal";

namespace
{
constexpr const char* kRiskAcknowledgement = "I_ACKNOWLEDGE_GPU_RESET_RISK";

struct Dimensions
{
	uint32_t width = 32;
	uint32_t height = 32;
	uint32_t depth = 16;
};

struct Options
{
	std::string kernelSet = "synthetic";
	std::string stage = "clear";
	Dimensions dimensions;
	std::string format = "rgba16f";
	std::string temporal = "off";
	std::string alias = "out-of-place";
	std::string synchronization = "encoder";
	std::string dispatch = "reflected";
	uint32_t iterations = 1;
	bool clientKernels = false;
	std::string effectRoot;
	std::string clientPackage;
	std::string ledgerPath;
	std::string reportPath;
};

struct ClientStage
{
	std::string name;
	std::string function;
	std::string sha256;
	MTLSize threadgroup = MTLSizeMake( 1, 1, 1 );
	NSData* __strong bytecode = nil;
};

struct ClientPackage
{
	std::map<std::string, ClientStage> stages;
	std::string manifestSha256;
};

const char* kSyntheticSource = R"(
#include <metal_stdlib>
using namespace metal;

inline bool inside(texture3d<float, access::write> texture, uint3 p)
{
	return p.x < texture.get_width() && p.y < texture.get_height() && p.z < texture.get_depth();
}

kernel void writeSynthetic(texture3d<float, access::write> output [[texture(0)]],
	uint3 p [[thread_position_in_grid]])
{
	if (!inside(output, p)) return;
	float3 denominator = max(float3(output.get_width() - 1, output.get_height() - 1,
		output.get_depth() - 1), float3(1.0));
	float3 q = float3(p) / denominator;
	output.write(float4(0.125 + q.x, 0.25 + q.y, 0.375 + q.z, 1.0), p);
}

kernel void filterSynthetic(texture3d<float, access::read> input [[texture(0)]],
	texture3d<float, access::write> output [[texture(1)]], uint3 p [[thread_position_in_grid]])
{
	if (!inside(output, p)) return;
	uint3 hi = uint3(input.get_width() - 1, input.get_height() - 1, input.get_depth() - 1);
	uint3 next = min(p + uint3(1, 0, 0), hi);
	output.write(0.75 * input.read(p) + 0.25 * input.read(next), p);
}

kernel void inPlaceSynthetic(texture3d<float, access::read_write> texture [[texture(0)]],
	uint3 p [[thread_position_in_grid]])
{
	if (p.x >= texture.get_width() || p.y >= texture.get_height() || p.z >= texture.get_depth()) return;
	float4 value = texture.read(p);
	texture.write(float4(value.rgb * 0.875 + float3(0.03125), value.a), p);
}

kernel void mieSynthetic(texture3d<float, access::write> output [[texture(0)]],
	uint3 p [[thread_position_in_grid]])
{
	if (!inside(output, p)) return;
	float phase = float((p.x * 17 + p.y * 11 + p.z * 5) & 255) / 255.0;
	output.write(float4(phase, phase * phase, sqrt(phase), 1.0), p);
}

kernel void writeMieArraySynthetic(texture2d_array<float, access::write> output [[texture(0)]],
	uint3 p [[thread_position_in_grid]])
{
	if (p.x >= output.get_width() || p.y >= output.get_height() || p.z >= output.get_array_size()) return;
	float3 denominator = max(float3(output.get_width() - 1, output.get_height() - 1,
		output.get_array_size() - 1), float3(1.0));
	float3 q = float3(p) / denominator;
	output.write(float4(0.125 + 0.5 * q.x, 0.25 + 0.5 * q.y, 0.375 + 0.25 * q.z, 1.0), p.xy, p.z);
}

kernel void raymarchSynthetic(texture3d<float, access::read> input [[texture(0)]],
	texture3d<float, access::write> output [[texture(1)]], uint3 p [[thread_position_in_grid]])
{
	if (!inside(output, p)) return;
	float4 value = input.read(p);
	float attenuation = exp(-0.01 * float(p.z + 1));
	output.write(float4(value.rgb * attenuation, 1.0 - attenuation), p);
}

kernel void applySynthetic(texture3d<float, access::read> input [[texture(0)]],
	texture3d<float, access::write> output [[texture(1)]], uint3 p [[thread_position_in_grid]])
{
	if (!inside(output, p)) return;
	float4 value = input.read(p);
	output.write(float4(value.rgb / (1.0 + value.rgb), value.a), p);
}
)";

bool ParseDimensions( const char* text, Dimensions& dimensions )
{
	unsigned width = 0;
	unsigned height = 0;
	unsigned depth = 0;
	char trailing = '\0';
	if( std::sscanf( text, "%ux%ux%u%c", &width, &height, &depth, &trailing ) != 3 || width == 0 || height == 0 ||
		depth == 0 )
	{
		return false;
	}
	const uint64_t texels = static_cast<uint64_t>( width ) * height * depth;
	if( width > 512 || height > 512 || depth > 256 || texels > 16ull * 1024ull * 1024ull )
	{
		return false;
	}
	dimensions = { width, height, depth };
	return true;
}

bool ParseUnsigned( const char* text, uint32_t& value )
{
	char* end = nullptr;
	const unsigned long parsed = std::strtoul( text, &end, 10 );
	if( !end || *end != '\0' || parsed == 0 || parsed > std::numeric_limits<uint32_t>::max() )
	{
		return false;
	}
	value = static_cast<uint32_t>( parsed );
	return true;
}

bool InSet( const std::string& value, std::initializer_list<const char*> values )
{
	return std::any_of( values.begin(), values.end(), [&]( const char* item ) { return value == item; } );
}

void PrintUsage( const char* executable )
{
	std::fprintf( stderr,
				  "Usage: %s --kernel-set synthetic|client --stage clear|mie|calculate|filter|raymarch|apply|chain "
				  "--dimensions WxHxD --format r11g11b10|rgba16f|rgba32f --temporal off|on "
				  "--alias out-of-place|in-place --sync none|resource|encoder "
				  "--dispatch exact|rounded|reflected --iterations N --report PATH "
				  "[--ledger PATH] [--client-kernels --effect-root PATH --client-package PATH]\n",
				  executable );
}

bool ParseOptions( int argc, char** argv, Options& options )
{
	for( int i = 1; i < argc; ++i )
	{
		const std::string argument = argv[i];
		auto value = [&]() -> const char* {
			if( ++i >= argc )
			{
				return nullptr;
			}
			return argv[i];
		};
		if( argument == "--kernel-set" )
		{
			const char* parsed = value();
			if( !parsed )
				return false;
			options.kernelSet = parsed;
		}
		else if( argument == "--stage" )
		{
			const char* parsed = value();
			if( !parsed )
				return false;
			options.stage = parsed;
		}
		else if( argument == "--dimensions" )
		{
			const char* parsed = value();
			if( !parsed || !ParseDimensions( parsed, options.dimensions ) )
				return false;
		}
		else if( argument == "--format" )
		{
			const char* parsed = value();
			if( !parsed )
				return false;
			options.format = parsed;
		}
		else if( argument == "--temporal" )
		{
			const char* parsed = value();
			if( !parsed )
				return false;
			options.temporal = parsed;
		}
		else if( argument == "--alias" )
		{
			const char* parsed = value();
			if( !parsed )
				return false;
			options.alias = parsed;
		}
		else if( argument == "--sync" )
		{
			const char* parsed = value();
			if( !parsed )
				return false;
			options.synchronization = parsed;
		}
		else if( argument == "--dispatch" )
		{
			const char* parsed = value();
			if( !parsed )
				return false;
			options.dispatch = parsed;
		}
		else if( argument == "--iterations" )
		{
			const char* parsed = value();
			if( !parsed || !ParseUnsigned( parsed, options.iterations ) )
				return false;
		}
		else if( argument == "--effect-root" )
		{
			const char* parsed = value();
			if( !parsed )
				return false;
			options.effectRoot = parsed;
		}
		else if( argument == "--client-package" )
		{
			const char* parsed = value();
			if( !parsed )
				return false;
			options.clientPackage = parsed;
		}
		else if( argument == "--ledger" )
		{
			const char* parsed = value();
			if( !parsed )
				return false;
			options.ledgerPath = parsed;
		}
		else if( argument == "--report" )
		{
			const char* parsed = value();
			if( !parsed )
				return false;
			options.reportPath = parsed;
		}
		else if( argument == "--client-kernels" )
		{
			options.clientKernels = true;
		}
		else
		{
			return false;
		}
	}

	return InSet( options.kernelSet, { "synthetic", "client" } ) &&
		InSet( options.stage, { "clear", "mie", "calculate", "filter", "raymarch", "apply", "chain" } ) &&
		InSet( options.format, { "r11g11b10", "rgba16f", "rgba32f" } ) && InSet( options.temporal, { "off", "on" } ) &&
		InSet( options.alias, { "out-of-place", "in-place" } ) &&
		InSet( options.synchronization, { "none", "resource", "encoder" } ) &&
		InSet( options.dispatch, { "exact", "rounded", "reflected" } ) && !options.reportPath.empty();
}

MTLPixelFormat PixelFormat( const std::string& name )
{
	if( name == "r11g11b10" )
		return MTLPixelFormatRG11B10Float;
	if( name == "rgba32f" )
		return MTLPixelFormatRGBA32Float;
	return MTLPixelFormatRGBA16Float;
}

NSUInteger BytesPerPixel( MTLPixelFormat format )
{
	switch( format )
	{
	case MTLPixelFormatR32Float:
	case MTLPixelFormatR32Uint:
		return 4;
	case MTLPixelFormatRGBA16Float:
		return 8;
	case MTLPixelFormatRGBA32Float:
		return 16;
	case MTLPixelFormatRG11B10Float:
		return 4;
	default:
		return 0;
	}
}

enum class EncodedFloatClass
{
	Finite,
	Infinity,
	NaN,
};

struct NumericReadbackDiagnostics
{
	bool available = false;
	size_t components = 0;
	size_t finiteComponents = 0;
	size_t infiniteComponents = 0;
	size_t nanComponents = 0;
	size_t finiteTexels = 0;
	size_t nonfiniteTexels = 0;
};

EncodedFloatClass ClassifyEncodedFloat( uint32_t exponent, uint32_t mantissa, uint32_t maximumExponent )
{
	if( exponent != maximumExponent )
		return EncodedFloatClass::Finite;
	return mantissa == 0 ? EncodedFloatClass::Infinity : EncodedFloatClass::NaN;
}

void RecordEncodedFloat( NumericReadbackDiagnostics& diagnostics,
						 EncodedFloatClass classification,
						 bool& texelFinite )
{
	++diagnostics.components;
	switch( classification )
	{
	case EncodedFloatClass::Finite:
		++diagnostics.finiteComponents;
		break;
	case EncodedFloatClass::Infinity:
		++diagnostics.infiniteComponents;
		texelFinite = false;
		break;
	case EncodedFloatClass::NaN:
		++diagnostics.nanComponents;
		texelFinite = false;
		break;
	}
}

NumericReadbackDiagnostics AnalyzeNumericReadback( MTLPixelFormat format, const uint8_t* bytes, size_t texelCount )
{
	NumericReadbackDiagnostics diagnostics;
	diagnostics.available = true;
	for( size_t texel = 0; texel < texelCount; ++texel )
	{
		bool texelFinite = true;
		switch( format )
		{
		case MTLPixelFormatRG11B10Float:
		{
			uint32_t packed = 0;
			std::memcpy( &packed, bytes + texel * sizeof( packed ), sizeof( packed ) );
			for( const auto& component : { std::pair<uint32_t, uint32_t>{ packed & 0x7ffu, 6u },
									  std::pair<uint32_t, uint32_t>{ ( packed >> 11 ) & 0x7ffu, 6u },
									  std::pair<uint32_t, uint32_t>{ ( packed >> 22 ) & 0x3ffu, 5u } } )
			{
				const uint32_t mantissaMask = ( 1u << component.second ) - 1u;
				RecordEncodedFloat( diagnostics,
					ClassifyEncodedFloat( component.first >> component.second,
						component.first & mantissaMask,
						0x1fu ),
					texelFinite );
			}
			break;
		}
		case MTLPixelFormatRGBA16Float:
			for( size_t component = 0; component < 4; ++component )
			{
				uint16_t encoded = 0;
				std::memcpy( &encoded, bytes + texel * 8 + component * sizeof( encoded ), sizeof( encoded ) );
				RecordEncodedFloat( diagnostics,
					ClassifyEncodedFloat( ( encoded >> 10 ) & 0x1fu, encoded & 0x3ffu, 0x1fu ),
					texelFinite );
			}
			break;
		case MTLPixelFormatRGBA32Float:
			for( size_t component = 0; component < 4; ++component )
			{
				uint32_t encoded = 0;
				std::memcpy( &encoded, bytes + texel * 16 + component * sizeof( encoded ), sizeof( encoded ) );
				RecordEncodedFloat( diagnostics,
					ClassifyEncodedFloat( ( encoded >> 23 ) & 0xffu, encoded & 0x7fffffu, 0xffu ),
					texelFinite );
			}
			break;
		case MTLPixelFormatR32Float:
		{
			uint32_t encoded = 0;
			std::memcpy( &encoded, bytes + texel * sizeof( encoded ), sizeof( encoded ) );
			RecordEncodedFloat( diagnostics,
				ClassifyEncodedFloat( ( encoded >> 23 ) & 0xffu, encoded & 0x7fffffu, 0xffu ),
				texelFinite );
			break;
		}
		case MTLPixelFormatR32Uint:
			RecordEncodedFloat( diagnostics, EncodedFloatClass::Finite, texelFinite );
			break;
		default:
			diagnostics = {};
			return diagnostics;
		}
		if( texelFinite )
			++diagnostics.finiteTexels;
		else
			++diagnostics.nonfiniteTexels;
	}
	return diagnostics;
}

uint64_t Fnv1a64( const void* data, size_t size )
{
	const uint8_t* bytes = static_cast<const uint8_t*>( data );
	uint64_t hash = 1469598103934665603ull;
	for( size_t i = 0; i < size; ++i )
	{
		hash ^= bytes[i];
		hash *= 1099511628211ull;
	}
	return hash;
}

NSString* HexHash( uint64_t hash )
{
	return [NSString stringWithFormat:@"%016llx", static_cast<unsigned long long>( hash )];
}

std::string Sha256( NSData* data )
{
	if( !data || data.length > std::numeric_limits<CC_LONG>::max() )
	{
		return {};
	}
	unsigned char digest[CC_SHA256_DIGEST_LENGTH];
	CC_SHA256( data.bytes, static_cast<CC_LONG>( data.length ), digest );
	char output[CC_SHA256_DIGEST_LENGTH * 2 + 1];
	for( size_t i = 0; i < CC_SHA256_DIGEST_LENGTH; ++i )
	{
		std::snprintf( output + i * 2, 3, "%02x", digest[i] );
	}
	return output;
}

bool LoadClientPackage( const Options& options, ClientPackage& package )
{
	NSString* manifestPath = [NSString stringWithUTF8String:options.clientPackage.c_str()];
	NSError* error = nil;
	NSData* manifestData = [NSData dataWithContentsOfFile:manifestPath options:NSDataReadingMappedIfSafe error:&error];
	if( !manifestData )
	{
		std::fprintf( stderr, "Failed to read client package: %s\n", error.localizedDescription.UTF8String );
		return false;
	}
	id value = [NSJSONSerialization JSONObjectWithData:manifestData options:0 error:&error];
	if( ![value isKindOfClass:NSDictionary.class] )
	{
		std::fprintf( stderr,
					  "Invalid client package JSON: %s\n",
					  error.localizedDescription.UTF8String ?: "root is not an object" );
		return false;
	}
	NSDictionary* manifest = value;
	if( ![manifest[@"schemaVersion"] isEqual:@1] || ![manifest[@"deviceCreated"] isEqual:@NO] )
	{
		std::fprintf( stderr, "Client package schema or device-free provenance is invalid\n" );
		return false;
	}
	NSDictionary* stages = manifest[@"stages"];
	if( ![stages isKindOfClass:NSDictionary.class] )
	{
		std::fprintf( stderr, "Client package has no stages object\n" );
		return false;
	}
	NSString* packageDirectory = manifestPath.stringByDeletingLastPathComponent.stringByStandardizingPath;
	NSString* packagePrefix = [packageDirectory stringByAppendingString:@"/"];
	const std::array<const char*, 7> required = { "apply_vs", "apply_ps",     "mie",        "calculate",
												  "filter",   "raymarch_out", "raymarch_in" };
	for( const char* stageName : required )
	{
		NSString* key = [NSString stringWithUTF8String:stageName];
		NSDictionary* record = stages[key];
		if( ![record isKindOfClass:NSDictionary.class] )
		{
			std::fprintf( stderr, "Client package stage is missing: %s\n", stageName );
			return false;
		}
		NSString* blobName = record[@"blob"];
		NSString* functionName = record[@"function"];
		NSString* expectedSha = record[@"sha256"];
		NSArray* threadgroup = record[@"threadgroup"];
		NSNumber* expectedBytes = record[@"bytes"];
		if( ![blobName isKindOfClass:NSString.class] || ![functionName isKindOfClass:NSString.class] ||
			![expectedSha isKindOfClass:NSString.class] || ![threadgroup isKindOfClass:NSArray.class] ||
			threadgroup.count != 3 || ![expectedBytes isKindOfClass:NSNumber.class] )
		{
			std::fprintf( stderr, "Client package stage metadata is invalid: %s\n", stageName );
			return false;
		}
		NSString* blobPath = [packageDirectory stringByAppendingPathComponent:blobName].stringByStandardizingPath;
		if( ![blobPath hasPrefix:packagePrefix] )
		{
			std::fprintf( stderr, "Client package stage escapes its package directory: %s\n", stageName );
			return false;
		}
		NSData* bytecode = [NSData dataWithContentsOfFile:blobPath options:NSDataReadingMappedIfSafe error:&error];
		const std::string actualSha = Sha256( bytecode );
		if( !bytecode || bytecode.length != expectedBytes.unsignedLongLongValue || actualSha != expectedSha.UTF8String )
		{
			std::fprintf( stderr, "Client package bytecode hash/size mismatch: %s\n", stageName );
			return false;
		}
		ClientStage stage;
		stage.name = stageName;
		stage.function = functionName.UTF8String;
		stage.sha256 = actualSha;
		stage.threadgroup = MTLSizeMake( [threadgroup[0] unsignedLongLongValue],
										 [threadgroup[1] unsignedLongLongValue],
										 [threadgroup[2] unsignedLongLongValue] );
		stage.bytecode = bytecode;
		package.stages.emplace( stage.name, std::move( stage ) );
	}

	NSArray* containers = manifest[@"containers"];
	if( ![containers isKindOfClass:NSArray.class] || containers.count == 0 )
	{
		std::fprintf( stderr, "Client package has no source-container records\n" );
		return false;
	}
	for( NSDictionary* record in containers )
	{
		NSString* path = record[@"path"];
		NSString* expectedSha = record[@"sha256"];
		if( ![path isKindOfClass:NSString.class] || ![expectedSha isKindOfClass:NSString.class] )
		{
			return false;
		}
		NSData* data = [NSData dataWithContentsOfFile:path options:NSDataReadingMappedIfSafe error:&error];
		if( !data || Sha256( data ) != expectedSha.UTF8String )
		{
			std::fprintf( stderr, "Client source container no longer matches package: %s\n", path.UTF8String );
			return false;
		}
	}
	package.manifestSha256 = Sha256( manifestData );
	return true;
}

id<MTLFunction> LoadClientFunction( id<MTLDevice> device, const ClientStage& stage )
{
	void* bytes = std::malloc( stage.bytecode.length );
	if( !bytes )
	{
		return nil;
	}
	std::memcpy( bytes, stage.bytecode.bytes, stage.bytecode.length );
	dispatch_data_t data = dispatch_data_create( bytes,
												 stage.bytecode.length,
												 dispatch_get_global_queue( QOS_CLASS_USER_INITIATED, 0 ),
												 DISPATCH_DATA_DESTRUCTOR_FREE );
	NSError* error = nil;
	id<MTLLibrary> library = [device newLibraryWithData:data error:&error];
	if( !library )
	{
		std::fprintf(
			stderr, "Failed to load client stage %s: %s\n", stage.name.c_str(), error.localizedDescription.UTF8String );
		return nil;
	}
	id<MTLFunction> function = [library newFunctionWithName:[NSString stringWithUTF8String:stage.function.c_str()]];
	if( !function )
	{
		std::fprintf( stderr, "Client stage %s has no %s export\n", stage.name.c_str(), stage.function.c_str() );
	}
	return function;
}

id<MTLTexture> CreateTexture( id<MTLDevice> device, const Options& options )
{
	MTLTextureDescriptor* descriptor = [MTLTextureDescriptor new];
	descriptor.textureType = MTLTextureType3D;
	descriptor.pixelFormat = PixelFormat( options.format );
	descriptor.width = options.dimensions.width;
	descriptor.height = options.dimensions.height;
	descriptor.depth = options.dimensions.depth;
	descriptor.mipmapLevelCount = 1;
	descriptor.storageMode = MTLStorageModeShared;
	descriptor.usage = MTLTextureUsageShaderRead | MTLTextureUsageShaderWrite;
	return [device newTextureWithDescriptor:descriptor];
}

id<MTLTexture> Create2DTexture( id<MTLDevice> device,
								NSUInteger width,
								NSUInteger height,
								MTLPixelFormat format,
								MTLTextureUsage usage )
{
	MTLTextureDescriptor* descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:format
																						  width:width
																						 height:height
																					  mipmapped:NO];
	descriptor.storageMode = MTLStorageModeShared;
	descriptor.usage = usage;
	return [device newTextureWithDescriptor:descriptor];
}

id<MTLTexture> Create2DArrayTexture( id<MTLDevice> device,
									 NSUInteger width,
									 NSUInteger height,
									 NSUInteger layers,
									 MTLPixelFormat format,
									 MTLTextureUsage usage )
{
	MTLTextureDescriptor* descriptor = [MTLTextureDescriptor new];
	descriptor.textureType = MTLTextureType2DArray;
	descriptor.pixelFormat = format;
	descriptor.width = width;
	descriptor.height = height;
	descriptor.arrayLength = layers;
	descriptor.mipmapLevelCount = 1;
	descriptor.storageMode = MTLStorageModeShared;
	descriptor.usage = usage;
	return [device newTextureWithDescriptor:descriptor];
}

id<MTLTexture>
	CreateCubeTexture( id<MTLDevice> device, NSUInteger dimension, MTLPixelFormat format, MTLTextureUsage usage )
{
	MTLTextureDescriptor* descriptor = [MTLTextureDescriptor textureCubeDescriptorWithPixelFormat:format
																							 size:dimension
																						mipmapped:NO];
	descriptor.storageMode = MTLStorageModeShared;
	descriptor.usage = usage | MTLTextureUsagePixelFormatView;
	return [device newTextureWithDescriptor:descriptor];
}

bool FillTextureFromCpu( id<MTLTexture> texture, uint8_t value )
{
	const NSUInteger bytesPerPixel = BytesPerPixel( texture.pixelFormat );
	if( !texture || bytesPerPixel == 0 || texture.storageMode != MTLStorageModeShared )
	{
		return false;
	}
	const NSUInteger bytesPerRow = texture.width * bytesPerPixel;
	const NSUInteger bytesPerImage = bytesPerRow * texture.height;
	const NSUInteger depth = texture.textureType == MTLTextureType3D ? texture.depth : 1;
	NSUInteger slices = 1;
	if( texture.textureType == MTLTextureTypeCube )
		slices = 6;
	else if( texture.textureType == MTLTextureType2DArray )
		slices = texture.arrayLength;
	std::vector<uint8_t> bytes( bytesPerImage * depth, value );
	for( NSUInteger slice = 0; slice < slices; ++slice )
	{
		[texture replaceRegion:MTLRegionMake3D( 0, 0, 0, texture.width, texture.height, depth )
				   mipmapLevel:0
						 slice:slice
					 withBytes:bytes.data()
				   bytesPerRow:bytesPerRow
				 bytesPerImage:bytesPerImage];
	}
	return true;
}

bool FillR32FloatTextureFromCpu( id<MTLTexture> texture, float value )
{
	if( !texture || texture.pixelFormat != MTLPixelFormatR32Float || texture.textureType != MTLTextureType2D ||
		texture.storageMode != MTLStorageModeShared || !std::isfinite( value ) )
	{
		return false;
	}
	const NSUInteger bytesPerRow = texture.width * sizeof( float );
	std::vector<float> values( texture.width * texture.height, value );
	[texture replaceRegion:MTLRegionMake2D( 0, 0, texture.width, texture.height )
			 mipmapLevel:0
			   withBytes:values.data()
			 bytesPerRow:bytesPerRow];
	return true;
}

bool ZeroTextureFromCpu( id<MTLTexture> texture )
{
	return FillTextureFromCpu( texture, 0 );
}

bool ValidateCpuTextureFormatRoundTrips( id<MTLDevice> device )
{
	for( MTLPixelFormat format : { MTLPixelFormatR32Float, MTLPixelFormatR32Uint } )
	{
		id<MTLTexture> texture = Create2DTexture( device, 4, 4, format, MTLTextureUsageShaderRead );
		if( !texture || !FillTextureFromCpu( texture, 0x5a ) )
		{
			return false;
		}

		const NSUInteger bytesPerRow = texture.width * BytesPerPixel( format );
		std::vector<uint8_t> bytes( bytesPerRow * texture.height );
		[texture getBytes:bytes.data()
			  bytesPerRow:bytesPerRow
			   fromRegion:MTLRegionMake2D( 0, 0, texture.width, texture.height )
			  mipmapLevel:0];
		if( !std::all_of( bytes.begin(), bytes.end(), []( uint8_t value ) { return value == 0x5a; } ) )
		{
			return false;
		}
	}
	return true;
}

void WriteFloat( std::vector<uint8_t>& buffer, size_t offset, float value )
{
	if( offset + sizeof( value ) <= buffer.size() )
	{
		std::memcpy( buffer.data() + offset, &value, sizeof( value ) );
	}
}

void WriteUint( std::vector<uint8_t>& buffer, size_t offset, uint32_t value )
{
	if( offset + sizeof( value ) <= buffer.size() )
	{
		std::memcpy( buffer.data() + offset, &value, sizeof( value ) );
	}
}

void WriteIdentity( std::vector<uint8_t>& buffer, size_t offset )
{
	for( size_t i = 0; i < 4; ++i )
	{
		WriteFloat( buffer, offset + ( i * 4 + i ) * sizeof( float ), 1.0f );
	}
}

void WriteVector4( std::vector<uint8_t>& buffer, size_t offset, float x, float y, float z, float w )
{
	WriteFloat( buffer, offset, x );
	WriteFloat( buffer, offset + 4, y );
	WriteFloat( buffer, offset + 8, z );
	WriteFloat( buffer, offset + 12, w );
}

float ReadFloat( const std::vector<uint8_t>& buffer, size_t offset )
{
	float value = std::numeric_limits<float>::quiet_NaN();
	if( offset + sizeof( value ) <= buffer.size() )
		std::memcpy( &value, buffer.data() + offset, sizeof( value ) );
	return value;
}

uint32_t ReadUint( const std::vector<uint8_t>& buffer, size_t offset )
{
	uint32_t value = 0;
	if( offset + sizeof( value ) <= buffer.size() )
		std::memcpy( &value, buffer.data() + offset, sizeof( value ) );
	return value;
}

bool IsIdentity( const std::vector<uint8_t>& buffer, size_t offset )
{
	for( size_t row = 0; row < 4; ++row )
	{
		for( size_t column = 0; column < 4; ++column )
		{
			const float expected = row == column ? 1.0f : 0.0f;
			if( ReadFloat( buffer, offset + ( row * 4 + column ) * sizeof( float ) ) != expected )
				return false;
		}
	}
	return true;
}

std::vector<uint8_t> MakeFroxelConstants( const Options& options )
{
	constexpr float farDistance = 1000.0f;
	constexpr float opticalThickness = 1.0f;
	constexpr float baseDensity = opticalThickness / farDistance;
	std::vector<uint8_t> data( 2432, 0 );
	WriteIdentity( data, 0 );
	WriteIdentity( data, 64 );
	WriteUint( data, 128, options.dimensions.width );
	WriteUint( data, 132, options.dimensions.height );
	WriteUint( data, 136, options.dimensions.depth );
	WriteFloat( data, 156, farDistance );
	for( size_t offset : { size_t( 160 ), size_t( 164 ), size_t( 168 ) } )
		WriteFloat( data, offset, 0.1f );
	WriteFloat( data, 172, baseDensity );
	WriteFloat( data, 176, std::exp( -opticalThickness ) );
	WriteFloat( data, 180, -0.25f );
	WriteFloat( data, 184, 0.5f );
	WriteFloat( data, 188, 0.0f );
	for( size_t offset : { size_t( 192 ), size_t( 196 ), size_t( 200 ) } )
		WriteFloat( data, offset, 0.01f );
	WriteIdentity( data, 208 );
	WriteIdentity( data, 288 );
	WriteFloat( data, 376, 1.0f );
	WriteFloat( data, 380, 1.0f );
	for( size_t offset = 384; offset < 416; offset += sizeof( float ) )
		WriteFloat( data, offset, 1.0f );
	WriteIdentity( data, 416 );
	WriteFloat( data, 488, -1.0f );
	WriteFloat( data, 504, -1.0f );
	WriteFloat( data, 512, 1.0f );
	WriteFloat( data, 516, 1.0f );
	WriteFloat( data, 520, 1.0f );
	WriteFloat( data, 524, 1.0f );
	return data;
}

bool ValidateFroxelConstants( const std::vector<uint8_t>& data, const Options& options )
{
	if( data.size() != 2432 || !IsIdentity( data, 0 ) || !IsIdentity( data, 64 ) || !IsIdentity( data, 208 ) ||
		ReadUint( data, 128 ) != options.dimensions.width || ReadUint( data, 132 ) != options.dimensions.height ||
		ReadUint( data, 136 ) != options.dimensions.depth || ReadUint( data, 140 ) > 16 )
	{
		return false;
	}
	for( size_t offset : { size_t( 160 ), size_t( 164 ), size_t( 168 ), size_t( 192 ), size_t( 196 ),
			 size_t( 200 ), size_t( 512 ), size_t( 516 ), size_t( 520 ) } )
	{
		if( !std::isfinite( ReadFloat( data, offset ) ) || ReadFloat( data, offset ) < 0.0f )
			return false;
	}
	const float farDistance = ReadFloat( data, 156 );
	const float baseDensity = ReadFloat( data, 172 );
	const float maxDistanceVisibility = ReadFloat( data, 176 );
	const float lightG = ReadFloat( data, 180 );
	const float expectedVisibility = std::exp( -baseDensity * farDistance );
	return std::isfinite( farDistance ) && std::isfinite( baseDensity ) &&
		std::isfinite( maxDistanceVisibility ) && std::isfinite( lightG ) && farDistance > 0.0f &&
		baseDensity > 0.0f && maxDistanceVisibility > 0.0f && maxDistanceVisibility < 1.0f && lightG > -1.0f &&
		lightG < 0.0f && std::abs( maxDistanceVisibility - expectedVisibility ) <= 1.0e-6f;
}

NSDictionary* FroxelConstantContractDictionary( const std::vector<uint8_t>& data, const Options& options )
{
	const float farDistance = ReadFloat( data, 156 );
	const float baseDensity = ReadFloat( data, 172 );
	return @{
		@"byteSize": @( data.size() ),
		@"contractPassed": @( ValidateFroxelConstants( data, options ) ),
		@"resolution": @[@( ReadUint( data, 128 ) ), @( ReadUint( data, 132 ) ), @( ReadUint( data, 136 ) )],
		@"numDynamicLights": @( ReadUint( data, 140 ) ),
		@"far": @( farDistance ),
		@"scattering": @[
			@( ReadFloat( data, 160 ) ),
			@( ReadFloat( data, 164 ) ),
			@( ReadFloat( data, 168 ) ),
		],
		@"baseDensity": @( baseDensity ),
		@"maxDistanceVisibility": @( ReadFloat( data, 176 ) ),
		@"expectedMaxDistanceVisibility": @( std::exp( -baseDensity * farDistance ) ),
		@"lightG": @( ReadFloat( data, 180 ) ),
		@"environmentIntensity": @( ReadFloat( data, 184 ) ),
		@"inverseShadowMapAtlasSize": @( ReadFloat( data, 188 ) ),
	};
}

Options MakeMieWorkOptions( const Options& options )
{
	Options result = options;
	const uint32_t dimension = std::max( options.dimensions.width, options.dimensions.height );
	result.dimensions = { dimension, dimension, 6 };
	return result;
}

std::vector<uint8_t> MakeMieConstants( const Options& options )
{
	std::vector<uint8_t> data( 24, 0 );
	WriteUint( data, 0, options.dimensions.width );
	WriteFloat( data, 8, 0.25f );
	WriteFloat( data, 12, 0.75f );
	WriteFloat( data, 16, -0.25f );
	WriteFloat( data, 20, 0.25f );
	return data;
}

bool ValidateMieConstants( const std::vector<uint8_t>& data, const Options& options )
{
	if( data.size() != 24 || ReadUint( data, 0 ) != options.dimensions.width )
		return false;
	for( size_t offset : { size_t( 8 ), size_t( 12 ), size_t( 16 ), size_t( 20 ) } )
	{
		if( !std::isfinite( ReadFloat( data, offset ) ) )
			return false;
	}
	const float blendWeight = ReadFloat( data, 20 );
	return blendWeight > 0.0f && blendWeight <= 1.0f;
}

NSDictionary* MieConstantContractDictionary( const std::vector<uint8_t>& data, const Options& options )
{
	return @{
		@"byteSize": @( data.size() ),
		@"contractPassed": @( ValidateMieConstants( data, options ) ),
		@"resolution": @( ReadUint( data, 0 ) ),
		@"dispatchDimensions": @[
			@( options.dimensions.width ),
			@( options.dimensions.height ),
			@( options.dimensions.depth ),
		],
		@"jitter": @[@( ReadFloat( data, 8 ) ), @( ReadFloat( data, 12 ) )],
		@"environmentG": @( ReadFloat( data, 16 ) ),
		@"blendWeight": @( ReadFloat( data, 20 ) ),
	};
}

std::vector<uint8_t> MakeApplyConstants( const Options& options )
{
	constexpr size_t bufferSize = 1888;
	constexpr size_t viewportOffset = 256;
	constexpr size_t renderTargetOffset = 272;
	constexpr size_t projectionDataOffset = 320;
	constexpr size_t volumetricSlicesOffset = 368;
	constexpr size_t projectionInverseOffset = 1488;
	constexpr size_t froxelOffset = 1808;
	static_assert( froxelOffset + 80 == bufferSize );

	std::vector<uint8_t> data( bufferSize, 0 );
	WriteIdentity( data, 0 );
	WriteIdentity( data, 64 );
	WriteIdentity( data, 128 );
	WriteVector4( data,
		viewportOffset,
		0.0f,
		0.0f,
		static_cast<float>( options.dimensions.width ),
		static_cast<float>( options.dimensions.height ) );
	WriteVector4( data,
		renderTargetOffset,
		static_cast<float>( options.dimensions.width ),
		static_cast<float>( options.dimensions.height ),
		1.0f,
		0.0f );
	WriteVector4( data, projectionDataOffset, 1.0f, 1.0f, 1.0f, 1.0f );
	WriteVector4( data, volumetricSlicesOffset, 1000.0f, 10000.0f, 100000.0f, 1000000.0f );
	WriteIdentity( data, projectionInverseOffset );

	constexpr float maxDistance = 1000.0f;
	constexpr float opticalThickness = 1.0f;
	constexpr float baseDensity = opticalThickness / maxDistance;
	const float maxDistanceVisibility = std::exp( -opticalThickness );
	WriteFloat( data, froxelOffset + 0, 0.5f );
	WriteFloat( data, froxelOffset + 4, 0.6f );
	WriteFloat( data, froxelOffset + 8, 0.7f );
	WriteFloat( data, froxelOffset + 12, 0.1f );
	WriteFloat( data, froxelOffset + 16, baseDensity );
	WriteFloat( data, froxelOffset + 20, maxDistance );
	WriteFloat( data, froxelOffset + 24, maxDistanceVisibility );
	WriteFloat( data, froxelOffset + 28, 0.0f );
	WriteFloat( data, froxelOffset + 32, -0.25f );
	return data;
}

bool ValidateApplyConstants( const std::vector<uint8_t>& data, const Options& options )
{
	constexpr size_t bufferSize = 1888;
	constexpr size_t renderTargetOffset = 272;
	constexpr size_t volumetricSlicesOffset = 368;
	constexpr size_t projectionInverseOffset = 1488;
	constexpr size_t froxelOffset = 1808;
	if( data.size() != bufferSize || !IsIdentity( data, 0 ) || !IsIdentity( data, projectionInverseOffset ) ||
		ReadFloat( data, renderTargetOffset ) != static_cast<float>( options.dimensions.width ) ||
		ReadFloat( data, renderTargetOffset + 4 ) != static_cast<float>( options.dimensions.height ) )
	{
		return false;
	}
	for( size_t offset = volumetricSlicesOffset; offset < volumetricSlicesOffset + 16; offset += sizeof( float ) )
	{
		if( !std::isfinite( ReadFloat( data, offset ) ) || ReadFloat( data, offset ) <= 0.0f )
			return false;
	}
	const float baseDensity = ReadFloat( data, froxelOffset + 16 );
	const float maxDistance = ReadFloat( data, froxelOffset + 20 );
	const float maxDistanceVisibility = ReadFloat( data, froxelOffset + 24 );
	const float expectedVisibility = std::exp( -baseDensity * maxDistance );
	return std::isfinite( baseDensity ) && std::isfinite( maxDistance ) && std::isfinite( maxDistanceVisibility ) &&
		baseDensity > 0.0f && maxDistance > 0.0f && maxDistanceVisibility > 0.0f &&
		maxDistanceVisibility < 1.0f && std::abs( maxDistanceVisibility - expectedVisibility ) <= 1.0e-6f;
}

NSDictionary* ApplyConstantContractDictionary( const std::vector<uint8_t>& data, const Options& options )
{
	constexpr size_t renderTargetOffset = 272;
	constexpr size_t volumetricSlicesOffset = 368;
	constexpr size_t projectionInverseOffset = 1488;
	constexpr size_t froxelOffset = 1808;
	const float baseDensity = ReadFloat( data, froxelOffset + 16 );
	const float maxDistance = ReadFloat( data, froxelOffset + 20 );
	const float maxDistanceVisibility = ReadFloat( data, froxelOffset + 24 );
	return @{
		@"byteSize": @( data.size() ),
		@"contractPassed": @( ValidateApplyConstants( data, options ) ),
		@"viewInverseTransposeIdentity": @( IsIdentity( data, 0 ) ),
		@"projectionInverseIdentity": @( IsIdentity( data, projectionInverseOffset ) ),
		@"targetResolution": @[
			@( ReadFloat( data, renderTargetOffset ) ),
			@( ReadFloat( data, renderTargetOffset + 4 ) ),
		],
		@"volumetricSlices": @[
			@( ReadFloat( data, volumetricSlicesOffset ) ),
			@( ReadFloat( data, volumetricSlicesOffset + 4 ) ),
			@( ReadFloat( data, volumetricSlicesOffset + 8 ) ),
			@( ReadFloat( data, volumetricSlicesOffset + 12 ) ),
		],
		@"froxelFog": @{
			@"backgroundVisibility": @( ReadFloat( data, froxelOffset + 12 ) ),
			@"baseDensity": @( baseDensity ),
			@"maxDistance": @( maxDistance ),
			@"maxDistanceVisibility": @( maxDistanceVisibility ),
			@"expectedMaxDistanceVisibility": @( std::exp( -baseDensity * maxDistance ) ),
			@"environmentIntensity": @( ReadFloat( data, froxelOffset + 28 ) ),
			@"environmentG": @( ReadFloat( data, froxelOffset + 32 ) ),
		},
	};
}

bool DispatchDimensions( const Options& options, MTLSize threads, MTLSize& groups )
{
	if( options.dispatch == "exact" &&
		( options.dimensions.width % threads.width || options.dimensions.height % threads.height ||
		  options.dimensions.depth % threads.depth ) )
	{
		return false;
	}
	groups = MTLSizeMake( ( options.dimensions.width + threads.width - 1 ) / threads.width,
						  ( options.dimensions.height + threads.height - 1 ) / threads.height,
						  ( options.dimensions.depth + threads.depth - 1 ) / threads.depth );
	return true;
}

void Synchronize( TrinityALImpl::MetalWorkQueue* queue,
				  const Options& options,
				  id<MTLTexture> first,
				  id<MTLTexture> second )
{
	if( options.synchronization == "encoder" )
	{
		queue->EndEncoder();
	}
	else if( options.synchronization == "resource" )
	{
		id<MTLComputeCommandEncoder> encoder = queue->GetComputeEncoder( @"Synthetic resource barrier" );
		id<MTLResource> resources[] = { first, second };
		[encoder memoryBarrierWithResources:resources count:second ? 2 : 1];
		queue->ReleaseEncoder( false );
	}
}

bool EncodeSyntheticFunction( TrinityALImpl::MetalWorkQueue* queue,
							  id<MTLLibrary> library,
							  NSString* functionName,
							  id<MTLTexture> input,
							  id<MTLTexture> output,
							  const Options& options,
							  NSString* label )
{
	id<MTLFunction> function = [library newFunctionWithName:functionName];
	if( !function )
	{
		return false;
	}
	const MTLSize threads = MTLSizeMake( 8, 8, 4 );
	MTLSize groups;
	if( !DispatchDimensions( options, threads, groups ) )
	{
		std::fprintf( stderr, "Exact dispatch requires dimensions divisible by 8x8x4\n" );
		return false;
	}

	TrinityALImpl::ShaderResourceMask masks[Tr2RenderContextEnum::SHADER_TYPE_COUNT] = {};
	masks[Tr2RenderContextEnum::COMPUTE_SHADER].textureMask = output == input ? 1u : 3u;
	id<MTLTexture> textures[2] = { input, output };
	if( output == input )
	{
		textures[0] = output;
		textures[1] = nil;
	}
	queue->SetTextures( Tr2RenderContextEnum::COMPUTE_SHADER, textures, NSMakeRange( 0, 2 ) );
	queue->SetShaders( nil, nil, function, threads, masks );
	id<MTLComputeCommandEncoder> encoder = queue->GetComputeEncoder( label );
	queue->ReleaseEncoder( false );
	queue->Dispatch( static_cast<uint32_t>( groups.width ),
					 static_cast<uint32_t>( groups.height ),
					 static_cast<uint32_t>( groups.depth ) );
	return true;
}

bool EncodeSynthetic( Tr2RenderContextAL& renderContext,
					  id<MTLLibrary> library,
					  id<MTLTexture> first,
					  id<MTLTexture> second,
					  const Options& options,
					  id<MTLTexture> __strong& finalTexture )
{
	auto* queue = renderContext.GetMetalWorkQueue();
	const float clearValue[] = { 0.125f, 0.25f, 0.5f, 1.0f };
	if( options.stage == "clear" )
	{
		queue->ClearTexture( first, 0, clearValue );
		finalTexture = first;
		return true;
	}

	auto encode = [&]( NSString* outOfPlace,
					   NSString* inPlace,
					   NSString* label,
					   id<MTLTexture> __strong& input,
					   id<MTLTexture> __strong& output ) {
		const bool useInPlace = options.alias == "in-place" && inPlace != nil;
		const bool result = EncodeSyntheticFunction(
			queue, library, useInPlace ? inPlace : outOfPlace, input, useInPlace ? input : output, options, label );
		if( result )
		{
			Synchronize( queue, options, input, useInPlace ? nil : output );
			if( !useInPlace )
			{
				std::swap( input, output );
			}
		}
		return result;
	};

	id<MTLTexture> input = first;
	id<MTLTexture> output = second;
	if( !EncodeSyntheticFunction( queue, library, @"writeSynthetic", input, input, options, @"Synthetic initialize" ) )
	{
		return false;
	}
	Synchronize( queue, options, input, nil );

	for( uint32_t iteration = 0; iteration < options.iterations; ++iteration )
	{
		if( options.stage == "mie" )
		{
			if( !EncodeSyntheticFunction( queue, library, @"mieSynthetic", input, input, options, @"Synthetic Mie" ) )
				return false;
			Synchronize( queue, options, input, nil );
		}
		else if( options.stage == "calculate" )
		{
			if( !EncodeSyntheticFunction(
					queue, library, @"writeSynthetic", input, input, options, @"Synthetic calculate" ) )
				return false;
			Synchronize( queue, options, input, nil );
		}
		else if( options.stage == "filter" )
		{
			if( !encode( @"filterSynthetic", @"inPlaceSynthetic", @"Synthetic filter", input, output ) )
				return false;
		}
		else if( options.stage == "raymarch" )
		{
			if( !encode( @"raymarchSynthetic", @"inPlaceSynthetic", @"Synthetic raymarch", input, output ) )
				return false;
		}
		else if( options.stage == "apply" )
		{
			if( !encode( @"applySynthetic", @"inPlaceSynthetic", @"Synthetic apply", input, output ) )
				return false;
		}
		else if( options.stage == "chain" )
		{
			if( !encode( @"filterSynthetic", @"inPlaceSynthetic", @"Synthetic filter", input, output ) ||
				!encode( @"raymarchSynthetic", @"inPlaceSynthetic", @"Synthetic raymarch", input, output ) )
				return false;
			if( options.temporal == "on" &&
				!encode( @"filterSynthetic", @"inPlaceSynthetic", @"Synthetic temporal", input, output ) )
				return false;
			if( !encode( @"applySynthetic", @"inPlaceSynthetic", @"Synthetic apply", input, output ) )
				return false;
		}
		else
		{
			return false;
		}
	}
	finalTexture = input;
	return true;
}

bool EncodeClientCompute( TrinityALImpl::MetalWorkQueue* queue,
						  id<MTLFunction> function,
						  const ClientStage& stage,
						  const Options& options,
						  const std::array<id<MTLTexture>, METAL_MAX_BOUND_TEXTURES>& textures,
						  uint32_t textureMask,
						  const std::vector<uint8_t>& constants,
						  uint32_t constantIndex,
						  NSString* label )
{
	if( !function )
	{
		return false;
	}
	MTLSize groups;
	if( !DispatchDimensions( options, stage.threadgroup, groups ) )
	{
		std::fprintf( stderr,
					  "Exact dispatch is not divisible by reflected %lux%lux%lu threadgroup\n",
					  stage.threadgroup.width,
					  stage.threadgroup.height,
					  stage.threadgroup.depth );
		return false;
	}
	TrinityALImpl::ShaderResourceMask masks[Tr2RenderContextEnum::SHADER_TYPE_COUNT] = {};
	masks[Tr2RenderContextEnum::COMPUTE_SHADER].textureMask = textureMask;
	TrinityALImpl::ConstantBufferToken token;
	if( !constants.empty() )
	{
		token.Invalidate();
		masks[Tr2RenderContextEnum::COMPUTE_SHADER].constantBufferMask = 1u << constantIndex;
		queue->SetConstants( Tr2RenderContextEnum::COMPUTE_SHADER,
							 constants.data(),
							 static_cast<uint32_t>( constants.size() ),
							 token,
							 constantIndex );
	}
	queue->SetTextures( Tr2RenderContextEnum::COMPUTE_SHADER, textures.data(), NSMakeRange( 0, textures.size() ) );
	queue->SetShaders( nil, nil, function, stage.threadgroup, masks );
	queue->GetComputeEncoder( label );
	queue->ReleaseEncoder( false );
	queue->Dispatch( static_cast<uint32_t>( groups.width ),
					 static_cast<uint32_t>( groups.height ),
					 static_cast<uint32_t>( groups.depth ) );
	return true;
}

bool EncodeClientApply( TrinityALImpl::MetalWorkQueue* queue,
						id<MTLDevice> device,
						id<MTLFunction> vertexFunction,
						id<MTLFunction> fragmentFunction,
						id<MTLTexture> froxel,
						id<MTLTexture> mieCube,
						id<MTLTexture> depth,
						id<MTLTexture> output,
						const std::vector<uint8_t>& constants )
{
	if( !vertexFunction || !fragmentFunction || !froxel || !mieCube || !depth || !output || constants.size() != 1888 )
	{
		return false;
	}
	MTLVertexDescriptor* vertexDescriptor = [MTLVertexDescriptor vertexDescriptor];
	vertexDescriptor.attributes[0].format = MTLVertexFormatFloat4;
	vertexDescriptor.attributes[0].offset = 0;
	vertexDescriptor.attributes[0].bufferIndex = 0;
	vertexDescriptor.layouts[0].stride = sizeof( float ) * 4;

	MTLRenderPipelineDescriptor* descriptor = [MTLRenderPipelineDescriptor new];
	descriptor.label = @"Client froxel apply";
	descriptor.vertexFunction = vertexFunction;
	descriptor.fragmentFunction = fragmentFunction;
	descriptor.vertexDescriptor = vertexDescriptor;
	descriptor.colorAttachments[0].pixelFormat = output.pixelFormat;
	NSError* error = nil;
	id<MTLRenderPipelineState> pipeline = [device newRenderPipelineStateWithDescriptor:descriptor error:&error];
	if( !pipeline )
	{
		std::fprintf( stderr, "Client apply pipeline failed: %s\n", error.localizedDescription.UTF8String );
		return false;
	}
	const float vertices[3][4] = {
		{ -1.0f, -1.0f, 0.0f, 1.0f },
		{ 3.0f, -1.0f, 0.0f, 1.0f },
		{ -1.0f, 3.0f, 0.0f, 1.0f },
	};
	id<MTLBuffer> vertexBuffer = [device newBufferWithBytes:vertices
													 length:sizeof( vertices )
													options:MTLResourceStorageModeShared];
	id<MTLBuffer> constantBuffer = [device newBufferWithBytes:constants.data()
													   length:constants.size()
													  options:MTLResourceStorageModeShared];
	if( !vertexBuffer || !constantBuffer )
	{
		return false;
	}
	queue->SetRenderAttachments( output, 0 );
	id<MTLRenderCommandEncoder> encoder = queue->GetRenderEncoder( @"Client froxel apply" );
	if( !encoder )
	{
		return false;
	}
	[encoder setRenderPipelineState:pipeline];
	[encoder setViewport:MTLViewport{ 0.0,
									  0.0,
									  static_cast<double>( output.width ),
									  static_cast<double>( output.height ),
									  0.0,
									  1.0 }];
	[encoder setVertexBuffer:vertexBuffer offset:0 atIndex:0];
	[encoder setFragmentTexture:froxel atIndex:0];
	[encoder setFragmentTexture:mieCube atIndex:1];
	[encoder setFragmentTexture:depth atIndex:2];
	[encoder setFragmentBuffer:constantBuffer offset:0 atIndex:6];
	[encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];
	queue->ReleaseEncoder( false );
	return true;
}

NSDictionary* ReadbackTexture( id<MTLTexture> texture, bool& canariesIntact )
{
	const NSUInteger bytesPerPixel = BytesPerPixel( texture.pixelFormat );
	const NSUInteger bytesPerRow = bytesPerPixel * texture.width;
	const NSUInteger bytesPerImage = bytesPerRow * texture.height;
	NSUInteger slices = 1;
	NSUInteger depth = 1;
	if( texture.textureType == MTLTextureType3D )
	{
		depth = texture.depth;
	}
	else if( texture.textureType == MTLTextureTypeCube )
	{
		slices = 6;
	}
	else if( texture.textureType == MTLTextureType2DArray )
	{
		slices = texture.arrayLength;
	}
	const size_t rawSize = bytesPerImage * depth * slices;
	constexpr size_t guardSize = 64;
	std::vector<uint8_t> storage( rawSize + guardSize * 2, 0xa5 );
	for( NSUInteger slice = 0; slice < slices; ++slice )
	{
		[texture getBytes:storage.data() + guardSize + slice * bytesPerImage * depth
			  bytesPerRow:bytesPerRow
			bytesPerImage:bytesPerImage
			   fromRegion:MTLRegionMake3D( 0, 0, 0, texture.width, texture.height, depth )
			  mipmapLevel:0
					slice:slice];
	}
	canariesIntact =
		std::all_of( storage.begin(), storage.begin() + guardSize, []( uint8_t value ) { return value == 0xa5; } ) &&
		std::all_of( storage.end() - guardSize, storage.end(), []( uint8_t value ) { return value == 0xa5; } );
	const uint8_t* bytes = storage.data() + guardSize;
	const size_t texelCount = bytesPerPixel == 0 ? 0 : rawSize / bytesPerPixel;
	const NumericReadbackDiagnostics numeric = AnalyzeNumericReadback( texture.pixelFormat, bytes, texelCount );
	const size_t nonzero =
		static_cast<size_t>( std::count_if( bytes, bytes + rawSize, []( uint8_t value ) { return value != 0; } ) );
	size_t differingTexels = 0;
	for( size_t offset = bytesPerPixel; offset < rawSize; offset += bytesPerPixel )
	{
		if( std::memcmp( bytes, bytes + offset, bytesPerPixel ) != 0 )
			++differingTexels;
	}
	return @{
		@"rawHashFNV1a64": HexHash( Fnv1a64( bytes, rawSize ) ),
		@"rawBytes": @( rawSize ),
		@"texelCount": @( texelCount ),
		@"nonzeroBytes": @( nonzero ),
		@"differingTexelsFromFirst": @( differingTexels ),
		@"nonuniform": @( differingTexels != 0 ),
		@"numericValidationAvailable": @( numeric.available ),
		@"numericComponents": @( numeric.components ),
		@"finiteComponents": @( numeric.finiteComponents ),
		@"infiniteComponents": @( numeric.infiniteComponents ),
		@"nanComponents": @( numeric.nanComponents ),
		@"finiteTexels": @( numeric.finiteTexels ),
		@"nonfiniteTexels": @( numeric.nonfiniteTexels ),
		@"allFinite": @( numeric.available && numeric.nonfiniteTexels == 0 ),
		@"readbackCanariesIntact": @( canariesIntact ),
		@"textureType": @( texture.textureType ),
		@"width": @( texture.width ),
		@"height": @( texture.height ),
		@"depth": @( depth ),
		@"slices": @( slices ),
	};
}

NSDictionary* SubmissionDictionary( const Tr2MetalSubmissionDiagnostics& diagnostics )
{
	NSMutableArray* encoders = [NSMutableArray array];
	for( const auto& item : diagnostics.encoders )
	{
		[encoders addObject:@{
			@"label": [NSString stringWithUTF8String:item.label.c_str()],
			@"errorState": @( item.errorState ),
		}];
	}
	NSMutableArray* bindings = [NSMutableArray array];
	for( const auto& item : diagnostics.bindings )
	{
		[bindings addObject:@{
			@"name": [NSString stringWithUTF8String:item.name.c_str()],
			@"kind": [NSString stringWithUTF8String:item.kind.c_str()],
			@"index": @( item.index ),
			@"requiredBytes": @( item.requiredBytes ),
			@"boundBytes": @( item.boundBytes ),
			@"valid": @( item.valid ),
			@"failure": [NSString stringWithUTF8String:item.failure.c_str()],
		}];
	}
	return @{
		@"commandStatus": @( diagnostics.commandStatus ),
		@"errorCode": @( diagnostics.errorCode ),
		@"errorDomain": [NSString stringWithUTF8String:diagnostics.errorDomain.c_str()],
		@"errorDescription": [NSString stringWithUTF8String:diagnostics.errorDescription.c_str()],
		@"cpuEncodeSeconds": @( diagnostics.cpuEncodeSeconds ),
		@"cpuWaitSeconds": @( diagnostics.cpuWaitSeconds ),
		@"gpuStartTime": @( diagnostics.gpuStartTime ),
		@"gpuEndTime": @( diagnostics.gpuEndTime ),
		@"pipelineUid": @( diagnostics.pipelineUid ),
		@"validationEnabled": @( diagnostics.validationEnabled ),
		@"bindingPreflightPassed": @( diagnostics.bindingPreflightPassed ),
		@"encoders": encoders,
		@"bindings": bindings,
	};
}

bool WriteJson( const std::string& outputPath, NSDictionary* report )
{
	NSError* error = nil;
	NSData* data = [NSJSONSerialization dataWithJSONObject:report
												   options:NSJSONWritingPrettyPrinted | NSJSONWritingSortedKeys
													 error:&error];
	if( !data )
	{
		std::fprintf( stderr, "Failed to serialize report: %s\n", error.localizedDescription.UTF8String );
		return false;
	}
	NSString* path = [NSString stringWithUTF8String:outputPath.c_str()];
	NSString* directory = path.stringByDeletingLastPathComponent;
	if( ![[NSFileManager defaultManager] createDirectoryAtPath:directory
								   withIntermediateDirectories:YES
													attributes:nil
														 error:&error] )
	{
		std::fprintf( stderr, "Failed to create report directory: %s\n", error.localizedDescription.UTF8String );
		return false;
	}
	if( ![data writeToFile:path options:NSDataWritingAtomic error:&error] )
	{
		std::fprintf( stderr, "Failed to write JSON report: %s\n", error.localizedDescription.UTF8String );
		return false;
	}
	const int file = open( outputPath.c_str(), O_RDONLY );
	if( file < 0 || fsync( file ) != 0 )
	{
		if( file >= 0 )
			close( file );
		std::fprintf( stderr, "Failed to fsync JSON report %s\n", outputPath.c_str() );
		return false;
	}
	close( file );
	const std::string directoryPath = path.stringByDeletingLastPathComponent.UTF8String;
	const int directoryFd = open( directoryPath.c_str(), O_RDONLY );
	if( directoryFd >= 0 )
	{
		fsync( directoryFd );
		close( directoryFd );
	}
	return true;
}

bool WriteReport( const Options& options, NSDictionary* report )
{
	return WriteJson( options.reportPath, report );
}

bool RunLabTool( NSArray<NSString*>* arguments )
{
	NSTask* task = [NSTask new];
	task.executableURL = [NSURL fileURLWithPath:@"/usr/bin/python3"];
	NSMutableArray<NSString*>* completeArguments =
		[NSMutableArray arrayWithObject:[NSString stringWithUTF8String:TRINITY_FROXEL_LAB_TOOL]];
	[completeArguments addObjectsFromArray:arguments];
	task.arguments = completeArguments;
	NSPipe* output = [NSPipe pipe];
	task.standardOutput = output;
	task.standardError = output;
	NSError* error = nil;
	if( ![task launchAndReturnError:&error] )
	{
		std::fprintf( stderr, "Failed to launch froxel lab ledger tool: %s\n", error.localizedDescription.UTF8String );
		return false;
	}
	[task waitUntilExit];
	NSData* outputData = [output.fileHandleForReading readDataToEndOfFile];
	if( task.terminationStatus != 0 )
	{
		NSString* detail = [[NSString alloc] initWithData:outputData encoding:NSUTF8StringEncoding];
		std::fprintf( stderr, "Froxel lab ledger tool failed: %s\n", detail.UTF8String ?: "unknown error" );
		return false;
	}
	return true;
}

bool PersistSubmissionPreflight( Tr2RenderContextAL& renderContext, const Options& options )
{
	if( options.ledgerPath.empty() )
	{
		return true;
	}
	Tr2MetalSubmissionDiagnostics preflight;
	if( FAILED( renderContext.GetHeadlessSubmissionPreflight( &preflight ) ) )
	{
		std::fprintf( stderr, "Metal binding preflight failed before GPU submission\n" );
		return false;
	}
	NSString* ledger = [NSString stringWithUTF8String:options.ledgerPath.c_str()];
	NSString* preflightPath =
		[ledger.stringByDeletingLastPathComponent stringByAppendingPathComponent:@"submission-preflight.json"];
	NSDictionary* record = @{
		@"schemaVersion": @1,
		@"kernelSet": [NSString stringWithUTF8String:options.kernelSet.c_str()],
		@"stage": [NSString stringWithUTF8String:options.stage.c_str()],
		@"dimensions": @{
			@"width": @( options.dimensions.width ),
			@"height": @( options.dimensions.height ),
			@"depth": @( options.dimensions.depth ),
		},
		@"format": [NSString stringWithUTF8String:options.format.c_str()],
		@"dispatch": [NSString stringWithUTF8String:options.dispatch.c_str()],
		@"alias": [NSString stringWithUTF8String:options.alias.c_str()],
		@"synchronization": [NSString stringWithUTF8String:options.synchronization.c_str()],
		@"temporal": [NSString stringWithUTF8String:options.temporal.c_str()],
		@"metal": SubmissionDictionary( preflight ),
	};
	if( !WriteJson( preflightPath.UTF8String, record ) )
	{
		return false;
	}
	return RunLabTool( @[
		@"_mark-submitted",
		@"--ledger",
		ledger,
		@"--preflight",
		preflightPath,
	] );
}

int RunSynthetic( const Options& options )
{
	Tr2PrimaryRenderContextAL renderContext;
	Tr2RenderContextAL::SetPrimaryRenderContext( &renderContext );
	if( FAILED( renderContext.CreateHeadlessDevice( 0, true ) ) )
	{
		std::fprintf( stderr, "Failed to create TrinityAL headless Metal device\n" );
		return 1;
	}

	id<MTLDevice> device = renderContext.GetMetalContext()->GetDevice();
	if( !ValidateCpuTextureFormatRoundTrips( device ) )
	{
		std::fprintf( stderr, "Synthetic scalar texture CPU round-trip failed\n" );
		return 1;
	}
	NSError* error = nil;
	NSString* source = [NSString stringWithUTF8String:kSyntheticSource];
	id<MTLLibrary> library = [device newLibraryWithSource:source options:nil error:&error];
	if( !library )
	{
		std::fprintf( stderr, "Synthetic Metal library failed: %s\n", error.localizedDescription.UTF8String );
		return 1;
	}

	id<MTLTexture> first = CreateTexture( device, options );
	id<MTLTexture> second = CreateTexture( device, options );
	if( !first || !second )
	{
		std::fprintf( stderr, "Failed to allocate synthetic 3D textures\n" );
		return 1;
	}
	id<MTLTexture> finalTexture = nil;
	if( !EncodeSynthetic( renderContext, library, first, second, options, finalTexture ) )
	{
		std::fprintf( stderr, "Synthetic froxel encoding failed\n" );
		return 1;
	}
	if( !PersistSubmissionPreflight( renderContext, options ) )
	{
		return 1;
	}

	Tr2MetalSubmissionDiagnostics diagnostics;
	const ALResult submitResult = renderContext.SubmitHeadlessAndWait( &diagnostics );
	if( FAILED( submitResult ) )
	{
		WriteReport(
			options, @{
				@"schemaVersion": @1,
				@"passed": @NO,
				@"kernelSet": @"synthetic",
				@"submission": SubmissionDictionary( diagnostics ),
			} );
		std::fprintf( stderr, "Synthetic Metal submission failed: %s\n", diagnostics.errorDescription.c_str() );
		return 1;
	}

	const NSUInteger bytesPerPixel = BytesPerPixel( finalTexture.pixelFormat );
	const NSUInteger bytesPerRow = bytesPerPixel * finalTexture.width;
	const NSUInteger bytesPerImage = bytesPerRow * finalTexture.height;
	std::vector<uint8_t> bytes( bytesPerImage * finalTexture.depth );
	[finalTexture getBytes:bytes.data()
			   bytesPerRow:bytesPerRow
			 bytesPerImage:bytesPerImage
				fromRegion:MTLRegionMake3D( 0, 0, 0, finalTexture.width, finalTexture.height, finalTexture.depth )
			   mipmapLevel:0
					 slice:0];
	const size_t nonzero =
		static_cast<size_t>( std::count_if( bytes.begin(), bytes.end(), []( uint8_t value ) { return value != 0; } ) );
	const uint64_t hash = Fnv1a64( bytes.data(), bytes.size() );
	const bool passed = nonzero != 0 && diagnostics.bindingPreflightPassed;

	NSDictionary* report = @{
		@"schemaVersion": @1,
		@"passed": @( passed ),
		@"kernelSet": @"synthetic",
		@"stage": [NSString stringWithUTF8String:options.stage.c_str()],
		@"dimensions": @{
			@"width": @( options.dimensions.width ),
			@"height": @( options.dimensions.height ),
			@"depth": @( options.dimensions.depth ),
		},
		@"format": [NSString stringWithUTF8String:options.format.c_str()],
		@"temporal": [NSString stringWithUTF8String:options.temporal.c_str()],
		@"alias": [NSString stringWithUTF8String:options.alias.c_str()],
		@"synchronization": [NSString stringWithUTF8String:options.synchronization.c_str()],
		@"dispatch": [NSString stringWithUTF8String:options.dispatch.c_str()],
		@"iterations": @( options.iterations ),
		@"threadgroup": @[@8, @8, @4],
		@"rawHashFNV1a64": HexHash( hash ),
		@"rawBytes": @( bytes.size() ),
		@"nonzeroBytes": @( nonzero ),
		@"canariesIntact": @YES,
		@"cpuScalarTextureRoundTrips": @YES,
		@"submission": SubmissionDictionary( diagnostics ),
	};
	if( !WriteReport( options, report ) )
	{
		return 1;
	}
	std::printf( "Synthetic froxel report: %s\n", options.reportPath.c_str() );
	return passed ? 0 : 1;
}

int RunClient( const Options& options, const ClientPackage& package )
{
	Tr2PrimaryRenderContextAL renderContext;
	Tr2RenderContextAL::SetPrimaryRenderContext( &renderContext );
	if( FAILED( renderContext.CreateHeadlessDevice( 0, true ) ) )
	{
		std::fprintf( stderr, "Failed to create TrinityAL headless Metal device\n" );
		return 1;
	}
	id<MTLDevice> device = renderContext.GetMetalContext()->GetDevice();
	if( !ValidateCpuTextureFormatRoundTrips( device ) )
	{
		std::fprintf( stderr, "Client scalar texture CPU round-trip failed\n" );
		return 1;
	}
	auto stage = [&]( const char* name ) -> const ClientStage& { return package.stages.at( name ); };
	id<MTLFunction> applyVS = LoadClientFunction( device, stage( "apply_vs" ) );
	id<MTLFunction> applyPS = LoadClientFunction( device, stage( "apply_ps" ) );
	id<MTLFunction> mieFunction = LoadClientFunction( device, stage( "mie" ) );
	id<MTLFunction> calculateFunction = LoadClientFunction( device, stage( "calculate" ) );
	id<MTLFunction> filterFunction = LoadClientFunction( device, stage( "filter" ) );
	id<MTLFunction> raymarchOutFunction = LoadClientFunction( device, stage( "raymarch_out" ) );
	id<MTLFunction> raymarchInFunction = LoadClientFunction( device, stage( "raymarch_in" ) );
	if( !applyVS || !applyPS || !mieFunction || !calculateFunction || !filterFunction || !raymarchOutFunction ||
		!raymarchInFunction )
	{
		return 1;
	}

	const MTLPixelFormat froxelFormat = PixelFormat( options.format );
	id<MTLTexture> froxelA = CreateTexture( device, options );
	id<MTLTexture> froxelB = CreateTexture( device, options );
	id<MTLTexture> froxelC = CreateTexture( device, options );
	id<MTLTexture> lightProfiles =
		Create2DArrayTexture( device, 4, 4, 1, MTLPixelFormatRGBA16Float, MTLTextureUsageShaderRead );
	id<MTLTexture> shadowAtlas = Create2DTexture( device, 4, 4, MTLPixelFormatR32Float, MTLTextureUsageShaderRead );
	id<MTLTexture> environmentCube =
		CreateCubeTexture( device, 8, MTLPixelFormatRGBA16Float, MTLTextureUsageShaderRead );
	id<MTLTexture> depth = Create2DTexture( device,
											options.dimensions.width,
											options.dimensions.height,
											MTLPixelFormatR32Float,
											MTLTextureUsageShaderRead );
	id<MTLTexture> applyOutput = Create2DTexture( device,
												  options.dimensions.width,
												  options.dimensions.height,
												  froxelFormat,
												  MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead );
	if( !froxelA || !froxelB || !froxelC || !lightProfiles || !shadowAtlas || !environmentCube || !depth ||
		!applyOutput )
	{
		std::fprintf( stderr, "Failed to allocate client-stage resources\n" );
		return 1;
	}
	if( !FillTextureFromCpu( lightProfiles, 0x38 ) || !FillTextureFromCpu( environmentCube, 0x38 ) ||
		!FillTextureFromCpu( shadowAtlas, 0x00 ) || !FillR32FloatTextureFromCpu( depth, 0.5f ) ||
		!ZeroTextureFromCpu( froxelA ) || !ZeroTextureFromCpu( froxelB ) || !ZeroTextureFromCpu( froxelC ) ||
		!ZeroTextureFromCpu( applyOutput ) )
	{
		std::fprintf( stderr, "Failed to initialize client-stage resources\n" );
		return 1;
	}

	NSError* error = nil;
	id<MTLLibrary> syntheticLibrary = [device newLibraryWithSource:[NSString stringWithUTF8String:kSyntheticSource]
														   options:nil
															 error:&error];
	if( !syntheticLibrary )
	{
		std::fprintf( stderr, "Synthetic prelude compilation failed: %s\n", error.localizedDescription.UTF8String );
		return 1;
	}
	auto* queue = renderContext.GetMetalWorkQueue();
	if( !EncodeSyntheticFunction(
			queue, syntheticLibrary, @"writeSynthetic", froxelA, froxelA, options, @"Source-controlled client input" ) )
	{
		return 1;
	}
	Synchronize( queue, options, froxelA, nil );

	const Options mieWorkOptions = MakeMieWorkOptions( options );
	const std::vector<uint8_t> froxelConstants = MakeFroxelConstants( options );
	const std::vector<uint8_t> mieConstants = MakeMieConstants( mieWorkOptions );
	const std::vector<uint8_t> applyConstants = MakeApplyConstants( options );
	if( !ValidateFroxelConstants( froxelConstants, options ) ||
		!ValidateMieConstants( mieConstants, mieWorkOptions ) || !ValidateApplyConstants( applyConstants, options ) )
	{
		std::fprintf( stderr, "Client constant-buffer contract validation failed\n" );
		return 1;
	}
	NSDictionary* froxelConstantContract = FroxelConstantContractDictionary( froxelConstants, options );
	NSDictionary* mieConstantContract = MieConstantContractDictionary( mieConstants, mieWorkOptions );
	NSDictionary* applyConstantContract = ApplyConstantContractDictionary( applyConstants, options );
	std::array<id<MTLTexture>, METAL_MAX_BOUND_TEXTURES> textures = {};
	id<MTLTexture> finalTexture = nil;
	id<MTLTexture> mieOutput = nil;
	id<MTLTexture> mieCube = nil;
	if( options.stage == "mie" )
	{
		mieOutput = Create2DArrayTexture( device,
										  mieWorkOptions.dimensions.width,
										  mieWorkOptions.dimensions.height,
										  mieWorkOptions.dimensions.depth,
										  froxelFormat,
										  MTLTextureUsageShaderRead | MTLTextureUsageShaderWrite );
	}
	else
	{
		const NSUInteger dimension = std::max( options.dimensions.width, options.dimensions.height );
		mieCube = CreateCubeTexture(
			device, dimension, froxelFormat, MTLTextureUsageShaderRead | MTLTextureUsageShaderWrite );
		mieOutput = [mieCube newTextureViewWithPixelFormat:froxelFormat
											   textureType:MTLTextureType2DArray
													levels:NSMakeRange( 0, 1 )
													slices:NSMakeRange( 0, 6 )];
	}
	if( !mieOutput )
	{
		std::fprintf( stderr, "Failed to allocate writable Mie texture\n" );
		return 1;
	}
	if( options.stage == "mie" || options.stage == "chain" )
	{
		if( !EncodeSyntheticFunction( queue,
				syntheticLibrary,
				@"writeMieArraySynthetic",
				mieOutput,
				mieOutput,
				mieWorkOptions,
				@"Source-controlled Mie input" ) )
		{
			return 1;
		}
		Synchronize( queue, options, mieOutput, nil );
	}
	else if( !ZeroTextureFromCpu( mieOutput ) )
	{
		std::fprintf( stderr, "Failed to initialize writable Mie texture\n" );
		return 1;
	}

	auto synchronize = [&]( id<MTLTexture> first, id<MTLTexture> second ) {
		Synchronize( queue, options, first, second );
	};
	auto encodeMie = [&]() {
		textures.fill( nil );
		textures[0] = environmentCube;
		textures[1] = mieOutput;
		const bool result = EncodeClientCompute(
			queue, mieFunction, stage( "mie" ), mieWorkOptions, textures, 0x3, mieConstants, 4, @"Client Mie" );
		if( result )
			synchronize( environmentCube, mieOutput );
		return result;
	};
	auto encodeCalculate = [&]( id<MTLTexture> output ) {
		textures.fill( nil );
		textures[0] = lightProfiles;
		textures[1] = output;
		textures[2] = shadowAtlas;
		const bool result = EncodeClientCompute( queue,
												 calculateFunction,
												 stage( "calculate" ),
												 options,
												 textures,
												 0x7,
												 froxelConstants,
												 7,
												 @"Client calculate" );
		if( result )
			synchronize( output, nil );
		return result;
	};
	auto encodeFilter = [&]( id<MTLTexture> input, id<MTLTexture> previous, id<MTLTexture> output ) {
		textures.fill( nil );
		textures[0] = input;
		textures[1] = previous;
		textures[2] = output;
		const bool result = EncodeClientCompute(
			queue, filterFunction, stage( "filter" ), options, textures, 0x7, froxelConstants, 7, @"Client filter" );
		if( result )
			synchronize( input, output );
		return result;
	};
	auto encodeRaymarch = [&]( id<MTLTexture> input, id<MTLTexture> output ) {
		textures.fill( nil );
		const bool inPlace = options.alias == "in-place";
		textures[0] = input;
		if( !inPlace )
			textures[1] = output;
		const ClientStage& selectedStage = stage( inPlace ? "raymarch_in" : "raymarch_out" );
		const bool result = EncodeClientCompute( queue,
												 inPlace ? raymarchInFunction : raymarchOutFunction,
												 selectedStage,
												 options,
												 textures,
												 inPlace ? 0x1 : 0x3,
												 froxelConstants,
												 7,
												 @"Client raymarch" );
		if( result )
			synchronize( input, inPlace ? nil : output );
		return result;
	};
	auto encodeApply = [&]( id<MTLTexture> froxel ) {
		if( !mieCube )
		{
			mieCube = CreateCubeTexture( device, 8, froxelFormat, MTLTextureUsageShaderRead );
			if( !mieCube || !FillTextureFromCpu( mieCube, 0x38 ) )
				return false;
		}
		const bool result =
			EncodeClientApply( queue, device, applyVS, applyPS, froxel, mieCube, depth, applyOutput, applyConstants );
		if( result )
			synchronize( applyOutput, nil );
		return result;
	};

	bool encoded = true;
	for( uint32_t iteration = 0; iteration < options.iterations && encoded; ++iteration )
	{
		if( options.stage == "mie" )
		{
			encoded = encodeMie();
			finalTexture = mieOutput;
		}
		else if( options.stage == "calculate" )
		{
			encoded = encodeCalculate( froxelB );
			finalTexture = froxelB;
		}
		else if( options.stage == "filter" )
		{
			encoded = encodeFilter( froxelA, froxelB, froxelC );
			finalTexture = froxelC;
		}
		else if( options.stage == "raymarch" )
		{
			encoded = encodeRaymarch( froxelA, froxelB );
			finalTexture = options.alias == "in-place" ? froxelA : froxelB;
		}
		else if( options.stage == "apply" )
		{
			encoded = encodeApply( froxelA );
			finalTexture = applyOutput;
		}
		else if( options.stage == "chain" )
		{
			encoded = encodeMie() && encodeCalculate( froxelB );
			id<MTLTexture> current = froxelB;
			if( encoded && options.temporal == "on" )
			{
				encoded = encodeFilter( current, froxelA, froxelC );
				current = froxelC;
			}
			id<MTLTexture> raymarchOutput = current == froxelA ? froxelB : froxelA;
			encoded = encoded && encodeRaymarch( current, raymarchOutput );
			if( options.alias != "in-place" )
				current = raymarchOutput;
			encoded = encoded && encodeApply( current );
			finalTexture = applyOutput;
		}
		else
		{
			encoded = false;
		}
	}
	if( !encoded || !finalTexture )
	{
		std::fprintf( stderr, "Client froxel stage encoding failed\n" );
		return 1;
	}
	if( !PersistSubmissionPreflight( renderContext, options ) )
	{
		return 1;
	}
	Tr2MetalSubmissionDiagnostics diagnostics;
	const ALResult submitResult = renderContext.SubmitHeadlessAndWait( &diagnostics );
	if( FAILED( submitResult ) )
	{
		WriteReport(
			options, @{
				@"schemaVersion": @1,
				@"passed": @NO,
				@"kernelSet": @"client",
				@"packageSha256": [NSString stringWithUTF8String:package.manifestSha256.c_str()],
				@"froxelConstantContract": froxelConstantContract,
				@"mieConstantContract": mieConstantContract,
				@"applyConstantContract": applyConstantContract,
				@"submission": SubmissionDictionary( diagnostics ),
			} );
		return 1;
	}
	bool canariesIntact = false;
	NSDictionary* readback = ReadbackTexture( finalTexture, canariesIntact );
	const bool passed = canariesIntact && diagnostics.bindingPreflightPassed &&
		[readback[@"nonzeroBytes"] unsignedLongLongValue] != 0 &&
		[readback[@"differingTexelsFromFirst"] unsignedLongLongValue] != 0 && [readback[@"allFinite"] boolValue];
	NSMutableDictionary* stageHashes = [NSMutableDictionary dictionary];
	for( const auto& item : package.stages )
	{
		stageHashes[[NSString stringWithUTF8String:item.first.c_str()]] =
			[NSString stringWithUTF8String:item.second.sha256.c_str()];
	}
	NSDictionary* report = @{
		@"schemaVersion": @1,
		@"passed": @( passed ),
		@"kernelSet": @"client",
		@"stage": [NSString stringWithUTF8String:options.stage.c_str()],
		@"dimensions": @{
			@"width": @( options.dimensions.width ),
			@"height": @( options.dimensions.height ),
			@"depth": @( options.dimensions.depth ),
		},
		@"format": [NSString stringWithUTF8String:options.format.c_str()],
		@"temporal": [NSString stringWithUTF8String:options.temporal.c_str()],
		@"alias": [NSString stringWithUTF8String:options.alias.c_str()],
		@"synchronization": [NSString stringWithUTF8String:options.synchronization.c_str()],
		@"dispatch": [NSString stringWithUTF8String:options.dispatch.c_str()],
		@"iterations": @( options.iterations ),
		@"packageSha256": [NSString stringWithUTF8String:package.manifestSha256.c_str()],
		@"stageHashes": stageHashes,
		@"froxelConstantContract": froxelConstantContract,
		@"mieConstantContract": mieConstantContract,
		@"applyConstantContract": applyConstantContract,
		@"readback": readback,
		@"submission": SubmissionDictionary( diagnostics ),
	};
	if( !WriteReport( options, report ) )
	{
		return 1;
	}
	std::printf( "Client froxel report: %s\n", options.reportPath.c_str() );
	return passed ? 0 : 1;
}

bool RunCpuSelfTests()
{
	Options options;
	options.dimensions = { 8, 8, 4 };
	std::vector<uint8_t> froxelConstants = MakeFroxelConstants( options );
	if( !ValidateFroxelConstants( froxelConstants, options ) )
		return false;
	WriteFloat( froxelConstants, 176, 1.0f );
	if( ValidateFroxelConstants( froxelConstants, options ) )
		return false;
	froxelConstants = MakeFroxelConstants( options );
	WriteFloat( froxelConstants, 180, 0.25f );
	if( ValidateFroxelConstants( froxelConstants, options ) )
		return false;
	froxelConstants = MakeFroxelConstants( options );
	WriteUint( froxelConstants, 128, 0 );
	if( ValidateFroxelConstants( froxelConstants, options ) )
		return false;

	const Options mieWorkOptions = MakeMieWorkOptions( options );
	if( mieWorkOptions.dimensions.width != 8 || mieWorkOptions.dimensions.height != 8 ||
		mieWorkOptions.dimensions.depth != 6 )
	{
		return false;
	}
	Options rectangularOptions = options;
	rectangularOptions.dimensions = { 65, 47, 33 };
	const Options rectangularMieOptions = MakeMieWorkOptions( rectangularOptions );
	if( rectangularMieOptions.dimensions.width != 65 || rectangularMieOptions.dimensions.height != 65 ||
		rectangularMieOptions.dimensions.depth != 6 )
	{
		return false;
	}
	std::vector<uint8_t> mieConstants = MakeMieConstants( mieWorkOptions );
	if( !ValidateMieConstants( mieConstants, mieWorkOptions ) )
		return false;
	WriteFloat( mieConstants, 20, 0.0f );
	if( ValidateMieConstants( mieConstants, mieWorkOptions ) )
		return false;
	mieConstants = MakeMieConstants( mieWorkOptions );
	WriteUint( mieConstants, 0, 0 );
	if( ValidateMieConstants( mieConstants, mieWorkOptions ) )
		return false;

	std::vector<uint8_t> constants = MakeApplyConstants( options );
	if( !ValidateApplyConstants( constants, options ) )
		return false;
	WriteFloat( constants, 272, 0.0f );
	if( ValidateApplyConstants( constants, options ) )
		return false;

	uint32_t r11g11b10 = 0;
	NumericReadbackDiagnostics numeric = AnalyzeNumericReadback(
		MTLPixelFormatRG11B10Float, reinterpret_cast<const uint8_t*>( &r11g11b10 ), 1 );
	if( !numeric.available || numeric.finiteComponents != 3 || numeric.finiteTexels != 1 ||
		numeric.nonfiniteTexels != 0 )
	{
		return false;
	}
	r11g11b10 = 0x1fu << 6;
	numeric = AnalyzeNumericReadback(
		MTLPixelFormatRG11B10Float, reinterpret_cast<const uint8_t*>( &r11g11b10 ), 1 );
	if( numeric.infiniteComponents != 1 || numeric.nonfiniteTexels != 1 )
		return false;
	r11g11b10 |= 1u;
	numeric = AnalyzeNumericReadback(
		MTLPixelFormatRG11B10Float, reinterpret_cast<const uint8_t*>( &r11g11b10 ), 1 );
	if( numeric.nanComponents != 1 || numeric.nonfiniteTexels != 1 )
		return false;

	const std::array<uint16_t, 4> rgba16 = { 0x3c00u, 0x7c00u, 0u, 0u };
	numeric = AnalyzeNumericReadback(
		MTLPixelFormatRGBA16Float, reinterpret_cast<const uint8_t*>( rgba16.data() ), 1 );
	if( numeric.finiteComponents != 3 || numeric.infiniteComponents != 1 || numeric.nonfiniteTexels != 1 )
		return false;

	const std::array<uint32_t, 4> rgba32 = { 0x3f800000u, 0x7fc00000u, 0u, 0u };
	numeric = AnalyzeNumericReadback(
		MTLPixelFormatRGBA32Float, reinterpret_cast<const uint8_t*>( rgba32.data() ), 1 );
	if( numeric.finiteComponents != 3 || numeric.nanComponents != 1 || numeric.nonfiniteTexels != 1 )
		return false;

	std::puts( "Froxel probe CPU self-tests passed" );
	return true;
}
}

int main( int argc, char** argv )
{
	@autoreleasepool
	{
		if( argc == 2 && std::strcmp( argv[1], "--self-test" ) == 0 )
			return RunCpuSelfTests() ? 0 : 1;
		Options options;
		if( !ParseOptions( argc, argv, options ) )
		{
			PrintUsage( argv[0] );
			return 2;
		}
		if( options.kernelSet == "client" )
		{
			if( !options.clientKernels || options.ledgerPath.empty() || options.clientPackage.empty() ||
				std::getenv( "TRINITY_FROXEL_GPU_LAB" ) == nullptr ||
				std::strcmp( std::getenv( "TRINITY_FROXEL_GPU_LAB" ), kRiskAcknowledgement ) != 0 )
			{
				std::fprintf( stderr, "Client kernels require the sacrificial-lab authorization gate\n" );
				return 3;
			}
			if( !RunLabTool( @[
					@"_validate-authorization",
					@"--ledger",
					[NSString stringWithUTF8String:options.ledgerPath.c_str()],
				] ) )
			{
				return 3;
			}
			ClientPackage package;
			if( !LoadClientPackage( options, package ) )
			{
				return 4;
			}
			return RunClient( options, package );
		}
		return RunSynthetic( options );
	}
}
