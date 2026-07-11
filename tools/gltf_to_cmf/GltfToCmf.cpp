// Copyright (c) 2026 CCP Games

#include "GltfToCmf.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <memory>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <CcpMath.h>
#include <cmf/bounds.h>
#include <cmf/bufferstreams.h>
#include <cmf/memallocator.h>
#include <cmf/transforms.h>
#include <cmf/utils.h>
#include <cmf/writer.h>
#include <draco/compression/decode.h>
#include <draco/mesh/mesh.h>

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <webp/decode.h>

namespace
{

constexpr uint32_t kInvalidParent = 0xffffffffu;
constexpr uint8_t kNoSkeleton = 0xffu;

struct ImportedVertex
{
	float position[3] = {};
	float normal[3] = {};
	float tangent[4] = { 1.0f, 0.0f, 0.0f, 1.0f };
	float binormal[3] = { 0.0f, 0.0f, 1.0f };
	float texcoord[2] = {};
	float color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	uint16_t joints[4] = {};
	float weights[4] = {};
};

struct DecodedImage
{
	int width = 0;
	int height = 0;
	std::vector<uint8_t> pixels;
};

struct PendingMesh
{
	struct Area
	{
		std::string name;
		uint32_t firstElement = 0;
		uint32_t elementCount = 0;
		CcpMath::AxisAlignedBox bounds = {};
	};

