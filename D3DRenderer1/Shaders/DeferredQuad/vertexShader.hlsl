struct VS_INPUT
{
	float3 position : POSITION;
	float3 normal : NORMAL;
	float3 texcoord : TEXCOORD;
};

struct VS_OUT
{
	float4 position : SV_POSITION;
	float2 texcoord : TEXCOORD0;
	float3 campos : TEXCOORD1;
	matrix shadowCam : TEXCOORD2;
};


cbuffer camera : register(b0)
{
	matrix projection;
	matrix view;
	vector campos;
}

cbuffer shadowCamera : register(b2)
{
	matrix sproj;
	matrix sview;
}

VS_OUT main(VS_INPUT input)
{
	VS_OUT output;
	output.position = float4(input.position, 1.0f);
	output.texcoord = input.texcoord;
	output.campos = campos;
	output.shadowCam = mul(sproj, sview);
	return output;
}