// Copyright (c) 2026 CCP Games

#include <metal_stdlib>
using namespace metal;

struct VS_INPUT
{
	float3 position [[attribute(0)]];
	float4 color [[attribute(1)]];
	float2 texcoord [[attribute(2)]];
};

struct VS_OUTPUT
{
	float4 position [[position]];
	float4 color;
	float2 texcoord;
};

vertex VS_OUTPUT mainVS( VS_INPUT input [[stage_in]] )
{
	VS_OUTPUT output;
	output.position = float4( input.position, 1.0 );
	output.color = input.color;
	output.texcoord = input.texcoord;
	return output;
}
