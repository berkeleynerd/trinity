// Copyright © 2026 CCP ehf.

#include <metal_stdlib>
using namespace metal;

struct VS_INPUT
{
	float3 position [[attribute(0)]];
	float4 color [[attribute(1)]];
};

struct VS_OUTPUT
{
	float4 position [[position]];
	float4 color;
};

vertex VS_OUTPUT mainVS( VS_INPUT input [[stage_in]] )
{
	VS_OUTPUT output;
	output.position = float4( input.position, 1.0 );
	output.color = input.color;
	return output;
}
