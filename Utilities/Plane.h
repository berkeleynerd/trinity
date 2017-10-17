#pragma once

#include "Vector3.h"

#ifdef _WIN32

typedef D3DXPLANE Plane;

#else

#include "CcpMath/include/Plane.h"

#endif

// --------------------------------------------------------------------------------------
inline std::pair<bool, Vector3> IntersectLine(
	const Plane& p,
	const Vector3& v1,
	const Vector3& v2 )
{
	Vector3 direction = v2 - v1;
	Vector3 normal;
	normal.x = p.a;
	normal.y = p.b;
	normal.z = p.c;

	float dot = Dot( normal, direction );
	if( dot == 0.0f )
	{
		return std::make_pair( false, Vector3( 0, 0, 0 ) );
	}
	float temp = ( p.d + Dot( normal, v1 ) ) / dot;
	return std::make_pair( true, Vector3(
		v1.x - temp * direction.x,
		v1.y - temp * direction.y,
		v1.z - temp * direction.z ) );
}

// --------------------------------------------------------------------------------------
inline Plane Normalize( const Plane& plane )
{
	float l = 1.0f / Length( reinterpret_cast<const Vector3&>( plane ) );
	return Plane(
		plane.a * l,
		plane.b * l,
		plane.c * l,
		plane.d * l );
}

// --------------------------------------------------------------------------------------
inline float DotCoord( const Plane& p, const Vector3& v )
{
	return p.a * v.x + p.b * v.y + p.c * v.z + p.d;
}

// --------------------------------------------------------------------------------------
inline float DotNormal( const Plane& p, const Vector3& v )
{
	return p.a * v.x + p.b * v.y + p.c * v.z;
}

