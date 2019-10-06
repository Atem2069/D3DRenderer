struct VS_INPUT
{
	float3 position : POSITION;
	float3 normal : NORMAL;
	float2 texcoord : TEXCOORD;
};

struct VS_OUT
{
	float4 position : SV_POSITION;
	float2 texcoord : TEXCOORD0;
	matrix projection : TEXCOORD1;
	float4x4 view : NORMAL0;
};

cbuffer camera : register(b0)
{
	matrix projection;
	matrix view;
	vector campos;
};

VS_OUT main(VS_INPUT input)
{
	VS_OUT output;
	output.position = float4(input.position, 1.0f);
	output.texcoord = input.texcoord;
	output.projection = projection;
	output.view = view;
	return output;
}