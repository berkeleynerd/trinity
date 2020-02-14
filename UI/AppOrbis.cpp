////////////////////////////////////////////////////////////////////////////////
//
// Created:		September 2013
// Copyright:	CCP 2013
//

#include "StdAfx.h"
#include "App.h"

#if defined(__ANDROID__)

uint32_t App::GetKeyState( uint32_t vKeyCode )
{
	return 0;
}

bool App::GetKeyName( uint32_t vKeyCode, char* buffer, size_t bufferSize )
{
	return false;
}

int App::GetVirtualScreenWidth() const
{
	return 1920;
}

int App::GetVirtualScreenHeight() const
{
	return 1080;
}

#endif