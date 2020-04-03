#include "../common.hlsli"

cbuffer camera : register(b0)
{
	matrix projection;
	matrix view;
	vector campos;
	matrix orthovoxel;
};

cbuffer object : register(b1)
{
	matrix model;
	matrix inverseModel;
}

cbuffer shadowCamera : register(b2)
{
	matrix sproj;
	matrix sview;
}

VS_OUT main(VS_INPUT input)
{
	VS_OUT output = (VS_OUT)0;
	float4x4 transformation = mul(orthovoxel, model);
	output.position = mul(transformation, float4(input.position, 1.0f));
	output.fragpos = output.position.xyz;
	output.fragposviewspace.xyz = input.position.xyz;
	output.normal = mul((float3x3)inverseModel,input.normal);
	output.texcoord = input.texcoord;
	output.shadowCam = mul(sproj, sview);
	return output;
}