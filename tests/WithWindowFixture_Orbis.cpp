#include "StdAfx.h"
#ifdef __ORBIS__

#include "WithWindowFixture.h"
#include <user_service.h>
#include <pad.h>

namespace
{

int32_t s_handle = 0;
uint32_t s_prevButtons = 0;

}

void WithWindow::SetUpTestCase() 
{
	sceUserServiceInitialize( nullptr );
	SceUserServiceUserId initialUserId;
	sceUserServiceGetInitialUser( &initialUserId );
	scePadInit();
	s_handle = scePadOpen( initialUserId, SCE_PAD_PORT_TYPE_STANDARD, 0, nullptr );
	s_prevButtons = 0;
}

void WithWindow::TearDownTestCase() 
{
	scePadClose( s_handle );
	sceUserServiceTerminate();
}

void WithWindow::BeginLoopProcessing()
{
	s_prevButtons = 0;
}

bool WithWindow::DoLoopProcessing()
{
	ScePadData data;
	scePadReadState( s_handle, &data );

	if( s_prevButtons & ( ~data.buttons ) )
	{
		return false;
	}

	s_prevButtons = data.buttons;

	return true;
}

Tr2WindowHandle WithWindow::GetWindowHandle()
{
	return 0;
}

#endif