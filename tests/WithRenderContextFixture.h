#pragma once
#ifndef WithRenderContextFixture_H
#define WithRenderContextFixture_H

#include "WithWindowFixture.h"

struct WithRenderContext: public WithWindow
{
public:
	void SetUp()
	{
		renderContext = new Tr2PrimaryRenderContextAL();
		Tr2PrimaryRenderContextAL::SetPrimaryRenderContext( renderContext );
	}

	void TearDown()
	{
		delete renderContext;
		Tr2PrimaryRenderContextAL::SetPrimaryRenderContext( nullptr );
	}

	Tr2PrimaryRenderContextAL* renderContext;
};

#endif