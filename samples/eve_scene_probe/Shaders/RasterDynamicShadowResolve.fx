// Copyright (c) 2026 CCP Games

Texture2D<float> SceneDepth;
Texture2D<float> ShadowAtlas;
RWTexture2D<uint4> ShadowMask;

#define MAX_SHADOW_LIGHTS 6
#define MAX_SHADOW_FACES 36

cbuffer RasterDynamicShadowResolveData : register( b2 )
{
	float4x4 InverseViewProjection;
	float4 TargetDimensions;
	float4 AtlasDimensions;
	float4 ShadowSettings;
	float4 LightPositionRadius[MAX_SHADOW_LIGHTS];
	uint4 LightFaceRanges[MAX_SHADOW_LIGHTS];
	float4x4 FaceViewProjection[MAX_SHADOW_FACES];
	float4 FaceAtlasRect[MAX_SHADOW_FACES];
	uint4 ResolveCounts;
};

uint SelectPointLightFace( float3 fromLight )
{
	float3 absoluteDirection = abs( fromLight );
	if( absoluteDirection.x >= absoluteDirection.y && absoluteDirection.x >= absoluteDirection.z )
	{
		return fromLight.x < 0.0 ? 0 : 3;
	}
	if( absoluteDirection.y >= absoluteDirection.z )
	{
		return fromLight.y < 0.0 ? 1 : 4;
	}
	return fromLight.z < 0.0 ? 2 : 5;
}

float LoadShadowDepth( uint2 atlasPixel, float4 rect )
{
	uint2 rectMin = uint2( rect.xy ) + 1;
	uint2 rectMax = uint2( rect.xy + rect.zz ) - 2;
	atlasPixel = clamp( atlasPixel, rectMin, rectMax );
	return ShadowAtlas[atlasPixel];
}

bool IsShadowed( float3 worldPosition, uint faceIndex )
{
	float4 shadowClip = mul( FaceViewProjection[faceIndex], float4( worldPosition, 1.0 ) );
	if( shadowClip.w <= 0.0 )
	{
		return false;
	}

	float3 shadowNdc = shadowClip.xyz / shadowClip.w;
	if( any( abs( shadowNdc.xy ) > 1.0 ) || shadowNdc.z < 0.0 || shadowNdc.z > 1.0 )
	{
		return false;
	}

	float4 rect = FaceAtlasRect[faceIndex];
	float2 faceUv = float2( shadowNdc.x * 0.5 + 0.5, -shadowNdc.y * 0.5 + 0.5 );
	float2 atlasPixel = rect.xy + faceUv * rect.z;
	uint2 sampleBase = uint2( atlasPixel - 0.5 );
	float receiverDepth = shadowNdc.z;
	float bias = max( ShadowSettings.x, ShadowSettings.y / max( rect.z, 1.0 ) );
	uint occludedSamples = 0;
	occludedSamples += receiverDepth + bias < LoadShadowDepth( sampleBase + uint2( 0, 0 ), rect );
	occludedSamples += receiverDepth + bias < LoadShadowDepth( sampleBase + uint2( 1, 0 ), rect );
	occludedSamples += receiverDepth + bias < LoadShadowDepth( sampleBase + uint2( 0, 1 ), rect );
	occludedSamples += receiverDepth + bias < LoadShadowDepth( sampleBase + uint2( 1, 1 ), rect );
	return occludedSamples >= 2;
}

[numthreads( 8, 8, 1 )]
void MainCS( uint3 dispatchThreadId : SV_DispatchThreadID )
{
	uint2 pixel = dispatchThreadId.xy;
	if( pixel.x >= ResolveCounts.z || pixel.y >= ResolveCounts.w )
	{
		return;
	}
	float sceneDepth = SceneDepth[pixel];
	if( sceneDepth <= 0.0 )
	{
		ShadowMask[pixel] = uint4( 0u, 0u, 0u, 0u );
		return;
	}

	float2 uv = ( float2( pixel ) + 0.5 ) * TargetDimensions.zw;
	float4 clipPosition = float4( uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, sceneDepth, 1.0 );
	float4 worldPositionH = mul( InverseViewProjection, clipPosition );
	float3 worldPosition = worldPositionH.xyz / worldPositionH.w;

	uint mask = 0;
	for( uint lightSlot = 0; lightSlot < ResolveCounts.x; ++lightSlot )
	{
		float4 light = LightPositionRadius[lightSlot];
		float3 fromLight = worldPosition - light.xyz;
		if( dot( fromLight, fromLight ) > light.w * light.w )
		{
			continue;
		}

		uint4 faceRange = LightFaceRanges[lightSlot];
		uint faceOffset = faceRange.x;
		uint faceCount = faceRange.y;
		uint faceIndex = faceOffset;
		if( faceCount == 6 )
		{
			faceIndex += SelectPointLightFace( fromLight );
		}
		if( faceIndex < ResolveCounts.y && IsShadowed( worldPosition, faceIndex ) )
		{
			mask |= faceRange.z;
		}
	}
	ShadowMask[pixel] = uint4( mask, 0u, 0u, 0u );
}

technique Main
{
	pass MainPass
	{
		computeshader = compile cs_5_0 MainCS();
	}
}
