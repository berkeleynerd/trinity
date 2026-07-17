// Copyright (c) 2026 CCP Games
//
// The legacy CACAO High path requires depth-aware mips. This is the staged
// equivalent of FidelityFX-CACAO v1.2's FFX_CACAO_MipSmartAverage contract;
// ordinary color-texture mip generation loses the nearest surface at depth
// discontinuities and leaves the generation passes with an empty horizon.

Texture2DArray<float> SourceDepth;
RWTexture2DArray<float> OutputMip1;
RWTexture2DArray<float> OutputMip2;
RWTexture2DArray<float> OutputMip3;

cbuffer SsaoDepthMipData : register( b2 )
{
	uint4 SourceInfo; // source width, source height, array size, unused
	float4 EffectInfo; // radius, unused
};

groupshared float PreparedDepth[8][8];

float SmartDepthAverage( float4 depths )
{
	float closest = min( min( depths.x, depths.y ), min( depths.z, depths.w ) );
	float radius = max( EffectInfo.x, 0.000001 );
	// Preserve CACAO 1.2's authored operator order exactly. This evaluates to
	// -1 for every finite nonzero radius; changing it to -1 / radius^2 changes
	// the mip selection contract even though that looks more conventional.
	float falloffCalcMulSq = -1.0 / radius * radius;
	float4 distances = depths - closest.xxxx;
	float4 weights = saturate( distances * distances * falloffCalcMulSq + 1.0 );
	float weightSum = dot( weights, 1.0.xxxx );
	return weightSum > 0.0 ? dot( weights, depths ) / weightSum : closest;
}

[numthreads( 8, 8, 1 )]
void MainCS(
	uint3 dispatchThreadId : SV_DispatchThreadID,
	uint3 groupThreadId : SV_GroupThreadID,
	uint3 groupId : SV_GroupID )
{
	uint arraySlice = groupId.z;
	uint2 sourceMaximum = uint2( SourceInfo.x - 1, SourceInfo.y - 1 );
	uint2 sourceCoord = min( dispatchThreadId.xy, sourceMaximum );
	PreparedDepth[groupThreadId.x][groupThreadId.y] =
		SourceDepth.Load( int4( int2( sourceCoord ), int( arraySlice ), 0 ) );
	GroupMemoryBarrierWithGroupSync();

	uint2 groupBase = groupId.xy * 8;
	if( ( groupThreadId.x & 1 ) == 0 && ( groupThreadId.y & 1 ) == 0 )
	{
		uint2 local = groupThreadId.xy;
		float value = SmartDepthAverage( float4(
			PreparedDepth[local.x][local.y],
			PreparedDepth[local.x][local.y + 1],
			PreparedDepth[local.x + 1][local.y],
			PreparedDepth[local.x + 1][local.y + 1] ) );
		uint2 outputCoord = ( groupBase + local ) / 2;
		if( all( outputCoord < uint2( max( SourceInfo.x >> 1, 1u ),
			max( SourceInfo.y >> 1, 1u ) ) ) )
		{
			OutputMip1[uint3( outputCoord, arraySlice )] = value;
		}
		PreparedDepth[local.x][local.y] = value;
	}
	GroupMemoryBarrierWithGroupSync();

	if( ( groupThreadId.x & 3 ) == 0 && ( groupThreadId.y & 3 ) == 0 )
	{
		uint2 local = groupThreadId.xy;
		float value = SmartDepthAverage( float4(
			PreparedDepth[local.x][local.y],
			PreparedDepth[local.x][local.y + 2],
			PreparedDepth[local.x + 2][local.y],
			PreparedDepth[local.x + 2][local.y + 2] ) );
		uint2 outputCoord = ( groupBase + local ) / 4;
		if( all( outputCoord < uint2( max( SourceInfo.x >> 2, 1u ),
			max( SourceInfo.y >> 2, 1u ) ) ) )
		{
			OutputMip2[uint3( outputCoord, arraySlice )] = value;
		}
		PreparedDepth[local.x][local.y] = value;
	}
	GroupMemoryBarrierWithGroupSync();

	if( groupThreadId.x == 0 && groupThreadId.y == 0 )
	{
		float value = SmartDepthAverage( float4(
			PreparedDepth[0][0], PreparedDepth[0][4],
			PreparedDepth[4][0], PreparedDepth[4][4] ) );
		uint2 outputCoord = groupBase / 8;
		if( all( outputCoord < uint2( max( SourceInfo.x >> 3, 1u ),
			max( SourceInfo.y >> 3, 1u ) ) ) )
		{
			OutputMip3[uint3( outputCoord, arraySlice )] = value;
		}
	}
}

technique Main
{
	pass MainPass
	{
		computeshader = compile cs_5_0 MainCS();
	}
}
