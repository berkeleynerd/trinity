////////////////////////////////////////////////////////////
//
//    Created:   May 2015
//    Copyright: CCP 2015
//
#include "StdAfx.h"
#include "EveSOFUtils.h"

#include "Utilities/StringUtils.h"

// --------------------------------------------------------------------------------
// Description:
//   Initialize data members and do all initial analyzing
// --------------------------------------------------------------------------------
EveSOFUtilsParameterName::EveSOFUtilsParameterName( const std::vector<std::string>& prefixes, const char* parameterName ) : m_fullname( parameterName ), m_shortname( parameterName ), m_materialIdx( -1 )
{
	// try to find the material prefix and with that indentify the index
	for( size_t i = 0; i < prefixes.size(); ++i )
	{
		if( StringStartsWithI( m_shortname.c_str(), prefixes[i].c_str() ) )
		{
			if(i < prefixes.size() )
			{
				m_materialIdx = int32_t(i);
				StringRemove( m_shortname, prefixes[i].c_str() );
				return;
			}
		}
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Is this a valid parameter name?
// --------------------------------------------------------------------------------
bool EveSOFUtilsParameterName::IsValid() const
{
	return m_materialIdx != -1;
}

// --------------------------------------------------------------------------------
// Description:
//   Return the material index
// --------------------------------------------------------------------------------
int32_t EveSOFUtilsParameterName::GetMaterialIdx() const
{
	return m_materialIdx;
}

// --------------------------------------------------------------------------------
// Description:
//   Return the parameter name without the material prefix
// --------------------------------------------------------------------------------
const char* EveSOFUtilsParameterName::GetShortName() const
{
	return m_shortname.c_str();
}

// --------------------------------------------------------------------------------
// Description:
//   Return a changed parameter with a new material index
// --------------------------------------------------------------------------------
std::string EveSOFUtilsParameterName::ChangeMaterialIdx( const EveSOFDataMgr::GenericData* genericData, int32_t idx ) const
{
	// insert the prefix based on the index
	std::string result = m_shortname;
	result.insert( 0, genericData->materialPrefixes[ idx ] );
	return result;
}

// --------------------------------------------------------------------------------
// Description:
//   Fill structs from other structs
// --------------------------------------------------------------------------------
void EveSOFUtils::GeneratePatternProjectionData( EveSOFDataMgr::PatternProjectionData& ppd, const EveSOFDataPatternTransform* patternTransform )
{
	ppd.enabled = true;
	ppd.position = patternTransform->m_position;
	ppd.scaling = patternTransform->m_scaling;
	ppd.rotation = patternTransform->m_rotation;
	ppd.isMirrored = patternTransform->m_isMirrored;
}

// --------------------------------------------------------------------------------
// Description:
//   Translate  SOF pattern projection enum into AL redner context enum
// --------------------------------------------------------------------------------
Tr2RenderContextEnum::TextureAddressMode EveSOFUtils::GetTextureAddressMode( EveSOFDataPatternLayer::ProjectionType projectionType )
{
	switch( projectionType )
	{
	case EveSOFDataPatternLayer::PROJECTION_BORDER:
		return Tr2RenderContextEnum::TA_BORDER;
	case EveSOFDataPatternLayer::PROJECTION_CLAMP:
		return Tr2RenderContextEnum::TA_CLAMP;
	default:
		return Tr2RenderContextEnum::TA_WRAP;
	}
}
