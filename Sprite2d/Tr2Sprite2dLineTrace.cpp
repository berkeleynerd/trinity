//////////////////////////////////////////////////////////////////////////
//
// Created: December 2010
// Copyright CCP 2010
//

#include "StdAfx.h"
#include "Tr2Sprite2dLineTrace.h"
#include "Tr2Sprite2dTexture.h"
#include "Tr2Sprite2dScene.h"

Tr2Sprite2dLineTraceVertex::Tr2Sprite2dLineTraceVertex( IRoot* lockobj /*= NULL */ ) :
m_position( 0.0f, 0.0f ),
	m_color( 1.0f, 1.0f, 1.0f, 1.0f )
{
}

Tr2Sprite2dLineTrace::Tr2Sprite2dLineTrace( IRoot* lockobj /*= NULL */ ) :
PARENTLOCK( m_vertices ),
	m_lineWidth( 1.0f ),
	m_length( 0.0f ),
	m_start( 0.0f ),
	m_end( 1.0f ),
	m_isLoop( false ),
	m_renderIndices( "Tr2Sprite2dLineTrace/m_renderIndices" ),
	m_renderVertices( "Tr2Sprite2dLineTrace/m_renderVertices" ),
	m_textureWidth( 1.0f),
	m_textureOffset( 0.0f ),
	m_textureOffsetAccum( 0.0f ),
	m_cornerType( CORNERTYPE_MITER )
{
	m_vertices.SetNotify( this );
}

Tr2Sprite2dLineTrace::~Tr2Sprite2dLineTrace()
{
	ClearVertices();
}

void Tr2Sprite2dLineTrace::GatherSprites( Tr2Sprite2dScene* renderer )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( !m_display || m_vertices.empty() || (m_end <= m_start) || (m_spriteEffect == TR2_SFX_NONE) )
	{
		if( !m_renderVertices.empty() )
		{
			ClearVertices();
		}
		return;
	}

	if( m_isDirty )
	{
		ClearVertices();

		if( !ValidateAndSetTextures( renderer ) )
		{
			return;
		}

		renderer->SetSpriteEffect( m_spriteEffect );
		renderer->SetTileMode( S2D_TS_TILE_X | S2D_TS_TILE_Y);

		SetRegularRenderState( renderer );

		renderer->PushTranslation( m_translation );

		m_renderVertices.reserve( GetEstimatedVertexCount() );

		m_length = CalcLength();
		float lastRelativeLength = 0.0f;
		float length = 0.0f;

		bool hasReachedStart = (m_start == 0.0f);
		bool hasReachedEnd = false;

		Vector2 fromPos = m_vertices[0]->m_position;
		Color fromColor = m_vertices[0]->m_color;

		float prevCapAngle = 0.0f;

		float end = m_end;
		if( end > m_start + 1.0f )
		{
			// Prevent looping around multiple times
			end = m_start + 1.0f;
		}

		auto it = m_vertices.begin() + 1;
		while( !hasReachedEnd )
		{
			const Vector2& toPos = (*it)->m_position;
			const Color& toColor = (*it)->m_color;

			Vector2 d = toPos - fromPos;
			float curLength = D3DXVec2Length( &d );
			length += curLength;
			float relativeLength = length / m_length;

			Vector2 adjustedFromPos;
			Color adjustedFromColor;

			if( !hasReachedStart && (relativeLength > m_start) )
			{
				// Line is starting - adjust fromPos
				float linePortionOfTotal = curLength / m_length;
				float portionOfLineToRender = (m_start - lastRelativeLength) / linePortionOfTotal;
				adjustedFromPos = toPos*portionOfLineToRender + fromPos*(1.0f - portionOfLineToRender);
				adjustedFromColor = toColor*portionOfLineToRender + fromColor*(1.0f - portionOfLineToRender);
				hasReachedStart = true;
			}
			else
			{
				adjustedFromPos = fromPos;
				adjustedFromColor = fromColor;
			}

			if( hasReachedStart )
			{
				if( relativeLength > end )
				{
					// Line is ending - adjust toPos
					float linePortionOfTotal = curLength / m_length;
					float portionOfLineToRender = (m_end - lastRelativeLength) / linePortionOfTotal;
					Vector2 adjustedToPos = toPos*portionOfLineToRender + fromPos*(1.0f - portionOfLineToRender);
					Color adjustedToColor = toColor*portionOfLineToRender + fromColor*(1.0f - portionOfLineToRender);
					AddSegment( renderer, adjustedFromPos, adjustedFromColor, prevCapAngle, adjustedToPos, adjustedToColor, 0.0f);
					hasReachedEnd = true;
				}
				else
				{
					float capAngleTo = 0.0f;
					if( m_lineWidth > 1.0f )
					{
						auto nextIt = it + 1;
						if( nextIt == m_vertices.end() && m_isLoop )
						{
							nextIt = m_vertices.begin();
						}

						if( nextIt != m_vertices.end() )
						{
							const Vector2& toPos2 = (*nextIt)->m_position;
							Vector2 d2 = toPos2 - toPos;

							D3DXVec2Normalize( &d, &d );
							D3DXVec2Normalize( &d2, &d2 );

							capAngleTo = atan2(d2.y,d2.x) - atan2(d.y,d.x);
						}
					}
					AddSegment( renderer, adjustedFromPos, adjustedFromColor, prevCapAngle, toPos, toColor, capAngleTo );
					prevCapAngle = capAngleTo;
				}
			}

			++it;

			if( it == m_vertices.end() )
			{
				if( m_isLoop )
				{
					// Wrap around, to allow rendering with start towards the
					// end of the trace, and the end near the beginning, i.e.
					// treat the trace as a closed loop.
					it = m_vertices.begin();
				}
				else
				{
					hasReachedEnd = true;
				}
			}

			fromPos = toPos;
			fromColor = toColor;
			lastRelativeLength = relativeLength;
		}

		renderer->PopTranslation();

		m_isDirty = false;
	}

	SetValidatedTextures( renderer );
	renderer->RenderTriangleVerts( &m_renderVertices[0], (unsigned int)m_renderVertices.size(), &m_renderIndices[0], (uint16_t)m_renderIndices.size() );
}

