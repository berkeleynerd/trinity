////////////////////////////////////////////////////////////////////////////////
//
// Created:		January 2015
// Copyright:	CCP 2015
//

#include "StdAfx.h"

#ifdef _WIN32

#include "Tr2SecondaryWindow.h"

BLUE_DEFINE( Tr2SecondaryWindow );

const Be::ClassInfo* Tr2SecondaryWindow::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2SecondaryWindow, "Secondary window" )
		MAP_ATTRIBUTE
		(
			"handle",
			m_handle,
			"Window handle - set by calling Create()",
			Be::READ
		)

		MAP_ATTRIBUTE
		(
			"eventHandler",
			m_eventHandler,
			"Python callable for handling Windows events for this window",
			Be::READWRITE
		)

		MAP_PROPERTY
		(
			"title",
			GetTitle, SetTitle,
			"The title bar text for this window"
		)

		MAP_ATTRIBUTE
		(
			"minWidth",
			m_minWidth,
			"Minimum width of this window",
			Be::READWRITE
		)

		MAP_ATTRIBUTE
		(
			"minHeight",
			m_minHeight,
			"Minimum height of this window",
			Be::READWRITE
		)

		MAP_METHOD_AND_WRAP
		(
			"Create",
			Create,
			"Create the window"
		)

		MAP_METHOD_AND_WRAP
		(
			"Close",
			Close,
			"Close the window"
		)
	EXPOSURE_END()
}

#endif
