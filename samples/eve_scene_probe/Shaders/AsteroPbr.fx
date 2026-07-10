// Copyright (c) 2026 CCP Games

Texture2D BaseColorMap < bool Tr2sRGB = true; >;
Texture2D NormalMap;
Texture2D RoughnessMap;
Texture2D MaterialMap;
Texture2D GlowMap;
Texture2D DirtMap;
Texture2D DistortionLayerMap;
Texture2D PaintMaskMap;

SamplerState MaterialSampler
{
	MinFilter = Anisotropic;
	MagFilter = Linear;
	MipFilter = Linear;
	AddressU = Wrap;
	AddressV = Wrap;
};

float4 ModelCenterScale;
float4 RotationTerms;
float4 ProjectionTerms;
float MaterialView;

struct VertexInput
{
	float3 position : POSITION0;
	float3 normal : NORMAL0;
	float4 tangent : TANGENT0;
	float2 texcoord : TEXCOORD0;
	float4 color : COLOR0;
};

struct VertexOutput
{
	float4 position : SV_Position;
	float3 normal : TEXCOORD1;
	float4 tangent : TEXCOORD2;
	float2 texcoord : TEXCOORD0;
	float4 color : COLOR0;
};

float3 RotateModelVector( float3 value )
{
	float3 yawed = float3(
		value.x * RotationTerms.y + value.z * RotationTerms.x,
		value.y,
		-value.x * RotationTerms.x + value.z * RotationTerms.y );
	return float3(
		yawed.x,
		yawed.y * RotationTerms.w - yawed.z * RotationTerms.z,
		yawed.y * RotationTerms.z + yawed.z * RotationTerms.w );
}

VertexOutput MainVS( VertexInput input )
{
	VertexOutput output;
	float3 modelPosition = ( input.position - ModelCenterScale.xyz ) * ModelCenterScale.w;
	float3 position = RotateModelVector( modelPosition );
	float cameraZ = position.z + ProjectionTerms.z;
	float projection = ProjectionTerms.y / max( cameraZ, 0.2 );
	output.position = float4(
		position.x * projection / max( ProjectionTerms.x, 0.1 ),
		position.y * projection,
		1.0 - saturate( ( cameraZ - 0.1 ) / ProjectionTerms.w ),
		1.0 );
	output.normal = normalize( RotateModelVector( input.normal ) );
	output.tangent = float4( normalize( RotateModelVector( input.tangent.xyz ) ), input.tangent.w );
	output.texcoord = input.texcoord;
	output.color = input.color;
	return output;
}

float3 SampleNormal( VertexOutput input )
{
	float2 tangentXY = NormalMap.Sample( MaterialSampler, input.texcoord ).xy * 2.0 - 1.0;
	float3 tangentNormal = float3( tangentXY, sqrt( saturate( 1.0 - dot( tangentXY, tangentXY ) ) ) );
	float3 normal = normalize( input.normal );
	float3 tangent = normalize( input.tangent.xyz - normal * dot( normal, input.tangent.xyz ) );
	float3 bitangent = normalize( cross( normal, tangent ) ) * input.tangent.w;
	return normalize( tangent * tangentNormal.x + bitangent * tangentNormal.y + normal * tangentNormal.z );
}

float4 MainPS( VertexOutput input ) : SV_Target
{
	if( MaterialView > 0.5 )
	{
		if( MaterialView < 1.5 )
		{
			return float4( BaseColorMap.Sample( MaterialSampler, input.texcoord ).rgb, 1.0 );
		}
		if( MaterialView < 2.5 )
		{
			return float4( NormalMap.Sample( MaterialSampler, input.texcoord ).rgb, 1.0 );
		}
		if( MaterialView < 3.5 )
		{
			return float4( RoughnessMap.Sample( MaterialSampler, input.texcoord ).rgb, 1.0 );
		}
		if( MaterialView < 4.5 )
		{
			return float4( MaterialMap.Sample( MaterialSampler, input.texcoord ).rgb, 1.0 );
		}
		if( MaterialView < 5.5 )
		{
			return float4( GlowMap.Sample( MaterialSampler, input.texcoord ).rgb, 1.0 );
		}
		if( MaterialView < 6.5 )
		{
			return float4( DirtMap.Sample( MaterialSampler, input.texcoord ).rgb, 1.0 );
		}
		if( MaterialView < 7.5 )
		{
			return float4( DistortionLayerMap.Sample( MaterialSampler, input.texcoord ).rgb, 1.0 );
		}
		return float4( PaintMaskMap.Sample( MaterialSampler, input.texcoord ).rgb, 1.0 );
	}

	float3 normal = SampleNormal( input );
	float3 albedo = BaseColorMap.Sample( MaterialSampler, input.texcoord ).rgb * input.color.rgb;
	float dirt = saturate( DirtMap.Sample( MaterialSampler, input.texcoord ).r );
	float paintMask = saturate( PaintMaskMap.Sample( MaterialSampler, input.texcoord ).r );
	albedo *= lerp( 1.0, 0.68, dirt );
	albedo = lerp( albedo, float3( 0.28, 0.015, 0.02 ), paintMask * 0.9 );
	float roughness = saturate( RoughnessMap.Sample( MaterialSampler, input.texcoord ).r );
	float material = saturate( MaterialMap.Sample( MaterialSampler, input.texcoord ).r );
	float glow = saturate( GlowMap.Sample( MaterialSampler, input.texcoord ).r );

	float3 keyDirection = normalize( float3( -0.45, 0.78, -0.44 ) );
	float3 fillDirection = normalize( float3( 0.55, 0.18, 0.42 ) );
	float key = saturate( dot( normal, keyDirection ) );
	float fill = saturate( dot( normal, fillDirection ) );
	float diffuse = 0.16 + key * 0.82 + fill * 0.18;

	float3 viewDirection = float3( 0.0, 0.0, -1.0 );
	float3 halfDirection = normalize( keyDirection + viewDirection );
	float specularPower = lerp( 96.0, 8.0, roughness );
	float specular = pow( saturate( dot( normal, halfDirection ) ), specularPower );
	float specularStrength = lerp( 0.08, 0.65, material ) * ( 1.0 - roughness * 0.6 );
	float3 emission = glow * float3( 0.25, 0.55, 1.0 );
	return float4( albedo * diffuse + specular * specularStrength + emission, 1.0 );
}

float4 DepthPS( VertexOutput input ) : SV_Target
{
	return float4( SampleNormal( input ) * 0.5 + 0.5, 1.0 );
}

technique Main
{
	pass MainPass
	{
		CullMode = None;
		vertexshader = compile vs_3_0 MainVS();
		pixelshader = compile ps_3_0 MainPS();
	}
}

technique Depth
{
	pass DepthPass
	{
		CullMode = None;
		vertexshader = compile vs_3_0 MainVS();
		pixelshader = compile ps_3_0 DepthPS();
	}
}
