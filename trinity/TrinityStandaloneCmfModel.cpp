// Copyright (c) 2026 CCP Games

#include "StdAfx.h"

#include "TrinityStandaloneCmfModel.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <fstream>
#include <limits>
#include <utility>

#include <cmf/bufferstreams.h>
#include <cmf/animation.h>
#include <cmf/compression.h>
#include <cmf/tangents.h>
#include <cmf/utils.h>

namespace
{

constexpr uint8_t kNoSkeleton = 0xffu;

struct SourceVertex
{
	float position[3];
	float normal[3];
	float texcoord[2];
	float color[4];
	uint16_t joints[4];
	float weights[4];
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

		const std::streamsize size = input.tellg();
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
		return m_sections.empty() ? nullptr : reinterpret_cast<const cmf::Data*>( m_sections[0].data.data() );
	}

	const void* GetViewData( const cmf::BufferView& view, std::string& error )
	{
		if( view.index >= m_sections.size() )
		{
			error = "CMF buffer view references an invalid section";
			return nullptr;
		}

		LoadedCmfSection& section = m_sections[view.index];
		if( section.section.compression != cmf::SectionCompression::None )
		{
			std::vector<uint8_t> decompressed( section.section.uncompressedSize );
			cmf::Decompress( decompressed.data(), section.section, section.data.data() );
			section.data = std::move( decompressed );
			section.section.compression = cmf::SectionCompression::None;
		}
		if( view.offset + view.size > section.data.size() )
		{
			error = "CMF buffer view is out of range";
			return nullptr;
		}
		return section.data.data() + view.offset;
	}

private:
	std::vector<LoadedCmfSection> m_sections;
};

bool LoadTexture(
	const std::string& path,
	const char* suffix,
	const std::array<uint8_t, 4>& fallback,
	TrinityStandaloneCmfTexture& texture,
	std::string& error )
{
	texture.width = 1;
	texture.height = 1;
	texture.rgba.assign( fallback.begin(), fallback.end() );
	const std::string metadataPath = path + "." + suffix + ".txt";
	std::ifstream metadata( metadataPath );
	if( !metadata )
	{
		return true;
	}

	uint32_t width = 0;
	uint32_t height = 0;
	if( !( metadata >> width >> height ) || width == 0 || height == 0 )
	{
		error = "Invalid texture metadata: " + metadataPath;
		return false;
	}

	const uint64_t expectedSize = static_cast<uint64_t>( width ) * height * 4ull;
	if( expectedSize > static_cast<uint64_t>( std::numeric_limits<std::streamsize>::max() ) )
	{
		error = "Texture is too large: " + metadataPath;
		return false;
	}

	const std::string rgbaPath = path + "." + suffix + ".rgba";
	std::ifstream input( rgbaPath, std::ios::binary | std::ios::ate );
	if( !input || input.tellg() != static_cast<std::streamsize>( expectedSize ) )
	{
		error = "Texture is missing or has the wrong size: " + rgbaPath;
		return false;
	}
	input.seekg( 0, std::ios::beg );

	texture.width = width;
	texture.height = height;
	texture.rgba.resize( static_cast<size_t>( expectedSize ) );
	if( !input.read( reinterpret_cast<char*>( texture.rgba.data() ), static_cast<std::streamsize>( expectedSize ) ) )
	{
		error = "Failed to read texture: " + rgbaPath;
		return false;
	}
	return true;
}

}