ITr2SpriteObject* Tr2Sprite2dLineTrace::PickPoint( float x, float y, Tr2Sprite2dScene* renderer )
{
	if( !m_display )
	{
		return nullptr;
	}

	if( m_pickState == TR2_SPS_ON )
	{
		if( m_vertices.size() < 2 )
		{
			return nullptr;
		}

		Vector2 fromPos = m_vertices[0]->m_position;
		float halfWidth = m_lineWidth * 0.5f;

		for( auto it = m_vertices.begin() + 1; it != m_vertices.end(); ++it )
		{
			const Vector2& toPos = (*it)->m_position;
			Vector2 d = toPos - fromPos;
			d = XMVector2Normalize( d );
			if( renderer->IsInsideLineSegment( Vector2( x, y ), fromPos + d, toPos - d, halfWidth ) )
			{
				return this;
			}
			fromPos = toPos;
		}

		if( m_isLoop )
		{
			const Vector2& toPos =  m_vertices[0]->m_position;
			Vector2 d = toPos - fromPos;
			d = XMVector2Normalize( d );
			if( renderer->IsInsideLineSegment( Vector2( x, y ), fromPos + d, toPos - d, halfWidth ) )
			{
				return this;
			}
		}
	}

	return nullptr;
}

float Tr2Sprite2dLineTrace::CalcLength()
{
	if( m_vertices.size() < 2 )
	{
		return 0.0f;
	}

	Vector2 fromPos = m_vertices[0]->m_position;
	float length = 0.0f;

	for( auto it = m_vertices.begin() + 1; it != m_vertices.end(); ++it )
	{
		const Vector2& toPos = (*it)->m_position;

		Vector2 d = toPos - fromPos;
		length += D3DXVec2Length( &d );

		fromPos = toPos;
	}

	if( m_isLoop )
	{
		Vector2 d = m_vertices[0]->m_position - fromPos;
		length += D3DXVec2Length( &d );
	}

	return length;
}

