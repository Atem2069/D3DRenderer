struct VS_OUT
{
	float4 position : SV_POSITION;
	float2 texcoord : TEXCOORD0;
	matrix projection : TEXCOORD1;
};

SamplerState samplerState : register(s0);
Texture2D fragpos : register(t3);	//Only need a few textures.
Texture2D normal : register(t4);

SamplerState aoTexState : register(s2);
Texture2D noiseTex : register(t5);

cbuffer kernels : register(b2)
{
	float4 samples[64];
}

cbuffer PerFrameFlags : register(b1)
{
	int doFXAA;
	int doSSAO;
	float ssaoRadius;
};

float4 main(VS_OUT input) : SV_TARGET
{
	float4 FragPos = fragpos.Sample(samplerState,input.texcoord);
	if (!FragPos.a)
		discard;
	float3 fragPos = FragPos.xyz;
	float3 norm = normal.Sample(samplerState, input.texcoord);
	float2 randomSamplingCoords = float2(1600.0f / 64.0f, 900.0f / 64.0f);
	float3 randomVec = noiseTex.Sample(aoTexState, input.texcoord*randomSamplingCoords).rgb;
	//return float4(randomVec, 1.0f);
	float3 tangent = normalize(randomVec - norm * dot(randomVec, norm));
	float3 bitangent = cross(norm, tangent);
	float3x3 TBN = float3x3(tangent, bitangent, norm);
	TBN = transpose(TBN);

	float occlusion = 0.0;
	for (int i = 0; i < 64; ++i)
	{
		float3 m_sample = mul(TBN,samples[i].xyz);
		m_sample = fragPos + m_sample * ssaoRadius;
		float4 offset = float4(m_sample, 1.0);

		offset = mul(input.projection,offset);
		offset.xyz /= offset.w;
		offset.x = offset.x / 2 + 0.5;
		offset.y = offset.y / -2 + 0.5;
		//offset.z = offset.z / 2 + 0.5;// - big no - D3D11 does 0-1 coords.

		float sampleDepth = fragpos.Sample(samplerState, offset.xy).z;
		//float sampleDepth = mul(input.view, float4(fragpos.Sample(samplerState, offset.xy))).z;
		float rangeCheck = smoothstep(0.0, 1.0, ssaoRadius / abs(fragPos.z - sampleDepth));
		occlusion += (sampleDepth >= m_sample.z + 0.0005 ? 0.0 : 1.0) * rangeCheck;
		//occlusion += (sampleDepth >= m_sample.z + 0.025 ? 1.0 : 0.0);
	}

	occlusion = 1.0 - (occlusion / 64.0f);
	occlusion = pow(occlusion, 2);
	return float4(occlusion, occlusion, occlusion, 1.0f);
}