class TrinityStandaloneCmfModel::Impl
{
public:
	bool Load( const std::string& path, std::string& error )
	{
		if( !m_cmf.Load( path, error ) )
		{
			return false;
		}
		const cmf::Data* data = m_cmf.GetData();
		if( !data || data->meshes.empty() )
		{
			error = "CMF must contain at least one mesh";
			return false;
		}

		m_vertices.clear();
		m_jointIndices.clear();
		m_indices.clear();
		bool boundsInitialized = false;
		for( const cmf::Mesh& mesh : data->meshes )
		{
			if( mesh.lods.empty() )
			{
				continue;
			}

			const cmf::Skeleton* skeleton = nullptr;
			std::vector<Matrix> worldTransforms;
			std::vector<int32_t> meshBoneToSkeletonBone;
			if( mesh.skeleton != kNoSkeleton )
			{
				if( mesh.skeleton >= data->skeletons.size() )
				{
					error = "CMF mesh references an invalid skeleton";
					return false;
				}
				skeleton = &data->skeletons[mesh.skeleton];
				cmf::SkeletonPose restPose;
				cmf::RestPose( restPose, *skeleton );
				cmf::ComputeWorldTransforms( worldTransforms, restPose );

				meshBoneToSkeletonBone.resize( mesh.boneBindings.size(), -1 );
				for( size_t meshBone = 0; meshBone < mesh.boneBindings.size(); ++meshBone )
				{
					const std::string_view bindingName = cmf::ToStdStringView( mesh.boneBindings[meshBone].name );
					for( size_t skeletonBone = 0; skeletonBone < skeleton->bones.size(); ++skeletonBone )
					{
						if( cmf::ToStdStringView( skeleton->bones[skeletonBone] ) == bindingName )
						{
							meshBoneToSkeletonBone[meshBone] = static_cast<int32_t>( skeletonBone );
							break;
						}
					}
					if( meshBoneToSkeletonBone[meshBone] < 0 )
					{
						error = "CMF mesh bone binding was not found in its skeleton";
						return false;
					}
				}
			}

			const cmf::MeshLod& lod = mesh.lods[0];
			if( lod.vb.stride != sizeof( SourceVertex ) )
			{
				error = "EVE rung 3 expects the glTF importer vertex layout";
				return false;
			}
			const auto* sourceVertices = static_cast<const SourceVertex*>( m_cmf.GetViewData( lod.vb, error ) );
			const void* indexData = m_cmf.GetViewData( lod.ib, error );
			if( !sourceVertices || !indexData )
			{
				return false;
			}

			const uint32_t vertexCount = cmf::GetStreamElementCount( lod.vb );
			const uint32_t indexCount = cmf::GetStreamElementCount( lod.ib );
			if( indexCount % 3 != 0 )
			{
				error = "CMF triangle index count is not divisible by three";
				return false;
			}

			const uint32_t baseVertex = static_cast<uint32_t>( m_vertices.size() );
			m_vertices.reserve( m_vertices.size() + vertexCount );
			for( uint32_t i = 0; i < vertexCount; ++i )
			{
				const SourceVertex& source = sourceVertices[i];
				const Vector3 sourcePosition( source.position[0], source.position[1], source.position[2] );
				const Vector3 sourceNormal( source.normal[0], source.normal[1], source.normal[2] );
				Vector3 position = sourcePosition;
				Vector3 normal = Normalize( sourceNormal );
				if( skeleton && !meshBoneToSkeletonBone.empty() )
				{
					Vector3 skinnedPosition( 0.0f, 0.0f, 0.0f );
					Vector3 skinnedNormal( 0.0f, 0.0f, 0.0f );
					float totalWeight = 0.0f;
					for( size_t influence = 0; influence < 4; ++influence )
					{
						const float weight = source.weights[influence];
						if( weight <= 0.0f || source.joints[influence] >= meshBoneToSkeletonBone.size() )
						{
							continue;
						}
						const int32_t skeletonBone = meshBoneToSkeletonBone[source.joints[influence]];
						if( skeletonBone < 0 || skeletonBone >= static_cast<int32_t>( worldTransforms.size() ) )
						{
							continue;
						}
						const Matrix skinMatrix = skeleton->invBindTransforms[skeletonBone] * worldTransforms[skeletonBone];
						skinnedPosition += TransformCoord( sourcePosition, skinMatrix ) * weight;
						skinnedNormal += TransformNormal( sourceNormal, skinMatrix ) * weight;
						totalWeight += weight;
					}
					if( totalWeight > 0.0f )
					{
						position = skinnedPosition / totalWeight;
						normal = Normalize( skinnedNormal / totalWeight );
					}
				}
				TrinityStandaloneCmfVertex vertex = {};
				vertex.position[0] = position.x;
				vertex.position[1] = position.y;
				vertex.position[2] = position.z;
				vertex.normal[0] = normal.x;
				vertex.normal[1] = normal.y;
				vertex.normal[2] = normal.z;
				vertex.texcoord[0] = source.texcoord[0];
				vertex.texcoord[1] = source.texcoord[1];
				std::memcpy( vertex.color, source.color, sizeof( vertex.color ) );
				m_vertices.push_back( vertex );
				m_jointIndices.push_back( { source.joints[0], source.joints[1], source.joints[2], source.joints[3] } );

				if( !boundsInitialized )
				{
					m_bounds.m_min = position;
					m_bounds.m_max = position;
					boundsInitialized = true;
				}
				else
				{
					m_bounds.m_min = Minimize( m_bounds.m_min, position );
					m_bounds.m_max = Maximize( m_bounds.m_max, position );
				}
			}

			m_indices.reserve( m_indices.size() + indexCount );
			for( uint32_t i = 0; i < indexCount; ++i )
			{
				uint32_t index = 0;
				if( lod.ib.stride == sizeof( uint16_t ) )
				{
					index = static_cast<const uint16_t*>( indexData )[i];
				}
				else if( lod.ib.stride == sizeof( uint32_t ) )
				{
					index = static_cast<const uint32_t*>( indexData )[i];
				}
				else
				{
					error = "Unsupported CMF index stride";
					return false;
				}
				if( index >= vertexCount )
				{
					error = "CMF index is outside the vertex buffer";
					return false;
				}

				m_indices.push_back( baseVertex + index );
			}
		}

		if( m_vertices.empty() || !boundsInitialized )
		{
			error = "CMF contains no renderable static triangles";
			return false;
		}

		m_center = ( m_bounds.m_min + m_bounds.m_max ) * 0.5f;
		const Vector3 extent = m_bounds.m_max - m_bounds.m_min;
		const float largestExtent = std::max( extent.x, std::max( extent.y, extent.z ) );
		m_fitScale = largestExtent > 0.0f ? 4.0f / largestExtent : 1.0f;
		BuildTangents();
		BuildEveV5Vertices();

		return LoadTexture( path, "basecolor", { 255, 255, 255, 255 }, m_baseColorTexture, error ) &&
			LoadTexture( path, "normal", { 128, 128, 255, 255 }, m_normalTexture, error ) &&
			LoadTexture( path, "roughness", { 180, 180, 180, 255 }, m_roughnessTexture, error ) &&
			LoadTexture( path, "material", { 0, 0, 0, 255 }, m_materialTexture, error ) &&
			LoadTexture( path, "glow", { 0, 0, 0, 255 }, m_glowTexture, error ) &&
			LoadTexture( path, "d", { 0, 0, 0, 255 }, m_dirtTexture, error ) &&
			LoadTexture( path, "mask", { 0, 0, 0, 255 }, m_maskTexture, error ) &&
			LoadTexture( path, "p3", { 0, 0, 0, 255 }, m_paintMaskTexture, error );
	}

