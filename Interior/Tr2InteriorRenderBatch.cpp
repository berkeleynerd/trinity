#include "StdAfx.h"

#include "Tr2InteriorRenderBatch.h"
#include "Resources/TriGeometryRes.h"


// -----------------------------------------------------------------------------------------------------
// Description
//   Draws the cubemap using a screenspace quad.
// -----------------------------------------------------------------------------------------------------
void Tr2InteriorBackgroundCubemapBatch::SubmitGeometry( Tr2RenderContext& renderContext )
{
	using namespace Tr2RenderContextEnum;
	
	// TODO, Replace this with new shaders 
	uint32_t oldCull;
	renderContext.GetRenderState( RS_CULLMODE, &oldCull );
	renderContext.SetRenderState( RS_CULLMODE, CULLMODE_NONE );
	Tr2Renderer::DrawCameraSpaceScreenQuad( this->GetShaderStateInterface() , this->GetShaderMaterialInterface() );
	renderContext.SetRenderState( RS_ZENABLE, TRUE );
	renderContext.SetRenderState( RS_ZWRITEENABLE, TRUE );
	renderContext.SetRenderState( RS_CULLMODE, oldCull );

}

