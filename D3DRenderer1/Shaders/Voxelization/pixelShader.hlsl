#include "../common.hlsli"

RWTexture3D<float4> outTexture : register(u0);

Texture2D albedoTex : register(t0);
SamplerState samplerState : register(s0);

SamplerComparisonState shadowSamplerState : register(s1);
Texture2D shadowTex : register(t4);

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

#define BIAS 0.0006

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

inline bool is_saturated(float a) { return a == saturate(a); }
inline bool is_saturated(float2 a) { return is_saturated(a.x) && is_saturated(a.y); }
inline bool is_saturated(float3 a) { return is_saturated(a.x) && is_saturated(a.y) && is_saturated(a.z); }
inline bool is_saturated(float4 a) { return is_saturated(a.x) && is_saturated(a.y) && is_saturated(a.z) && is_saturated(a.w); }

float DistributionGGX(float3 N, float3 H, float roughness);
float GeometrySchlickGGX(float NdotV, float roughness);
float GeometrySmith(float3 N, float3 V, float3 L, float roughness);
float3 fresnelSchlick(float cosTheta, float3 F0);

void main(VS_OUT input)
{
	if (!is_saturated(scaleAndBias(input.fragpos.xyz))) return;
	float4 albedocol = albedoTex.Sample(samplerState, input.texcoord).xyzw;
	float3 albedo = albedocol.xyzw;
	int width, height;
	albedoTex.GetDimensions(width, height);
	if (!width)
		albedo = 1.f;
	if (albedocol.w < 0.25f && width>0)
		return;
	albedocol.w = 0.0f;
	float3 voxel = scaleAndBias(input.fragpos.xyz);
	float3 dimensions = VOXELSIZE;	//Hardcoded for now
	float3 pixelCoord = voxel * dimensions;

	/*//Calculating basic light (ambient+diffuse term)
	float3 ambient = 0.2f * light.color.xyz;
	float3 norm = normalize(input.normal.xyz);
	float3 lightDir = normalize(-light.direction.xyz);

	float diffuse = max(dot(norm, lightDir), 0.0f) * 1.0f;

	float4 fragposlightspace = mul(input.shadowCam, float4(input.fragposviewspace.xyz, 1.0f));
	float shadowFactor = shadowCalculation(fragposlightspace, norm, lightDir);
	//float3 result = (ambient + diffuse) * albedo;
	float3 result = (ambient + (1.0 - shadowFactor) * (diffuse)) * albedo.xyz;*/

	//pbr test
	input.normal.xyz = normalize(input.normal.xyz);
	float roughness = 0.2f;
	float3 F0 = 0.04;
	F0 = lerp(F0, albedocol.xyz, albedocol.w);
	float3 Lo = 0.0f;
	// calculate per-light radiance
	float3 V = normalize(input.campos.xyz - input.fragposviewspace.xyz);
	float3 L = normalize(-light.direction.xyz);
	float3 H = normalize(V + L);
	float3 radiance = light.color.xyz * 20.f;

	// cook-torrance brdf
	float NDF = DistributionGGX(input.normal.xyz, H, roughness);
	float G = GeometrySmith(input.normal.xyz, V, L, roughness);
	float3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

	float3 kS = F;
	float3 kD = 1.0 - kS;
	kD *= 1.0 - albedocol.w;

	float3 numerator = NDF * G * F;
	float denominator = 4.0 * max(dot(input.normal.xyz, V), 0.0) * max(dot(input.normal.xyz, L), 0.0);
	float3 specular = numerator / max(denominator, 0.001);
	// add to outgoing radiance Lo
	float NdotL = max(dot(input.normal.xyz, L), 0.0);
	Lo += (kD * albedo.xyz / 3.1415926 + specular) * radiance * NdotL;

	float3 ambient = 0.1f * albedo.xyz;
	float4 fragposlightspace = mul(input.shadowCam, float4(input.fragposviewspace.xyz, 1.0f));
	float shadowFactor = shadowCalculation(fragposlightspace, input.normal.xyz, L);
	float3 result = (ambient + (1.0 - shadowFactor) * Lo);

	outTexture[pixelCoord] = float4(result, 1.0f);
}

float3 fresnelSchlick(float cosTheta, float3 F0)
{
	return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

float DistributionGGX(float3 N, float3 H, float roughness)
{
	float a = roughness * roughness;
	float a2 = a * a;
	float NdotH = max(dot(N, H), 0.0);
	float NdotH2 = NdotH * NdotH;

	float num = a2;
	float denom = (NdotH2 * (a2 - 1.0) + 1.0);
	denom = 3.1415926535 * denom * denom;

	return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
	float r = (roughness + 1.0);
	float k = (r*r) / 8.0;

	float num = NdotV;
	float denom = NdotV * (1.0 - k) + k;

	return num / denom;
}
float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
	float NdotV = max(dot(N, V), 0.0);
	float NdotL = max(dot(N, L), 0.0);
	float ggx2 = GeometrySchlickGGX(NdotV, roughness);
	float ggx1 = GeometrySchlickGGX(NdotL, roughness);

	return ggx1 * ggx2;
}