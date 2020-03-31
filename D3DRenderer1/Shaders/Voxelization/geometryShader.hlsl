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

	// Expand triangle to get fake Conservative Rasterization:
	float2 side0N = normalize(output[1].position.xy - output[0].position.xy);
	float2 side1N = normalize(output[2].position.xy - output[1].position.xy);
	float2 side2N = normalize(output[0].position.xy - output[2].position.xy);
	output[0].position.xy += normalize(side2N - side0N);
	output[1].position.xy += normalize(side0N - side1N);
	output[2].position.xy += normalize(side1N - side2N);

	outputStream.RestartStrip();
}