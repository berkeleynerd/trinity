////////////////////////////////////////////////////////////
//
//    Created:   June 2020
//    Copyright: CCP 2020
//

#include "StdAfx.h"
#if __APPLE__

#include "Tr2MouseCursor.h"
#import <Cocoa/Cocoa.h>


bool Tr2MouseCursor::Create_MacOS( const char* bits, uint32_t width, uint32_t height, int hotspotX, int hotspotY )
{
    auto rep = [[NSBitmapImageRep alloc]
                initWithBitmapDataPlanes:nil
                pixelsWide:width
                pixelsHigh:height
                bitsPerSample:8
                samplesPerPixel:4
                hasAlpha:YES
                isPlanar:NO
                colorSpaceName:NSCalibratedRGBColorSpace
                bitmapFormat:NSBitmapFormatAlphaNonpremultiplied
                bytesPerRow:width * 4
                bitsPerPixel:32];
    memcpy( [rep bitmapData], bits, width * height * 4 );
    auto image = [[NSImage alloc] initWithSize:NSMakeSize( width, height )];
    [image addRepresentation:rep];
    
    m_cursor = [[NSCursor alloc] initWithImage:image hotSpot:NSMakePoint( CGFloat( hotspotX ), CGFloat( hotspotY ) )];
    
    return true;
}

void Tr2MouseCursor::Apply_MacOS()
{
    [(NSCursor*)m_cursor set];
}

#endif