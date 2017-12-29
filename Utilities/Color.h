#pragma once


struct Color
{
	Color() {}
	Color( uint32_t argb );
	Color( const Vector4& );
	Color( float r, float g, float b, float a );

	operator uint32_t() const;
	operator Vector4() const;

	Color& operator += ( const Color& );
	Color& operator -= ( const Color& );
	Color& operator *= ( float );
	Color& operator /= ( float );

	// unary operators
	Color operator + () const;
	Color operator - () const;

	// binary operators
	Color operator + ( const Color& ) const;
	Color operator - ( const Color& ) const;
	Color operator * ( float ) const;
	Color operator / ( float ) const;

	bool operator == ( const Color& ) const;
	bool operator != ( const Color& ) const;

	float r, g, b, a;
};


// --------------------------------------------------------------------------------------
inline Color::Color( uint32_t dw )
{
	const float f = 1.0f / 255.0f;
	r = f * (float) (uint8_t) (dw >> 16);
	g = f * (float) (uint8_t) (dw >>  8);
	b = f * (float) (uint8_t) (dw >>  0);
	a = f * (float) (uint8_t) (dw >> 24);
}

// --------------------------------------------------------------------------------------
inline Color::Color( const Vector4& other )
	:r( other.x ),
	g( other.y ),
	b( other.z ),
	a( other.w )
{
}

// --------------------------------------------------------------------------------------
inline Color::Color( float fr, float fg, float fb, float fa )
{
	r = fr;
	g = fg;
	b = fb;
	a = fa;
}

// --------------------------------------------------------------------------------------
inline Color::operator uint32_t() const
{
	uint32_t dwR = r >= 1.0f ? 0xff : r <= 0.0f ? 0x00 : (uint32_t)( r * 255.0f + 0.5f );
	uint32_t dwG = g >= 1.0f ? 0xff : g <= 0.0f ? 0x00 : (uint32_t)( g * 255.0f + 0.5f );
	uint32_t dwB = b >= 1.0f ? 0xff : b <= 0.0f ? 0x00 : (uint32_t)( b * 255.0f + 0.5f );
	uint32_t dwA = a >= 1.0f ? 0xff : a <= 0.0f ? 0x00 : (uint32_t)( a * 255.0f + 0.5f );

	return ( dwA << 24 ) | ( dwR << 16 ) | ( dwG << 8 ) | dwB;
}

// --------------------------------------------------------------------------------------
inline Color::operator Vector4() const
{
	return Vector4( r, g, b, a );
}

// --------------------------------------------------------------------------------------
inline Color& Color::operator += ( const Color& color )
{
	r += color.r;
	g += color.g;
	b += color.b;
	a += color.a;
	return *this;
}

// --------------------------------------------------------------------------------------
inline Color& Color::operator -= ( const Color& color )
{
	r -= color.r;
	g -= color.g;
	b -= color.b;
	a -= color.a;
	return *this;
}

// --------------------------------------------------------------------------------------
inline Color& Color::operator *= ( float scale )
{
	r *= scale;
	g *= scale;
	b *= scale;
	a *= scale;
	return *this;
}

// --------------------------------------------------------------------------------------
inline Color& Color::operator /= ( float div )
{
	float scale = 1.0f / div;
	r *= scale;
	g *= scale;
	b *= scale;
	a *= scale;
	return *this;
}

// --------------------------------------------------------------------------------------
inline Color Color::operator + () const
{
	return *this;
}

// --------------------------------------------------------------------------------------
inline Color Color::operator - () const
{
	return Color( -r, -g, -b, -a );
}

// --------------------------------------------------------------------------------------
inline Color Color::operator + ( const Color& other ) const
{
	return Color( r + other.r, g + other.g, b + other.b, a + other.a );
}

// --------------------------------------------------------------------------------------
inline Color Color::operator - ( const Color& other ) const
{
	return Color( r - other.r, g - other.g, b - other.b, a - other.a );
}

// --------------------------------------------------------------------------------------
inline Color Color::operator * ( float scale ) const
{
	return Color( r * scale, g * scale, b * scale, a * scale );
}

// --------------------------------------------------------------------------------------
inline Color Color::operator / ( float div ) const
{
	return Color( *this ) /= div;
}

// --------------------------------------------------------------------------------------
inline bool Color::operator == ( const Color& other ) const
{
	return r == other.r && g == other.g && b == other.b && a == other.a;
}

// --------------------------------------------------------------------------------------
inline bool Color::operator != ( const Color& other ) const
{
	return r != other.r || g != other.g || b != other.b || a != other.a;
}

// --------------------------------------------------------------------------------------
inline Color operator * ( float scale, const Color& color )
{
	return Color( color.r * scale, color.g * scale, color.b * scale, color.a * scale );
}

// --------------------------------------------------------------------------------------
inline bool IsMatch( Be::Var* value, const Color& t )
{
	return (Be::Var*)&t == value;
}

// --------------------------------------------------------------------------------------
inline Color Lerp( const Color& v1, const Color& v2, float s )
{
	return v1 + ( v2 - v1 ) * s;
}
