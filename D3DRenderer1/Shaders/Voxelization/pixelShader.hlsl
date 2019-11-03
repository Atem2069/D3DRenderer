#include "../common.hlsli"

RWTexture3D<float4> outTexture : register(u0);

Texture2D albedoTex : register(t0);
SamplerState samplerState : register(s0);

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
	float3 result = (ambient + diffuse) * albedo;
	outTexture[pixelCoord] = float4(result, 1.0f);
}