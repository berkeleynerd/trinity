// Copyright (c) 2026 CCP Games

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <string>
#include <vector>

#import <Cocoa/Cocoa.h>
#import <QuartzCore/QuartzCore.h>

typedef id Tr2WindowHandle;

#include <TrinityAL.h>

#include <AxisAlignedBox.h>
#include <CcpMath.h>
#include <cmf/animation.h>
#include <cmf/bufferstreams.h>
#include <cmf/compression.h>
#include <cmf/utils.h>

const char* g_moduleName = "TrinityALAnimatedModel_metal";

namespace
{

constexpr uint32_t kWindowWidth = 640;
constexpr uint32_t kWindowHeight = 480;
constexpr uint32_t kMaximumQualityMsaaSamples = 4;
constexpr uint32_t kMaximumQualityAnisotropy = 16;
constexpr const char* kDefaultAssetPath = "Assets/Fox.cmf";
constexpr const char* kDefaultAnimation = "Run";
constexpr uint8_t kNoSkeleton = 0xffu;

bool g_shouldQuit = false;

std::string MakeAssetPath( const std::string& assetName )
{
	return "Assets/" + assetName + ".cmf";
}

const uint8_t kPositionColorVS[] = {
#include "PositionColor.vs.h"
};

const uint8_t kPositionColorPS[] = {
#include "PositionColor.ps.h"
};

struct SourceVertex
{
	float position[3];
	float normal[3];
	float texcoord[2];
	float color[4];
	uint16_t joints[4];
	float weights[4];
};

struct DemoVertex
{
	float position[3];
	float color[4];
	float texcoord[2];
};

struct DemoTriangle
{
	std::array<DemoVertex, 3> vertices;
	float sortDepth = 0.0f;
};

struct DemoResources
{
	Tr2ShaderAL vertexShader;
	Tr2ShaderAL pixelShader;
	Tr2ShaderProgramAL shaderProgram;
	Tr2BufferAL vertexBuffer;
	Tr2VertexLayoutAL vertexLayout;
	Tr2TextureAL msaaColorBuffer;
	Tr2TextureAL depthBuffer;
	Tr2TextureAL baseColorTexture;
	Tr2SamplerStateAL baseColorSampler;
	Tr2ResourceSetAL resourceSet;
	uint32_t vertexStride = 0;
	uint32_t vertexCapacity = 0;
	uint32_t renderWidth = kWindowWidth;
	uint32_t renderHeight = kWindowHeight;
	bool useMsaaRenderTarget = false;
};

struct TextureImage
{
	uint32_t width = 1;
	uint32_t height = 1;
	std::vector<uint8_t> rgba = { 255, 255, 255, 255 };
};

struct LoadedCmfSection
{
	cmf::Section section;
	std::vector<uint8_t> data;
};

class LoadedCmf
{
public:
	bool Load( const std::string& path, std::string& error )
	{
		std::ifstream input( path, std::ios::binary | std::ios::ate );
		if( !input )
		{
			error = "Failed to open CMF file: " + path;
			return false;
		}

		std::streamsize size = input.tellg();
		if( size <= 0 )
		{
			error = "CMF file is empty: " + path;
			return false;
		}
		input.seekg( 0, std::ios::beg );

		std::vector<uint8_t> bytes( static_cast<size_t>( size ) );
		if( !input.read( reinterpret_cast<char*>( bytes.data() ), size ) )
		{
			error = "Failed to read CMF file: " + path;
			return false;
		}

		auto validated = cmf::ValidateFile( bytes.data(), bytes.size(), { true, true, true, true } );
		if( !validated )
		{
			error = "Invalid CMF file: " + validated.error;
			return false;
		}

		const auto* header = reinterpret_cast<const cmf::Header*>( bytes.data() );
		m_sections.clear();
		m_sections.reserve( header->sections.size() );
		for( const cmf::Section& section : header->sections )
		{
			if( section.offset + section.compressedSize > bytes.size() )
			{
				error = "CMF section is out of range";
				return false;
			}
			LoadedCmfSection loadedSection;
			loadedSection.section = section;
			loadedSection.data.resize( section.compressedSize );
			std::memcpy( loadedSection.data.data(), bytes.data() + section.offset, section.compressedSize );
			m_sections.push_back( std::move( loadedSection ) );
		}

		if( m_sections.empty() )
		{
			error = "CMF has no sections";
			return false;
		}
		cmf::OffsetsToPointers( *reinterpret_cast<cmf::Data*>( m_sections[0].data.data() ) );
		return true;
	}

	const cmf::Data* GetData() const
	{
		if( m_sections.empty() )
		{
			return nullptr;
		}
		return reinterpret_cast<const cmf::Data*>( m_sections[0].data.data() );
	}

