
SamplerState samplerState : register(s0);
Texture2DMS<float4> resultTex : register(t0);

struct VS_OUT
{
	float4 position : SV_POSITION;
	float2 texcoord : TEXCOORD0;
};

float4 main(VS_OUT input) : SV_TARGET0
{

	uint3 newTexCoords;
	resultTex.GetDimensions(newTexCoords.x, newTexCoords.y, newTexCoords.z);
	newTexCoords.x = input.texcoord.x * newTexCoords.x;
	newTexCoords.y = input.texcoord.y * newTexCoords.y;

	float4 albedo = 0.0f;
	for (int i = 0; i < newTexCoords.z; i++)
		albedo += resultTex.sample[i][newTexCoords.xy];	//Combine all the samples
	albedo /= newTexCoords.z;	//And calculate average. MSAA!
	return albedo;
}