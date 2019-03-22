#include "StdAfx.h"
#include "Tr2GrannyStateRes.h"
#include "Utilities/GeometryUtils.h"


Tr2GrannyStateRes::Tr2GrannyStateRes( IRoot* lockobj )
{
	m_characterFile = NULL;
	m_data = NULL;
}


Tr2GrannyStateRes::~Tr2GrannyStateRes()
{
	if (m_characterFile)
	{
		GrannyFreeFile(m_characterFile);
	}
}

bool Tr2GrannyStateRes::IsMemoryUsageKnown()
{
	return !IsLoading();
}

size_t Tr2GrannyStateRes::GetMemoryUsage()
{
	if (m_characterFile)
	{
		return m_dataSize + sizeof(m_characterFile);
	}
	else
	{
		return 1024;
	}
}


BlueAsyncRes::LoadingResult Tr2GrannyStateRes::DoLoad()
{
	CCP_STATS_ZONE(__FUNCTION__);

	if (!m_dataStream->LockData(&m_data, 0))
	{
		return LR_FAILED;
	}

	m_dataSize = m_dataStream->GetSize();

	{
		if (m_characterFile)
		{
			GrannyFreeFile(m_characterFile);
			m_characterFile = NULL;
		}

		CCP_STATS_ZONE(__FUNCTION__ " reading Granny file");
		m_characterFile = ProtectedGrannyReadEntireFileFromMemory(m_path.c_str(), (uint32_t)m_dataSize, m_data);
	}

	if (!m_characterFile)
	{
		return LR_FAILED;
	}

	return LR_SUCCESS;
}

bool Tr2GrannyStateRes::DoPrepare()
{
	return true;
}

gstate_character_info* Tr2GrannyStateRes::GetCharacterInfo()
{
	if (m_characterFile)
	{
		return GStateGetCharacterInfo(m_characterFile);
	}
	return NULL;
}

