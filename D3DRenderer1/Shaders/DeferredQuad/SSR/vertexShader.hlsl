struct VS_IN
{
	float3 position : POSITION;
	float3 normal : NORMAL;
	float2 texcoord : TEXCOORD;
};


struct VS_OUT
{
	float4 position : SV_POSITION;
	float2 TexCoords : TEXCOORD0;
	float4x4 projection : TEXCOORD1;
	float4x4 view : TEXCOORD5;
};

cbuffer camera : register(b0)
{
	matrix projection;
	matrix view;
	matrix inverseview;
	vector campos;
};

VS_OUT main(VS_IN input)
{
	VS_OUT output;
	output.position = float4(input.position, 1.0f);
	output.TexCoords = input.texcoord;
	output.projection = projection;
	output.view = transpose(view);
	return output;
}