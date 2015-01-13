////////////////////////////////////////////////////////////////////////////////
//
// Created:		January 2015
// Copyright:	CCP 2015
//

#pragma once
#ifndef Tr2SecondaryWindow_h
#define Tr2SecondaryWindow_h

#ifdef _WIN32

BLUE_CLASS( Tr2SecondaryWindow ) : public IRoot
{
public:
	EXPOSE_TO_BLUE();

	Tr2SecondaryWindow( IRoot* lockobj = nullptr );
	~Tr2SecondaryWindow();

	void Create();
	void Close();

	std::wstring GetTitle();
	void SetTitle( const std::wstring& title );

	static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l);

protected:
	HWND m_handle;
	std::wstring m_windowTitle;
	uint32_t m_minWidth;
	uint32_t m_minHeight;

	// Python callable for handling Windows events
	BlueScriptCallback m_eventHandler;
};

TYPEDEF_BLUECLASS( Tr2SecondaryWindow );

#endif
#endif // Tr2SecondaryWindow_h