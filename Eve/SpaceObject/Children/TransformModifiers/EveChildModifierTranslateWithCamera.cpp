////////////////////////////////////////////////////////////
//
//    Created:   2018
//    Copyright: CCP 2018
//
#include "StdAfx.h"
#include "EveChildModifierTranslateWithCamera.h"
#include "Tr2Renderer.h"

EveChildModifierTranslateWithCamera::EveChildModifierTranslateWithCamera( IRoot* lockobj )
{
}

EveChildModifierTranslateWithCamera::~EveChildModifierTranslateWithCamera()
{
}

Matrix EveChildModifierTranslateWithCamera::ApplyTransform( const Matrix& transform, size_t, const granny_matrix_3x4* ) const
{
	Matrix result = transform;
	result.GetTranslation() += Tr2Renderer::GetViewPosition();
	return result;
}