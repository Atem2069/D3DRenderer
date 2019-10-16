struct VS_OUT
{
	float4 position : SV_POSITION;
	float2 texcoord : TEXCOORD0;
};

SamplerState samplerState : register(s0);
Texture2D inputTex : register(t0);

float4 main(VS_OUT input) : SV_TARGET
{
	float2 texelSize = 1.0 / float2(1600,900);
	float result = 0.0;
	for (int x = -2; x < 2; ++x)
	{
		for (int y = -2; y < 2; ++y)
		{
			float2 offset = float2(float(x), float(y)) * texelSize;
			result += inputTex.Sample(samplerState, input.texcoord + offset).r;
		}
	}
	//return inputTex.Sample(samplerState, input.texcoord);
	result = result / (4.0 * 4.0);
	return float4(result, result, result, 1.0f);
}