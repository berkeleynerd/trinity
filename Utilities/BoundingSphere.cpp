#include "StdAfx.h"
#include "Include/TriMath.h"
#include "BoundingSphere.h"

void BoundingSphereInitialize( Vector4& sphere )
{
	sphere = Vector4( 0.f, 0.f, 0.f, 0.f );
}

bool BoundingSphereIsInside( const Vector4& sphere, const Vector3& pos )
{
	Vector3 delta = pos - ( const Vector3& )sphere;
	return ( D3DXVec3LengthSq( &delta ) <= sphere.w * sphere.w );
}

bool BoundingSphereIsSphereInside( const Vector4& parentSphere, const Vector4& testSphere )
{
	// pre-chck radiuses
	if( parentSphere.w < testSphere.w )
	{
		return false;
	}
	Vector3 delta = ( const Vector3& )testSphere - ( const Vector3& )parentSphere;
	return ( D3DXVec3LengthSq( &delta ) <= ( parentSphere.w - testSphere.w ) * ( parentSphere.w - testSphere.w ) );
}

void BoundingSphereUpdate( const Vector3& pos, Vector4& sphere )
{
	// do not update if is inside
	if( BoundingSphereIsInside( sphere, pos ) )
	{
		return;
	}

	// extend sphere
	Vector3 delta = pos - ( Vector3& )sphere;
	float deltaLen = D3DXVec3Length( &delta );

	( Vector3& )sphere += 0.5f * ( 1.f - sphere.w / deltaLen ) * delta;
	sphere.w = 0.5f * ( sphere.w + deltaLen );
}

void BoundingSphereUpdate( const Vector4& addSphere, Vector4& resultSphere )
{
	// do not update if is inside
	if( BoundingSphereIsSphereInside( resultSphere, addSphere ) )
	{
		return;
	}
	if( BoundingSphereIsSphereInside( addSphere, resultSphere ) )
	{
		resultSphere = addSphere;
		return;
	}

	// extend sphere
	Vector3 delta = ( Vector3& )addSphere - ( Vector3& )resultSphere;
	float deltaLen = D3DXVec3Length( &delta );

	( Vector3& )resultSphere += 0.5f * ( 1.f + ( addSphere.w - resultSphere.w ) / deltaLen ) * delta;
	resultSphere.w = 0.5f * ( resultSphere.w + addSphere.w + deltaLen );
}

void BoundingSphereTransform( const Matrix& tf, Vector4& sphere )
{
	Vector3 center;
	// translate center
	D3DXVec3TransformCoord( (Vector3*)&sphere, (Vector3*)&sphere, &tf );
	// scale with highest scale factor
	float scaleX = D3DXVec3Length( &tf.GetX() );
	float scaleY = D3DXVec3Length( &tf.GetY() );
	float scaleZ = D3DXVec3Length( &tf.GetZ() );
	float scale = std::max( scaleX, std::max( scaleY, scaleZ ) );
	sphere.w *= scale;
}

bool IntersectSphereAxisAlignedBox( const Vector4& sphere, const Vector3& minBounds, const Vector3& maxBounds )
{
	XMVECTOR SphereCenter = XMVectorSet( sphere.x, sphere.y, sphere.z, 0.0f );
	XMVECTOR SphereRadius = XMVectorReplicatePtr( &sphere.w );

	XMVECTOR BoxMin = XMVectorSet( minBounds.x, minBounds.y, minBounds.z, 0.0f );
	XMVECTOR BoxMax = XMVectorSet( maxBounds.x, maxBounds.y, maxBounds.z, 0.0f );

	// Find the distance to the nearest point on the box.
	// for each i in (x, y, z)
	// if (SphereCenter(i) < BoxMin(i)) d2 += (SphereCenter(i) - BoxMin(i)) ^ 2
	// else if (SphereCenter(i) > BoxMax(i)) d2 += (SphereCenter(i) - BoxMax(i)) ^ 2

	XMVECTOR d = XMVectorZero();

	// Compute d for each dimension.
	XMVECTOR LessThanMin = XMVectorLess( SphereCenter, BoxMin );
	XMVECTOR GreaterThanMax = XMVectorGreater( SphereCenter, BoxMax );

	XMVECTOR MinDelta = SphereCenter - BoxMin;
	XMVECTOR MaxDelta = SphereCenter - BoxMax;

	// Choose value for each dimension based on the comparison.
	d = XMVectorSelect( d, MinDelta, LessThanMin );
	d = XMVectorSelect( d, MaxDelta, GreaterThanMax );

	// Use a dot-product to square them and sum them together.
	XMVECTOR d2 = XMVector3Dot( d, d );

	return XMVector4LessOrEqual( d2, XMVectorMultiply( SphereRadius, SphereRadius ) ) != 0;
}

