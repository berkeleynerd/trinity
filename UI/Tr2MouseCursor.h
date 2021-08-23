////////////////////////////////////////////////////////////
//
//    Created:   June 2012
//    Copyright: CCP 2012
//

#pragma once
#ifndef Tr2MouseCursor_H
#define Tr2MouseCursor_H

BLUE_DECLARE( Tr2HostBitmap );
BLUE_DECLARE( Tr2MouseCursor );
BLUE_DECLARE_VECTOR( Tr2MouseCursor );

// --------------------------------------------------------------------------------------
// Description:
//   Tr2MouseCursor encapsulates mouse cursor image management. There is a considerable
//   difference in how mouse cursors are handled in DX9 and DX11/GL.
// --------------------------------------------------------------------------------------
class Tr2MouseCursor: public IRoot, public Tr2DeviceResource
{
public:
    EXPOSE_TO_BLUE();

	Tr2MouseCursor( IRoot* lockobj = nullptr );
	~Tr2MouseCursor();

	void py__init__( Tr2HostBitmap* bitmap, unsigned hotspotX, unsigned hotspotY );

	bool IsValid() const;
	bool Create( Tr2HostBitmap* bitmap, int hotspotX, int hotspotY );
	void Apply();

	void ReleaseResources( TriStorage s );
private:
	virtual bool OnPrepareResources();

#if TRINITY_PLATFORM == TRINITY_DIRECTX9
	// Cursor as a surface
	CComPtr<IDirect3DSurface9> m_cursor;
	// Hotspot coordinates
	int m_hotspotX;
	int m_hotspotY;
#elif _WIN32
	// WinApi cursor (for DX11/GL)
	HCURSOR m_cursor;
#elif __APPLE__
    bool Create_MacOS( const char* bits, uint32_t width, uint32_t height, int hotspotX, int hotspotY );
    void Apply_MacOS();
    
    id m_cursor;  // NSCursor
#endif
};

TYPEDEF_BLUECLASS( Tr2MouseCursor );

#endif // Tr2MouseCursor_H
