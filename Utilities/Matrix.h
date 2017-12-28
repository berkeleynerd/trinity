#pragma once
#ifndef MATRIX_H
#define MATRIX_H

#ifdef _WIN32

#include "Vector3.h"
#include "Vector4.h"

// --------------------------------------------------------------------------------------
// Description:
//   This structure represents a 4x4 float matrix.  It inherits from D3DXMATRIX and is
//   convertible to XMMATRIX, for xnamath support.  It provides the standard set of
//   constructors, arithmetic, assignment, comparison, and conversion operators
// See Also:
//   Vector2, Vector3, Vector4, Quaternion
// --------------------------------------------------------------------------------------
struct Matrix : public D3DXMATRIX
{
	// ----------------------------------------------------------------------------------
	// Description
	//   Default constructor
	// ----------------------------------------------------------------------------------
	Matrix( void ) {}

	// ----------------------------------------------------------------------------------
	// Description
	//   Float-array constructor
	// Arguments:
	//   f - An array of 16 floats
	// ----------------------------------------------------------------------------------
	explicit Matrix( const float* f ) : D3DXMATRIX( f ) {}

	// ----------------------------------------------------------------------------------
	// Description
	//   Float component constructor (arguments omitted from docstring)
	// ----------------------------------------------------------------------------------
	Matrix( float _11, float _12, float _13, float _14,
		    float _21, float _22, float _23, float _24,
		    float _31, float _32, float _33, float _34,
			float _41, float _42, float _43, float _44 ) :
		D3DXMATRIX( _11, _12, _13, _14, 
		            _21, _22, _23, _24, 
					_31, _32, _33, _34, 
					_41, _42, _43, _44 ) {}

	// ----------------------------------------------------------------------------------
	// Description
	//   D3DMATRIX assignment constructor
	// Arguments:
	//   other - The D3DMATRIX to copy
	// ----------------------------------------------------------------------------------
	Matrix( const D3DMATRIX& other ) : D3DXMATRIX( other ){}
	
