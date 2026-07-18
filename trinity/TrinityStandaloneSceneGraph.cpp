// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "TrinityStandaloneSceneGraph.h"

#include "BlueExposure.h"
#include "BlueSharedString.h"
#include "BlueWeakRef.h"
#include "IBlueDict.h"
#include "IList.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <map>
#include <queue>
#include <set>
#include <sstream>

namespace
{

uint64_t HashBytes( uint64_t hash, const void* bytes, size_t size )
{
	const auto* source = static_cast<const uint8_t*>( bytes );
	for( size_t index = 0; index < size; ++index )
	{
		hash ^= source[index];
		hash *= 1099511628211ull;
	}
	return hash;
}

std::string JsonString( const std::string& value )
{
	std::ostringstream output;
	output << '"';
	for( unsigned char character : value )
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
				output << static_cast<char>( character );
			}
		}
	}
	output << '"';
	return output.str();
}

std::string Narrow( const wchar_t* value )
{
	if( !value )
	{
		return {};
	}
	std::string result;
	for( ; *value; ++value )
	{
		const uint32_t character = static_cast<uint32_t>( *value );
		result.push_back( character >= 0x20 && character < 0x7f ?
							  static_cast<char>( character ) :
							  '?' );
	}
	return result;
}

const char* TypeName( Be::VARTYPE type )
{
	switch( type )
	{
	case Be::LONG:
		return "long";
	case Be::ULONG:
		return "ulong";
	case Be::FLOAT:
		return "float";
	case Be::DOUBLE:
		return "double";
	case Be::BOOL:
		return "bool";
	case Be::IROOT:
		return "object";
	case Be::IROOTPTR:
		return "object-pointer";
	case Be::IROOTWEAKREF:
		return "weak-object";
	case Be::CHARARRAY:
		return "char-array";
	case Be::CSTRING:
		return "string";
	case Be::REFERENCE:
		return "reference";
	case Be::WCSTRING:
		return "wide-string";
	case Be::WREFERENCE:
		return "wide-reference";
	case Be::FLOATARRAY:
		return "float-array";
	case Be::DOUBLEARRAY:
		return "double-array";
	case Be::INTARRAY:
		return "int-array";
	case Be::STDSTRING:
		return "std-string";
	case Be::STDWSTRING:
		return "std-wide-string";
	case Be::BYTE:
		return "byte";
	case Be::SHORT:
		return "short";
	case Be::SHAREDSTRING:
		return "shared-string";
	case Be::SHAREDSTRINGW:
		return "shared-wide-string";
	case Be::INT64:
		return "int64";
	case Be::UINT64:
		return "uint64";
	default:
		return "unsupported";
	}
}

std::string Number( double value )
{
	if( !std::isfinite( value ) )
	{
		return JsonString( std::isnan( value ) ? "nan" : value < 0.0 ? "-inf" :
																	   "inf" );
	}
	std::ostringstream output;
	output << std::setprecision( 17 ) << value;
	return output.str();
}

struct Field
{
	std::string name;
	std::string type;
	std::string value;
	long flags = 0;
};

struct Edge
{
	uint32_t from = 0;
	uint32_t to = 0;
	std::string member;
	int64_t index = -1;
	std::string key;
};

struct Node
{
	IRoot* object = nullptr;
	std::string id;
	std::string path;
	std::string className;
	std::vector<Field> fields;
};

struct Omission
{
	std::string path;
	std::string member;
	std::string reason;
	bool renderRelevant = false;
};

class Walker
{
public:
	explicit Walker( const std::vector<TrinityStandaloneSceneGraphSnapshot::Root>& roots )
	{
		for( const auto& root : roots )
		{
			if( root.object )
			{
				const uint32_t node = AddNode( root.object, root.role );
				m_roots.emplace_back( root.role, node );
			}
		}
	}

