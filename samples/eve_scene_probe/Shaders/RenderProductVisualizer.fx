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
	if( RenderProductMode < 1.5 )
	{
		float depth = saturate( value.r );
		float visibleDepth = depth > 0.0 ? pow( depth, 0.25 ) : 0.0;
		return float4( visibleDepth, visibleDepth, visibleDepth, 1.0 );
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