void BoundingSphereFromBox( Vector4& sphere, const Vector3& minBounds, const Vector3& maxBounds, const Matrix* tf )
{
	Vector3 min( minBounds );
	Vector3 max( maxBounds );

	if( tf )
	{
		D3DXVec3TransformCoord( &min, &minBounds, tf );
		D3DXVec3TransformCoord( &max, &maxBounds, tf );
	}

	D3DXVec3Add( (Vector3*)&sphere, &min, &max );
	D3DXVec3Subtract( &min, &min, &max );
	sphere.w = D3DXVec3Length( &min );
	sphere *= 0.5f;
}

// --------------------------------------------------------------------------------
// Description:
//   Local recursive function used by the bounding sphere generation algorithm
// --------------------------------------------------------------------------------
Vector4 recurBoundingSphereCreate( Vector3 const** p, size_t len, size_t b )
{
	Vector4 mb;

	// in case of the "trivial" ones we have an easy solution
	switch( b )
	{
	case 0:
		BoundingSphereFromPoints( mb, nullptr, 0 );
		break;
	case 1:
		BoundingSphereFromPoints( mb, &p[-1], 1 );
		break;
	case 2:
		BoundingSphereFromPoints( mb, &p[-2], 2 );
		break;
	case 3:
		BoundingSphereFromPoints( mb, &p[-3], 3 );
		break;
	case 4:
		BoundingSphereFromPoints( mb, &p[-4], 4 );
		return mb;
	}

	for( size_t i = 0; i < len; ++i )
	{
		if( !BoundingSphereIsInside( mb, *p[i] ) )
		{
			// avoid duplicates in the four points
			bool isDuplicate = false;
			for( size_t j = 0; j < b; ++j )
			{
				if( TriVectorIsIdentical( p[i], p[-(int32_t)j - 1] ) )
				{
					isDuplicate = true;
					break;
				}
			}
			if( !isDuplicate )
			{
				for( size_t j = i; j > 0; --j )
				{
					const Vector3* t = p[j];
					p[j] = p[j - 1];
					p[j - 1] = t;
				}

				mb = recurBoundingSphereCreate( p + 1, i, b + 1 );
			}
		}
	}

	return mb;
}

// --------------------------------------------------------------------------------
// Description:
//   Create a bounding sphere by a given number of points.
// --------------------------------------------------------------------------------
void BoundingSphereFromPoints( Vector4& sphere, Vector3 const** points, size_t pointsCount )
{
	const float radiusEpsilon = 1e-4f;

	// if we have zero to four points, generation is easy!
	switch( pointsCount )
	{
	case 0:
		BoundingSphereInitialize( sphere );
		break;
	case 1:
		{
			sphere = Vector4( *points[0], 0.f + radiusEpsilon );
		}
		break;
	case 2:
		{
			Vector3 a = 0.5f * ( *points[1] - *points[0] );
			sphere = Vector4( a + *points[0], D3DXVec3Length( &a ) + radiusEpsilon );
		}
		break;
	case 3:
		{
			Vector3 a = *points[1] - *points[0];
			Vector3 b = *points[2] - *points[0];
			Vector3 axb, axbxa, bxaxb;
			D3DXVec3Cross( &axb, &a, &b );
			D3DXVec3Cross( &axbxa, &axb, &a );
			D3DXVec3Cross( &bxaxb, &b, &axb );
			float denom = 2.f * D3DXVec3Dot( &axb, &axb );
			float a2 = D3DXVec3Dot( &a, &a );
			float b2 = D3DXVec3Dot( &b, &b );
			Vector3 o = ( b2 * axbxa + a2 * bxaxb ) / denom;
			sphere = Vector4( o + *points[0], D3DXVec3Length( &o ) + radiusEpsilon );
		}
		break;
	case 4:
		{
			Vector3 a = *points[1] - *points[0];
			Vector3 b = *points[2] - *points[0];
			Vector3 c = *points[3] - *points[0];
			float denom = 2.f * ( a.x * (b.y * c.z - c.y * b.z) - b.x * (a.y * c.z - c.y * a.z) + c.x * (a.y * b.z - b.y * a.z) );
			if( denom == 0.f )
			{
				// fail, must try with three points
				CCP_LOGWARN("BoundingSphereFromPoints: failed because denominator is zero! Using fallback...");
				return BoundingSphereFromPoints( sphere, points, 3 );
			}
			float a2 = D3DXVec3Dot( &a, &a );
			float b2 = D3DXVec3Dot( &b, &b );
			float c2 = D3DXVec3Dot( &c, &c );
			Vector3 axb, cxa, bxc;
			D3DXVec3Cross( &axb, &a, &b );
			D3DXVec3Cross( &cxa, &c, &a );
			D3DXVec3Cross( &bxc, &b, &c );
			Vector3 o = ( c2 * axb + b2 * cxa + a2 * bxc ) / denom;
			sphere = Vector4( o + *points[0], D3DXVec3Length( &o ) + radiusEpsilon );
		}
		break;
	default:
		sphere = recurBoundingSphereCreate( points, pointsCount, 0 );
		break;
	}
}