	void BuildEveV5Vertices()
	{
		m_eveV5Vertices.clear();
		m_eveV5Vertices.reserve( m_vertices.size() );
		for( size_t vertexIndex = 0; vertexIndex < m_vertices.size(); ++vertexIndex )
		{
			const TrinityStandaloneCmfVertex& source = m_vertices[vertexIndex];
			const Vector3 normal( source.normal[0], source.normal[1], source.normal[2] );
			const Vector3 tangent( source.tangent[0], source.tangent[1], source.tangent[2] );
			const Vector3 bitangent = Cross( normal, tangent ) * source.tangent[3];
			const Vector4 packed = cmf::PackTangents(
				cmf::TangentCompression::PackedTangent,
				normal,
				tangent,
				bitangent );

			TrinityStandaloneEveV5Vertex vertex = {};
			vertex.position[0] = ( source.position[0] - m_center.x ) * m_fitScale;
			vertex.position[1] = ( source.position[1] - m_center.y ) * m_fitScale;
			vertex.position[2] = ( source.position[2] - m_center.z ) * m_fitScale;
			std::memcpy( vertex.joints, m_jointIndices[vertexIndex].data(), sizeof( vertex.joints ) );
			vertex.texcoord[0] = source.texcoord[0];
			vertex.texcoord[1] = source.texcoord[1];
			vertex.packedTangent[0] = packed.x;
			vertex.packedTangent[1] = packed.y;
			vertex.packedTangent[2] = packed.z;
			vertex.packedTangent[3] = packed.w;
			m_eveV5Vertices.push_back( vertex );
		}
	}

