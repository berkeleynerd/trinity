#pragma once

#ifndef Tr2GrannyStateRes_h
#define Tr2GrannyStateRes_h

#include "gstate.h"

BLUE_DECLARE( Tr2GrannyStateRes );

BLUE_CLASS( Tr2GrannyStateRes ):
	public BlueAsyncRes,
	public ICacheable
{
public:
	EXPOSE_TO_BLUE();
	Tr2GrannyStateRes(IRoot* lockobj = NULL);
	~Tr2GrannyStateRes();

	//////////////////////////////////////////////////////////////////////////
	// ICacheable
	bool IsMemoryUsageKnown();
	size_t GetMemoryUsage();

	//
	//////////////////////////////////////////////////////////////////////////

	granny_file* GetCharacterFile() const { return m_characterFile; }
	gstate_character_info *GetCharacterInfo();

protected:
	virtual LoadingResult DoLoad();
	bool DoPrepare();

private:
	size_t m_dataSize;
	size_t m_memoryUsage;
	void* m_data;
	granny_file *m_characterFile;
};

TYPEDEF_BLUECLASS_WR_SHUTDOWN( Tr2GrannyStateRes );
#endif