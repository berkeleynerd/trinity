////////////////////////////////////////////////////////////////////////////////
//
// Created:		January 2015
// Copyright:	CCP 2015
//

#include "StdAfx.h"

#ifdef _WIN32

#include "Tr2SecondaryWindow.h"
#include "App.h"
#include "WindowIcon.h"
#include <unordered_map>

#if BLUE_WITH_PYTHON
#include "CcpUtils/PyCpp.h"
#endif

extern HINSTANCE gInstance;

namespace
{
	static ATOM s_windowClass = NULL;

	ATOM RegisterWindowClass()
	{
		if( s_windowClass )
		{
			return s_windowClass;
		}

		HICON hIcon = GetWindowIcon();

		WNDCLASSEXW wclass = {sizeof(wclass)};
		wclass.lpfnWndProc = Tr2SecondaryWindow::WndProc;
		wclass.hInstance = gInstance;
		wclass.lpszClassName = L"Tr2SecondaryWindow";
		wclass.hIcon = hIcon;
		s_windowClass = RegisterClassExW(&wclass);
		return s_windowClass;
	}
}

Tr2SecondaryWindow::Tr2SecondaryWindow( IRoot* lockobj /*= nullptr */ ) :
	m_handle( NULL ),
	m_windowTitle( L"EVE Secondary Window" ),
	m_minWidth( 400 ),
	m_minHeight( 300 )
{
}

Tr2SecondaryWindow::~Tr2SecondaryWindow()
{
	Close();
}

void Tr2SecondaryWindow::Create()
{
	DWORD windowStyle = WS_POPUP | WS_SYSMENU | WS_VISIBLE | WS_MINIMIZEBOX | WS_CAPTION | WS_THICKFRAME | WS_MAXIMIZEBOX;

	m_handle = ::CreateWindowW(
		(LPCWSTR)RegisterWindowClass(),
		m_windowTitle.c_str(),
		windowStyle,
		0, 0,
		m_minWidth, m_minHeight,
		g_app->GetWindowHandle(),
		NULL,
		gInstance,
		0L);

	SetWindowLongPtr( m_handle, GWLP_USERDATA, reinterpret_cast<LONG>( this ) );
}

LRESULT CALLBACK Tr2SecondaryWindow::WndProc( HWND hwnd, UINT msg, WPARAM w, LPARAM l )
{
	auto obj = reinterpret_cast<Tr2SecondaryWindow*>( GetWindowLongPtr( hwnd, GWLP_USERDATA ) );
	
	switch( msg )
	{
	case WM_CLOSE:
		if( obj )
		{
			obj->Close();
			return 0;
		}
		break;

	case WM_GETMINMAXINFO:
		if( obj )
		{
			MINMAXINFO* minMaxInfo = reinterpret_cast<MINMAXINFO*>( l );
			minMaxInfo->ptMinTrackSize.x = obj->m_minWidth;
			minMaxInfo->ptMinTrackSize.y = obj->m_minHeight;

			return 0;
		}
		break;

	}

	if( obj && obj->m_eventHandler )
	{
#if BLUE_WITH_PYTHON
		Ccp::PyGilEnsure gilWrapper;
#endif
		bool handled = false;
		obj->m_eventHandler.Call( handled, msg, w, l );
		if( handled )
		{
			return 0;
		}
	}
	
	return DefWindowProcW( hwnd, msg, w, l );
}

void Tr2SecondaryWindow::Close()
{
	if( m_handle )
	{
		DestroyWindow( m_handle );
		m_handle = NULL;
	}
}

std::wstring Tr2SecondaryWindow::GetTitle()
{
	return m_windowTitle;
}

void Tr2SecondaryWindow::SetTitle( const std::wstring& title )
{
	m_windowTitle = title;
	SetWindowTextW( m_handle, m_windowTitle.c_str() );
}

#endif
