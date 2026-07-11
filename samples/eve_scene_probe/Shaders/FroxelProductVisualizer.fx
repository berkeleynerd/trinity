// Copyright (c) 2026 CCP Games

Texture3D BlitSource;

SamplerState ProductSampler
{
	MinFilter = Linear;
	MagFilter = Linear;
	MipFilter = Point;
	AddressU = Clamp;
	AddressV = Clamp;
	AddressW = Clamp;
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
	float3 maximumValue = 0.0;
	[unroll]
	for( int slice = 0; slice < 16; ++slice )
	{
		float z = ( float( slice ) + 0.5 ) / 16.0;
		maximumValue = max( maximumValue, BlitSource.SampleLevel( ProductSampler, float3( input.texcoord, z ), 0.0 ).rgb );
	}
	float3 display = pow( maximumValue / ( 1.0 + maximumValue ), 1.0 / 2.2 );
	return float4( display, 1.0 );
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