	std::string name;
	std::vector<ImportedVertex> vertices;
	std::vector<uint32_t> indices;
	std::vector<Area> areas;
	std::vector<std::string> boneBindings;
	uint32_t skeletonIndex = kNoSkeleton;
	bool hasTangents = false;
	CcpMath::AxisAlignedBox bounds = {};
};

struct PendingSkeleton
{
	std::string name;
	std::vector<std::string> bones;
	std::vector<uint32_t> parents;
	std::vector<cmf::Transform> restTransforms;
	std::vector<Matrix> invBindTransforms;
	const cgltf_skin* skin = nullptr;
	std::unordered_map<const cgltf_node*, uint32_t> jointToBone;
};

struct PendingCurve
{
	uint8_t valueDimension = 0;
	cmf::Interpolation interpolation = cmf::Interpolation::Step;
	std::vector<float> knots;
	std::vector<float> values;
};

struct PendingChannel
{
	std::string target;
	cmf::AnimationChannelTargetType targetType = cmf::AnimationChannelTargetType::BonePosition;
	uint32_t curveIndex = 0;
};

struct PendingAnimation
{
	std::string name;
	std::vector<PendingChannel> channels;
	std::vector<PendingCurve> curves;
	float duration = 0.0f;
};

struct ImportContext
{
	cgltf_data* data = nullptr;
	GltfToCmfResult* result = nullptr;
	std::vector<DecodedImage> images;
	std::vector<cgltf_size> baseColorImageIndices;
	std::vector<PendingSkeleton> skeletons;
	std::vector<PendingMesh> meshes;
	std::vector<PendingAnimation> animations;
};

struct CgltfDataDeleter
{
	void operator()( cgltf_data* data ) const
	{
		if( data )
		{
			cgltf_free( data );
		}
	}
};

std::string_view SafeName( const char* name )
{
	return name ? std::string_view( name ) : std::string_view();
}

std::string MakeName( const char* name, const char* fallback, size_t index )
{
	if( name && name[0] )
	{
		return name;
	}
	return std::string( fallback ) + std::to_string( index );
}

bool HasExtension( const cgltf_data& data, const char* extensionName )
{
	for( cgltf_size i = 0; i < data.extensions_used_count; ++i )
	{
		if( std::strcmp( data.extensions_used[i], extensionName ) == 0 )
		{
			return true;
		}
	}
	return false;
}

Matrix ConvertGltfMatrix( const cgltf_float* m )
{
	return Matrix(
		m[0], m[1], m[2], m[3], m[4], m[5], m[6], m[7], m[8], m[9], m[10], m[11], m[12], m[13], m[14], m[15] );
}

cmf::Transform ReadNodeTransform( const cgltf_node& node )
{
	if( node.has_matrix )
	{
		Vector3 scale;
		Quaternion rotation;
		Vector3 translation;
		Decompose( scale, rotation, translation, ConvertGltfMatrix( node.matrix ) );
		return { translation, Normalize( rotation ), scale };
	}

	Vector3 translation( 0.0f, 0.0f, 0.0f );
	if( node.has_translation )
	{
		translation = Vector3( node.translation[0], node.translation[1], node.translation[2] );
	}

	Quaternion rotation = IdentityQuaternion();
	if( node.has_rotation )
	{
		rotation = Normalize( Quaternion( node.rotation[0], node.rotation[1], node.rotation[2], node.rotation[3] ) );
	}

	Vector3 scale( 1.0f, 1.0f, 1.0f );
	if( node.has_scale )
	{
		scale = Vector3( node.scale[0], node.scale[1], node.scale[2] );
	}

	return { translation, rotation, scale };
}

Matrix ReadNodeMatrix( const cgltf_node& node )
{
	if( node.has_matrix )
	{
		return ConvertGltfMatrix( node.matrix );
	}
	return cmf::ToMatrix( ReadNodeTransform( node ) );
}

bool MatrixNearlyEqual( const Matrix& a, const Matrix& b, float epsilon )
{
	for( uint32_t row = 0; row < 4; ++row )
	{
		for( uint32_t col = 0; col < 4; ++col )
		{
			if( std::fabs( a( row, col ) - b( row, col ) ) > epsilon )
			{
				return false;
			}
		}
	}
	return true;
}

bool ValidateNodeMatrices( const cgltf_data& data, std::string& error )
{
	for( cgltf_size i = 0; i < data.nodes_count; ++i )
	{
		const cgltf_node& node = data.nodes[i];
		if( !node.has_matrix )
		{
			continue;
		}

		Vector3 scale;
		Quaternion rotation;
		Vector3 translation;
		const Matrix matrix = ConvertGltfMatrix( node.matrix );
		Decompose( scale, rotation, translation, matrix );
		const Matrix recomposed = TransformationMatrix( scale, Normalize( rotation ), translation );
		if( !MatrixNearlyEqual( matrix, recomposed, 0.001f ) )
		{
			error = "Node matrix cannot be decomposed into TRS for node '" + MakeName( node.name, "node", i ) + "'";
			return false;
		}
	}
	return true;
}

const cgltf_attribute* FindAttribute( const cgltf_primitive& primitive, cgltf_attribute_type type, cgltf_int index = 0 )
{
	for( cgltf_size i = 0; i < primitive.attributes_count; ++i )
	{
		const cgltf_attribute& attribute = primitive.attributes[i];
		if( attribute.type == type && attribute.index == index )
		{
			return &attribute;
		}
	}
	return nullptr;
}

Vector3 GetImportedPosition( const ImportedVertex& vertex )
{
	return Vector3( vertex.position[0], vertex.position[1], vertex.position[2] );
}

void SetImportedNormal( ImportedVertex& vertex, const Vector3& normal )
{
	vertex.normal[0] = normal.x;
	vertex.normal[1] = normal.y;
	vertex.normal[2] = normal.z;
}

void SetImportedColor( ImportedVertex& vertex, const float color[4] )
{
	vertex.color[0] = color[0];
	vertex.color[1] = color[1];
	vertex.color[2] = color[2];
	vertex.color[3] = color[3];
}

void StoreDecodedImage( DecodedImage& image, int width, int height, const uint8_t* rgba )
{
	image.width = width;
	image.height = height;
	image.pixels.assign( rgba, rgba + width * height * 4 );
}

bool DecodeImageWithStb( const uint8_t* encoded, cgltf_size encodedSize, DecodedImage& image )
{
	if( encodedSize > static_cast<cgltf_size>( std::numeric_limits<int>::max() ) )
	{
		return false;
	}

	int width = 0;
	int height = 0;
	int sourceChannels = 0;
	stbi_uc* decoded = stbi_load_from_memory( encoded, static_cast<int>( encodedSize ), &width, &height, &sourceChannels, 4 );
	if( !decoded )
	{
		return false;
	}

	StoreDecodedImage( image, width, height, decoded );
	stbi_image_free( decoded );
	return true;
}

bool DecodeImageWithWebP( const uint8_t* encoded, cgltf_size encodedSize, DecodedImage& image )
{
	int width = 0;
	int height = 0;
	uint8_t* decoded = WebPDecodeRGBA( encoded, encodedSize, &width, &height );
	if( !decoded )
	{
		return false;
	}

	StoreDecodedImage( image, width, height, decoded );
	WebPFree( decoded );
	return true;
}

void GenerateNormals( PendingMesh& mesh )
{
	std::vector<Vector3> normals( mesh.vertices.size(), Vector3( 0.0f, 0.0f, 0.0f ) );
	for( size_t i = 0; i + 2 < mesh.indices.size(); i += 3 )
	{
		const uint32_t i0 = mesh.indices[i + 0];
		const uint32_t i1 = mesh.indices[i + 1];
		const uint32_t i2 = mesh.indices[i + 2];
		if( i0 >= mesh.vertices.size() || i1 >= mesh.vertices.size() || i2 >= mesh.vertices.size() )
		{
			continue;
		}

		const Vector3 p0 = GetImportedPosition( mesh.vertices[i0] );
		const Vector3 p1 = GetImportedPosition( mesh.vertices[i1] );
		const Vector3 p2 = GetImportedPosition( mesh.vertices[i2] );
		const Vector3 faceNormal = Cross( p1 - p0, p2 - p0 );
		if( LengthSq( faceNormal ) <= 0.0000001f )
		{
			continue;
		}

		normals[i0] += faceNormal;
		normals[i1] += faceNormal;
		normals[i2] += faceNormal;
	}

	for( size_t vertexIndex = 0; vertexIndex < mesh.vertices.size(); ++vertexIndex )
	{
		const Vector3 normal = LengthSq( normals[vertexIndex] ) > 0.0000001f ? Normalize( normals[vertexIndex] ) : Vector3( 0.0f, 1.0f, 0.0f );
		SetImportedNormal( mesh.vertices[vertexIndex], normal );
	}
}

float WrapTexCoord( float value )
{
	value = std::fmod( value, 1.0f );
	return value < 0.0f ? value + 1.0f : value;
}

void SampleImageNearest( const DecodedImage& image, const float texcoord[2], float color[4] )
{
	if( image.width <= 0 || image.height <= 0 || image.pixels.empty() )
	{
		return;
	}

	const float u = WrapTexCoord( texcoord[0] );
	const float v = WrapTexCoord( texcoord[1] );
	const int x = std::max( 0, std::min( image.width - 1, static_cast<int>( u * static_cast<float>( image.width ) ) ) );
	const int y = std::max( 0, std::min( image.height - 1, static_cast<int>( ( 1.0f - v ) * static_cast<float>( image.height ) ) ) );
	const uint8_t* pixel = &image.pixels[( y * image.width + x ) * 4];
	color[0] *= static_cast<float>( pixel[0] ) / 255.0f;
	color[1] *= static_cast<float>( pixel[1] ) / 255.0f;
	color[2] *= static_cast<float>( pixel[2] ) / 255.0f;
	color[3] *= static_cast<float>( pixel[3] ) / 255.0f;
}

const cgltf_image* GetTextureImage( const cgltf_texture* texture )
{
	if( !texture )
	{
		return nullptr;
	}
	if( texture->has_webp && texture->webp_image )
	{
		return texture->webp_image;
	}
	return texture->image;
}

const DecodedImage* FindBaseColorImage(
	const ImportContext& context,
	const cgltf_primitive& primitive,
	float baseColorFactor[4],
	cgltf_int& texcoordIndex,
	cgltf_size& imageIndex )
{
	baseColorFactor[0] = 1.0f;
	baseColorFactor[1] = 1.0f;
	baseColorFactor[2] = 1.0f;
	baseColorFactor[3] = 1.0f;
	texcoordIndex = 0;
	imageIndex = static_cast<cgltf_size>( -1 );

	if( !primitive.material || !primitive.material->has_pbr_metallic_roughness )
	{
		return nullptr;
	}

	const cgltf_pbr_metallic_roughness& pbr = primitive.material->pbr_metallic_roughness;
	baseColorFactor[0] = pbr.base_color_factor[0];
	baseColorFactor[1] = pbr.base_color_factor[1];
	baseColorFactor[2] = pbr.base_color_factor[2];
	baseColorFactor[3] = pbr.base_color_factor[3];

	const cgltf_image* image = GetTextureImage( pbr.base_color_texture.texture );
	if( !image )
	{
		return nullptr;
	}

	texcoordIndex = pbr.base_color_texture.texcoord < 0 ? 0 : pbr.base_color_texture.texcoord;
	imageIndex = cgltf_image_index( context.data, image );
	if( imageIndex >= context.images.size() || context.images[imageIndex].pixels.empty() )
	{
		return nullptr;
	}
	return &context.images[imageIndex];
}

void AddUniqueImageIndex( std::vector<cgltf_size>& indices, cgltf_size imageIndex )
{
	if( std::find( indices.begin(), indices.end(), imageIndex ) == indices.end() )
	{
		indices.push_back( imageIndex );
	}
}

bool ReadFloatAccessor( const cgltf_accessor* accessor, cgltf_size index, float* out, cgltf_size elementCount, std::string& error )
{
	if( !accessor || !cgltf_accessor_read_float( accessor, index, out, elementCount ) )
	{
		error = "Failed to read float accessor";
		return false;
	}
	return true;
}

bool ReadUintAccessor( const cgltf_accessor* accessor, cgltf_size index, cgltf_uint* out, cgltf_size elementCount, std::string& error )
{
	if( !accessor || !cgltf_accessor_read_uint( accessor, index, out, elementCount ) )
	{
		error = "Failed to read integer accessor";
		return false;
	}
	return true;
}

const cgltf_attribute* FindDracoAttribute( const cgltf_primitive& primitive, cgltf_attribute_type type, cgltf_int index = 0 )
{
	if( !primitive.has_draco_mesh_compression )
	{
		return nullptr;
	}
	for( cgltf_size i = 0; i < primitive.draco_mesh_compression.attributes_count; ++i )
	{
		const cgltf_attribute& attribute = primitive.draco_mesh_compression.attributes[i];
		if( attribute.type == type && attribute.index == index )
		{
			return &attribute;
		}
	}
	return nullptr;
}

bool DecodeDracoMesh( const cgltf_primitive& primitive, std::unique_ptr<draco::Mesh>& mesh, std::string& error )
{
	if( !primitive.has_draco_mesh_compression )
	{
		return true;
	}

	const cgltf_buffer_view* view = primitive.draco_mesh_compression.buffer_view;
	const uint8_t* bytes = view ? cgltf_buffer_view_data( view ) : nullptr;
	if( !view || !bytes || view->size == 0 )
	{
		error = "Draco primitive has no compressed buffer data";
		return false;
	}

	draco::DecoderBuffer buffer;
	buffer.Init( reinterpret_cast<const char*>( bytes ), view->size );
	auto geometryType = draco::Decoder::GetEncodedGeometryType( &buffer );
	if( !geometryType.ok() )
	{
		error = "Failed to inspect Draco primitive: " + geometryType.status().error_msg_string();
		return false;
	}
	if( geometryType.value() != draco::TRIANGULAR_MESH )
	{
		error = "Draco primitive is not a triangle mesh";
		return false;
	}

	buffer.Init( reinterpret_cast<const char*>( bytes ), view->size );
	draco::Decoder decoder;
	auto decoded = decoder.DecodeMeshFromBuffer( &buffer );
	if( !decoded.ok() )
	{
		error = "Failed to decode Draco primitive: " + decoded.status().error_msg_string();
		return false;
	}
	mesh = std::move( decoded ).value();
	return mesh != nullptr;
}

const draco::PointAttribute* GetDracoAttribute(
	const ImportContext& context,
	const cgltf_primitive& primitive,
	const draco::Mesh& mesh,
	cgltf_attribute_type type,
	cgltf_int index,
	std::string& error )
{
	const cgltf_attribute* mapping = FindDracoAttribute( primitive, type, index );
	if( !mapping || !mapping->data )
	{
		return nullptr;
	}

	const ptrdiff_t uniqueId = mapping->data - context.data->accessors;
	if( uniqueId < 0 || uniqueId > static_cast<ptrdiff_t>( std::numeric_limits<uint32_t>::max() ) )
	{
		error = "Draco attribute has an invalid unique ID";
		return nullptr;
	}

	const draco::PointAttribute* attribute = mesh.GetAttributeByUniqueId( static_cast<uint32_t>( uniqueId ) );
	if( !attribute )
	{
		error = "Draco attribute unique ID was not found in the decoded mesh";
	}
	return attribute;
}

bool ReadDracoFloatAttribute(
	const draco::PointAttribute& attribute,
	cgltf_size pointIndex,
	float* out,
	int componentCount,
	std::string& error )
{
	if( attribute.num_components() != componentCount ||
		!attribute.ConvertValue<float>( attribute.mapped_index( draco::PointIndex( static_cast<uint32_t>( pointIndex ) ) ), componentCount, out ) )
	{
		error = "Failed to read decoded Draco attribute";
		return false;
	}
	return true;
}

void ExtendBounds( CcpMath::AxisAlignedBox& bounds, const Vector3& value, bool& initialized )
{
	if( !initialized )
	{
		bounds.m_min = value;
		bounds.m_max = value;
		initialized = true;
		return;
	}
	bounds.m_min = Minimize( bounds.m_min, value );
	bounds.m_max = Maximize( bounds.m_max, value );
}

uint32_t FindSkinIndex( const ImportContext& context, const cgltf_skin* skin )
{
	if( !skin )
	{
		return kNoSkeleton;
	}
	for( size_t i = 0; i < context.skeletons.size(); ++i )
	{
		if( context.skeletons[i].skin == skin )
		{
			return static_cast<uint32_t>( i );
		}
	}
	return kNoSkeleton;
}

bool DecodeImages( ImportContext& context, std::string& error )
{
	context.images.resize( context.data->images_count );
	for( cgltf_size imageIndex = 0; imageIndex < context.data->images_count; ++imageIndex )
	{
		const cgltf_image& image = context.data->images[imageIndex];
		if( !image.buffer_view )
		{
			context.result->warnings.push_back( "Skipping external glTF image '" + MakeName( image.name, "image", imageIndex ) + "'" );
			continue;
		}

		const uint8_t* encoded = cgltf_buffer_view_data( image.buffer_view );
		if( !encoded || image.buffer_view->size == 0 )
		{
			context.result->warnings.push_back( "Skipping glTF image '" + MakeName( image.name, "image", imageIndex ) + "' because it has no buffer data" );
			continue;
		}

		DecodedImage& out = context.images[imageIndex];
		if( !DecodeImageWithStb( encoded, image.buffer_view->size, out ) && !DecodeImageWithWebP( encoded, image.buffer_view->size, out ) )
		{
			context.result->warnings.push_back( "Skipping glTF image '" + MakeName( image.name, "image", imageIndex ) + "' because stb_image/libwebp could not decode it" );
			continue;
		}
	}

	return true;
}

bool ImportSkeletons( ImportContext& context, std::string& error )
{
	if( context.data->skins_count > 254 )
	{
		error = "CMF v1 supports at most 254 skeletons in this importer";
		return false;
	}

	context.skeletons.reserve( context.data->skins_count );
	for( cgltf_size skinIndex = 0; skinIndex < context.data->skins_count; ++skinIndex )
	{
		const cgltf_skin& skin = context.data->skins[skinIndex];
		PendingSkeleton skeleton;
		skeleton.name = MakeName( skin.name, "skin", skinIndex );
		skeleton.skin = &skin;
		skeleton.bones.reserve( skin.joints_count );
		skeleton.parents.resize( skin.joints_count, kInvalidParent );
		skeleton.restTransforms.resize( skin.joints_count );
		skeleton.invBindTransforms.resize( skin.joints_count, IdentityMatrix() );

		for( cgltf_size jointIndex = 0; jointIndex < skin.joints_count; ++jointIndex )
		{
			const cgltf_node* joint = skin.joints[jointIndex];
			skeleton.jointToBone[joint] = static_cast<uint32_t>( jointIndex );
			skeleton.bones.push_back( MakeName( joint ? joint->name : nullptr, "joint", jointIndex ) );
		}

		for( cgltf_size jointIndex = 0; jointIndex < skin.joints_count; ++jointIndex )
		{
			const cgltf_node* joint = skin.joints[jointIndex];
			if( !joint )
			{
				error = "Skin '" + skeleton.name + "' contains a null joint";
				return false;
			}
			skeleton.restTransforms[jointIndex] = ReadNodeTransform( *joint );
			if( joint->parent )
			{
				auto parent = skeleton.jointToBone.find( joint->parent );
				if( parent != skeleton.jointToBone.end() )
				{
					skeleton.parents[jointIndex] = parent->second;
				}
			}
		}

		if( skin.inverse_bind_matrices )
		{
			if( skin.inverse_bind_matrices->count != skin.joints_count )
			{
				error = "Skin '" + skeleton.name + "' inverse bind matrix count does not match joint count";
				return false;
			}

			for( cgltf_size jointIndex = 0; jointIndex < skin.joints_count; ++jointIndex )
			{
				float values[16] = {};
				if( !ReadFloatAccessor( skin.inverse_bind_matrices, jointIndex, values, 16, error ) )
				{
					error = "Failed to read inverse bind matrices for skin '" + skeleton.name + "'";
					return false;
				}
				skeleton.invBindTransforms[jointIndex] = ConvertGltfMatrix( values );
			}
		}

		context.skeletons.push_back( std::move( skeleton ) );
	}

	return true;
}

bool ImportPrimitive(
	ImportContext& context,
	const cgltf_node& node,
	const cgltf_mesh& mesh,
	const cgltf_primitive& primitive,
	const Matrix& nodeWorld,
	size_t primitiveIndex,
	std::string& error )
{
	if( primitive.type != cgltf_primitive_type_triangles )
	{
		error = "Only triangle primitives are supported";
		return false;
	}

	const cgltf_attribute* positionAttribute = FindAttribute( primitive, cgltf_attribute_type_position );
	if( !positionAttribute || !positionAttribute->data )
	{
		error = "Primitive is missing POSITION";
		return false;
	}

	const cgltf_accessor* positionAccessor = positionAttribute->data;
	const cgltf_attribute* normalAttribute = FindAttribute( primitive, cgltf_attribute_type_normal );
	const cgltf_attribute* tangentAttribute = FindAttribute( primitive, cgltf_attribute_type_tangent );
	const cgltf_attribute* texcoordAttribute = FindAttribute( primitive, cgltf_attribute_type_texcoord );
	const cgltf_attribute* jointsAttribute = FindAttribute( primitive, cgltf_attribute_type_joints );
	const cgltf_attribute* weightsAttribute = FindAttribute( primitive, cgltf_attribute_type_weights );
	const bool hasNormals = normalAttribute && normalAttribute->data;
	const bool hasTangents = tangentAttribute && tangentAttribute->data;
	float baseColorFactor[4] = {};
	cgltf_int baseColorTexcoordIndex = 0;
	cgltf_size baseColorImageIndex = static_cast<cgltf_size>( -1 );
	const DecodedImage* baseColorImage = FindBaseColorImage( context, primitive, baseColorFactor, baseColorTexcoordIndex, baseColorImageIndex );
	if( baseColorImage && ( !texcoordAttribute || !texcoordAttribute->data || texcoordAttribute->index != baseColorTexcoordIndex ) )
	{
		error = "Primitive material uses a base color texture but TEXCOORD_" + std::to_string( baseColorTexcoordIndex ) + " is missing";
		return false;
	}
	if( baseColorImage )
	{
		AddUniqueImageIndex( context.baseColorImageIndices, baseColorImageIndex );
	}

	const uint32_t skeletonIndex = FindSkinIndex( context, node.skin );
	const bool skinned = skeletonIndex != kNoSkeleton;
	if( primitive.has_draco_mesh_compression && skinned )
	{
		error = "Draco-compressed skinned primitives are not supported yet";
		return false;
	}
	if( skinned && ( !jointsAttribute || !weightsAttribute ) )
	{
		error = "Skinned primitive is missing JOINTS_0 or WEIGHTS_0";
		return false;
	}

	PendingMesh pending;
	pending.name = MakeName( mesh.name, "mesh", context.meshes.size() ) + "_" + std::to_string( primitiveIndex );
	pending.skeletonIndex = skeletonIndex;
	pending.hasTangents = hasTangents;

	std::unique_ptr<draco::Mesh> dracoMesh;
	if( !DecodeDracoMesh( primitive, dracoMesh, error ) )
	{
		return false;
	}
	const draco::PointAttribute* dracoPosition = nullptr;
	const draco::PointAttribute* dracoNormal = nullptr;
	const draco::PointAttribute* dracoTangent = nullptr;
	const draco::PointAttribute* dracoTexcoord = nullptr;
	if( dracoMesh )
	{
		dracoPosition = GetDracoAttribute( context, primitive, *dracoMesh, cgltf_attribute_type_position, 0, error );
		if( !dracoPosition )
		{
			error = error.empty() ? "Draco primitive is missing POSITION" : error;
			return false;
		}
		if( hasNormals )
		{
			dracoNormal = GetDracoAttribute( context, primitive, *dracoMesh, cgltf_attribute_type_normal, 0, error );
			if( !dracoNormal )
			{
				error = error.empty() ? "Draco primitive is missing NORMAL" : error;
				return false;
			}
		}
		if( hasTangents )
		{
			dracoTangent = GetDracoAttribute( context, primitive, *dracoMesh, cgltf_attribute_type_tangent, 0, error );
			if( !dracoTangent )
			{
				error = error.empty() ? "Draco primitive is missing TANGENT" : error;
				return false;
			}
		}
		if( texcoordAttribute && texcoordAttribute->data )
		{
			dracoTexcoord = GetDracoAttribute( context, primitive, *dracoMesh, cgltf_attribute_type_texcoord, texcoordAttribute->index, error );
			if( !dracoTexcoord )
			{
				error = error.empty() ? "Draco primitive is missing TEXCOORD" : error;
				return false;
			}
		}
		if( dracoMesh->num_points() != positionAccessor->count )
		{
			error = "Decoded Draco point count does not match POSITION accessor metadata";
			return false;
		}
	}

	const cgltf_size vertexCount = dracoMesh ? dracoMesh->num_points() : positionAccessor->count;
	pending.vertices.resize( vertexCount );
	bool boundsInitialized = false;

	for( cgltf_size vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex )
	{
		ImportedVertex& vertex = pending.vertices[vertexIndex];

		float position[3] = {};
		if( dracoPosition ? !ReadDracoFloatAttribute( *dracoPosition, vertexIndex, position, 3, error ) :
							!ReadFloatAccessor( positionAccessor, vertexIndex, position, 3, error ) )
		{
			return false;
		}

		Vector3 modelPosition( position[0], position[1], position[2] );
		if( !skinned )
		{
			modelPosition = TransformCoord( modelPosition, nodeWorld );
		}
		vertex.position[0] = modelPosition.x;
		vertex.position[1] = modelPosition.y;
		vertex.position[2] = modelPosition.z;
		ExtendBounds( pending.bounds, modelPosition, boundsInitialized );

		Vector3 normal( 0.0f, 1.0f, 0.0f );
		if( hasNormals )
		{
			float normalValues[3] = {};
			if( dracoNormal ? !ReadDracoFloatAttribute( *dracoNormal, vertexIndex, normalValues, 3, error ) :
							  !ReadFloatAccessor( normalAttribute->data, vertexIndex, normalValues, 3, error ) )
			{
				return false;
			}
			normal = Vector3( normalValues[0], normalValues[1], normalValues[2] );
			if( !skinned )
			{
				normal = TransformNormal( normal, nodeWorld );
			}
			normal = Normalize( normal );
		}
		vertex.normal[0] = normal.x;
		vertex.normal[1] = normal.y;
		vertex.normal[2] = normal.z;
		if( hasTangents )
		{
			float tangentValues[4] = {};
			if( dracoTangent ? !ReadDracoFloatAttribute( *dracoTangent, vertexIndex, tangentValues, 4, error ) :
							   !ReadFloatAccessor( tangentAttribute->data, vertexIndex, tangentValues, 4, error ) )
			{
				return false;
			}
			Vector3 tangent( tangentValues[0], tangentValues[1], tangentValues[2] );
			if( !skinned )
			{
				tangent = TransformNormal( tangent, nodeWorld );
			}
			tangent = Normalize( tangent );
			vertex.tangent[0] = tangent.x;
			vertex.tangent[1] = tangent.y;
			vertex.tangent[2] = tangent.z;
			vertex.tangent[3] = tangentValues[3] * ( !skinned && Determinant( nodeWorld ) < 0.0f ? -1.0f : 1.0f );
			const Vector3 binormal = Normalize( Cross( normal, tangent ) ) * vertex.tangent[3];
			vertex.binormal[0] = binormal.x;
			vertex.binormal[1] = binormal.y;
			vertex.binormal[2] = binormal.z;
		}
		SetImportedColor( vertex, baseColorFactor );
		if( texcoordAttribute && texcoordAttribute->data )
		{
			float texcoord[2] = {};
			if( dracoTexcoord ? !ReadDracoFloatAttribute( *dracoTexcoord, vertexIndex, texcoord, 2, error ) :
								!ReadFloatAccessor( texcoordAttribute->data, vertexIndex, texcoord, 2, error ) )
			{
				return false;
			}
			vertex.texcoord[0] = texcoord[0];
			vertex.texcoord[1] = texcoord[1];
		}

		if( skinned )
		{
			cgltf_uint joints[4] = {};
			float weights[4] = {};
			if( !ReadUintAccessor( jointsAttribute->data, vertexIndex, joints, 4, error ) )
			{
				return false;
			}
			if( !ReadFloatAccessor( weightsAttribute->data, vertexIndex, weights, 4, error ) )
			{
				return false;
			}
			for( size_t i = 0; i < 4; ++i )
			{
				if( joints[i] > std::numeric_limits<uint16_t>::max() )
				{
					error = "Joint index exceeds UInt16 storage";
					return false;
				}
				vertex.joints[i] = static_cast<uint16_t>( joints[i] );
				vertex.weights[i] = weights[i];
			}
		}
		else
		{
			vertex.weights[0] = 1.0f;
		}
	}

	if( dracoMesh )
	{
		pending.indices.resize( static_cast<size_t>( dracoMesh->num_faces() ) * 3 );
		for( uint32_t faceIndex = 0; faceIndex < dracoMesh->num_faces(); ++faceIndex )
		{
			const draco::Mesh::Face& face = dracoMesh->face( draco::FaceIndex( faceIndex ) );
			pending.indices[faceIndex * 3] = static_cast<uint32_t>( face[0].value() );
			pending.indices[faceIndex * 3 + 1] = static_cast<uint32_t>( face[1].value() );
			pending.indices[faceIndex * 3 + 2] = static_cast<uint32_t>( face[2].value() );
		}
	}
	else if( primitive.indices )
	{
		pending.indices.resize( primitive.indices->count );
		for( cgltf_size index = 0; index < primitive.indices->count; ++index )
		{
			pending.indices[index] = cgltf_accessor_read_index( primitive.indices, index );
		}
	}
	else
	{
		pending.indices.resize( pending.vertices.size() );
		for( uint32_t i = 0; i < pending.indices.size(); ++i )
		{
			pending.indices[i] = i;
		}
	}

	if( pending.indices.size() % 3 != 0 )
	{
		error = "Triangle primitive index count is not divisible by 3";
		return false;
	}
	if( !hasNormals )
	{
		GenerateNormals( pending );
	}

	if( skinned )
	{
		const PendingSkeleton& skeleton = context.skeletons[skeletonIndex];
		pending.boneBindings = skeleton.bones;
	}
	pending.areas.push_back( {
		pending.name,
		0,
		static_cast<uint32_t>( pending.indices.size() / 3 ),
		pending.bounds,
	} );

	context.meshes.push_back( std::move( pending ) );
	return true;
}

bool MergeImportedMeshes( ImportContext& context, std::string& error )
{
	if( context.meshes.size() < 2 )
	{
		return true;
	}

	PendingMesh merged;
	merged.name = "merged_mesh";
	merged.skeletonIndex = context.meshes.front().skeletonIndex;
	merged.boneBindings = context.meshes.front().boneBindings;
	bool boundsInitialized = false;
	for( const PendingMesh& source : context.meshes )
	{
		if( source.skeletonIndex != merged.skeletonIndex || source.boneBindings != merged.boneBindings )
		{
			error = "--merge-meshes requires compatible skeleton bindings";
			return false;
		}
		const uint32_t vertexOffset = static_cast<uint32_t>( merged.vertices.size() );
		const uint32_t firstElement = static_cast<uint32_t>( merged.indices.size() / 3 );
		merged.vertices.insert( merged.vertices.end(), source.vertices.begin(), source.vertices.end() );
		for( uint32_t index : source.indices )
		{
			merged.indices.push_back( index + vertexOffset );
		}
		for( const PendingMesh::Area& sourceArea : source.areas )
		{
			PendingMesh::Area area = sourceArea;
			area.firstElement += firstElement;
			merged.areas.push_back( std::move( area ) );
		}
		merged.hasTangents = merged.hasTangents || source.hasTangents;
		ExtendBounds( merged.bounds, source.bounds.m_min, boundsInitialized );
		ExtendBounds( merged.bounds, source.bounds.m_max, boundsInitialized );
	}
	context.meshes.clear();
	context.meshes.push_back( std::move( merged ) );
	return true;
}

bool ImportNodeMeshes( ImportContext& context, const cgltf_node& node, const Matrix& parentWorld, std::string& error )
{
	const Matrix nodeWorld = ReadNodeMatrix( node ) * parentWorld;
	if( node.mesh )
	{
		for( cgltf_size primitiveIndex = 0; primitiveIndex < node.mesh->primitives_count; ++primitiveIndex )
		{
			if( !ImportPrimitive( context, node, *node.mesh, node.mesh->primitives[primitiveIndex], nodeWorld, primitiveIndex, error ) )
			{
				error = "Node '" + MakeName( node.name, "node", 0 ) + "': " + error;
				return false;
			}
		}
	}

	for( cgltf_size childIndex = 0; childIndex < node.children_count; ++childIndex )
	{
		if( !ImportNodeMeshes( context, *node.children[childIndex], nodeWorld, error ) )
		{
			return false;
		}
	}

	return true;
}

bool ImportMeshes( ImportContext& context, std::string& error )
{
	if( context.data->scenes_count != 1 )
	{
		error = "Importer v1 supports exactly one glTF scene";
		return false;
	}

	const cgltf_scene* scene = context.data->scene ? context.data->scene : &context.data->scenes[0];
	for( cgltf_size nodeIndex = 0; nodeIndex < scene->nodes_count; ++nodeIndex )
	{
		if( !ImportNodeMeshes( context, *scene->nodes[nodeIndex], IdentityMatrix(), error ) )
		{
			return false;
		}
	}

	if( context.meshes.empty() )
	{
		error = "No supported meshes were found";
		return false;
	}

	return true;
}

bool FindSkeletonForNode( const ImportContext& context, const cgltf_node* node, const PendingSkeleton*& skeleton, uint32_t& boneIndex )
{
	for( const PendingSkeleton& candidate : context.skeletons )
	{
		auto found = candidate.jointToBone.find( node );
		if( found != candidate.jointToBone.end() )
		{
			skeleton = &candidate;
			boneIndex = found->second;
			return true;
		}
	}
	return false;
}

bool ImportAnimationCurve( const cgltf_animation_sampler& sampler, cmf::AnimationChannelTargetType targetType, PendingCurve& curve, std::string& error )
{
	if( sampler.interpolation != cgltf_interpolation_type_step && sampler.interpolation != cgltf_interpolation_type_linear )
	{
		error = "Unsupported animation interpolation; only STEP and LINEAR are supported";
		return false;
	}

	if( !sampler.input || !sampler.output )
	{
		error = "Animation sampler is missing input or output";
		return false;
	}

	if( targetType == cmf::AnimationChannelTargetType::BoneRotation )
	{
		curve.valueDimension = 4;
	}
	else
	{
		curve.valueDimension = 3;
	}
	curve.interpolation = sampler.interpolation == cgltf_interpolation_type_linear ? cmf::Interpolation::Linear : cmf::Interpolation::Step;
	curve.knots.resize( sampler.input->count );
	curve.values.resize( sampler.input->count * curve.valueDimension );

	if( sampler.output->count != sampler.input->count )
	{
		error = "Animation sampler output count does not match input count";
		return false;
	}

	for( cgltf_size keyIndex = 0; keyIndex < sampler.input->count; ++keyIndex )
	{
		float knot = 0.0f;
		if( !ReadFloatAccessor( sampler.input, keyIndex, &knot, 1, error ) )
		{
			return false;
		}
		curve.knots[keyIndex] = knot;

		if( !ReadFloatAccessor( sampler.output, keyIndex, curve.values.data() + keyIndex * curve.valueDimension, curve.valueDimension, error ) )
		{
			return false;
		}

		if( targetType == cmf::AnimationChannelTargetType::BoneRotation )
		{
			Quaternion q(
				curve.values[keyIndex * 4 + 0],
				curve.values[keyIndex * 4 + 1],
				curve.values[keyIndex * 4 + 2],
				curve.values[keyIndex * 4 + 3] );
			q = Normalize( q );
			curve.values[keyIndex * 4 + 0] = q.x;
			curve.values[keyIndex * 4 + 1] = q.y;
			curve.values[keyIndex * 4 + 2] = q.z;
			curve.values[keyIndex * 4 + 3] = q.w;
		}
	}

	return true;
}

bool ImportAnimations( ImportContext& context, std::string& error )
{
	for( cgltf_size animationIndex = 0; animationIndex < context.data->animations_count; ++animationIndex )
	{
		const cgltf_animation& sourceAnimation = context.data->animations[animationIndex];
		PendingAnimation animation;
		animation.name = MakeName( sourceAnimation.name, "animation", animationIndex );

		for( cgltf_size channelIndex = 0; channelIndex < sourceAnimation.channels_count; ++channelIndex )
		{
			const cgltf_animation_channel& sourceChannel = sourceAnimation.channels[channelIndex];
			cmf::AnimationChannelTargetType targetType = cmf::AnimationChannelTargetType::Other;
			switch( sourceChannel.target_path )
			{
			case cgltf_animation_path_type_translation:
				targetType = cmf::AnimationChannelTargetType::BonePosition;
				break;
			case cgltf_animation_path_type_rotation:
				targetType = cmf::AnimationChannelTargetType::BoneRotation;
				break;
			case cgltf_animation_path_type_scale:
				targetType = cmf::AnimationChannelTargetType::BoneScale;
				break;
			case cgltf_animation_path_type_weights:
				error = "Morph target animation is out of scope for importer v1";
				return false;
			default:
				error = "Unsupported animation channel target";
				return false;
			}

			const PendingSkeleton* skeleton = nullptr;
			uint32_t boneIndex = 0;
			if( !FindSkeletonForNode( context, sourceChannel.target_node, skeleton, boneIndex ) )
			{
				context.result->warnings.push_back( "Skipping animation channel targeting a node outside imported skeletons" );
				continue;
			}

			PendingCurve curve;
			if( !ImportAnimationCurve( *sourceChannel.sampler, targetType, curve, error ) )
			{
				error = "Animation '" + animation.name + "': " + error;
				return false;
			}

			const uint32_t curveIndex = static_cast<uint32_t>( animation.curves.size() );
			if( !curve.knots.empty() )
			{
				animation.duration = std::max( animation.duration, curve.knots.back() );
			}
			animation.curves.push_back( std::move( curve ) );
			animation.channels.push_back( { skeleton->bones[boneIndex], targetType, curveIndex } );
		}

		if( animation.channels.empty() )
		{
			context.result->warnings.push_back( "Skipping animation '" + animation.name + "' because it has no supported skeleton channels" );
			continue;
		}

		context.animations.push_back( std::move( animation ) );
	}

	return true;
}

template <typename T>
cmf::Span<T> CopySpan( cmf::MemoryAllocator& allocator, const std::vector<T>& values )
{
	if( values.empty() )
	{
		return {};
	}
	cmf::Span<T> span = allocator.AllocateSpan<T>( values.size() );
	std::copy( values.begin(), values.end(), span.begin() );
	return span;
}

cmf::Span<char> CopyString( cmf::MemoryAllocator& allocator, const std::string& value )
{
	return allocator.AllocateString( value );
}

cmf::Span<uint8_t> CopyFloatBytes( cmf::MemoryAllocator& allocator, const std::vector<float>& values )
{
	if( values.empty() )
	{
		return {};
	}
	cmf::Span<uint8_t> span = allocator.AllocateSpan<uint8_t>( values.size() * sizeof( float ) );
	std::memcpy( span.data(), values.data(), span.byteSize );
	return span;
}

cmf::Span<cmf::VertexElement> BuildVertexDecl( cmf::MemoryAllocator& allocator, bool includeTangents, bool includeSkinning )
{
	cmf::Span<cmf::VertexElement> decl = allocator.AllocateSpan<cmf::VertexElement>( 4 + ( includeTangents ? 2 : 0 ) + ( includeSkinning ? 2 : 0 ) );
	size_t index = 0;
	decl[index++] = { cmf::Usage::Position, 0, cmf::ElementType::Float32, 3, offsetof( ImportedVertex, position ) };
	decl[index++] = { cmf::Usage::Normal, 0, cmf::ElementType::Float32, 3, offsetof( ImportedVertex, normal ) };
	if( includeTangents )
	{
		decl[index++] = { cmf::Usage::Tangent, 0, cmf::ElementType::Float32, 4, offsetof( ImportedVertex, tangent ) };
		decl[index++] = { cmf::Usage::Binormal, 0, cmf::ElementType::Float32, 3, offsetof( ImportedVertex, binormal ) };
	}
	decl[index++] = { cmf::Usage::TexCoord, 0, cmf::ElementType::Float32, 2, offsetof( ImportedVertex, texcoord ) };
	decl[index++] = { cmf::Usage::Color, 0, cmf::ElementType::Float32, 4, offsetof( ImportedVertex, color ) };
	if( includeSkinning )
	{
		decl[index++] = { cmf::Usage::BoneIndices, 0, cmf::ElementType::UInt16, 4, offsetof( ImportedVertex, joints ) };
		decl[index++] = { cmf::Usage::BoneWeights, 0, cmf::ElementType::Float32, 4, offsetof( ImportedVertex, weights ) };
	}
	return decl;
}

std::vector<uint8_t> BuildIndexBuffer( const std::vector<uint32_t>& indices, uint32_t& stride )
{
	const uint32_t maxIndex = indices.empty() ? 0 : *std::max_element( indices.begin(), indices.end() );
	if( maxIndex <= std::numeric_limits<uint16_t>::max() )
	{
		stride = sizeof( uint16_t );
		std::vector<uint8_t> bytes( indices.size() * sizeof( uint16_t ) );
		uint16_t* out = reinterpret_cast<uint16_t*>( bytes.data() );
		for( size_t i = 0; i < indices.size(); ++i )
		{
			out[i] = static_cast<uint16_t>( indices[i] );
		}
		return bytes;
	}

	stride = sizeof( uint32_t );
	std::vector<uint8_t> bytes( indices.size() * sizeof( uint32_t ) );
	std::memcpy( bytes.data(), indices.data(), bytes.size() );
	return bytes;
}

std::vector<uint8_t> SerializeCmf( const ImportContext& context )
{
	cmf::MemoryAllocator allocator;
	cmf::BufferManager buffers( allocator );
	std::vector<std::vector<uint8_t>> indexBuffers;
	indexBuffers.reserve( context.meshes.size() );

	cmf::Data data;
	data.meshes = allocator.AllocateSpan<cmf::Mesh>( context.meshes.size() );
	data.skeletons = allocator.AllocateSpan<cmf::Skeleton>( context.skeletons.size() );
	data.animations = allocator.AllocateSpan<cmf::Animation>( context.animations.size() );

	for( size_t skeletonIndex = 0; skeletonIndex < context.skeletons.size(); ++skeletonIndex )
	{
		const PendingSkeleton& src = context.skeletons[skeletonIndex];
		cmf::Skeleton& dst = data.skeletons[skeletonIndex] = {};
		dst.name = CopyString( allocator, src.name );
		dst.bones = allocator.AllocateSpan<cmf::String>( src.bones.size() );
		for( size_t boneIndex = 0; boneIndex < src.bones.size(); ++boneIndex )
		{
			dst.bones[boneIndex] = CopyString( allocator, src.bones[boneIndex] );
		}
		dst.parents = CopySpan( allocator, src.parents );
		dst.restTransforms = CopySpan( allocator, src.restTransforms );
		dst.invBindTransforms = CopySpan( allocator, src.invBindTransforms );
		dst.boneMasks = {};
	}

	for( size_t meshIndex = 0; meshIndex < context.meshes.size(); ++meshIndex )
	{
		const PendingMesh& src = context.meshes[meshIndex];
		cmf::Mesh& dst = data.meshes[meshIndex] = {};
		dst.name = CopyString( allocator, src.name );
		dst.decl = BuildVertexDecl( allocator, src.hasTangents, !src.boneBindings.empty() );
		dst.lods = allocator.AllocateSpan<cmf::MeshLod>( 1 );
		dst.areas = allocator.AllocateSpan<cmf::MeshArea>( src.areas.size() );
		dst.boneBindings = allocator.AllocateSpan<cmf::BoneBinding>( src.boneBindings.size() );
		dst.uvDensities = allocator.AllocateSpan<float>( 1 );
		dst.uvDensities[0] = 0.0f;
		dst.bounds = src.bounds;
		dst.topology = cmf::MeshTopology::TriangleList;
		dst.skeleton = static_cast<uint8_t>( src.skeletonIndex );

		for( size_t boneIndex = 0; boneIndex < src.boneBindings.size(); ++boneIndex )
		{
			dst.boneBindings[boneIndex].name = CopyString( allocator, src.boneBindings[boneIndex] );
			dst.boneBindings[boneIndex].bounds = src.bounds;
		}

		for( size_t areaIndex = 0; areaIndex < src.areas.size(); ++areaIndex )
		{
			cmf::MeshArea& area = dst.areas[areaIndex] = {};
			area.name = CopyString( allocator, src.areas[areaIndex].name );
			area.bounds = src.areas[areaIndex].bounds;
			area.affectedByBones = !src.boneBindings.empty();
			area.affectedByMorphTargets = false;
			if( !src.boneBindings.empty() )
			{
				area.bones = allocator.AllocateSpan<uint16_t>( src.boneBindings.size() );
				for( size_t boneIndex = 0; boneIndex < src.boneBindings.size(); ++boneIndex )
				{
					area.bones[boneIndex] = static_cast<uint16_t>( boneIndex );
				}
			}
		}

		cmf::MeshLod& lod = dst.lods[0] = {};
		lod.vb = buffers.AddBuffer( const_cast<ImportedVertex*>( src.vertices.data() ), static_cast<uint32_t>( src.vertices.size() * sizeof( ImportedVertex ) ), sizeof( ImportedVertex ) );

		uint32_t indexStride = 0;
		indexBuffers.push_back( BuildIndexBuffer( src.indices, indexStride ) );
		std::vector<uint8_t>& indexBytes = indexBuffers.back();
		lod.ib = buffers.AddBuffer( indexBytes.data(), static_cast<uint32_t>( indexBytes.size() ), indexStride );
		lod.areas = allocator.AllocateSpan<cmf::LodMeshArea>( src.areas.size() );
		for( size_t areaIndex = 0; areaIndex < src.areas.size(); ++areaIndex )
		{
			lod.areas[areaIndex].firstElement = src.areas[areaIndex].firstElement;
			lod.areas[areaIndex].elementCount = src.areas[areaIndex].elementCount;
		}
		lod.morphTargets = {};
		lod.threshold = cmf::MeshLod::MAX_THRESHOLD;
	}

	for( size_t animationIndex = 0; animationIndex < context.animations.size(); ++animationIndex )
	{
		const PendingAnimation& src = context.animations[animationIndex];
		cmf::Animation& dst = data.animations[animationIndex] = {};
		dst.name = CopyString( allocator, src.name );
		dst.duration = src.duration;
		dst.channels = allocator.AllocateSpan<cmf::AnimationChannel>( src.channels.size() );
		dst.curves = allocator.AllocateSpan<cmf::AnimationCurve>( src.curves.size() );

		for( size_t channelIndex = 0; channelIndex < src.channels.size(); ++channelIndex )
		{
			dst.channels[channelIndex].target = CopyString( allocator, src.channels[channelIndex].target );
			dst.channels[channelIndex].targetType = src.channels[channelIndex].targetType;
			dst.channels[channelIndex].curveIndex = src.channels[channelIndex].curveIndex;
		}

		for( size_t curveIndex = 0; curveIndex < src.curves.size(); ++curveIndex )
		{
			const PendingCurve& srcCurve = src.curves[curveIndex];
			cmf::AnimationCurve& dstCurve = dst.curves[curveIndex] = {};
			dstCurve.valueDimension = srcCurve.valueDimension;
			dstCurve.interpolation = srcCurve.interpolation;
			dstCurve.knotType = cmf::ElementType::Float32;
			dstCurve.valueType = cmf::ElementType::Float32;
			dstCurve.knotCount = static_cast<uint32_t>( srcCurve.knots.size() );
			dstCurve.knots = CopyFloatBytes( allocator, srcCurve.knots );
			dstCurve.values = CopyFloatBytes( allocator, srcCurve.values );
		}
	}

	return cmf::BuildFile( data, buffers );
}

void PopulateResultTextures( const ImportContext& context )
{
	if( context.baseColorImageIndices.empty() )
	{
		return;
	}
	if( context.baseColorImageIndices.size() > 1 )
	{
		context.result->warnings.push_back( "Importer v1 writes only the first base color texture sidecar" );
	}

	const cgltf_size imageIndex = context.baseColorImageIndices.front();
	if( imageIndex >= context.images.size() )
	{
		return;
	}

	const DecodedImage& source = context.images[imageIndex];
	if( source.width <= 0 || source.height <= 0 || source.pixels.empty() )
	{
		return;
	}

	GltfToCmfTexture texture;
	texture.name = MakeName( context.data->images[imageIndex].name, "basecolor", imageIndex );
	texture.width = static_cast<uint32_t>( source.width );
	texture.height = static_cast<uint32_t>( source.height );
	texture.rgba = source.pixels;
	context.result->baseColorTextures.push_back( std::move( texture ) );
}

bool LoadGltf( const GltfToCmfOptions& options, std::unique_ptr<cgltf_data, CgltfDataDeleter>& data, std::string& error )
{
	cgltf_options cgltfOptions = {};
	cgltf_data* rawData = nullptr;
	cgltf_result result = cgltf_parse_file( &cgltfOptions, options.inputPath.c_str(), &rawData );
	if( result != cgltf_result_success )
	{
		error = "Failed to parse glTF file: " + options.inputPath;
		return false;
	}
	data.reset( rawData );

	result = cgltf_load_buffers( &cgltfOptions, data.get(), options.inputPath.c_str() );
	if( result != cgltf_result_success )
	{
		error = "Failed to load glTF buffers: " + options.inputPath;
		return false;
	}

	result = cgltf_validate( data.get() );
	if( result != cgltf_result_success )
	{
		error = "glTF validation failed: " + options.inputPath;
		return false;
	}

	return true;
}

}

