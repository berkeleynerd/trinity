// Copyright (c) 2026 CCP Games

Texture2D BlitSource;

SamplerState ProductSampler
{
	MinFilter = Point;
	MagFilter = Point;
	MipFilter = Point;
	AddressU = Clamp;
	AddressV = Clamp;
};

float RenderProductMode;
float AoUsesAlpha;

struct VertexInput
{
	float4 position : POSITION0;
	float2 texcoord : TEXCOORD0;
};

struct VertexOutput
{
	float4 position : SV_Position;
	float2 texcoord : TEXCOORD0;
};

VertexOutput MainVS( VertexInput input )
{
	VertexOutput output;
	output.position = input.position;
	output.texcoord = input.texcoord;
	return output;
}

float4 MainPS( VertexOutput input ) : SV_Target
{
	float4 value = BlitSource.SampleLevel( ProductSampler, input.texcoord, 0.0 );
	if( RenderProductMode < 1.5 || ( RenderProductMode >= 3.5 && RenderProductMode < 4.5 ) )
	{
		float depth = saturate( value.r );
		float visibleDepth = depth > 0.0 ? pow( depth, 0.25 ) : 0.0;
		return float4( visibleDepth, visibleDepth, visibleDepth, 1.0 );
	}
	if( RenderProductMode >= 2.5 && RenderProductMode < 3.5 )
	{
		return float4( value.rrr, 1.0 );
	}
	if( RenderProductMode >= 4.5 && RenderProductMode < 5.5 )
	{
		float ao = AoUsesAlpha > 0.5 ? value.a * 0.5 + 0.5 : value.r;
		return float4( ao, ao, ao, 1.0 );
	}
	if( RenderProductMode >= 5.5 && RenderProductMode < 6.5 )
	{
		return float4( value.rgb * 0.5 + 0.5, 1.0 );
	}
	if( RenderProductMode >= 6.5 )
	{
		float3 hdr = max( value.rgb, 0.0 );
		float3 reinhard = hdr / ( 1.0 + hdr );
		return float4( pow( reinhard, 1.0 / 2.2 ), 1.0 );
	}
	return float4( value.rgb, 1.0 );
}

technique Main
{
	pass MainPass
	{
		CullMode = None;
		ZEnable = False;
		ZWriteEnable = False;
		vertexshader = compile vs_3_0 MainVS();
		pixelshader = compile ps_3_0 MainPS();
	}
}
