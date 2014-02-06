#pragma once
#ifndef WithValidRenderContextFixture_H
#define WithValidRenderContextFixture_H

#include "WithWindowFixture.h"

struct WithValidRenderContext: public WithWindow
{
public:
	WithValidRenderContext();

	static void SetUpTestCase();
	static void TearDownTestCase();

	static void MakeScreenShot( const char* outFilePath );
	void MakeTestScreenShot();

	static void SaveImageSetReport();

	static Tr2PresentParametersAL presentParameters;
	static Tr2PrimaryRenderContextAL* renderContext;
private:
	bool m_madeScreenshot;
};

#endif