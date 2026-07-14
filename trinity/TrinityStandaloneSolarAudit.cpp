// Copyright (c) 2026 CCP Games

#include "StdAfx.h"

#include "TrinityStandaloneSolarAudit.h"

#include "Eve/EvePlanet.h"
#include "Eve/EveSpaceScene.h"
#include "Eve/SpaceObject/Children/IEveSpaceObjectChild.h"
#include "Particle/Tr2DynamicEmitter.h"
#include "PostProcess/Tr2PostProcess2.h"
#include "Shader/Parameter/TriTextureParameter.h"
#include "Shader/Tr2Effect.h"
#include "Tr2MeshArea.h"
#include "Tr2MeshBase.h"
#include "Tr2Renderer.h"
#include "Tr2VolumetricsRenderer.h"

#include <BlueMemberIterator.h>
#include <IList.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace
{
struct AuditValue
{
	std::string json;
};

struct AuditDetail
{
	std::string id;
	std::string parentId;
	std::string relation;
	std::string className;
	std::map<std::string, AuditValue> attributes;
};

struct AuditNode
{
	std::string id;
	std::string parentId;
	std::string name;
	std::string className;
	bool display = true;
	int displayFilter = 5;
	bool qualitySelected = true;
	bool reachable = true;
	bool retainedAfterPreparation = false;
	size_t curveSetCount = 0;
	size_t controllerCount = 0;
	size_t particleSystemCount = 0;
	size_t emitterCount = 0;
	size_t meshAreaCount = 0;
	std::vector<std::string> effectPaths;
	std::vector<std::string> resourcePaths;
	std::vector<AuditDetail> details;
};

struct RuntimeNode
{
	size_t renderableCount = 0;
	uint64_t aliveParticleCount = 0;
	uint64_t peakParticleCount = 0;
	uint64_t emittedParticleCount = 0;
};

std::pair<const Be::VarEntry*, Be::Var*> FindEntry( IRoot* object, const char* name )
{
	if( !object )
	{
		return std::make_pair( nullptr, nullptr );
	}
	const Be::ClassInfo* type = object->ClassType();
	ssize_t offset = 0;
	for( ; type; offset += type->mOffsetToParent, type = type->mParentClassInfo )
	{
		if( !type->mMemberTable )
		{
			continue;
		}
		for( const Be::VarEntry* entry = type->mMemberTable; entry->mName; ++entry )
		{
			if( !entry->mGetProperty && std::strcmp( entry->mName, name ) == 0 )
			{
				return std::make_pair( entry, BLUEMAPMEMBEROFFSET( object, entry, type, offset ) );
			}
		}
	}
	return std::make_pair( nullptr, nullptr );
}

IRoot* GetRootAttribute( IRoot* object, const char* name )
{
	const auto entry = FindEntry( object, name );
	if( !entry.first || !entry.second )
	{
		return nullptr;
	}
	if( entry.first->mType == Be::IROOTPTR )
	{
		return entry.second->mIRootPtr;
	}
	if( entry.first->mType == Be::IROOT )
	{
		return reinterpret_cast<IRoot*>( entry.second );
	}
	return nullptr;
}

IListPtr GetListAttribute( IRoot* object, const char* name )
{
	return BlueCastPtr( GetRootAttribute( object, name ) );
}

bool ReadBool( IRoot* object, const char* name, bool& value )
{
	const auto entry = FindEntry( object, name );
	if( !entry.first || !entry.second || entry.first->mType != Be::BOOL )
	{
		return false;
	}
	value = entry.second->mBool;
	return true;
}

bool ReadLong( IRoot* object, const char* name, int64_t& value )
{
	const auto entry = FindEntry( object, name );
	if( !entry.first || !entry.second )
	{
		return false;
	}
	switch( entry.first->mType )
	{
	case Be::LONG:
		value = entry.second->mLong;
		return true;
	case Be::ULONG:
		value = entry.second->mULong;
		return true;
	case Be::BYTE:
		value = entry.second->mByte;
		return true;
	case Be::SHORT:
		value = entry.second->mShort;
		return true;
	case Be::INT64:
		value = entry.second->mInt64;
		return true;
	case Be::UINT64:
		value = static_cast<int64_t>( entry.second->mUInt64 );
		return true;
	default:
		return false;
	}
}

bool ReadFloat( IRoot* object, const char* name, double& value )
{
	const auto entry = FindEntry( object, name );
	if( !entry.first || !entry.second )
	{
		return false;
	}
	if( entry.first->mType == Be::FLOAT )
	{
		value = entry.second->mFloat;
		return true;
	}
	if( entry.first->mType == Be::DOUBLE )
	{
		value = entry.second->mDouble;
		return true;
	}
	return false;
}

std::string JsonString( const std::string& value )
{
	std::ostringstream output;
	output << '"';
	for( const unsigned char character : value )
	{
		switch( character )
		{
		case '"':
			output << "\\\"";
			break;
		case '\\':
			output << "\\\\";
			break;
		case '\b':
			output << "\\b";
			break;
		case '\f':
			output << "\\f";
			break;
		case '\n':
			output << "\\n";
			break;
		case '\r':
			output << "\\r";
			break;
		case '\t':
			output << "\\t";
			break;
		default:
			if( character < 0x20 )
			{
				output << "\\u" << std::hex << std::setw( 4 ) << std::setfill( '0' )
					   << static_cast<unsigned>( character ) << std::dec;
			}
			else
			{
				output << character;
			}
			break;
		}
	}
	output << '"';
	return output.str();
}

std::string ClassName( IRoot* object )
{
	const Be::ClassInfo* type = object ? object->ClassType() : nullptr;
	return type && type->mClassId && type->mClassId->GetName() ? type->mClassId->GetName() : "unknown";
}

std::string NarrowPath( const wchar_t* value )
{
	std::string result;
	if( !value )
	{
		return result;
	}
	for( ; *value; ++value )
	{
		result.push_back( static_cast<char>( *value ) );
	}
	return result;
}

std::string StableSegment( const std::string& value )
{
	std::ostringstream output;
	for( const unsigned char character : value )
	{
		if( std::isalnum( character ) || character == '-' || character == '_' || character == '.' )
		{
			output << character;
		}
		else
		{
			output << '%' << std::uppercase << std::hex << std::setw( 2 ) << std::setfill( '0' )
				   << static_cast<unsigned>( character ) << std::nouppercase << std::dec;
		}
	}
	return output.str();
}

std::string ScalarJson( const Be::VarEntry& entry, Be::Var* value )
{
	if( !value )
	{
		return "null";
	}
	std::ostringstream output;
	output << std::setprecision( 9 );
	switch( entry.mType )
	{
	case Be::BOOL:
		return value->mBool ? "true" : "false";
	case Be::LONG:
		output << value->mLong;
		return output.str();
	case Be::ULONG:
		output << value->mULong;
		return output.str();
	case Be::BYTE:
		output << static_cast<unsigned>( value->mByte );
		return output.str();
	case Be::SHORT:
		output << value->mShort;
		return output.str();
	case Be::INT64:
		output << value->mInt64;
		return output.str();
	case Be::UINT64:
		output << value->mUInt64;
		return output.str();
	case Be::FLOAT:
		if( std::isfinite( value->mFloat ) )
		{
			output << value->mFloat;
			return output.str();
		}
		return "null";
	case Be::DOUBLE:
		if( std::isfinite( value->mDouble ) )
		{
			output << value->mDouble;
			return output.str();
		}
		return "null";
	case Be::CHARARRAY:
		return JsonString( reinterpret_cast<const char*>( value ) );
	case Be::CSTRING:
		return JsonString( value->mCharPtr ? value->mCharPtr : "" );
	case Be::STDSTRING:
		return JsonString( *reinterpret_cast<const std::string*>( value ) );
	case Be::SHAREDSTRING:
		return JsonString( reinterpret_cast<const BlueSharedString*>( value )->c_str() );
	case Be::FLOATARRAY: {
		const size_t count = std::min<size_t>( entry.GetFloatArraySize(), 16 );
		const float* values = reinterpret_cast<const float*>( value );
		output << '[';
		for( size_t index = 0; index < count; ++index )
		{
			if( index )
			{
				output << ',';
			}
			if( std::isfinite( values[index] ) )
			{
				output << values[index];
			}
			else
			{
				output << "null";
			}
		}
		output << ']';
		return output.str();
	}
	case Be::DOUBLEARRAY: {
		const size_t count = std::min<size_t>( entry.GetDoubleArraySize(), 16 );
		const double* values = reinterpret_cast<const double*>( value );
		output << '[';
		for( size_t index = 0; index < count; ++index )
		{
			if( index )
			{
				output << ',';
			}
			if( std::isfinite( values[index] ) )
			{
				output << values[index];
			}
			else
			{
				output << "null";
			}
		}
		output << ']';
		return output.str();
	}
	case Be::INTARRAY: {
		const size_t count = std::min<size_t>( entry.GetIntArraySize(), 16 );
		const int* values = reinterpret_cast<const int*>( value );
		output << '[';
		for( size_t index = 0; index < count; ++index )
		{
			if( index )
			{
				output << ',';
			}
			output << values[index];
		}
		output << ']';
		return output.str();
	}
	default:
		return std::string();
	}
}

std::map<std::string, AuditValue> CaptureScalarAttributes( IRoot* object )
{
	std::map<std::string, AuditValue> attributes;
	if( !object )
	{
		return attributes;
	}
	const Be::ClassInfo* type = object->ClassType();
	ssize_t offset = 0;
	for( ; type; offset += type->mOffsetToParent, type = type->mParentClassInfo )
	{
		if( !type->mMemberTable )
		{
			continue;
		}
		for( const Be::VarEntry* entry = type->mMemberTable; entry->mName; ++entry )
		{
			if( entry->mGetProperty )
			{
				continue;
			}
			Be::Var* value = BLUEMAPMEMBEROFFSET( object, entry, type, offset );
			const std::string json = ScalarJson( *entry, value );
			if( !json.empty() )
			{
				attributes[entry->mName] = { json };
			}
		}
	}
	return attributes;
}

size_t ListSize( IRoot* object, const char* name )
{
	if( IListPtr list = GetListAttribute( object, name ) )
	{
		return static_cast<size_t>( std::max<ssize_t>( 0, list->GetSize() ) );
	}
	return 0;
}

const char* DisplayFilterName( int filter )
{
	switch( filter )
	{
	case 0:
		return "low";
	case 1:
		return "low-medium";
	case 2:
		return "medium";
	case 3:
		return "medium-high";
	case 4:
		return "high";
	case 5:
		return "all";
	case 6:
		return "reflections-only";
	default:
		return "unknown";
	}
}

bool FilterSelectedAtHigh( int filter )
{
	return filter == 3 || filter == 4 || filter == 5;
}

void CaptureDetails(
	IRoot* owner,
	const std::string& parentId,
	std::vector<AuditDetail>& details,
	unsigned depth )
{
	if( !owner || depth > 5 )
	{
		return;
	}
	static const char* relationships[] = {
		"curveSets",
		"controllers",
		"particleSystems",
		"particleEmitters",
		"generators",
		"elements",
		"forces",
		"constraints",
		"curves",
		"bindings",
		"ranges",
		"actions",
		"transformModifiers",
	};
	for( const char* relationship : relationships )
	{
		IListPtr list = GetListAttribute( owner, relationship );
		if( !list )
		{
			continue;
		}
		for( ssize_t index = 0; index < list->GetSize(); ++index )
		{
			IRoot* child = list->GetAt( index );
			if( !child )
			{
				continue;
			}
			AuditDetail detail;
			detail.parentId = parentId;
			detail.relation = relationship;
			detail.id = parentId + "@" + relationship + "[" + std::to_string( index ) + "]";
			detail.className = ClassName( child );
			detail.attributes = CaptureScalarAttributes( child );
			details.push_back( detail );
			CaptureDetails( child, detail.id, details, depth + 1 );
		}
	}
}

void CaptureMesh( IRoot* child, AuditNode& node )
{
	Tr2MeshBasePtr mesh = BlueCastPtr( GetRootAttribute( child, "mesh" ) );
	if( !mesh )
	{
		return;
	}
	const std::string geometryPath = NarrowPath( mesh->GetGeometryResPath() );
	if( !geometryPath.empty() )
	{
		node.resourcePaths.push_back( geometryPath );
	}
	for( int batchType = 0; batchType < TRIBATCHTYPE_COUNT_OF_BATCH_TYPES; ++batchType )
	{
		const Tr2MeshAreaVector* areas = mesh->GetAreas( static_cast<TriBatchType>( batchType ) );
		if( !areas )
		{
			continue;
		}
		node.meshAreaCount += areas->size();
		for( Tr2MeshArea* area : *areas )
		{
			Tr2Effect* effect = area ? area->GetMaterialInterface() : nullptr;
			if( !effect )
			{
				continue;
			}
			if( effect->GetEffectPathName() && effect->GetEffectPathName()[0] )
			{
				node.effectPaths.emplace_back( effect->GetEffectPathName() );
			}
			for( ITriEffectResourceParameter* resource : effect->m_resources )
			{
				TriTextureParameterPtr texture = BlueCastPtr( resource );
				if( texture && texture->GetAuthoredResourcePath() && texture->GetAuthoredResourcePath()[0] )
				{
					node.resourcePaths.emplace_back( texture->GetAuthoredResourcePath() );
				}
			}
		}
	}
	std::sort( node.effectPaths.begin(), node.effectPaths.end() );
	node.effectPaths.erase( std::unique( node.effectPaths.begin(), node.effectPaths.end() ), node.effectPaths.end() );
	std::sort( node.resourcePaths.begin(), node.resourcePaths.end() );
	node.resourcePaths.erase(
		std::unique( node.resourcePaths.begin(), node.resourcePaths.end() ), node.resourcePaths.end() );
}

void CaptureNodeRecursive(
	IEveSpaceObjectChild* child,
	const std::string& parentId,
	bool parentSelected,
	size_t sameNameOrdinal,
	std::vector<AuditNode>& nodes )
{
	if( !child )
	{
		return;
	}
	AuditNode node;
	node.parentId = parentId;
	node.name = child->GetName() ? child->GetName() : "";
	node.className = ClassName( child );
	const std::string segment = StableSegment( node.name.empty() ? node.className : node.name );
	node.id = parentId + "/" + segment + "[" + std::to_string( sameNameOrdinal ) + "]";
	ReadBool( child, "display", node.display );
	int64_t filter = 5;
	ReadLong( child, "displayFilter", filter );
	node.displayFilter = static_cast<int>( filter );
	node.qualitySelected = parentSelected && FilterSelectedAtHigh( node.displayFilter );
	node.reachable = node.name != "InsideTheSphereCover";
	node.curveSetCount = ListSize( child, "curveSets" );
	node.controllerCount = ListSize( child, "controllers" );
	node.particleSystemCount = ListSize( child, "particleSystems" );
	node.emitterCount = ListSize( child, "particleEmitters" );
	CaptureMesh( child, node );
	CaptureDetails( child, node.id, node.details, 0 );
	const size_t nodeIndex = nodes.size();
	nodes.push_back( node );

	IListPtr objects = GetListAttribute( child, "objects" );
	if( objects )
	{
		std::map<std::string, size_t> ordinals;
		for( ssize_t index = 0; index < objects->GetSize(); ++index )
		{
			IEveSpaceObjectChildPtr nested = BlueCastPtr( objects->GetAt( index ) );
			if( !nested )
			{
				continue;
			}
			const std::string name = nested->GetName() ? nested->GetName() : ClassName( nested );
			const size_t ordinal = ordinals[name]++;
			CaptureNodeRecursive(
				nested, nodes[nodeIndex].id, nodes[nodeIndex].qualitySelected, ordinal, nodes );
		}
	}
	IEveSpaceObjectChildPtr referenced = BlueCastPtr( GetRootAttribute( child, "child" ) );
	if( referenced )
	{
		CaptureNodeRecursive( referenced, nodes[nodeIndex].id, nodes[nodeIndex].qualitySelected, 0, nodes );
	}
}

std::vector<AuditNode> CaptureGraph( EvePlanet& sun )
{
	std::vector<AuditNode> nodes;
	std::map<std::string, size_t> ordinals;
	for( IEveSpaceObjectChild* child : sun.GetChildren() )
	{
		const std::string name = child && child->GetName() ? child->GetName() : ClassName( child );
		CaptureNodeRecursive( child, "/Sun", true, ordinals[name]++, nodes );
	}
	for( auto nodeIt = nodes.rbegin(); nodeIt != nodes.rend(); ++nodeIt )
	{
		AuditNode& node = *nodeIt;
		if( !node.qualitySelected || node.displayFilter != 5 || node.meshAreaCount != 0 ||
			node.particleSystemCount != 0 || node.emitterCount != 0 )
		{
			continue;
		}
		bool hasChild = false;
		bool hasSelectedChild = false;
		for( const AuditNode& candidate : nodes )
		{
			if( candidate.parentId == node.id )
			{
				hasChild = true;
				hasSelectedChild = hasSelectedChild || candidate.qualitySelected;
			}
		}
		if( hasChild && !hasSelectedChild )
		{
			node.qualitySelected = false;
		}
	}
	return nodes;
}

void CaptureRuntimeRecursive(
	IEveSpaceObjectChild* child,
	const std::string& parentId,
	size_t sameNameOrdinal,
	std::set<std::string>& ids,
	std::map<std::string, RuntimeNode>* runtime )
{
	if( !child )
	{
		return;
	}
	const std::string name = child->GetName() ? child->GetName() : "";
	const std::string segment = StableSegment( name.empty() ? ClassName( child ) : name );
	const std::string id = parentId + "/" + segment + "[" + std::to_string( sameNameOrdinal ) + "]";
	ids.insert( id );
	if( runtime )
	{
		RuntimeNode& value = ( *runtime )[id];
		std::vector<ITr2Renderable*> renderables;
		child->GetRenderables( renderables );
		value.renderableCount = renderables.size();
		if( IListPtr particleSystems = GetListAttribute( child, "particleSystems" ) )
		{
			for( ssize_t index = 0; index < particleSystems->GetSize(); ++index )
			{
				IRoot* particleSystem = particleSystems->GetAt( index );
				int64_t count = 0;
				if( ReadLong( particleSystem, "aliveCount", count ) )
				{
					value.aliveParticleCount += static_cast<uint64_t>( std::max<int64_t>( 0, count ) );
				}
				if( ReadLong( particleSystem, "peakAliveCount", count ) )
				{
					value.peakParticleCount += static_cast<uint64_t>( std::max<int64_t>( 0, count ) );
				}
			}
		}
		if( IListPtr emitters = GetListAttribute( child, "particleEmitters" ) )
		{
			for( ssize_t index = 0; index < emitters->GetSize(); ++index )
			{
				Tr2DynamicEmitterPtr emitter = BlueCastPtr( emitters->GetAt( index ) );
				if( emitter )
				{
					value.emittedParticleCount += emitter->GetEmittedParticleCount();
				}
			}
		}
	}

	if( IListPtr objects = GetListAttribute( child, "objects" ) )
	{
		std::map<std::string, size_t> ordinals;
		for( ssize_t index = 0; index < objects->GetSize(); ++index )
		{
			IEveSpaceObjectChildPtr nested = BlueCastPtr( objects->GetAt( index ) );
			if( !nested )
			{
				continue;
			}
			const std::string nestedName = nested->GetName() ? nested->GetName() : ClassName( nested );
			CaptureRuntimeRecursive( nested, id, ordinals[nestedName]++, ids, runtime );
		}
	}
}

void CaptureRuntime(
	EvePlanet& sun,
	std::set<std::string>& ids,
	std::map<std::string, RuntimeNode>* runtime )
{
	std::map<std::string, size_t> ordinals;
	for( IEveSpaceObjectChild* child : sun.GetChildren() )
	{
		const std::string name = child && child->GetName() ? child->GetName() : ClassName( child );
		CaptureRuntimeRecursive( child, "/Sun", ordinals[name]++, ids, runtime );
	}
}

void WriteStringArray( std::ostream& output, const std::vector<std::string>& values )
{
	output << '[';
	for( size_t index = 0; index < values.size(); ++index )
	{
		if( index )
		{
			output << ',';
		}
		output << JsonString( values[index] );
	}
	output << ']';
}

void WriteAttributes( std::ostream& output, const std::map<std::string, AuditValue>& attributes )
{
	output << '{';
	size_t index = 0;
	for( const auto& attribute : attributes )
	{
		if( index++ )
		{
			output << ',';
		}
		output << JsonString( attribute.first ) << ':' << attribute.second.json;
	}
	output << '}';
}
}

