#include "../common.hlsli"

RWTexture3D<float4> outTexture : register(u0);

Texture2D albedoTex : register(t0);
SamplerState samplerState : register(s0);

SamplerComparisonState shadowSamplerState : register(s1);
Texture2D shadowTex : register(t1);

struct DirectionalLight
{
	float4 color;
	float4 direction;
	float specularPower;
};

cbuffer LightInformation : register(b0)
{
	DirectionalLight light;
}

float3 scaleAndBias(float3 p)
{
	float3 res;
	res.x = 0.5f * p.x + 0.5;
	res.y = -0.5f * p.y + 0.5;
	res.z = 0.5f * p.z + 0.5;
	return res;
}

bool isInsideCube(float3 p, float e) { return abs(p.x) < 1 + e && abs(p.y) < 1 + e && abs(p.z) < 1 + e; }

#define BIAS 0.06

float shadowCalculation(float4 fragPosLightSpace, float3 normal, float3 lightDir)
{
	float3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
	projCoords.x = projCoords.x / 2 + 0.5;
	projCoords.y = projCoords.y / -2 + 0.5;	//Flipped because Direct3D does UV flipping.
	//The Z is not transformed given that unlike OpenGL, the Z is already 0-1. No need unless you don't like shadows..

	float currentDepth = projCoords.z;

	float bias = max((BIAS * 10) * (1.0 - dot(normal, lightDir)), BIAS);

	float shadow = 0.0;
	float2 texelSize;
	shadowTex.GetDimensions(texelSize.x, texelSize.y);
	texelSize = 1.0f / texelSize;
	for (int x = -2; x <= 2; ++x)
	{
		for (int y = -2; y <= 2; ++y)
		{
			//float pcfDepth = shadowTex.Sample(shadowSampler, projCoords.xy + float2(x, y) * texelSize).r;
			//shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
			shadow += shadowTex.SampleCmpLevelZero(shadowSamplerState, projCoords.xy + float2(x, y)*texelSize, currentDepth - bias).r;
		}
	}
	shadow /= 25.0;
	return shadow;
}

void main(VS_OUT input)
{
	if (!isInsideCube(input.fragpos.xyz, 0)) return;
	float3 albedo = albedoTex.Sample(samplerState, input.texcoord).xyz;
	float3 voxel = scaleAndBias(input.fragpos.xyz);
	float3 dimensions = float3(256, 256, 256);	//Hardcoded for now
	float3 pixelCoord = voxel * dimensions;

	//Calculating basic light (ambient+diffuse term)
	float ambient = 0.3f * light.color.xyz;
	float3 norm = normalize(input.normal.xyz);
	float3 lightDir = normalize(-light.direction.xyz);

	float diffuse = max(dot(norm, lightDir), 0.0f) * 1.5f;

	float4 fragposlightspace = mul(input.shadowCam, float4(input.fragposviewspace.xyz, 1.0f));
	float shadowFactor = shadowCalculation(fragposlightspace, norm, lightDir);
	//float3 result = (ambient + diffuse) * albedo;
	float3 result = (ambient + (1.0 - shadowFactor) * (diffuse)) * albedo.xyz;
	outTexture[pixelCoord] = float4(result, 1.0f);
}