#include "..\common.hlsli"

cbuffer shadowCamera : register(b0)
{
	matrix shadowProj;
	matrix shadowView;
	vector campos;
	matrix orthovoxel;
}

cbuffer object : register(b1)
{
	matrix model;
	matrix inversemodel;
}



VS_OUT main(VS_INPUT input)
{
	VS_OUT output = (VS_OUT)0;
	matrix transMatrix = mul(shadowProj, shadowView);
	transMatrix = mul(transMatrix, model);
	output.position = mul(transMatrix, float4(input.position, 1.0f));
	return output;
}