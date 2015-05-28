#include "StdAfx.h"
#include "TriTransformParameter.h"
#include "Utilities/Matrix.h"
#include "Tr2Renderer.h"
#include "include/TriMath.h"

TriTransformParameter::TriTransformParameter(IRoot* lockobj):
	m_scaling( 1.0f, 1.0f, 1.0f ),
	m_rotationCenter( 0.0f, 0.0f, 0.0f ),
	m_rotation( 0.0f, 0.0f, 0.0f, 1.0f ),
	m_translation( 0.0f, 0.0f, 0.0f ),
	m_transformBase( TRITB_FIXED ),
	m_worldTransform( Tr2Renderer::GetIdentityTransform() )
{
}

size_t TriTransformParameter::GetValueSize() const
{
	return sizeof(Matrix);
}

unsigned TriTransformParameter::GetHashValue( unsigned startingHash ) const
{
	auto name = m_name.c_str();
	return CcpHashFNV1( &name, sizeof( name ), 
		CcpHashFNV1( &m_transformBase, sizeof( TRITRANSFORMBASE ) + 3 * sizeof( Vector3 ) + sizeof( Quaternion ) + sizeof( Matrix ), startingHash ) );
}

// --------------------------------------------------------------------------------------
// Description:
//   Determines whether the determinant of the matrix underlying this transform parameter 
//   is within epsilon of 0 and can be ignored when building the material situation.
// Return Value:
//   true, if the determinant of the matrix is within epsilon of 0
//   false, otherwise
// --------------------------------------------------------------------------------------
bool TriTransformParameter::IsZeroOrNull( void ) const
{
	static const float epsilon = 0.0000001f;

	return XMVectorGetX( 
		XMMatrixDeterminant( 
			XMMatrixTransformation( 
				XMVectorSet( 0.0f, 0.0f, 0.0f, 1.0f ),
				XMVectorSet( 0.0f, 0.0f, 0.0f, 1.0f ),
				m_scaling,
				m_rotationCenter,
				m_rotation,
				m_translation
			)
		)
	) < epsilon;
}

void TriTransformParameter::CopyValueToEffect(	Tr2RenderContextEnum::ShaderType inputType, 
												unsigned char* dest, 
												size_t size,
												Tr2RenderContext &renderContext ) const
{
	// Calculate transform - this code is based on TriTexture and emulates old Trinity:	
	Matrix original;
	D3DXMatrixTransformation(
		&original,
		NULL,
		NULL,
		&m_scaling,
		&m_rotationCenter,
		&m_rotation,
		&m_translation
		);

	Matrix texTransform;
	if( m_transformBase == TRITB_FIXED )
	{
		texTransform = original;
	}
	else
	{
		texTransform = Tr2Renderer::GetInverseViewTransform();
		texTransform._41 = texTransform._42 = texTransform._43 = 0.0f;			

		if( m_transformBase == TRITB_OBJECT )
		{
			// this is maybe a bit un orthodox. What is done here is that the inverse
			// of the rotation part of the view matrix is taken to make the 
			// texture move against the camera (this is neccesary to project 
			// cube maps correctly, strange why this is needed to be this explicit should
			// be the default behariour, well). Then the rotation part of the object
			// is also taken to make the texture rotatio with the object. Translation
			// is not used because it is difficould to having make sense (only scenario I can 
			// think of is some kind of cube spot lights, or something)
			// Later it turned out that this could be used to perfectly bumpmap
			// rotating asteroids.
			Matrix mat;
			if( !D3DXMatrixInverse( &mat, NULL, &m_worldTransform ) )
			{
				D3DXMatrixIdentity(&mat);
			}
			mat._41 = mat._42 = mat._43 = 0.0f;			
			D3DXMatrixMultiply( &texTransform, &texTransform, &mat );
		}
		else if(  (m_transformBase == TRITB_CAMERA_ROTATION)  
				||(m_transformBase == TRITB_CAMERA_ROTATION_ALIGNED) )
		{
			// Note that this has to be from the scenes camera as the UI will not get
			// rotation effects if the view matrix is used.
			texTransform._41 = texTransform._42 = texTransform._43 = 0.0f;			
		}

		texTransform *= original;
	}

	// column_major for shaders, copy it over, but pay attentiob to the size of the registers
	TriMatrixTranspose( (Matrix*)dest, &texTransform, (unsigned int)( size < sizeof( texTransform ) ? size : sizeof( texTransform ) ) );
}