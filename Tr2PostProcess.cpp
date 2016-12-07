#include "StdAfx.h"
#include "Tr2PostProcess.h"
#include "Tr2VariableStore.h"

namespace {

	/////////////////////////////////////////////////////////////////////////////////////
	// ResourceHelper
	// This is currently used to register variable store variables used by eve post processing.
	class ResourceHelper
	{
	public:
		ResourceHelper();
		~ResourceHelper();
	};

	ResourceHelper s_resourceHelper;
}

ResourceHelper::ResourceHelper()
{
	// register these variables once here in static's constructor, since they are needed throughout the whole game
	GlobalStore().RegisterVariable( "BlitOriginal", static_cast<ITr2TextureProvider*>( nullptr ) );
	GlobalStore().RegisterVariable( "BlitCurrent", static_cast<ITr2TextureProvider*>( nullptr ) );
	GlobalStore().RegisterVariable( "g_texelSize", Vector4() );
}

ResourceHelper::~ResourceHelper()
{
	GlobalStore().UnregisterVariable( "BlitOriginal" );
	GlobalStore().UnregisterVariable( "BlitCurrent" );
	GlobalStore().UnregisterVariable( "g_texelSize" );
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Tr2PostProcess
Tr2PostProcess::Tr2PostProcess( IRoot* lockobj ) :
	PARENTLOCK( m_stages )
{
}

Tr2PostProcess::~Tr2PostProcess()
{
}

bool Tr2PostProcess::Initialize()
{
    return true;
}
