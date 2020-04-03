#include "..\..\common.hlsli"

cbuffer camera : register(b0)
{
	matrix projection;
	matrix view;
	matrix inverseview;
	vector campos;
	matrix voxelCam;
};

VS_OUT main(VS_INPUT input)
{
	VS_OUT output = (VS_OUT)0;
	output.position = float4(input.position, 1.0f);
	output.texcoord = input.texcoord;
	output.projection = projection;
	output.view = transpose(view);
	output.campos = campos;
	output.voxelProj = voxelCam;
	return output;
}