unsigned int Tr2Sprite2dLineTrace::GetEstimatedVertexCount()
{
	unsigned int vertsNeeded = (unsigned int)m_vertices.size() * 4 + 2;

	// Add verts for line joints. Actual number of vertices used varies
	// depending on line thickness and joint angle.
	unsigned int w = (unsigned int)m_lineWidth;
	vertsNeeded += (unsigned int)m_vertices.size() * w;

	return vertsNeeded;
}

void Tr2Sprite2dLineTrace::AddSegment( 
	Tr2Sprite2dScene* renderer, 
	const Vector2& from, 
	const Color& fromColor, 
	float capAngleFrom,
	const Vector2& to, 
	const Color& toColor, 
	float capAngleTo )
{
	Vector2 d = to - from;
	float segmentLength = D3DXVec2Length( &d );
	D3DXVec2Normalize( &d, &d );

	// calculate texture offset
	float texOffset1 = m_textureOffsetAccum / m_textureWidth - m_textureOffset;
	m_textureOffsetAccum += segmentLength;
	float texOffset2 = m_textureOffsetAccum / m_textureWidth - m_textureOffset;

	// Anti-aliased lines are rendered with a quad that is larger. This is then 
	// compensated for in the pixel shader, using the extra pixels to fill in
	// alpha values to do the anti-aliasing

	bool isAA = m_spriteEffect == TR2_SFX_FILL_AA;
	float pixelWidthInTexels;

	float halfWidth = 0.5f * m_lineWidth;

	if( isAA )
	{
		halfWidth += 2.0f;
		pixelWidthInTexels = 1.0f / (m_lineWidth + 4.0f);
	}

	capAngleFrom = ClampAngle(capAngleFrom);
	capAngleTo = ClampAngle(capAngleTo);

	// Shorten lines to account for corners
	Vector2 adjustedFrom, adjustedTo;
	if ( m_cornerType != CORNERTYPE_NONE )
	{
		float shorteningForCap = halfWidth * tan( std::abs( capAngleTo ) * 0.5f );
		adjustedTo = to - shorteningForCap * d;

		shorteningForCap = halfWidth * tan( std::abs( capAngleFrom ) * 0.5f );
		adjustedFrom = from + shorteningForCap * d;
	}
	else
	{
		adjustedTo = to;
		adjustedFrom = from;
	}
	
	d = adjustedTo - adjustedFrom;
	D3DXVec2Normalize( &d, &d );

	// Rotate 90 degrees
	Vector2 normal(d.y, -d.x);

	Tr2Sprite2dVertexBase verts[4];

	// v0 - top left
	Tr2Sprite2dVertexBase& v0 = verts[0];
	v0.position.x = adjustedFrom.x - normal.x*halfWidth;
	v0.position.y = adjustedFrom.y - normal.y*halfWidth;
	v0.position.z = m_depth;
	v0.color = fromColor;
	v0.texCoord[0] = Vector2( texOffset1, 0.0f );
	if(	isAA )
		v0.texCoord[1] = Vector2( -pixelWidthInTexels, m_lineWidth );

	// v1 - bottom left
	Tr2Sprite2dVertexBase& v1 = verts[1];
	v1.position.x = adjustedFrom.x + normal.x*halfWidth;
	v1.position.y = adjustedFrom.y + normal.y*halfWidth;
	v1.position.z = m_depth;
	v1.color = fromColor;
	v1.texCoord[0] = Vector2( texOffset1, 1.0f );
	if( isAA )
		v1.texCoord[1] = Vector2( 1.0f + pixelWidthInTexels, m_lineWidth );

	// v2 - bottom right
	Tr2Sprite2dVertexBase& v2 = verts[2];
	v2.position.x = adjustedTo.x + normal.x*halfWidth;
	v2.position.y = adjustedTo.y + normal.y*halfWidth;
	v2.position.z = m_depth;
	v2.color = toColor;
	v2.texCoord[0] = Vector2( texOffset2, 1.0f );
	if( isAA )
		v2.texCoord[1] = Vector2( 1.0f + pixelWidthInTexels, m_lineWidth );

	// v3 - top right
	Tr2Sprite2dVertexBase& v3 = verts[3];
	v3.position.x = adjustedTo.x - normal.x*halfWidth;
	v3.position.y = adjustedTo.y - normal.y*halfWidth;
	v3.position.z = m_depth;
	v3.color = toColor;
	v3.texCoord[0] = Vector2( texOffset2, 0.0f );
	if( isAA )
		v3.texCoord[1] = Vector2( -pixelWidthInTexels, m_lineWidth );

	// Create verticies
	uint16_t oldSize = (uint16_t)m_renderVertices.size();
	m_renderVertices.resize( oldSize + 4 );

	renderer->PrepareTriangleVerts( 
		&m_renderVertices[oldSize], 
		verts, 
		sizeof( Tr2Sprite2dVertexBase ), 
		4 );

	// Update index buffer
	m_renderIndices.push_back( 0 + oldSize );
	m_renderIndices.push_back( 1 + oldSize );
	m_renderIndices.push_back( 3 + oldSize );
	m_renderIndices.push_back( 3 + oldSize );
	m_renderIndices.push_back( 1 + oldSize );
	m_renderIndices.push_back( 2 + oldSize );

	// Construct joint
	if( capAngleTo != 0.0f)
	{
		if ( m_cornerType == CORNERTYPE_MITER)
		{
			AddMiterJoint(
				renderer,
				capAngleTo,
				v2,
				v3,
				normal,
				toColor,	
				isAA,
				pixelWidthInTexels,
				halfWidth
			);
		}
		else if (m_cornerType == CORNERTYPE_ROUND)
		{
			AddRoundJoint(
				renderer,
				capAngleTo,
				v2,
				v3,
				normal,
				toColor,
				isAA,
				pixelWidthInTexels,
				halfWidth
				);
		}
	}
}