	const void* GetSection( uint32_t index, std::string& error )
	{
		if( index >= m_sections.size() )
		{
			error = "CMF buffer view references an invalid section";
			return nullptr;
		}

		LoadedCmfSection& section = m_sections[index];
		if( section.section.compression == cmf::SectionCompression::None )
		{
			return section.data.data();
		}

		std::vector<uint8_t> decompressed( section.section.uncompressedSize );
		cmf::Decompress( decompressed.data(), section.section, section.data.data() );
		section.data = std::move( decompressed );
		section.section.compression = cmf::SectionCompression::None;
		return section.data.data();
	}

	const void* GetViewData( const cmf::BufferView& view, std::string& error )
	{
		const auto* section = static_cast<const uint8_t*>( GetSection( view.index, error ) );
		if( !section )
		{
			return nullptr;
		}
		if( view.offset + view.size > m_sections[view.index].section.uncompressedSize )
		{
			error = "CMF buffer view is out of range";
			return nullptr;
		}
		return section + view.offset;
	}

private:
	std::vector<LoadedCmfSection> m_sections;
};

struct MeshRuntime
{
	const cmf::Mesh* mesh = nullptr;
	const cmf::MeshLod* lod = nullptr;
	const SourceVertex* vertices = nullptr;
	uint32_t vertexCount = 0;
	std::vector<uint32_t> indices;
	std::vector<int32_t> meshBoneToSkeletonBone;
};

class AnimatedModel
{
public:
	bool Load( const std::string& path, const std::string& animationName, bool animate, std::string& error )
	{
		if( !m_cmf.Load( path, error ) )
		{
			return false;
		}

		m_data = m_cmf.GetData();
		if( !m_data || m_data->meshes.empty() )
		{
			error = "CMF must contain at least one mesh";
			return false;
		}

		m_animate = animate;
		m_skeleton = m_data->skeletons.empty() ? nullptr : &m_data->skeletons[0];
		if( m_animate )
		{
			if( !m_skeleton )
			{
				error = "Animated playback requires a CMF skeleton";
				return false;
			}
			if( m_data->animations.empty() )
			{
				error = "CMF contains no animations";
				return false;
			}
			m_animation = FindAnimation( animationName );
			if( !m_animation && animationName != kDefaultAnimation )
			{
				error = "Animation '" + animationName + "' was not found";
				return false;
			}
			if( !m_animation )
			{
				m_animation = FindAnimation( kDefaultAnimation );
			}
			if( !m_animation )
			{
				m_animation = &m_data->animations[0];
				m_animationName = cmf::ToStdString( m_animation->name );
			}
		}

		if( !BuildMeshes( error ) )
		{
			return false;
		}
		if( !LoadBaseColorTexture( path, error ) )
		{
			return false;
		}

		if( m_skeleton )
		{
			cmf::RestPose( m_pose, *m_skeleton );
			if( m_animate )
			{
				m_sequencer = std::make_unique<cmf::AnimationSequencer>( *m_skeleton );
				auto player = m_sequencer->PlayAnimation( *m_animation );
				player->SetStartTime( 0.0f );
				player->SetLoopCount( cmf::AnimationPlayer::INFINITE_LOOP );
				player->SetSpeed( 1.0f );
			}
			cmf::ComputeWorldTransforms( m_worldTransforms, m_pose );
		}
		else
		{
			m_worldTransforms.clear();
		}
		return true;
	}

	uint32_t MaxExpandedVertexCount() const
	{
		uint32_t total = 0;
		for( const MeshRuntime& mesh : m_meshes )
		{
			total += static_cast<uint32_t>( mesh.indices.size() );
		}
		return total;
	}

	const std::string& AnimationName() const
	{
		return m_animationName;
	}

	bool IsAnimated() const
	{
		return m_animate;
	}

	const TextureImage& BaseColorTexture() const
	{
		return m_baseColorTexture;
	}