	void BuildTangents()
	{
		std::vector<Vector3> tangents( m_vertices.size(), Vector3( 0.0f, 0.0f, 0.0f ) );
		std::vector<Vector3> bitangents( m_vertices.size(), Vector3( 0.0f, 0.0f, 0.0f ) );
		for( size_t i = 0; i < m_indices.size(); i += 3 )
		{
			const uint32_t i0 = m_indices[i + 0];
			const uint32_t i1 = m_indices[i + 1];
			const uint32_t i2 = m_indices[i + 2];
			const TrinityStandaloneCmfVertex& v0 = m_vertices[i0];
			const TrinityStandaloneCmfVertex& v1 = m_vertices[i1];
			const TrinityStandaloneCmfVertex& v2 = m_vertices[i2];
			const Vector3 p0( v0.position[0], v0.position[1], v0.position[2] );
			const Vector3 p1( v1.position[0], v1.position[1], v1.position[2] );
			const Vector3 p2( v2.position[0], v2.position[1], v2.position[2] );
			const Vector3 edge1 = p1 - p0;
			const Vector3 edge2 = p2 - p0;
			const float du1 = v1.texcoord[0] - v0.texcoord[0];
			const float dv1 = v1.texcoord[1] - v0.texcoord[1];
			const float du2 = v2.texcoord[0] - v0.texcoord[0];
			const float dv2 = v2.texcoord[1] - v0.texcoord[1];
			const float determinant = du1 * dv2 - du2 * dv1;
			if( std::abs( determinant ) < 1.0e-8f )
			{
				continue;
			}
			const float inverse = 1.0f / determinant;
			const Vector3 tangent = ( edge1 * dv2 - edge2 * dv1 ) * inverse;
			const Vector3 bitangent = ( edge2 * du1 - edge1 * du2 ) * inverse;
			tangents[i0] += tangent;
			tangents[i1] += tangent;
			tangents[i2] += tangent;
			bitangents[i0] += bitangent;
			bitangents[i1] += bitangent;
			bitangents[i2] += bitangent;
		}

		for( size_t i = 0; i < m_vertices.size(); ++i )
		{
			TrinityStandaloneCmfVertex& vertex = m_vertices[i];
			const Vector3 normal( vertex.normal[0], vertex.normal[1], vertex.normal[2] );
			Vector3 tangent = tangents[i] - normal * Dot( normal, tangents[i] );
			if( Length( tangent ) < 1.0e-6f )
			{
				tangent = Cross( normal, std::abs( normal.y ) < 0.99f ? Vector3( 0.0f, 1.0f, 0.0f ) : Vector3( 1.0f, 0.0f, 0.0f ) );
			}
			tangent = Normalize( tangent );
			vertex.tangent[0] = tangent.x;
			vertex.tangent[1] = tangent.y;
			vertex.tangent[2] = tangent.z;
			vertex.tangent[3] = Dot( Cross( normal, tangent ), bitangents[i] ) < 0.0f ? -1.0f : 1.0f;
		}
	}