	// ----------------------------------------------------------------------------------
	// Description
	//   XMMATRIX assignment constructor
	// Arguments:
	//   other - The XMMATRIX to copy
	// ----------------------------------------------------------------------------------
	explicit Matrix( const XMMATRIX& other )
	{
		XMStoreFloat4x4( (XMFLOAT4X4*)this, other );
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Copy constructor
	// Arguments:
	//   other - The Matrix to copy
	// ----------------------------------------------------------------------------------
	Matrix( const Matrix& other ) :
		D3DXMATRIX( other._11, other._12, other._13, other._14,
			        other._21, other._22, other._23, other._24,
					other._31, other._32, other._33, other._34,
					other._41, other._42, other._43, other._44 ) {}

	// ----------------------------------------------------------------------------------
	// Description
	//   Access grant operator
	// Arguments:
	//   row - The row of the element to access
	//   col - The column of the element to access
	// Return Value:
	//   Reference to the element in position m[row][col]
	// ----------------------------------------------------------------------------------
	float& operator()( unsigned int row, unsigned int col ) 
	{ 
		return m[row][col]; 
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Const access grant operator
	// Arguments:
	//   row - The row of the element to access
	//   col - The column of the element to access
	// Return Value:
	//   Copy of the element in position m[row][col]
	// ----------------------------------------------------------------------------------
	float operator()( unsigned int row, unsigned int col ) const 
	{ 
		return m[row][col]; 
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Float-array conversion operator
	// Return Value:
	//   Pointer to the _11-component of the Matrix
	// ----------------------------------------------------------------------------------
	operator float*( void ) 
	{ 
		return &_11; 
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Const float-array conversion operator
	// Return Value:
	//   Pointer to the _11-component of the Matrix
	// ----------------------------------------------------------------------------------
	operator const float*( void ) const 
	{ 
		return &_11; 
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Const XMMATRIX conversion operator
	// Return Value:
	//   XMMATRIX constructed from the Matrix
	// ----------------------------------------------------------------------------------
	operator XMMATRIX() const 
	{ 
		return XMLoadFloat4x4( (const XMFLOAT4X4*)this );
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   XMMATRIX assignment operator
	// Arguments:
	//   other - the XMMATRIX to assign to this Matrix
	// Return Value:
	//   Reference to this Matrix
	// ----------------------------------------------------------------------------------
	const Matrix& operator=( const XMMATRIX& other )
	{
		XMStoreFloat4x4( (XMFLOAT4X4*)this, other );
		return *this;
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Addition-assignment operator
	// Arguments:
	//   other - The Matrix to add to this Matrix
	// Return Value:
	//   This Matrix, after adding the other Matrix
	// ----------------------------------------------------------------------------------
	Matrix& operator+=( const Matrix& other )
	{
		_11 += other._11;
		_12 += other._12;
		_13 += other._13;
		_14 += other._14;

		_21 += other._21;
		_22 += other._22;
		_23 += other._23;
		_24 += other._24;

		_31 += other._31;
		_32 += other._32;
		_33 += other._33;
		_34 += other._34;

		_41 += other._41;
		_42 += other._42;
		_43 += other._43;
		_44 += other._44;

		return *this;
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Subtraction-assignment operator
	// Arguments:
	//   other - The Matrix to subtract from this Matrix
	// Return Value:
	//   This Matrix, after subtracting off the other Matrix
	// ----------------------------------------------------------------------------------
	Matrix& operator-=( const Matrix& other )
	{
		_11 += other._11;
		_12 += other._12;
		_13 += other._13;
		_14 += other._14;

		_21 += other._21;
		_22 += other._22;
		_23 += other._23;
		_24 += other._24;

		_31 += other._31;
		_32 += other._32;
		_33 += other._33;
		_34 += other._34;

		_41 += other._41;
		_42 += other._42;
		_43 += other._43;
		_44 += other._44;

		return *this;
	}
	
	// ----------------------------------------------------------------------------------
	// Description
	//   Multiplication-assignment operator
	// Arguments:
	//   other - The Matrix by which to multiply this Matrix
	// Return Value:
	//   This Matrix, after multiplying by the other Matrix
	// ----------------------------------------------------------------------------------
	Matrix& operator*=( const Matrix& other )
	{
		D3DXMatrixMultiply( this, this, &other );
		return *this;
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Scalar multiplication-assignment operator
	// Arguments:
	//   f - The scalar by which to multiply this Matrix
	// Return Value:
	//   This Matrix, after multiplying each component by the scalar
	// ----------------------------------------------------------------------------------
	Matrix& operator*=( float f )
	{
		_11 *= f;
		_12 *= f;
		_13 *= f;
		_14 *= f;

		_21 *= f;
		_22 *= f;
		_23 *= f;
		_24 *= f;

		_31 *= f;
		_32 *= f;
		_33 *= f;
		_34 *= f;

		_41 *= f;
		_42 *= f;
		_43 *= f;
		_44 *= f;

		return *this;
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Scalar division-assignment operator
	// Arguments:
	//   f - The scalar by which to divide this Matrix
	// Return Value:
	//   This Matrix, after dividing by the scalar
	// ----------------------------------------------------------------------------------
	Matrix& operator/=( float f )
	{
		const float fDiv = 1.0f / f;

		_11 *= fDiv;
		_12 *= fDiv;
		_13 *= fDiv;
		_14 *= fDiv;

		_21 *= fDiv;
		_22 *= fDiv;
		_23 *= fDiv;
		_24 *= fDiv;

		_31 *= fDiv;
		_32 *= fDiv;
		_33 *= fDiv;
		_34 *= fDiv;

		_41 *= fDiv;
		_42 *= fDiv;
		_43 *= fDiv;
		_44 *= fDiv;

		return *this;
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Unary + operator (effectively a no-op)
	// Return Value:
	//   A copy of this Matrix
	// ----------------------------------------------------------------------------------
	Matrix operator+( void ) const 
	{ 
		return Matrix( *this ); 
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Unary negation operator
	// Return Value:
	//   A copy of this Matrix, with each component negated
	// ----------------------------------------------------------------------------------
	Matrix operator-( void ) const
	{
		return Matrix( -_11, -_12, -_13, -_14,
			           -_21, -_22, -_23, -_24,
					   -_31, -_32, -_33, -_34,
					   -_41, -_42, -_43, -_44 );
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Binary addition operator
	// Arguments:
	//   other - The Matrix to add to this Matrix
	// Return Value:
	//   Matrix containing the sum of this Matrix and the other operand
	// ----------------------------------------------------------------------------------
	const Matrix operator+( const Matrix& other ) const
	{
		return Matrix( *this ) += other;
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Binary subtraction operator
	// Arguments:
	//   other - The Matrix to subtract from this Matrix
	// Return Value:
	//   Matrix containing the result of the subtraction of the other operand from 
	//   this Matrix
	// ----------------------------------------------------------------------------------
	const Matrix operator-( const Matrix& other ) const
	{
		return Matrix( *this ) -= other;
	}


	// ----------------------------------------------------------------------------------
	// Description
	//   Binary multiplication operator
	// Arguments:
	//   other - The Matrix by which to multiply this Matrix
	// Return Value:
	//   Matrix containing the product of this Matrix and the other operand
	// ----------------------------------------------------------------------------------
	const Matrix operator*( const Matrix& other ) const
	{
		return Matrix( *this ) *= other;
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Binary scalar multiplication operator
	// Arguments:
	//   f - The scalar by which to multiply this Matrix
	// Return Value:
	//   Matrix containing product of this Matrix and the scalar operand
	// ----------------------------------------------------------------------------------
	const Matrix operator*( float f ) const
	{
		return Matrix( *this ) *= f;
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Binary scalar division operator
	// Arguments:
	//   f - The scalar by which to divide this Matrix
	// Return Value:
	//   Matrix containing this Matrix divided by the scalar operand
	// ----------------------------------------------------------------------------------
	const Matrix operator/( float f ) const
	{
		return Matrix( *this ) /= f;
	}

	// Declare friend operator
	friend const Matrix operator*( float f, const Matrix& other );

	// ----------------------------------------------------------------------------------
	// Description
	//   Equality comparison operator
	// Arguments:
	//   other - The Matrix with which to compare for equality
	// Return Value:
	//   true, if the two Matrices are component-wise equal
	//   false, if the two Matrices are not component-wise equal
	// ----------------------------------------------------------------------------------
	bool operator==( const Matrix& other ) const
	{
		return ( 
			_11 == other._11 && _12 == other._12 && _13 == other._13 && _14 == other._14 &&
			_21 == other._21 && _22 == other._22 && _23 == other._23 && _24 == other._24 &&
			_31 == other._31 && _32 == other._32 && _33 == other._33 && _34 == other._34 &&
			_41 == other._41 && _42 == other._42 && _43 == other._43 && _44 == other._44 );
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Inequality comparison operator
	// Arguments:
	//   other - The Matrix with which to compare for inequality
	// Return Value:
	//   true, if the two Matrices are not component-wise equal
	//   false, if the two Matrices are component-wise equal
	// ----------------------------------------------------------------------------------
	bool operator!=( const Matrix& other ) const
	{
		return ( 
			_11 != other._11 || _12 != other._12 || _13 != other._13 || _14 != other._14 ||
			_21 != other._21 || _22 != other._22 || _23 != other._23 || _24 != other._24 ||
			_31 != other._31 || _32 != other._32 || _33 != other._33 || _34 != other._34 ||
			_41 != other._41 || _42 != other._42 || _43 != other._43 || _44 != other._44 );
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Const translation getter.  Gets the translational component of the Matrix as a
	//   reference to a const Vector3.
	// Return Value:
	//   Const reference to the fourth row of the Matrix, as a Vector3
	// ----------------------------------------------------------------------------------
	const Vector3& GetTranslation( void ) const 
	{ 
		return *(const Vector3*)&_41; 
	}

	// ----------------------------------------------------------------------------------
	// Description
	//  Translation getter.  Gets the translational component of the Matrix as a
	//   reference to a Vector3.
	// Return Value:
	//   Reference to the fourth row of the Matrix, as a Vector3
	// ----------------------------------------------------------------------------------
	Vector3& GetTranslation( void ) 
	{ 
		return *(Vector3*)&_41; 
	}

	// ----------------------------------------------------------------------------------
	// Description
	//  Translation setter.  Sets the translational component of the Matrix with a
	//   Vector3.
	// ----------------------------------------------------------------------------------
	void SetTranslation( const Vector3* translation ) 
	{ 
		*((Vector3*)&_41) = *translation;
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Const x-axis getter.  Gets the x-axis of the Matrix as a reference to a const 
	//   Vector3.
	// Return Value:
	//   Const reference to the first row of the Matrix, as a Vector3
	// ----------------------------------------------------------------------------------
	const Vector3& GetX( void ) const 
	{ 
		return *(const Vector3*)&_11; 
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   X-axis getter.  Gets the x-axis of the Matrix as a reference to a Vector3.
	// Return Value:
	//   Reference to the first row of the Matrix, as a Vector3
	// ----------------------------------------------------------------------------------
	Vector3& GetX( void ) 
	{ 
		return *(Vector3*)&_11; 
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Const y-axis getter.  Gets the y-axis of the Matrix as a reference to a const 
	//   Vector3.
	// Return Value:
	//   Const reference to the second row of the Matrix, as a Vector3
	// ----------------------------------------------------------------------------------
	const Vector3& GetY( void ) const 
	{ 
		return *(const Vector3*)&_21; 
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Y-axis getter.  Gets the y-axis of the Matrix as a reference to a Vector3.
	// Return Value:
	//   Reference to the second row of the Matrix, as a Vector3
	// ----------------------------------------------------------------------------------
	Vector3& GetY( void ) 
	{ 
		return *(Vector3*)&_21; 
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Const z-axis getter.  Gets the z-axis of the Matrix as a reference to a const 
	//   Vector3.
	// Return Value:
	//   Const reference to the third row of the Matrix, as a Vector3
	// ----------------------------------------------------------------------------------
	const Vector3& GetZ( void ) const 
	{ 
		return *(const Vector3*)&_31; 
	}

	// ----------------------------------------------------------------------------------
	// Description
	//   Z-axis getter.  Gets the z-axis of the Matrix as a reference to a Vector3.
	// Return Value:
	//   Reference to the third row of the Matrix, as a Vector3
	// ----------------------------------------------------------------------------------
	Vector3& GetZ( void ) 
	{ 
		return *(Vector3*)&_31; 
	}
};

// --------------------------------------------------------------------------------------
// Description
//   Global binary scalar multiplication operator
// Arguments:
//   f - The scalar by which to multiply the Matrix
//   other - The Matrix to scale
// Return Value:
//   Matrix containing product of the Matrix and the scalar operand
// --------------------------------------------------------------------------------------
inline const Matrix operator*( float f, const Matrix& other )
{
	return Matrix( other ) *= f;
}

inline Vector4 operator*( Vector4 p, const Matrix& m )
{
	return Vector4( 
		p.x * m._11 + p.y * m._21 + p.z * m._31 + p.w * m._41,
		p.x * m._12 + p.y * m._22 + p.z * m._32 + p.w * m._42,
		p.x * m._13 + p.y * m._23 + p.z * m._33 + p.w * m._43,
		p.x * m._14 + p.y * m._24 + p.z * m._34 + p.w * m._44
		);
}

inline Vector4 operator*( const Matrix& m, Vector4 p )
{
	return Vector4( 
		m._11 * p.x + m._12 * p.y + m._13 * p.z + m._14 * p.w,
		m._21 * p.x + m._22 * p.y + m._23 * p.z + m._24 * p.w,
		m._31 * p.x + m._32 * p.y + m._33 * p.z + m._34 * p.w,
		m._41 * p.x + m._42 * p.y + m._43 * p.z + m._44 * p.w
		);
}

#else

#include "CcpMath/include/Matrix.h"

#endif

#include "Quaternion.h"

inline bool IsMatch( Be::Var* value, const Matrix& t )
{
	return (Be::Var*)&t == value;
}

// --------------------------------------------------------------------------------------
inline Matrix IdentityMatrix()
{
	return Matrix(
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1 );
}

// --------------------------------------------------------------------------------------
inline Matrix TranslationMatrix( float x, float y, float z )
{
	return Matrix(
		1.f, 0.f, 0.f, 0.f,
		0.f, 1.f, 0.f, 0.f,
		0.f, 0.f, 1.f, 0.f,
		x, y, z, 1.f );
}

// --------------------------------------------------------------------------------------
inline Matrix TranslationMatrix( const Vector3& translation )
{
	return Matrix(
		1.f, 0.f, 0.f, 0.f,
		0.f, 1.f, 0.f, 0.f,
		0.f, 0.f, 1.f, 0.f,
		translation.x, translation.y, translation.z, 1.f );
}

// --------------------------------------------------------------------------------------
inline Matrix ScalingMatrix( float sx, float sy, float sz )
{
	return Matrix(
		sx, 0.f, 0.f, 0.f,
		0.f, sy, 0.f, 0.f,
		0.f, 0.f, sz, 0.f,
		0.f, 0.f, 0.f, 1.f );
}

// --------------------------------------------------------------------------------------
inline Matrix ScalingMatrix( const Vector3& scaling )
{
	return Matrix(
		scaling.x, 0.f, 0.f, 0.f,
		0.f, scaling.y, 0.f, 0.f,
		0.f, 0.f, scaling.z, 0.f,
		0.f, 0.f, 0.f, 1.f );
}

// --------------------------------------------------------------------------------------
inline Matrix RotationMatrix( const Quaternion& q )
{
	Matrix out;
	out.m[0][0] = 1.0f - 2.0f * ( q.y * q.y + q.z * q.z );
	out.m[0][1] = 2.0f * ( q.x *q.y + q.z * q.w );
	out.m[0][2] = 2.0f * ( q.x * q.z - q.y * q.w );
	out.m[0][3] = 0.0f;
	out.m[1][0] = 2.0f * ( q.x * q.y - q.z * q.w );
	out.m[1][1] = 1.0f - 2.0f * ( q.x * q.x + q.z * q.z );
	out.m[1][2] = 2.0f * ( q.y *q.z + q.x *q.w );
	out.m[1][3] = 0.0f;
	out.m[2][0] = 2.0f * ( q.x * q.z + q.y * q.w );
	out.m[2][1] = 2.0f * ( q.y *q.z - q.x *q.w );
	out.m[2][2] = 1.0f - 2.0f * ( q.x * q.x + q.y * q.y );
	out.m[2][3] = 0.0f;
	out.m[3][0] = 0.0f;
	out.m[3][1] = 0.0f;
	out.m[3][2] = 0.0f;
	out.m[3][3] = 1.0f;
	return out;
}

// --------------------------------------------------------------------------------------
inline Matrix Transpose( const Matrix& m )
{
	Matrix out;
	out._11 = m._11; out._12 = m._21; out._13 = m._31; out._14 = m._41;
	out._21 = m._12; out._22 = m._22; out._23 = m._32; out._24 = m._42;
	out._31 = m._13; out._32 = m._23; out._33 = m._33; out._34 = m._43;
	out._41 = m._14; out._42 = m._24; out._43 = m._34; out._44 = m._44;
	return out;
}

// --------------------------------------------------------------------------------------
inline float Determinant( const Matrix& m )
{
	float a0 = m._11 * m._22 - m._12 * m._21;
	float a1 = m._11 * m._23 - m._13 * m._21;
	float a2 = m._11 * m._24 - m._14 * m._21;
	float a3 = m._12 * m._23 - m._13 * m._22;
	float a4 = m._12 * m._24 - m._14 * m._22;
	float a5 = m._13 * m._24 - m._14 * m._23;
	float b0 = m._31 * m._42 - m._32 * m._41;
	float b1 = m._31 * m._43 - m._33 * m._41;
	float b2 = m._31 * m._44 - m._34 * m._41;
	float b3 = m._32 * m._43 - m._33 * m._42;
	float b4 = m._32 * m._44 - m._34 * m._42;
	float b5 = m._33 * m._44 - m._34 * m._43;
	return a0 * b5 - a1 * b4 + a2 * b3 + a3 * b2 - a4 * b1 + a5 * b0;
}

// --------------------------------------------------------------------------------------
inline Matrix Inverse( const Matrix& m )
{
	Vector4 v, vec[3];
	Matrix out;

	float det = Determinant( m );
	if( !det )
	{
		out = m;
	}
	else
	{
		for( int i = 0; i < 4; i++ )
		{
			float signedDet = ( i & 1 ) ? -1.f : 1.f;
			signedDet /= det;
			for( int j = 0; j < 4; j++ )
			{
				if( j != i )
				{
					int a = j;
					if( j > i )
					{
						a = a - 1;
					}
					vec[a].x = m.m[j][0];
					vec[a].y = m.m[j][1];
					vec[a].z = m.m[j][2];
					vec[a].w = m.m[j][3];
				}
			}
			v = Cross( vec[0], vec[1], vec[2] );
			out.m[0][i] = signedDet * v.x;
			out.m[1][i] = signedDet * v.y;
			out.m[2][i] = signedDet * v.z;
			out.m[3][i] = signedDet * v.w;
		}
	}
	return out;
}

// --------------------------------------------------------------------------------------
inline Matrix OrthoNormalBasisZ( const Vector3& z )
{
	Matrix out = IdentityMatrix();
	out.GetZ() = Normalize( z );
	if( std::abs( out.GetZ().x ) > 0.99f )
	{
		out.GetX() = Vector3( 0, 1, 0 );
	}
	else
	{
		out.GetX() = Vector3( 1, 0, 0 );
	}
	out.GetY() = Normalize( Cross( out.GetX(), out.GetZ() ) );
	out.GetX() = Cross( out.GetY(), out.GetZ() );
	return out;
}

// --------------------------------------------------------------------------------------
inline Matrix OrthographicProjection( float width, float height, float zNear, float zFar )
{
	Matrix out = IdentityMatrix();
	out.m[0][0] = 2.0f / width;
	out.m[1][1] = 2.0f / height;
	out.m[2][2] = 1.0f / ( zNear - zFar );
	out.m[3][2] = zNear / ( zNear - zFar );
	return out;
}

// --------------------------------------------------------------------------------------
inline Vector3 TransformCoord( const Vector3& coord, const Matrix& transform )
{
	Vector3 out;
	float norm = transform.m[0][3] * coord.x + transform.m[1][3] * coord.y + transform.m[2][3] * coord.z + transform.m[3][3];
	if( norm != 0 )
	{
		out.x = ( coord.x * transform._11 + coord.y * transform._21 + coord.z * transform._31 + transform._41 ) / norm;
		out.y = ( coord.x * transform._12 + coord.y * transform._22 + coord.z * transform._32 + transform._42 ) / norm;
		out.z = ( coord.x * transform._13 + coord.y * transform._23 + coord.z * transform._33 + transform._43 ) / norm;
	}
	else
	{
		out.x = 0.0f;
		out.y = 0.0f;
		out.z = 0.0f;
	}
	return out;
}

// --------------------------------------------------------------------------------------
inline Vector3 TransformNormal( const Vector3& normal, const Matrix& transform )
{
	return Vector3(
		normal.x * transform._11 + normal.y * transform._21 + normal.z * transform._31,
		normal.x * transform._12 + normal.y * transform._22 + normal.z * transform._32,
		normal.x * transform._13 + normal.y * transform._23 + normal.z * transform._33 );
}

// --------------------------------------------------------------------------------------
inline Vector4 Transform( const Vector4& point, const Matrix& transform )
{
	return Vector4(
		point.x * transform._11 + point.y * transform._21 + point.z * transform._31 + point.w * transform._41,
		point.x * transform._12 + point.y * transform._22 + point.z * transform._32 + point.w * transform._42,
		point.x * transform._13 + point.y * transform._23 + point.z * transform._33 + point.w * transform._43,
		point.x * transform._14 + point.y * transform._24 + point.z * transform._34 + point.w * transform._44 );
}

// --------------------------------------------------------------------------------------
inline Vector4 Transform( const Vector3& point, const Matrix& transform )
{
	return Vector4(
		point.x * transform._11 + point.y * transform._21 + point.z * transform._31 + transform._41,
		point.x * transform._12 + point.y * transform._22 + point.z * transform._32 + transform._42,
		point.x * transform._13 + point.y * transform._23 + point.z * transform._33 + transform._43,
		transform._14 + transform._24 + transform._34 + transform._44 );
}

// --------------------------------------------------------------------------------------
inline void TransformCoords( void* coords, size_t count, size_t stride, const Matrix& transform )
{
	auto stream = static_cast<uint8_t*>( coords );
	for( size_t i = 0; i < count; ++i )
	{
		*reinterpret_cast<Vector3*>( stream ) = TransformCoord( *reinterpret_cast<Vector3*>( stream ), transform );
		stream += stride;
	}
}

// --------------------------------------------------------------------------------------
inline void TransformCoords( Vector3* coords, size_t size, const Matrix& transform )
{
	for( size_t i = 0; i < size; ++i )
	{
		coords[i] = TransformCoord( coords[i], transform );
	}
}

// --------------------------------------------------------------------------------------
template <size_t Size>
inline void TransformCoords( Vector3 (&coords)[Size], const Matrix& transform )
{
	for( size_t i = 0; i < Size; ++i )
	{
		coords[i] = TransformCoord( coords[i], transform );
	}
}

// --------------------------------------------------------------------------------------
template <typename OutIter, typename InIter>
inline void TransformCoords( OutIter dest, InIter first, InIter last, const Matrix& transform )
{
	while( first != last )
	{
		*dest = TransformCoord( *first, transform );
		++dest;
		++first;
	}
}

// --------------------------------------------------------------------------------------
inline Matrix TransformationMatrix(
	const Vector3* scalingCenter,
	const Quaternion* scalingRotation,
	const Vector3* scaling,
	const Vector3* rotationCenter,
	const Quaternion *rotation,
	const Vector3* translation )
{
	Matrix m1, m2, m3, m4, m5, m6, m7, p1, p2, p3, p4, p5;
	Quaternion prc;
	Vector3 psc, pt;

	if( !scalingCenter )
	{
		psc.x = 0.0f;
		psc.y = 0.0f;
		psc.z = 0.0f;
	}
	else
	{
		psc.x = scalingCenter->x;
		psc.y = scalingCenter->y;
		psc.z = scalingCenter->z;
	}
	if( !rotationCenter )
	{
		prc.x = 0.0f;
		prc.y = 0.0f;
		prc.z = 0.0f;
	}
	else
	{
		prc.x = rotationCenter->x;
		prc.y = rotationCenter->y;
		prc.z = rotationCenter->z;
	}
	if( !translation )
	{
		pt.x = 0.0f;
		pt.y = 0.0f;
		pt.z = 0.0f;
	}
	else
	{
		pt.x = translation->x;
		pt.y = translation->y;
		pt.z = translation->z;
	}
	m1 = TranslationMatrix( -psc );
	if( !scalingRotation )
	{
		m2 = IdentityMatrix();
		m4 = IdentityMatrix();
	}
	else
	{
		m4 = RotationMatrix( *scalingRotation );
		m2 = Inverse( m4 );
	}
	if( !scaling )
	{
		m3 = IdentityMatrix();
	}
	else
	{
		m3 = ScalingMatrix( *scaling );
	}
	if( !rotation )
	{
		m6 = IdentityMatrix();
	}
	else
	{
		m6 = RotationMatrix( *rotation );
	}
	m5 = TranslationMatrix( psc.x - prc.x, psc.y - prc.y, psc.z - prc.z );
	m7 = TranslationMatrix( prc.x + pt.x, prc.y + pt.y, prc.z + pt.z );
	return m1 * m2 * m3 * m4 * m5 * m6 * m7;
}

// --------------------------------------------------------------------------------------
inline Matrix TransformationMatrix(
	const Vector3& scaling,
	const Quaternion& rotation,
	const Vector3& translation )
{
	return ScalingMatrix( scaling ) * RotationMatrix( rotation ) * TranslationMatrix( translation );
}

#endif // MATRIX_H