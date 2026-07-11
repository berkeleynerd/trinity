// Copyright (c) 2026 CCP Games

TextureCube BlitSource;

SamplerState ReflectionSampler
{
	MinFilter = Linear;
	MagFilter = Linear;
	MipFilter = Linear;
	AddressU = Clamp;
	AddressV = Clamp;
};

float ReflectionMip;

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

float3 CubeDirection( int face, float2 local )
{
	if( face == 0 )
		return float3( 1.0, -local.y, -local.x );
	if( face == 1 )
		return float3( -1.0, -local.y, local.x );
	if( face == 2 )
		return float3( local.x, 1.0, local.y );
	if( face == 3 )
		return float3( local.x, -1.0, -local.y );
	if( face == 4 )
		return float3( local.x, -local.y, 1.0 );
	return float3( -local.x, -local.y, -1.0 );
}

float4 MainPS( VertexOutput input ) : SV_Target
{
	float2 grid = input.texcoord * float2( 3.0, 2.0 );
	float2 tile = min( floor( grid ), float2( 2.0, 1.0 ) );
	float2 local = frac( grid ) * 2.0 - 1.0;
	int face = (int)tile.x + (int)tile.y * 3;
	float3 hdr = max( BlitSource.SampleLevel( ReflectionSampler, CubeDirection( face, local ), ReflectionMip ).rgb, 0.0 );
	float3 mapped = hdr / ( 1.0 + hdr );
	return float4( pow( mapped, 1.0 / 2.2 ), 1.0 );
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