	bool BuildFrameVertices( float time, std::vector<DemoVertex>& outVertices )
	{
		if( m_skeleton )
		{
			cmf::RestPose( m_pose, *m_skeleton );
			if( m_animate )
			{
				m_sequencer->Sample( m_pose, time );
			}
			cmf::ComputeWorldTransforms( m_worldTransforms, m_pose );
		}

		std::vector<DemoTriangle> triangles;
		triangles.reserve( MaxExpandedVertexCount() / 3 );
		for( const MeshRuntime& mesh : m_meshes )
		{
			AppendMeshTriangles( mesh, time, triangles );
		}

		outVertices.clear();
		outVertices.reserve( triangles.size() * 3 );
		for( const DemoTriangle& triangle : triangles )
		{
			outVertices.push_back( triangle.vertices[0] );
			outVertices.push_back( triangle.vertices[1] );
			outVertices.push_back( triangle.vertices[2] );
		}
		return !outVertices.empty();
	}

private:
	bool LoadBaseColorTexture( const std::string& path, std::string& error )
	{
		m_baseColorTexture = TextureImage();

		const std::string metadataPath = path + ".basecolor.txt";
		std::ifstream metadata( metadataPath );
		if( !metadata )
		{
			return true;
		}

		uint32_t width = 0;
		uint32_t height = 0;
		if( !( metadata >> width >> height ) || width == 0 || height == 0 )
		{
			error = "Invalid base color texture metadata: " + metadataPath;
			return false;
		}

		const uint64_t expectedSize = static_cast<uint64_t>( width ) * static_cast<uint64_t>( height ) * 4ull;
		if( expectedSize > static_cast<uint64_t>( std::numeric_limits<std::streamsize>::max() ) )
		{
			error = "Base color texture is too large: " + metadataPath;
			return false;
		}

		const std::string rgbaPath = path + ".basecolor.rgba";
		std::ifstream input( rgbaPath, std::ios::binary | std::ios::ate );
		if( !input )
		{
			error = "Failed to open base color texture: " + rgbaPath;
			return false;
		}
		const std::streamsize size = input.tellg();
		if( size != static_cast<std::streamsize>( expectedSize ) )
		{
			error = "Base color texture size does not match metadata: " + rgbaPath;
			return false;
		}
		input.seekg( 0, std::ios::beg );

		TextureImage texture;
		texture.width = width;
		texture.height = height;
		texture.rgba.resize( static_cast<size_t>( expectedSize ) );
		if( !input.read( reinterpret_cast<char*>( texture.rgba.data() ), size ) )
		{
			error = "Failed to read base color texture: " + rgbaPath;
			return false;
		}

		m_baseColorTexture = std::move( texture );
		return true;
	}

	const cmf::Animation* FindAnimation( const std::string& requested )
	{
		if( requested.empty() )
		{
			return nullptr;
		}
		for( const cmf::Animation& animation : m_data->animations )
		{
			if( cmf::ToStdStringView( animation.name ) == requested )
			{
				m_animationName = requested;
				return &animation;
			}
		}
		return nullptr;
	}

	bool BuildMeshes( std::string& error )
	{
		m_meshes.clear();
		bool boundsInitialized = false;
		for( const cmf::Mesh& mesh : m_data->meshes )
		{
			if( mesh.lods.empty() )
			{
				continue;
			}
			const bool meshIsSkinned = mesh.skeleton != kNoSkeleton;
			if( meshIsSkinned )
			{
				if( mesh.skeleton >= m_data->skeletons.size() )
				{
					error = "Mesh references an invalid skeleton index";
					return false;
				}
				if( !m_skeleton )
				{
					m_skeleton = &m_data->skeletons[mesh.skeleton];
				}
				if( &m_data->skeletons[mesh.skeleton] != m_skeleton )
				{
					error = "Animated sample supports one skeleton per CMF";
					return false;
				}
			}
			else if( m_animate )
			{
				error = "Animated playback requires meshes to reference a skeleton";
				return false;
			}

			MeshRuntime runtime;
			runtime.mesh = &mesh;
			runtime.lod = &mesh.lods[0];
			runtime.vertexCount = cmf::GetStreamElementCount( runtime.lod->vb );
			runtime.vertices = static_cast<const SourceVertex*>( m_cmf.GetViewData( runtime.lod->vb, error ) );
			if( !runtime.vertices )
			{
				return false;
			}
			if( runtime.lod->vb.stride != sizeof( SourceVertex ) )
			{
				error = "Animated sample expects the glTF importer vertex layout";
				return false;
			}

			const void* indexData = m_cmf.GetViewData( runtime.lod->ib, error );
			if( !indexData )
			{
				return false;
			}
			const uint32_t indexCount = cmf::GetStreamElementCount( runtime.lod->ib );
			runtime.indices.resize( indexCount );
			if( runtime.lod->ib.stride == sizeof( uint16_t ) )
			{
				const auto* src = static_cast<const uint16_t*>( indexData );
				for( uint32_t i = 0; i < indexCount; ++i )
				{
					runtime.indices[i] = src[i];
				}
			}
			else if( runtime.lod->ib.stride == sizeof( uint32_t ) )
			{
				const auto* src = static_cast<const uint32_t*>( indexData );
				std::copy( src, src + indexCount, runtime.indices.begin() );
			}
			else
			{
				error = "Unsupported CMF index stride";
				return false;
			}

			if( meshIsSkinned )
			{
				runtime.meshBoneToSkeletonBone.resize( mesh.boneBindings.size(), -1 );
				for( size_t meshBone = 0; meshBone < mesh.boneBindings.size(); ++meshBone )
				{
					const std::string_view bindingName = cmf::ToStdStringView( mesh.boneBindings[meshBone].name );
					for( size_t skeletonBone = 0; skeletonBone < m_skeleton->bones.size(); ++skeletonBone )
					{
						if( cmf::ToStdStringView( m_skeleton->bones[skeletonBone] ) == bindingName )
						{
							runtime.meshBoneToSkeletonBone[meshBone] = static_cast<int32_t>( skeletonBone );
							break;
						}
					}
					if( runtime.meshBoneToSkeletonBone[meshBone] < 0 )
					{
						error = "Mesh bone binding was not found in the skeleton";
						return false;
					}
				}
			}

			ExtendBounds( mesh.bounds.m_min, boundsInitialized );
			ExtendBounds( mesh.bounds.m_max, boundsInitialized );
			m_meshes.push_back( std::move( runtime ) );
		}

		if( m_meshes.empty() )
		{
			error = "CMF contains no renderable meshes";
			return false;
		}

		const Vector3 extent = m_bounds.m_max - m_bounds.m_min;
		const float largestExtent = std::max( extent.x, std::max( extent.y, extent.z ) );
		m_center = ( m_bounds.m_min + m_bounds.m_max ) * 0.5f;
		m_fitScale = largestExtent > 0.0f ? 2.85f / largestExtent : 1.0f;
		return true;
	}

