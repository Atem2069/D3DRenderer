
SamplerState samplerState : register(s0);
Texture2DMS<float4> resultTex : register(t0);

struct VS_OUT
{
	float4 position : SV_POSITION;
	float2 texcoord : TEXCOORD0;
};

float4 MSAAResolve(Texture2DMS<float4> inputTexture, int numSamples, uint2 pixelTexCoords)
{
	float4 result = 0.0f;
	for (int i = 0; i < numSamples; i++)
		result += inputTexture.sample[i][pixelTexCoords];	//Combine all the samples
	result /= numSamples;	//And calculate average. MSAA!
	return result;
}

float4 main(VS_OUT input) : SV_TARGET0
{

	uint3 newTexCoords;
	resultTex.GetDimensions(newTexCoords.x, newTexCoords.y, newTexCoords.z);
	newTexCoords.x = input.texcoord.x * newTexCoords.x;
	newTexCoords.y = input.texcoord.y * newTexCoords.y;
	
	float4 albedo = MSAAResolve(resultTex, newTexCoords.z, newTexCoords.xy);
	return albedo;
}