void Tr2Sprite2dLineTrace::AddRoundJoint(
	Tr2Sprite2dScene* renderer,
	float capAngleTo, 
	Tr2Sprite2dVertexBase v2, 
	Tr2Sprite2dVertexBase v3, 
	Vector2 normal, 
	Color color, 
	bool isAA, 
	float pixelWidthInTexels,
	float halfWidth)
{
	uint16_t fanBase;
	Vector2 fanBaseTranslation;
	auto fanVertexBase = m_renderVertices.size();
	float sign = 1.0f;
	float jointWidth = halfWidth * 2.0f;

	// Determine fan base point
	if( capAngleTo < 0.0f )
	{
		fanBase = 2 + (uint16_t)fanVertexBase - 4;
		fanBaseTranslation = v2.position;
		sign = -1.0f;
	}
	else if( capAngleTo > 0.0f )
	{
		fanBase = 3 + (uint16_t)fanVertexBase - 4;
		fanBaseTranslation = v3.position;
	}

	// calculate joint angle
	float startAngle = atan2( normal.y, normal.x );
	float endAngle = startAngle + capAngleTo;
	float angleDiff = endAngle - startAngle;

	// calculate arc length and number of steps
	float arcLength = angleDiff * m_lineWidth;
	unsigned int numSteps = (unsigned int)arcLength / 4;

	if( numSteps < 2 )
		numSteps = 2;
	float stepSize = angleDiff / (float)numSteps;
	++numSteps;
	
	if( numSteps > jointWidth )
	{
		numSteps = (unsigned int)m_lineWidth;
		stepSize = angleDiff / (float)(numSteps - 1);
	}

	// Create verticies
	Tr2Sprite2dVertexBase* fanVerts = static_cast<Tr2Sprite2dVertexBase*>( CCP_MALLOC( 
		"Tr2Sprite2dLineTrace/fanVerts", 
		numSteps * sizeof( Tr2Sprite2dVertexBase ) ) );
	Tr2Sprite2dVertexBase* currentVertex = fanVerts;

	// Construct outer joint vertexes
	float a = startAngle;
	uint16_t vertexCount = (uint16_t)m_renderVertices.size();
	
	for( unsigned int i = 0; i < numSteps; ++i )
	{
		// Set position
		Tr2Sprite2dVertexBase& v = *currentVertex++;
		v.position.x = sign * cos( a ) * jointWidth + fanBaseTranslation.x;
		v.position.y = sign * sin( a ) * jointWidth + fanBaseTranslation.y;
		v.position.z = m_depth;
		v.color = color;

		// Set texture coordinates
		if( isAA )
		{
			if( sign > 0.0f )
				v.texCoord[1] = Vector2( 1.0f + pixelWidthInTexels, m_lineWidth );
			else
				v.texCoord[1] = Vector2( -pixelWidthInTexels, m_lineWidth );
		}
		else
		{
			m_textureOffsetAccum += abs(arcLength) / float(numSteps);
			float texOffset = m_textureOffsetAccum / m_textureWidth - m_textureOffset;
			if( sign > 0.0f )
				v.texCoord[0] = Vector2( texOffset, 1.0f );
			else
				v.texCoord[0] = Vector2( texOffset, 0.0f );
		}

		// Update index buffer
		if( i > 0 )
		{
			m_renderIndices.push_back( -1 + vertexCount );
			m_renderIndices.push_back(  0 + vertexCount );
			m_renderIndices.push_back( fanBase );
		}

		a += stepSize;
		++vertexCount;
	}

	m_renderVertices.resize( vertexCount );
	renderer->PrepareTriangleVerts( 
		&m_renderVertices[fanVertexBase], 
		fanVerts, 
		sizeof( Tr2Sprite2dVertexBase ), 
		numSteps );

	CCP_FREE( fanVerts );
}