	void ExtendBounds( const Vector3& value, bool& initialized )
	{
		if( !initialized )
		{
			m_bounds.m_min = value;
			m_bounds.m_max = value;
			initialized = true;
			return;
		}
		m_bounds.m_min = Minimize( m_bounds.m_min, value );
		m_bounds.m_max = Maximize( m_bounds.m_max, value );
	}

	Vector3 SkinPosition( const MeshRuntime& mesh, const SourceVertex& vertex ) const
	{
		if( mesh.meshBoneToSkeletonBone.empty() )
		{
			return Vector3( vertex.position[0], vertex.position[1], vertex.position[2] );
		}

		Vector3 skinned( 0.0f, 0.0f, 0.0f );
		float totalWeight = 0.0f;
		for( size_t i = 0; i < 4; ++i )
		{
			const float weight = vertex.weights[i];
			if( weight <= 0.0f || vertex.joints[i] >= mesh.meshBoneToSkeletonBone.size() )
			{
				continue;
			}
			const int32_t skeletonBone = mesh.meshBoneToSkeletonBone[vertex.joints[i]];
			if( skeletonBone < 0 || skeletonBone >= static_cast<int32_t>( m_worldTransforms.size() ) )
			{
				continue;
			}
			const Matrix skinMatrix = m_skeleton->invBindTransforms[skeletonBone] * m_worldTransforms[skeletonBone];
			const Vector3 sourcePosition( vertex.position[0], vertex.position[1], vertex.position[2] );
			skinned += TransformCoord( sourcePosition, skinMatrix ) * weight;
			totalWeight += weight;
		}
		return totalWeight > 0.0f ? skinned / totalWeight : Vector3( vertex.position[0], vertex.position[1], vertex.position[2] );
	}

	Vector3 SkinNormal( const MeshRuntime& mesh, const SourceVertex& vertex ) const
	{
		Vector3 sourceNormal( vertex.normal[0], vertex.normal[1], vertex.normal[2] );
		if( mesh.meshBoneToSkeletonBone.empty() )
		{
			return Normalize( sourceNormal );
		}

		Vector3 skinned( 0.0f, 0.0f, 0.0f );
		float totalWeight = 0.0f;
		for( size_t i = 0; i < 4; ++i )
		{
			const float weight = vertex.weights[i];
			if( weight <= 0.0f || vertex.joints[i] >= mesh.meshBoneToSkeletonBone.size() )
			{
				continue;
			}
			const int32_t skeletonBone = mesh.meshBoneToSkeletonBone[vertex.joints[i]];
			if( skeletonBone < 0 || skeletonBone >= static_cast<int32_t>( m_worldTransforms.size() ) )
			{
				continue;
			}
			const Matrix skinMatrix = m_skeleton->invBindTransforms[skeletonBone] * m_worldTransforms[skeletonBone];
			skinned += TransformNormal( sourceNormal, skinMatrix ) * weight;
			totalWeight += weight;
		}
		return totalWeight > 0.0f ? Normalize( skinned / totalWeight ) : Normalize( sourceNormal );
	}

