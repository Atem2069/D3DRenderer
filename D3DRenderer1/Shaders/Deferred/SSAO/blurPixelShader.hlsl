#include "../../common.hlsli"
SamplerState samplerState : register(s0);
Texture2D inputTex : register(t0);

cbuffer PerFrameFlags : register(b1)
{
	FrameFlags frameFlags;
}

float4 main(VS_OUT input) : SV_TARGET
{
	float2 texelSize = 1.0 / (frameFlags.resolution);
	float result = 0.0;
	for (int x = -2; x < 2; ++x)
	{
		for (int y = -2; y < 2; ++y)
		{
			float2 offset = float2(float(x), float(y)) * texelSize;
			result += inputTex.Gather(samplerState, input.texcoord + offset);
		}
	}
	result = result / (4.0 * 4.0);
	return float4(result, result, result, 1.0f);
}