void Tr2Sprite2dLineTrace::AddMiterJoint(
	Tr2Sprite2dScene* renderer,
	float capAngleTo, 
	Tr2Sprite2dVertexBase v2, 
	Tr2Sprite2dVertexBase v3, 
	Vector2 normal, 
	Color color, 
	bool isAA, 
	float pixelWidthInTexels,
	float halfWidth)
{
	uint16_t fanBase;
	Vector2 basePoint, topPoint;
	auto fanVertexBase = m_renderVertices.size();
	float sign = 1.0f;

	// Determine fan base point
	if( capAngleTo < 0.0f )
	{
		fanBase = 2 + (uint16_t)fanVertexBase - 4;
		basePoint = v2.position;
		topPoint = v3.position;
		sign = -1.0f;
	}
	else if( capAngleTo > 0.0f )
	{
		fanBase = 3 + (uint16_t)fanVertexBase - 4;
		basePoint = v3.position;
		topPoint = v2.position;
	}

	// calculate joint angle
	float startAngle = atan2( normal.y, normal.x );
	float endAngle = startAngle + capAngleTo;
	float angleDiff = endAngle - startAngle;

	// Create vertexes
	Tr2Sprite2dVertexBase* fanVerts = static_cast<Tr2Sprite2dVertexBase*>( CCP_MALLOC( 
		"Tr2Sprite2dLineTrace/fanVerts", 
		sizeof( Tr2Sprite2dVertexBase ) ) );
	Tr2Sprite2dVertexBase* currentVertex = fanVerts;
	
	// Construct outer joint vertexes
	float a = startAngle + angleDiff / 2.0f;
	uint16_t vertexCount = (uint16_t)m_renderVertices.size();
	
	// Create vertex
	Tr2Sprite2dVertexBase& v = *currentVertex++;
	Vector2 p = GetMiterPoint(halfWidth, basePoint, startAngle, endAngle, sign);
	v.position.x = p.x;
	v.position.y = p.y;
	v.position.z = m_depth;
	v.color = color;

	// Set texture coordinates
	if( isAA )
	{
		if( sign > 0.0f )
			v.texCoord[1] = Vector2( 1.0f + pixelWidthInTexels, m_lineWidth );
		else
			v.texCoord[1] = Vector2( -pixelWidthInTexels, m_lineWidth );
	}
	else
	{	
		float arcLength;
		XMStoreFloat(&arcLength, XMVector2Length(p - topPoint));
		m_textureOffsetAccum += abs(arcLength);
		float texOffset = m_textureOffsetAccum / m_textureWidth - m_textureOffset;
		if( sign > 0.0f )
			v.texCoord[0] = Vector2( texOffset, 1.0f );
		else
			v.texCoord[0] = Vector2( texOffset, 0.0f );
	}

	// Update render index
	if (sign == 1.0)
	{
		m_renderIndices.push_back( -2 + vertexCount );
		m_renderIndices.push_back( fanBase );
		m_renderIndices.push_back(  0 + vertexCount );

		m_renderIndices.push_back( 2 + vertexCount );
		m_renderIndices.push_back( fanBase );
		m_renderIndices.push_back(  0 + vertexCount );
	}
	else
	{
		m_renderIndices.push_back( -1 + vertexCount );
		m_renderIndices.push_back( fanBase );
		m_renderIndices.push_back(  0 + vertexCount );

		m_renderIndices.push_back( 1 + vertexCount );
		m_renderIndices.push_back( fanBase );
		m_renderIndices.push_back( 0 + vertexCount );
	}

	m_renderVertices.resize( vertexCount + 1 );
	renderer->PrepareTriangleVerts( 
		&m_renderVertices[fanVertexBase], 
		fanVerts,
		sizeof( Tr2Sprite2dVertexBase ), 
		1 );

	CCP_FREE( fanVerts );
}