	TrinityStandaloneSceneGraphSnapshot Capture()
	{
		while( !m_pending.empty() )
		{
			const uint32_t index = m_pending.front();
			m_pending.pop();
			ReadNode( index );
		}

		std::ostringstream output;
		output << "{\"roots\":[";
		for( size_t index = 0; index < m_roots.size(); ++index )
		{
			output << ( index ? "," : "" ) << "{\"role\":"
				   << JsonString( m_roots[index].first ) << ",\"node\":"
				   << JsonString( m_nodes[m_roots[index].second].id ) << '}';
		}
		output << "],\"nodes\":[";
		for( size_t index = 0; index < m_nodes.size(); ++index )
		{
			const Node& node = m_nodes[index];
			output << ( index ? "," : "" ) << "{\"id\":" << JsonString( node.id )
				   << ",\"path\":" << JsonString( node.path )
				   << ",\"class\":" << JsonString( node.className ) << ",\"fields\":[";
			for( size_t fieldIndex = 0; fieldIndex < node.fields.size(); ++fieldIndex )
			{
				const Field& field = node.fields[fieldIndex];
				output << ( fieldIndex ? "," : "" ) << "{\"name\":"
					   << JsonString( field.name ) << ",\"type\":" << JsonString( field.type )
					   << ",\"flags\":" << field.flags << ",\"value\":" << field.value << '}';
			}
			output << "]}";
		}
		output << "],\"edges\":[";
		for( size_t index = 0; index < m_edges.size(); ++index )
		{
			const Edge& edge = m_edges[index];
			output << ( index ? "," : "" ) << "{\"from\":"
				   << JsonString( m_nodes[edge.from].id ) << ",\"to\":"
				   << JsonString( m_nodes[edge.to].id ) << ",\"member\":"
				   << JsonString( edge.member );
			if( edge.index >= 0 )
			{
				output << ",\"index\":" << edge.index;
			}
			if( !edge.key.empty() )
			{
				output << ",\"key\":" << JsonString( edge.key );
			}
			output << '}';
		}
		output << "],\"omissions\":[";
		for( size_t index = 0; index < m_omissions.size(); ++index )
		{
			const Omission& omission = m_omissions[index];
			output << ( index ? "," : "" ) << "{\"path\":" << JsonString( omission.path )
				   << ",\"member\":" << JsonString( omission.member )
				   << ",\"reason\":" << JsonString( omission.reason )
				   << ",\"renderRelevant\":" << ( omission.renderRelevant ? "true" : "false" ) << '}';
		}
		output << "]}";

		TrinityStandaloneSceneGraphSnapshot result;
		result.json = output.str();
		result.normalizedHash = HashBytes( 1469598103934665603ull, result.json.data(), result.json.size() );
		result.nodeCount = static_cast<uint32_t>( m_nodes.size() );
		result.edgeCount = static_cast<uint32_t>( m_edges.size() );
		result.omissionCount = static_cast<uint32_t>( m_omissions.size() );
		return result;
	}

private:
	uint32_t AddNode( IRoot* object, const std::string& path )
	{
		// Blue classes routinely inherit IRoot through several interfaces.
		// BeMapMemberOffset requires the canonical object base, not an arbitrary
		// interface subobject returned by a scene list or diagnostic root.
		object = object ? object->GetRootObject() : nullptr;
		if( !object )
		{
			return 0;
		}
		auto found = m_indices.find( object );
		if( found != m_indices.end() )
		{
			return found->second;
		}
		const uint32_t index = static_cast<uint32_t>( m_nodes.size() );
		Node node;
		node.object = object;
		std::ostringstream id;
		id << "object-" << std::setw( 6 ) << std::setfill( '0' ) << index;
		node.id = id.str();
		node.path = path;
		if( const Be::ClassInfo* type = object->ClassType() )
		{
			node.className = type->mClassId ? type->mClassId->GetName() : "unknown";
		}
		m_indices[object] = index;
		m_nodes.push_back( std::move( node ) );
		m_pending.push( index );
		return index;
	}

	void AddEdge( uint32_t from, IRoot* object, const std::string& member, int64_t index = -1, const std::string& key = {} )
	{
		if( !object )
		{
			return;
		}
		std::ostringstream path;
		path << m_nodes[from].path << '/' << member;
		if( index >= 0 )
		{
			path << '[' << index << ']';
		}
		if( !key.empty() )
		{
			path << '[' << key << ']';
		}
		const uint32_t to = AddNode( object, path.str() );
		m_edges.push_back( { from, to, member, index, key } );
	}

	void ReadObjectMember( uint32_t nodeIndex, const Be::VarEntry& entry, Be::Var* value )
	{
		IRoot* object = entry.mType == Be::IROOTPTR ? value->mIRootPtr : reinterpret_cast<IRoot*>( value );
		if( !object )
		{
			m_nodes[nodeIndex].fields.push_back( { entry.mName, TypeName( entry.mType ), "null", entry.mEditFlags } );
			return;
		}

		IListPtr list = BlueCastPtr( object );
		if( list )
		{
			m_nodes[nodeIndex].fields.push_back( { entry.mName, "list-size", std::to_string( list->GetSize() ), entry.mEditFlags } );
			for( ssize_t index = 0; index < list->GetSize(); ++index )
			{
				AddEdge( nodeIndex, list->GetAt( index ), entry.mName, index );
			}
			return;
		}

		IBlueDictPtr dictionary = BlueCastPtr( object );
		if( dictionary )
		{
			m_nodes[nodeIndex].fields.push_back( { entry.mName, "dict-size", std::to_string( dictionary->GetLength() ), entry.mEditFlags } );
			std::vector<std::string> keys;
			for( size_t index = 0; index < dictionary->GetLength(); ++index )
			{
				keys.emplace_back( dictionary->GetKey( index ) ? dictionary->GetKey( index ) : "" );
			}
			std::sort( keys.begin(), keys.end() );
			for( const std::string& key : keys )
			{
				AddEdge( nodeIndex, dictionary->Subscript( key.c_str() ), entry.mName, -1, key );
			}
			return;
		}

		AddEdge( nodeIndex, object, entry.mName );
	}

