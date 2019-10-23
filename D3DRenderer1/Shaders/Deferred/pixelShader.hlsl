struct VS_OUT
{
	float4 position : SV_POSITION;
	float3 normal : NORMAL0;
	float3 campos : NORMAL1;
	float3 fragpos : NORMAL2;
	float4 fragposviewspace : NORMAL3;
	float3 normalviewspace : NORMAL4;
	float2 texCoord : TEXCOORD0;
};

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
	int doFXAA;
	int doSSAO;
	int doSSR;
	int doTexturing;
	float ssaoRadius;
	int kernelSize;
	int ssaoPower;
};

Texture2D albedoTex : register(t0);
Texture2D normalMap : register(t1);
SamplerState samplerState : register(s0);

PS_OUT main(VS_OUT input)
{
	PS_OUT output;
	int width, height;
	albedoTex.GetDimensions(width, height);
	output.albedo = albedoTex.Sample(samplerState, input.texCoord);
	if (output.albedo.w < 0.5f && width>0)
		clip(-1);
	if (!width || !doTexturing)
		output.albedo = 1;
	output.fragpos = float4(input.fragpos, 1.0f);
	float3 norm = normalize(input.normal.xyz);
	output.normal = float4(norm, 1.0f);
	output.fragposviewspace = input.fragposviewspace;
	output.normalviewspace = float4(normalize(input.normalviewspace), 1.0f);
	return output;
}