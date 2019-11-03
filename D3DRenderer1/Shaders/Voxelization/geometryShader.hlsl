#include "../common.hlsli"

[maxvertexcount(3)]
void main(triangle VS_OUT input[3], inout TriangleStream<VS_OUT> outputStream)
{
	VS_OUT output[3];
	float3 facenormal = abs(input[0].normal + input[1].normal + input[2].normal);
	uint maxi = facenormal[1] > facenormal[0] ? 1 : 0;
	maxi = facenormal[2] > facenormal[maxi] ? 2 : maxi;

	for (uint i = 0; i < 3; i++)
	{
		output[i].position = input[i].position;
		output[i].normal = input[i].normal;
		output[i].texcoord = input[i].texcoord;
		if (maxi == 0)
		{
			output[i].position.xyz = output[i].position.zyx;
		}
		else if (maxi == 1)
		{
			output[i].position.xyz = output[i].position.xzy;
		}
		output[i].fragpos = output[i].position.xyz;
		outputStream.Append(input[i]);
	}

	outputStream.RestartStrip();
}