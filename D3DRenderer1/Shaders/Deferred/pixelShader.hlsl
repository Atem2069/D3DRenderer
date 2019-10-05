struct VS_OUT
{
	float4 position : SV_POSITION;
	float3 normal : NORMAL0;
	float3 campos : NORMAL1;
	float3 fragpos : NORMAL2;
	float4 fragposlightspace : NORMAL3;
	float2 texCoord : TEXCOORD0;
};

struct PS_OUT
{
	float4 albedo : SV_TARGET0;
	float4 fragpos : SV_TARGET1;
	float4 fragposlightspace : SV_TARGET2;
	float4 normal : SV_TARGET3;
};

Texture2D albedoTex : register(t0);
SamplerState samplerState : register(s0);

PS_OUT main(VS_OUT input)
{
	PS_OUT output;

	output.albedo = albedoTex.Sample(samplerState, input.texCoord);
	if (output.albedo.w < 0.5f)
		discard;
	output.fragpos = float4(input.fragpos, 1.0f);
	output.fragposlightspace = input.fragposlightspace;
	float3 norm = normalize(input.normal.xyz);
	output.normal = float4(norm, 1.0f);

	return output;
}