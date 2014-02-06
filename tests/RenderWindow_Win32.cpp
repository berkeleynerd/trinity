#include "StdAfx.h"
#ifdef _WIN32
#include "RenderWindow.h"

RenderWindow::RenderWindow( uint32_t width, uint32_t height )
{
	static WNDCLASS wndClass;
	static bool wndClassInitialized = false;
	if( !wndClassInitialized )
	{
		wndClass.style         = CS_HREDRAW | CS_VREDRAW;
		wndClass.lpfnWndProc   = DefWindowProc;
		wndClass.cbClsExtra    = 0;
		wndClass.cbWndExtra    = 0;
		wndClass.hInstance     = GetModuleHandle( nullptr );
		wndClass.hIcon         = nullptr;
		wndClass.hCursor       = nullptr;
		wndClass.hbrBackground = nullptr;
		wndClass.lpszMenuName  = nullptr;
		wndClass.lpszClassName = g_moduleName;
     
		RegisterClass( &wndClass );
		wndClassInitialized = true;
	}
	m_handle = ::CreateWindow(
		wndClass.lpszClassName,
		g_moduleName,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 
		CW_USEDEFAULT,
		width, 
		height,
		0L,
		NULL,
		wndClass.hInstance,
		0L);
	::ShowWindow( m_handle, SW_SHOW );
}

RenderWindow::~RenderWindow()
{
	::DestroyWindow( m_handle );
}

uint32_t RenderWindow::GetClientWidth() const
{
	RECT rect = { 0, 0, 0, 0 };
	::GetClientRect( m_handle, &rect );
	return rect.right - rect.left;
}

uint32_t RenderWindow::GetClientHeight() const
{
	RECT rect = { 0, 0, 0, 0 };
	::GetClientRect( m_handle, &rect );
	return rect.bottom - rect.top;
}
#endif