#include "../common.hlsli"

RWTexture3D<float4> outTexture : register(u0);

Texture2D albedoTex : register(t0);
SamplerState samplerState : register(s0);

float3 scaleAndBias(float3 p)
{
	float3 res;
	res.x = 0.5f * p.x + 0.5;
	res.y = -0.5f * p.y + 0.5;
	res.z = 0.5f * p.z + 0.5;
	return res;
}

void main(VS_OUT input)
{
	float3 albedo = albedoTex.Sample(samplerState, input.texcoord).xyz;
	float3 voxel = scaleAndBias(input.fragpos.xyz);
	float3 dimensions = float3(256, 256, 256);	//Hardcoded for now
	float3 pixelCoord = voxel * dimensions;
	outTexture[pixelCoord] = float4(albedo, 1.0f);
}