Vector2 Tr2Sprite2dLineTrace::GetMiterPoint(float halfWidth, Vector2 basePoint, float startAngle, float endAngle, float sign)
{
	// Returns the outer intersection point of the two line segments for a miter joint
	Vector2 ret, p1, p2, v1, v2;
	float k = halfWidth * 2.0f * sign;
	v1 = Vector2(cos(startAngle) * k, sin(startAngle) * k);
	p1 = basePoint + v1;
	v2 = Vector2(cos(endAngle) * k, sin(endAngle) * k);
	p2 = basePoint + v2;
	ret = XMVector2IntersectLine(p1, p1 - Vector2(-v1.y, v1.x), p2, p2 - Vector2(-v2.y, v2.x));
	return ret;
}

float Tr2Sprite2dLineTrace::ClampAngle(float angle)
{
	if (angle < -XM_PI)
	{
		return angle + 2 * XM_PI;
	}
	else if (angle > XM_PI)
	{
		return angle - 2 * XM_PI;
	}
	return angle;
}

void Tr2Sprite2dLineTrace::ClearVertices()
{
	m_renderVertices.clear();
	m_renderVertices.shrink_to_fit();
	m_renderIndices.clear();
	m_renderIndices.shrink_to_fit();
	m_textureOffsetAccum = 0.0f;
}

void Tr2Sprite2dLineTrace::OnListModified( long event, /* BLUELISTEVENT values */ ssize_t key, ssize_t key2, IRoot* value, const IList* theList )
{
	switch( event )
	{ 
	case BELIST_INSERTED:
	case BELIST_REMOVED:
	case BELIST_UNLOADSTART:
		SetDirty();
		break;
	}
}