struct TrinityStandaloneSolarAudit::Impl
{
	std::vector<AuditNode> sourceNodes;
	std::set<std::string> preparedIds;
	bool sourceCaptured = false;
	bool preparedCaptured = false;
};

TrinityStandaloneSolarAudit::TrinityStandaloneSolarAudit() :
	m_impl( std::make_unique<Impl>() )
{
}

TrinityStandaloneSolarAudit::~TrinityStandaloneSolarAudit() = default;

bool TrinityStandaloneSolarAudit::CaptureSourceGraph( EvePlanet& sun, std::string& error )
{
	m_impl->sourceNodes = CaptureGraph( sun );
	m_impl->sourceCaptured = !m_impl->sourceNodes.empty();
	if( !m_impl->sourceCaptured )
	{
		error = "Solar audit found no authored Sun children";
		return false;
	}
	return true;
}

void TrinityStandaloneSolarAudit::CapturePreparedGraph( EvePlanet& sun )
{
	m_impl->preparedIds.clear();
	CaptureRuntime( sun, m_impl->preparedIds, nullptr );
	for( AuditNode& node : m_impl->sourceNodes )
	{
		node.retainedAfterPreparation = m_impl->preparedIds.count( node.id ) != 0;
	}
	m_impl->preparedCaptured = true;
}

