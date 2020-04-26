#include "../common.hlsli"

[maxvertexcount(3)]
void main(triangle VS_OUT input[3], inout TriangleStream<VS_OUT> outputStream)
{
	VS_OUT output[3];
	output[0] = (VS_OUT)0;
	output[1] = (VS_OUT)0;
	output[2] = (VS_OUT)0;
	float3 facenormal = abs(input[0].normal + input[1].normal + input[2].normal);
	uint maxi = facenormal[1] > facenormal[0] ? 1 : 0;
	maxi = facenormal[2] > facenormal[maxi] ? 2 : maxi;

	for (uint i = 0; i < 3; i++)
	{
		output[i].position = input[i].position;
		output[i].normal = input[i].normal;
		output[i].texcoord = input[i].texcoord;
		output[i].fragpos = input[i].fragpos.xyz;
		output[i].fragposviewspace = input[i].fragposviewspace;
		output[i].shadowCam = input[i].shadowCam;
		output[i].position.xyz = (output[i].position.xyz) * VOXELSIZE;
		output[i].campos = input[i].campos;
		if (maxi == 0)
		{
			output[i].position.xyz = output[i].position.zyx;
		}
		else if (maxi == 1)
		{
			output[i].position.xyz = output[i].position.xzy;
		}
	}

	// Expand triangle to get fake Conservative Rasterization:
	float2 side0N = normalize(output[1].position.xy - output[0].position.xy);
	float2 side1N = normalize(output[2].position.xy - output[1].position.xy);
	float2 side2N = normalize(output[0].position.xy - output[2].position.xy);
	output[0].position.xy += normalize(side2N - side0N);
	output[1].position.xy += normalize(side0N - side1N);
	output[2].position.xy += normalize(side1N - side2N);

	for (uint i = 0; i < 3; i++)
	{
		output[i].position.xy *= 1.0f / VOXELSIZE;
		output[i].position.zw = 1;
		outputStream.Append(output[i]);
	}

	outputStream.RestartStrip();
}