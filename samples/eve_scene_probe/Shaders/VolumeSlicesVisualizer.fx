// Copyright (c) 2026 CCP Games

Texture2DArray BlitSource;

SamplerState ProductSampler
{
	MinFilter = Point;
	MagFilter = Point;
	MipFilter = Point;
	AddressU = Clamp;
	AddressV = Clamp;
};

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
	float2 tiled = saturate( input.texcoord ) * 2.0;
	uint2 tile = min( uint2( tiled ), uint2( 1, 1 ) );
	float layer = float( tile.x + tile.y * 2 );
	float4 value = BlitSource.SampleLevel( ProductSampler, float3( frac( tiled ), layer ), 0.0 );
	float3 hdr = max( value.rgb, 0.0 );
	float3 display = pow( hdr / ( 1.0 + hdr ), 1.0 / 2.2 );
	return float4( display, saturate( value.a ) );
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