	LoadedCmf m_cmf;
	std::vector<TrinityStandaloneCmfVertex> m_vertices;
	std::vector<std::array<uint16_t, 4>> m_jointIndices;
	std::vector<TrinityStandaloneEveV5Vertex> m_eveV5Vertices;
	std::vector<uint32_t> m_indices;
	CcpMath::AxisAlignedBox m_bounds = {};
	Vector3 m_center = {};
	float m_fitScale = 1.0f;
	TrinityStandaloneCmfTexture m_baseColorTexture;
	TrinityStandaloneCmfTexture m_normalTexture;
	TrinityStandaloneCmfTexture m_roughnessTexture;
	TrinityStandaloneCmfTexture m_materialTexture;
	TrinityStandaloneCmfTexture m_glowTexture;
	TrinityStandaloneCmfTexture m_dirtTexture;
	TrinityStandaloneCmfTexture m_maskTexture;
	TrinityStandaloneCmfTexture m_paintMaskTexture;
};

TrinityStandaloneCmfModel::TrinityStandaloneCmfModel() :
	m_impl( std::make_unique<Impl>() )
{
}

TrinityStandaloneCmfModel::~TrinityStandaloneCmfModel() = default;

bool TrinityStandaloneCmfModel::Load( const std::string& path, std::string& error )
{
	return m_impl->Load( path, error );
}

uint32_t TrinityStandaloneCmfModel::VertexCount() const
{
	return static_cast<uint32_t>( m_impl->m_vertices.size() );
}

uint32_t TrinityStandaloneCmfModel::IndexCount() const
{
	return static_cast<uint32_t>( m_impl->m_indices.size() );
}

const std::vector<TrinityStandaloneCmfVertex>& TrinityStandaloneCmfModel::Vertices() const
{
	return m_impl->m_vertices;
}

const std::vector<TrinityStandaloneEveV5Vertex>& TrinityStandaloneCmfModel::EveV5Vertices() const
{
	return m_impl->m_eveV5Vertices;
}

const std::vector<uint32_t>& TrinityStandaloneCmfModel::Indices() const
{
	return m_impl->m_indices;
}

void TrinityStandaloneCmfModel::GetCenterAndScale( float output[4] ) const
{
	output[0] = m_impl->m_center.x;
	output[1] = m_impl->m_center.y;
	output[2] = m_impl->m_center.z;
	output[3] = m_impl->m_fitScale;
}

const TrinityStandaloneCmfTexture& TrinityStandaloneCmfModel::BaseColorTexture() const
{
	return m_impl->m_baseColorTexture;
}

const TrinityStandaloneCmfTexture& TrinityStandaloneCmfModel::NormalTexture() const
{
	return m_impl->m_normalTexture;
}

const TrinityStandaloneCmfTexture& TrinityStandaloneCmfModel::RoughnessTexture() const
{
	return m_impl->m_roughnessTexture;
}

const TrinityStandaloneCmfTexture& TrinityStandaloneCmfModel::MaterialTexture() const
{
	return m_impl->m_materialTexture;
}

const TrinityStandaloneCmfTexture& TrinityStandaloneCmfModel::GlowTexture() const
{
	return m_impl->m_glowTexture;
}

const TrinityStandaloneCmfTexture& TrinityStandaloneCmfModel::DirtTexture() const
{
	return m_impl->m_dirtTexture;
}

const TrinityStandaloneCmfTexture& TrinityStandaloneCmfModel::MaskTexture() const
{
	return m_impl->m_maskTexture;
}

const TrinityStandaloneCmfTexture& TrinityStandaloneCmfModel::PaintMaskTexture() const
{
	return m_impl->m_paintMaskTexture;
}
