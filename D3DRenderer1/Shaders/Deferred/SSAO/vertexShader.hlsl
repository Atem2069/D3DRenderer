struct VS_INPUT
{
	float3 position : POSITION;
	float3 normal : NORMAL;
	float2 texcoord : TEXCOORD;
	float3 tangent : TANGENT;
	float3 bitangent : BITANGENT;
};

struct VS_OUT
{
	float4 position : SV_POSITION;
	float2 texcoord : TEXCOORD0;
	matrix projection : TEXCOORD1;
	matrix view : TEXCOORD5;
};

cbuffer camera : register(b0)
{
	matrix projection;
	matrix view;
	matrix inverseview;
	vector campos;
};

VS_OUT main(VS_INPUT input)
{
	VS_OUT output;
	output.position = float4(input.position, 1.0f);
	output.texcoord = input.texcoord;
	output.projection = projection;
	output.view = transpose(view);
	return output;
}