bool BuildCmfFromGltf( const GltfToCmfOptions& options, GltfToCmfResult& result, std::string& error )
{
	result = {};

	std::unique_ptr<cgltf_data, CgltfDataDeleter> data;
	if( !LoadGltf( options, data, error ) )
	{
		return false;
	}

	if( HasExtension( *data, "EXT_meshopt_compression" ) )
	{
		error = "EXT_meshopt_compression is not supported by importer v1";
		return false;
	}
	if( !ValidateNodeMatrices( *data, error ) )
	{
		return false;
	}

	ImportContext context;
	context.data = data.get();
	context.result = &result;

	if( !DecodeImages( context, error ) )
	{
		return false;
	}
	if( !ImportSkeletons( context, error ) )
	{
		return false;
	}
	if( !ImportMeshes( context, error ) )
	{
		return false;
	}
	if( options.mergeMeshes && !MergeImportedMeshes( context, error ) )
	{
		return false;
	}
	if( !ImportAnimations( context, error ) )
	{
		return false;
	}

	result.cmfBytes = SerializeCmf( context );
	auto validated = cmf::ValidateFile( result.cmfBytes.data(), result.cmfBytes.size(), { true, true, true, true } );
	if( !validated )
	{
		error = "Generated CMF failed validation: " + validated.error;
		return false;
	}

	result.meshCount = static_cast<uint32_t>( context.meshes.size() );
	result.skeletonCount = static_cast<uint32_t>( context.skeletons.size() );
	for( const PendingAnimation& animation : context.animations )
	{
		result.animationNames.push_back( animation.name );
	}
	PopulateResultTextures( context );

	return true;
}
