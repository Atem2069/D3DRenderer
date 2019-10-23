#include "..\..\common.hlsli"

VS_OUT main(VS_INPUT input)
{
	VS_OUT output = (VS_OUT)0;
	output.position = float4(input.position, 1.0f);
	output.texcoord = input.texcoord;
	return output;
}