	void ReadNode( uint32_t nodeIndex )
	{
		IRoot* object = m_nodes[nodeIndex].object;
		const std::string path = m_nodes[nodeIndex].path;
		const Be::ClassInfo* type = object->ClassType();
		ssize_t parentOffset = 0;
		std::set<std::string> memberNames;
		for( ; type; parentOffset += type->mOffsetToParent, type = type->mParentClassInfo )
		{
			for( const Be::VarEntry* entry = type->mMemberTable; entry && entry->mName; ++entry )
			{
				if( !memberNames.insert( entry->mName ).second )
				{
					continue;
				}
				const bool readable = ( entry->mEditFlags & ( Be::READ | Be::PERSIST | Be::RPERSIST ) ) != 0;
				if( !readable )
				{
					continue;
				}
				if( entry->mGetProperty || entry->mType == Be::_PYPROPERTY )
				{
					m_omissions.push_back( { path, entry->mName, "property-getter", false } );
					continue;
				}
				Be::Var* value = BLUEMAPMEMBEROFFSET( object, entry, type, parentOffset );
				if( entry->mType == Be::IROOT || entry->mType == Be::IROOTPTR )
				{
					ReadObjectMember( nodeIndex, *entry, value );
					continue;
				}
				Field field{ entry->mName, TypeName( entry->mType ), "null", entry->mEditFlags };
				switch( entry->mType )
				{
				case Be::LONG:
					field.value = std::to_string( value->mLong );
					break;
				case Be::ULONG:
					field.value = std::to_string( value->mULong );
					break;
				case Be::BYTE:
					field.value = std::to_string( value->mByte );
					break;
				case Be::SHORT:
					field.value = std::to_string( value->mShort );
					break;
				case Be::INT64:
					field.value = std::to_string( value->mInt64 );
					break;
				case Be::UINT64:
					field.value = std::to_string( value->mUInt64 );
					break;
				case Be::FLOAT:
					field.value = Number( value->mFloat );
					break;
				case Be::DOUBLE:
					field.value = Number( value->mDouble );
					break;
				case Be::BOOL:
					field.value = value->mBool ? "true" : "false";
					break;
				case Be::CHARARRAY:
					field.value = JsonString( reinterpret_cast<const char*>( value ) );
					break;
				case Be::CSTRING:
				case Be::REFERENCE:
					field.value = value->mCharPtr ? JsonString( value->mCharPtr ) : "null";
					break;
				case Be::WCSTRING:
				case Be::WREFERENCE:
					field.value = value->mWCharPtr ? JsonString( Narrow( value->mWCharPtr ) ) : "null";
					break;
				case Be::STDSTRING:
					field.value = JsonString( *reinterpret_cast<const std::string*>( value ) );
					break;
				case Be::STDWSTRING:
					field.value = JsonString( Narrow( reinterpret_cast<const std::wstring*>( value )->c_str() ) );
					break;
				case Be::SHAREDSTRING:
					field.value = JsonString( reinterpret_cast<const BlueSharedString*>( value )->c_str() );
					break;
				case Be::SHAREDSTRINGW:
					field.value = JsonString( Narrow( reinterpret_cast<const BlueSharedStringW*>( value )->c_str() ) );
					break;
				case Be::FLOATARRAY:
				case Be::DOUBLEARRAY:
				case Be::INTARRAY: {
					const size_t elementSize = entry->mType == Be::DOUBLEARRAY ? sizeof( double ) : sizeof( float );
					const size_t count = entry->mType == Be::INTARRAY ? entry->GetIntArraySize() :
						entry->mType == Be::DOUBLEARRAY               ? entry->GetDoubleArraySize() :
																		entry->GetFloatArraySize();
					std::ostringstream array;
					array << '[';
					for( size_t index = 0; index < count; ++index )
					{
						array << ( index ? "," : "" );
						if( entry->mType == Be::FLOATARRAY )
							array << Number( reinterpret_cast<float*>( value )[index] );
						else if( entry->mType == Be::DOUBLEARRAY )
							array << Number( reinterpret_cast<double*>( value )[index] );
						else
							array << reinterpret_cast<int*>( value )[index];
					}
					array << ']';
					field.value = array.str();
					CCP_UNUSED( elementSize );
					break;
				}
				default:
					m_omissions.push_back( { path, entry->mName, "unsupported-type", false } );
					continue;
				}
				m_nodes[nodeIndex].fields.push_back( std::move( field ) );
			}
		}
	}

	std::vector<Node> m_nodes;
	std::vector<Edge> m_edges;
	std::vector<Omission> m_omissions;
	std::vector<std::pair<std::string, uint32_t>> m_roots;
	std::map<IRoot*, uint32_t> m_indices;
	std::queue<uint32_t> m_pending;
};

} // namespace

TrinityStandaloneSceneGraphSnapshot TrinityStandaloneCaptureBlueGraph(
	const std::vector<TrinityStandaloneSceneGraphSnapshot::Root>& roots )
{
	return Walker( roots ).Capture();
}