bool TrinityStandaloneSolarAudit::WriteReport(
	const char* reportPath,
	EvePlanet* preparedSun,
	EveSpaceScene& scene,
	uint64_t renderedFrames,
	std::string& error )
{
	if( !reportPath || !reportPath[0] || !m_impl->sourceCaptured || !m_impl->preparedCaptured )
	{
		error = "Solar audit report requires source and prepared graph snapshots";
		return false;
	}
	std::set<std::string> settledIds;
	std::map<std::string, RuntimeNode> runtime;
	if( preparedSun )
	{
		CaptureRuntime( *preparedSun, settledIds, &runtime );
	}

	double fogStart = 0.0;
	double fogEnd = 0.0;
	double fogMax = 0.0;
	IRoot* sceneRoot = static_cast<ITr2Scene*>( &scene );
	ReadFloat( sceneRoot, "fogStart", fogStart );
	ReadFloat( sceneRoot, "fogEnd", fogEnd );
	ReadFloat( sceneRoot, "fogMax", fogMax );
	const auto fogColorEntry = FindEntry( sceneRoot, "fogColor" );
	const float* fogColor = fogColorEntry.second ? reinterpret_cast<const float*>( fogColorEntry.second ) : nullptr;
	Tr2PostProcess2Ptr postProcess = scene.GetPostProcess();
	Tr2PPFogEffectPtr postFog = postProcess ? postProcess->GetFogIfAvailable( PostProcess::HIGH ) : Tr2PPFogEffectPtr{};
	Tr2PPFogEffectPtr defaultPostFog = scene.m_sceneDefaultPostProcess ?
		scene.m_sceneDefaultPostProcess->GetFogIfAvailable( PostProcess::HIGH ) :
		Tr2PPFogEffectPtr{};
	const size_t froxelSettingsCount = scene.m_componentRegistry ?
		scene.m_componentRegistry->ComponentCount<ITr2FroxelFogSettings>() :
		0;
	const bool froxelActive = scene.m_volumetricsRenderer && scene.m_volumetricsRenderer->HasFog();

	std::ofstream output( reportPath );
	if( !output )
	{
		error = std::string( "Unable to create solar audit report: " ) + reportPath;
		return false;
	}
	output << std::setprecision( 9 );
	output << "{\n";
	output << "  \"schema\": \"trinity.solar-audit.v1\",\n";
	output << "  \"assetPolicy\": \"logical-identifiers-hashes-and-authored-values-only\",\n";
	output << "  \"source\": \"res:/dx9/model/celestial/sun/sun_yellow_small_01b.black\",\n";
	output << "  \"shaderModel\": " << JsonString( Tr2Renderer::GetShaderModelString( Tr2Renderer::GetShaderModel() ) ) << ",\n";
	output << "  \"renderedFrames\": " << renderedFrames << ",\n";
	output << "  \"graph\": [\n";
	for( size_t nodeIndex = 0; nodeIndex < m_impl->sourceNodes.size(); ++nodeIndex )
	{
		const AuditNode& node = m_impl->sourceNodes[nodeIndex];
		const auto runtimeIt = runtime.find( node.id );
		const RuntimeNode* settled = runtimeIt == runtime.end() ? nullptr : &runtimeIt->second;
		const bool currentProbeActive = settled &&
			( settled->renderableCount != 0 || settled->aliveParticleCount != 0 );
		const char* disposition = !node.qualitySelected ? "not-selected" :
														  ( !node.reachable ? "selected-unreachable" : "required-parity" );
		output << "    {\n";
		output << "      \"id\": " << JsonString( node.id ) << ",\n";
		output << "      \"parentId\": " << JsonString( node.parentId ) << ",\n";
		output << "      \"name\": " << JsonString( node.name ) << ",\n";
		output << "      \"class\": " << JsonString( node.className ) << ",\n";
		output << "      \"present\": true,\n";
		output << "      \"display\": " << ( node.display ? "true" : "false" ) << ",\n";
		output << "      \"displayFilter\": {\"value\": " << node.displayFilter << ", \"name\": "
			   << JsonString( DisplayFilterName( node.displayFilter ) ) << "},\n";
		output << "      \"qualitySelected\": " << ( node.qualitySelected ? "true" : "false" ) << ",\n";
		output << "      \"reachable\": " << ( node.reachable ? "true" : "false" ) << ",\n";
		output << "      \"retainedAfterPreparation\": " << ( node.retainedAfterPreparation ? "true" : "false" ) << ",\n";
		output << "      \"runtimeActiveInCurrentProbe\": " << ( currentProbeActive ? "true" : "false" ) << ",\n";
		output << "      \"clientRuntimeActive\": null,\n";
		output << "      \"visuallyContributing\": " << ( settled ? ( currentProbeActive ? "true" : "false" ) : "null" ) << ",\n";
		output << "      \"disposition\": " << JsonString( disposition ) << ",\n";
		output << "      \"counts\": {\"meshAreas\": " << node.meshAreaCount
			   << ", \"curveSets\": " << node.curveSetCount << ", \"controllers\": " << node.controllerCount
			   << ", \"particleSystems\": " << node.particleSystemCount << ", \"emitters\": " << node.emitterCount
			   << ", \"settledRenderables\": " << ( settled ? settled->renderableCount : 0 )
			   << ", \"settledAliveParticles\": " << ( settled ? settled->aliveParticleCount : 0 )
			   << ", \"settledPeakParticles\": " << ( settled ? settled->peakParticleCount : 0 )
			   << ", \"settledEmittedParticles\": " << ( settled ? settled->emittedParticleCount : 0 ) << "},\n";
		output << "      \"effectPaths\": ";
		WriteStringArray( output, node.effectPaths );
		output << ",\n      \"resourcePaths\": ";
		WriteStringArray( output, node.resourcePaths );
		output << ",\n      \"details\": [";
		for( size_t detailIndex = 0; detailIndex < node.details.size(); ++detailIndex )
		{
			const AuditDetail& detail = node.details[detailIndex];
			if( detailIndex )
			{
				output << ',';
			}
			output << "{\"id\":" << JsonString( detail.id ) << ",\"parentId\":"
				   << JsonString( detail.parentId ) << ",\"relation\":" << JsonString( detail.relation )
				   << ",\"class\":" << JsonString( detail.className ) << ",\"attributes\":";
			WriteAttributes( output, detail.attributes );
			output << '}';
		}
		output << "]\n    }" << ( nodeIndex + 1 == m_impl->sourceNodes.size() ? "\n" : ",\n" );
	}
	output << "  ],\n";
	output << "  \"fog\": {\n";
	output << "    \"sceneDistance\": {\"present\": true, \"fogColor\": ["
		   << ( fogColor ? fogColor[0] : 0.0f ) << ',' << ( fogColor ? fogColor[1] : 0.0f ) << ','
		   << ( fogColor ? fogColor[2] : 0.0f ) << ',' << ( fogColor ? fogColor[3] : 1.0f )
		   << "], \"fogStart\": " << fogStart << ", \"fogEnd\": " << fogEnd << ", \"fogMax\": " << fogMax
		   << ", \"runtimeActive\": " << ( fogMax > 0.0 ? "true" : "false" )
		   << ", \"owner\": \"EveSpaceScene\", \"consumer\": \"per-frame FogFactors/FogColor material constants\"},\n";
	output << "    \"postProcess\": {\"present\": " << ( postFog ? "true" : "false" )
		   << ", \"defaultOwnerPresent\": " << ( defaultPostFog ? "true" : "false" )
		   << ", \"runtimeActive\": " << ( postFog && postFog->IsActive() ? "true" : "false" )
		   << ", \"totalAmount\": " << ( postFog ? postFog->m_totalAmount : 0.0f )
		   << ", \"intensity\": " << ( postFog ? postFog->m_intensity : 0.0f )
		   << ", \"owner\": \"EveSpaceScene combined postprocess\"},\n";
	output << "    \"froxel\": {\"present\": " << ( froxelSettingsCount ? "true" : "false" )
		   << ", \"settingsCount\": " << froxelSettingsCount
		   << ", \"runtimeActive\": " << ( froxelActive ? "true" : "false" )
		   << ", \"authoredSystem\": false, \"owner\": \"EveComponentRegistry/Tr2VolumetricsRenderer\"}\n";
	output << "  },\n";
	output << "  \"reportComplete\": true\n";
	output << "}\n";
	if( !output.good() )
	{
		error = std::string( "Failed while writing solar audit report: " ) + reportPath;
		return false;
	}
	return true;
}