	DemoVertex ProjectVertex( const Vector3& modelPosition, const Vector3& normal, const float color[4], const float texcoord[2], float time, float& sortDepth ) const
	{
		const float staticSpin = m_animate ? 0.0f : time * 0.45f;
		const Matrix presentation = RotationYMatrix( -1.8f + staticSpin ) * RotationXMatrix( -0.45f );
		Vector3 viewPosition = TransformCoord( ( modelPosition - m_center ) * m_fitScale, presentation );

		float projectionScale = 0.42f;
		float depth = std::max( 0.0f, std::min( 1.0f, ( viewPosition.z + 3.0f ) / 6.0f ) );
		sortDepth = viewPosition.z;
		if( m_animate )
		{
			const float cameraZ = viewPosition.z + 3.0f;
			sortDepth = cameraZ;
			projectionScale = 1.35f / std::max( cameraZ, 0.2f );
			depth = std::max( 0.0f, std::min( 1.0f, ( cameraZ - 0.2f ) / 6.0f ) );
		}

		const Vector3 litNormal = Normalize( TransformNormal( normal, presentation ) );
		const Vector3 keyLightDirection = Normalize( Vector3( -0.45f, 0.78f, -0.44f ) );
		const Vector3 fillLightDirection = Normalize( Vector3( 0.55f, 0.18f, 0.42f ) );
		const float keyLight = std::max( 0.0f, Dot( litNormal, keyLightDirection ) );
		const float fillLight = std::max( 0.0f, Dot( litNormal, fillLightDirection ) );
		const float light = std::min( 1.18f, 0.18f + keyLight * 0.92f + fillLight * 0.18f );

		return {
			{ viewPosition.x * projectionScale, viewPosition.y * projectionScale, depth },
			{ color[0] * light, color[1] * light, color[2] * light, color[3] },
			{ texcoord[0], texcoord[1] },
		};
	}

	void AppendMeshTriangles( const MeshRuntime& mesh, float time, std::vector<DemoTriangle>& triangles ) const
	{
		for( size_t i = 0; i + 2 < mesh.indices.size(); i += 3 )
		{
			DemoTriangle triangle;
			bool validTriangle = true;
			for( size_t corner = 0; corner < 3; ++corner )
			{
				const uint32_t vertexIndex = mesh.indices[i + corner];
				if( vertexIndex >= mesh.vertexCount )
				{
					validTriangle = false;
					break;
				}
				const SourceVertex& source = mesh.vertices[vertexIndex];
				float sortDepth = 0.0f;
				const Vector3 position = m_animate ? SkinPosition( mesh, source ) : Vector3( source.position[0], source.position[1], source.position[2] );
				const Vector3 normal = m_animate ? SkinNormal( mesh, source ) : Normalize( Vector3( source.normal[0], source.normal[1], source.normal[2] ) );
				triangle.vertices[corner] = ProjectVertex( position, normal, source.color, source.texcoord, time, sortDepth );
				triangle.sortDepth += sortDepth;
			}
			if( validTriangle )
			{
				triangles.push_back( triangle );
			}
		}
	}

