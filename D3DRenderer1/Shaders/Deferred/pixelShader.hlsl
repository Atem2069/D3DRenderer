#include "..\common.hlsli"

struct PS_OUT
{
	float4 albedo : SV_TARGET0;
	float4 fragpos : SV_TARGET1;
	float4 normal : SV_TARGET2;
	float4 fragposviewspace : SV_TARGET3;
	float4 normalviewspace : SV_TARGET4;
};

cbuffer PerFrameFlags : register(b1)
{
	FrameFlags frameFlags;
};

Texture2D albedoTex : register(t0);
Texture2D specularMap : register(t1);
SamplerState samplerState : register(s0);

PS_OUT main(VS_OUT input)
{
	PS_OUT output;
	int width, height;
	albedoTex.GetDimensions(width, height);
	output.albedo = albedoTex.Sample(samplerState, input.texcoord);
	if (output.albedo.w < 0.25f && width>0)
		discard;
	if (!width || !frameFlags.doTexturing)
		output.albedo = 1;
	output.albedo.w = specularMap.Sample(samplerState, input.texcoord).r; //alpha component of albedo map is the specular color.
	specularMap.GetDimensions(width, height);
	if (!width)	//If no spec tex is loaded..
		output.albedo.w = frameFlags.ssrReflectiveness;
	output.fragpos = float4(input.fragpos, 1.0f);
	float3 norm = normalize(input.normal.xyz);
	output.normal = float4(norm, 1.0f);
	output.fragposviewspace = input.fragposviewspace;
	output.normalviewspace = float4(normalize(input.normalviewspace), 1.0f);
	return output;
}