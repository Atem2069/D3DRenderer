#include "..\common.hlsli"

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
	VS_OUT output = (VS_OUT)0;
	output.position = float4(input.position, 1.0f);
	output.texcoord = input.texcoord;
	output.campos = campos;
	output.shadowCam = mul(sproj, sview);
	return output;
}