	LoadedCmf m_cmf;
	const cmf::Data* m_data = nullptr;
	const cmf::Skeleton* m_skeleton = nullptr;
	const cmf::Animation* m_animation = nullptr;
	std::string m_animationName;
	bool m_animate = false;
	std::vector<MeshRuntime> m_meshes;
	std::unique_ptr<cmf::AnimationSequencer> m_sequencer;
	cmf::SkeletonPose m_pose;
	std::vector<Matrix> m_worldTransforms;
	CcpMath::AxisAlignedBox m_bounds = {};
	Vector3 m_center = {};
	float m_fitScale = 1.0f;
	TextureImage m_baseColorTexture;
};

bool HasCmfExtension( const std::string& path )
{
	return path.size() >= 4 && path.compare( path.size() - 4, 4, ".cmf" ) == 0;
}

std::string ResolveAssetPath( const std::string& asset )
{
	if( asset.find( '/' ) != std::string::npos || asset.find( '\\' ) != std::string::npos || HasCmfExtension( asset ) )
	{
		return asset;
	}
	return MakeAssetPath( asset );
}

void PrintUsage( const char* executable )
{
	std::cerr << "Usage: " << executable << " [--asset Fox|Box] [--input PATH] [--frames N] [--animate] [--animation NAME]\n";
}

bool ParseArgs( int argc, char** argv, int& frameCount, std::string& assetPath, std::string& animationName, bool& animate )
{
	frameCount = -1;
	assetPath = kDefaultAssetPath;
	animationName = kDefaultAnimation;
	animate = false;
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
			frameCount = static_cast<int>( value );
		}
		else if( std::strcmp( argv[i], "--asset" ) == 0 )
		{
			if( i + 1 >= argc )
			{
				return false;
			}
			assetPath = ResolveAssetPath( argv[++i] );
		}
		else if( std::strcmp( argv[i], "--input" ) == 0 )
		{
			if( i + 1 >= argc )
			{
				return false;
			}
			assetPath = argv[++i];
		}
		else if( std::strcmp( argv[i], "--animation" ) == 0 )
		{
			if( i + 1 >= argc )
			{
				return false;
			}
			animationName = argv[++i];
			animate = true;
		}
		else if( std::strcmp( argv[i], "--animate" ) == 0 )
		{
			animate = true;
		}
		else if( std::strcmp( argv[i], "--help" ) == 0 || std::strcmp( argv[i], "-h" ) == 0 )
		{
			PrintUsage( argv[0] );
			std::exit( 0 );
		}
		else
		{
			return false;
		}
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

#define RETURN_FALSE_IF_FAILED( expression )          \
	do                                                \
	{                                                 \
		if( Failed( ( expression ), #expression ) )   \
		{                                             \
			return false;                             \
		}                                             \
	} while( false )

uint32_t GetBackingDimension( uint32_t logicalDimension, CGFloat backingScale )
{
	const auto scaled = static_cast<uint32_t>( std::max( 1.0, std::round( static_cast<double>( logicalDimension ) * static_cast<double>( backingScale ) ) ) );
	return std::max( logicalDimension, scaled );
}

bool CreateResources(
	Tr2PrimaryRenderContextAL& renderContext,
	DemoResources& resources,
	uint32_t vertexCapacity,
	const TextureImage& baseColorTexture,
	uint32_t renderWidth,
	uint32_t renderHeight )
{
	using namespace Tr2RenderContextEnum;

	auto vertexShaderInput =
		Tr2ShaderSignatureAL()
			.Add( Tr2VertexDefinition::POSITION, 0, 0, Tr2ShaderPipelineInputAL::FLOAT, 3 )
			.Add( Tr2VertexDefinition::COLOR, 0, 1, Tr2ShaderPipelineInputAL::FLOAT, 4 )
			.Add( Tr2VertexDefinition::TEXCOORD, 0, 2, Tr2ShaderPipelineInputAL::FLOAT, 2 );
	auto pixelShaderInput =
		Tr2ShaderSignatureAL()
			.Add( Tr2ShaderRegisterAL::SRV_TEXTURE2D, 0 )
			.Add( Tr2ShaderRegisterAL::SAMPLER, 0 );
	RETURN_FALSE_IF_FAILED( resources.vertexShader.Create( VERTEX_SHADER, kPositionColorVS, vertexShaderInput, "", renderContext ) );
	RETURN_FALSE_IF_FAILED( resources.pixelShader.Create( PIXEL_SHADER, kPositionColorPS, pixelShaderInput, "", renderContext ) );

	Tr2ShaderAL shaders[] = { resources.vertexShader, resources.pixelShader };
	RETURN_FALSE_IF_FAILED( resources.shaderProgram.Create( shaders, 2, renderContext ) );

	Tr2SubresourceData textureData = {};
	textureData.m_sysMem = baseColorTexture.rgba.data();
	textureData.m_sysMemPitch = baseColorTexture.width * 4;
	textureData.m_sysMemSlicePitch = static_cast<uint32_t>( baseColorTexture.rgba.size() );
	RETURN_FALSE_IF_FAILED( resources.baseColorTexture.Create(
		Tr2BitmapDimensions( baseColorTexture.width, baseColorTexture.height, 1, PIXEL_FORMAT_R8G8B8A8_UNORM ),
		Tr2GpuUsage::SHADER_RESOURCE,
		&textureData,
		renderContext ) );
	RETURN_FALSE_IF_FAILED( resources.baseColorSampler.Create( Tr2SamplerDescription( TF_ANISOTROPIC, TA_WRAP, kMaximumQualityAnisotropy ), renderContext ) );

	Tr2ResourceSetDescriptionAL resourceDescription( resources.shaderProgram );
	if( !resourceDescription.SetSrv( PIXEL_SHADER, 0, resources.baseColorTexture, COLOR_SPACE_SRGB ) )
	{
		std::cerr << "Failed to bind base color texture SRV\n";
		return false;
	}
	if( !resourceDescription.SetSampler( PIXEL_SHADER, 0, resources.baseColorSampler ) )
	{
		std::cerr << "Failed to bind base color sampler\n";
		return false;
	}
	RETURN_FALSE_IF_FAILED( resources.resourceSet.Create( resourceDescription, resources.shaderProgram, renderContext ) );

	resources.vertexStride = sizeof( DemoVertex );
	resources.vertexCapacity = vertexCapacity;
	std::vector<DemoVertex> emptyVertices( vertexCapacity );
	RETURN_FALSE_IF_FAILED( resources.vertexBuffer.Create(
		resources.vertexStride,
		vertexCapacity,
		Tr2GpuUsage::VERTEX_BUFFER,
		Tr2CpuUsage::WRITE_OFTEN,
		emptyVertices.data(),
		renderContext ) );

	Tr2VertexDefinition vertexDefinition;
	vertexDefinition.Add( Tr2VertexDefinition::FLOAT32_3, Tr2VertexDefinition::POSITION );
	vertexDefinition.Add( Tr2VertexDefinition::FLOAT32_4, Tr2VertexDefinition::COLOR );
	vertexDefinition.Add( Tr2VertexDefinition::FLOAT32_2, Tr2VertexDefinition::TEXCOORD );
	RETURN_FALSE_IF_FAILED( resources.vertexLayout.Create( vertexDefinition, renderContext ) );

	resources.renderWidth = renderWidth;
	resources.renderHeight = renderHeight;
	const Tr2MsaaDesc maximumQualityMsaa( kMaximumQualityMsaaSamples, 0 );
	const Tr2BitmapDimensions colorDimensions( renderWidth, renderHeight, 1, PIXEL_FORMAT_B8G8R8A8_UNORM );
	const Tr2BitmapDimensions depthDimensions( renderWidth, renderHeight, 1, PIXEL_FORMAT_D24_UNORM_S8_UINT );

	ALResult msaaColorResult = resources.msaaColorBuffer.Create(
		colorDimensions,
		maximumQualityMsaa,
		Tr2GpuUsage::RENDER_TARGET,
		renderContext );
	if( SUCCEEDED( msaaColorResult.GetResult() ) )
	{
		ALResult msaaDepthResult = resources.depthBuffer.Create(
			depthDimensions,
			maximumQualityMsaa,
			Tr2GpuUsage::DEPTH_STENCIL,
			renderContext );
		if( SUCCEEDED( msaaDepthResult.GetResult() ) )
		{
			resources.useMsaaRenderTarget = true;
		}
		else
		{
			std::cerr << "4x MSAA depth buffer was unavailable; falling back to single-sample rendering\n";
			resources.msaaColorBuffer = Tr2TextureAL();
			RETURN_FALSE_IF_FAILED( resources.depthBuffer.Create(
				depthDimensions,
				Tr2GpuUsage::DEPTH_STENCIL,
				renderContext ) );
		}
	}
	else
	{
		std::cerr << "4x MSAA render target was unavailable; falling back to single-sample rendering\n";
		RETURN_FALSE_IF_FAILED( resources.depthBuffer.Create(
			depthDimensions,
			Tr2GpuUsage::DEPTH_STENCIL,
			renderContext ) );
	}

	return true;
}

bool DrawFrame( Tr2PrimaryRenderContextAL& renderContext, DemoResources& resources, AnimatedModel& model, int frameIndex )
{
	using namespace Tr2RenderContextEnum;

	std::vector<DemoVertex> vertices;
	if( !model.BuildFrameVertices( frameIndex / 60.0f, vertices ) )
	{
		std::cerr << "No vertices generated for animated model frame\n";
		return false;
	}
	if( vertices.size() > resources.vertexCapacity )
	{
		std::cerr << "Animated model generated more vertices than the dynamic buffer can hold\n";
		return false;
	}

	RETURN_FALSE_IF_FAILED( resources.vertexBuffer.UpdateBuffer( 0, static_cast<uint32_t>( vertices.size() * sizeof( DemoVertex ) ), vertices.data(), renderContext ) );

	RETURN_FALSE_IF_FAILED( renderContext.BeginScene() );
	RETURN_FALSE_IF_FAILED( renderContext.PushDepthStencil() );
	if( resources.useMsaaRenderTarget )
	{
		RETURN_FALSE_IF_FAILED( renderContext.SetRenderTarget( resources.msaaColorBuffer ) );
	}
	else
	{
		RETURN_FALSE_IF_FAILED( renderContext.SetRenderTarget( renderContext.GetDefaultBackBuffer() ) );
	}
	RETURN_FALSE_IF_FAILED( renderContext.SetDepthStencil( resources.depthBuffer ) );
	RETURN_FALSE_IF_FAILED( renderContext.Clear( CLEARFLAGS_TARGET | CLEARFLAGS_ZBUFFER, 0xff000000, 1.0f ) );
	RETURN_FALSE_IF_FAILED( renderContext.SetStreamSource( 0, resources.vertexBuffer, 0, resources.vertexStride ) );
	RETURN_FALSE_IF_FAILED( renderContext.SetVertexLayout( resources.vertexLayout ) );
	RETURN_FALSE_IF_FAILED( renderContext.SetShaderProgram( resources.shaderProgram ) );
	RETURN_FALSE_IF_FAILED( renderContext.SetResourceSet( resources.resourceSet ) );
	RETURN_FALSE_IF_FAILED( renderContext.SetTopology( TOP_TRIANGLES ) );
	RETURN_FALSE_IF_FAILED( renderContext.SetRenderState( RS_FILLMODE, FM_SOLID ) );
	RETURN_FALSE_IF_FAILED( renderContext.SetRenderState( RS_ZENABLE, 1 ) );
	RETURN_FALSE_IF_FAILED( renderContext.SetRenderState( RS_ZWRITEENABLE, 1 ) );
	RETURN_FALSE_IF_FAILED( renderContext.SetRenderState( RS_ZFUNC, CMP_LESSEQUAL ) );
	RETURN_FALSE_IF_FAILED( renderContext.SetRenderState( RS_CULLMODE, CULLMODE_NONE ) );
	RETURN_FALSE_IF_FAILED( renderContext.DrawPrimitive( 0, static_cast<uint32_t>( vertices.size() / 3 ) ) );
	RETURN_FALSE_IF_FAILED( renderContext.SetRenderState( RS_FILLMODE, FM_SOLID ) );
	if( resources.useMsaaRenderTarget )
	{
		RETURN_FALSE_IF_FAILED( resources.msaaColorBuffer.Resolve( renderContext.GetDefaultBackBuffer(), renderContext ) );
		RETURN_FALSE_IF_FAILED( renderContext.SetRenderTarget( renderContext.GetDefaultBackBuffer() ) );
	}
	RETURN_FALSE_IF_FAILED( renderContext.PopDepthStencil() );
	RETURN_FALSE_IF_FAILED( renderContext.EndScene() );
	RETURN_FALSE_IF_FAILED( renderContext.Present() );

	return true;
}

bool CreatePresentParameters( Tr2WindowHandle windowHandle, Tr2PresentParametersAL& presentParameters, uint32_t renderWidth, uint32_t renderHeight )
{
	presentParameters = {};
	RETURN_FALSE_IF_FAILED( Tr2VideoAdapterInfo::GetAdapterDisplayMode( Tr2VideoAdapterInfo::DEFAULT_ADAPTER, presentParameters.mode ) );
	presentParameters.mode.width = renderWidth;
	presentParameters.mode.height = renderHeight;
	presentParameters.mode.format = Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_UNORM;
	presentParameters.backBufferCount = 1;
	presentParameters.msaaType = 1;
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

@interface AnimatedModelWindowDelegate : NSObject <NSWindowDelegate>
@end

@implementation AnimatedModelWindowDelegate
- (BOOL)windowShouldClose:(id)sender
{
	(void)sender;
	g_shouldQuit = true;
	return YES;
}
@end

@interface AnimatedModelContentView : NSView
@end

@implementation AnimatedModelContentView
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
		std::string assetPath;
		std::string animationName;
		bool animate = false;
		if( !ParseArgs( argc, argv, maxFrames, assetPath, animationName, animate ) )
		{
			PrintUsage( argv[0] );
			return 2;
		}

		AnimatedModel model;
		std::string error;
		if( !model.Load( assetPath, animationName, animate, error ) )
		{
			std::cerr << error << "\n";
			return 1;
		}

		[NSApplication sharedApplication];
		[NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

		NSRect frame = NSMakeRect( 0, 0, kWindowWidth, kWindowHeight );
		NSWindow* window = [[NSWindow alloc] initWithContentRect:frame
													   styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable
														 backing:NSBackingStoreBuffered
														   defer:NO];
		[window setTitle:( animate ? @"TrinityAL Animated Model" : @"TrinityAL Static Model" )];
		[window center];

		AnimatedModelContentView* view = [[AnimatedModelContentView alloc] initWithFrame:frame];
		view.wantsLayer = YES;
		[window setContentView:view];
		[window makeFirstResponder:view];

		AnimatedModelWindowDelegate* delegate = [[AnimatedModelWindowDelegate alloc] init];
		[window setDelegate:delegate];
		[window makeKeyAndOrderFront:nil];
		[NSApp activateIgnoringOtherApps:YES];

		const CGFloat backingScale = std::max( static_cast<CGFloat>( 1.0 ), [window backingScaleFactor] );
		const uint32_t renderWidth = GetBackingDimension( kWindowWidth, backingScale );
		const uint32_t renderHeight = GetBackingDimension( kWindowHeight, backingScale );
		if( [view.layer isKindOfClass:CAMetalLayer.class] )
		{
			CAMetalLayer* metalLayer = (CAMetalLayer*)view.layer;
			metalLayer.contentsScale = backingScale;
			metalLayer.drawableSize = CGSizeMake( renderWidth, renderHeight );
		}

		Tr2PrimaryRenderContextAL renderContext;
		Tr2PrimaryRenderContextAL::SetPrimaryRenderContext( &renderContext );

		Tr2WindowHandle windowHandle = [window contentView];
		Tr2PresentParametersAL presentParameters;
		if( !CreatePresentParameters( windowHandle, presentParameters, renderWidth, renderHeight ) )
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
		if( !CreateResources( renderContext, resources, model.MaxExpandedVertexCount(), model.BaseColorTexture(), renderWidth, renderHeight ) )
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
				if( !DrawFrame( renderContext, resources, model, renderedFrames ) )
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
