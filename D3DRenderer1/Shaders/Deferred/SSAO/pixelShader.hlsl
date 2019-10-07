struct VS_OUT
{
	float4 position : SV_POSITION;
	float2 texcoord : TEXCOORD0;
	matrix projection : TEXCOORD1;
	float4x4 view : NORMAL0;
};

SamplerState samplerState : register(s0);
Texture2D fragpos : register(t1);	//Only need a few textures.
Texture2D normal : register(t3);

SamplerState aoTexState : register(s1);
Texture2D noiseTex : register(t4);

cbuffer kernels : register(b2)
{
	float4 samples[32];
}

float radius = 5.0f;


float4 main(VS_OUT input) : SV_TARGET
{
	float3 fragPos = fragpos.Sample(samplerState,input.texcoord).xyz;
	fragPos = mul(input.view, float4(fragPos, 1.0f)).xyz;
	
	float3 norm = normal.Sample(samplerState, input.texcoord);
	norm = normalize(mul((float3x3)input.view, norm));
	float2 randomSamplingCoords = float2(1600.0f / 4.0f, 900.0f / 4.0f);
	float3 randomVec = noiseTex.Sample(aoTexState, input.texcoord*float2(1600.0f/4.0f,900/4.0f)).rgb;
	float3 tangent = normalize(randomVec - norm * dot(randomVec, norm));
	float3 bitangent = cross(norm, tangent);
	float3x3 TBN = float3x3(tangent, bitangent, norm);
	TBN = transpose(TBN);

	float occlusion = 0.0;
	for (int i = 0; i < 32; ++i)
	{
		float3 m_sample = mul(TBN,samples[i].xyz);
		m_sample = fragPos + m_sample * radius;
		float4 offset = float4(m_sample, 1.0);

		offset = mul(input.projection,offset);
		offset.xyz /= offset.w;
		offset.x = offset.x / 2 + 0.5;
		offset.y = offset.y / -2 + 0.5;
		//offset.z = offset.z / 2 + 0.5;// - big no - D3D11 does 0-1 coords.

		//float sampleDepth = fragpos.Sample(samplerState, offset.xy).z;
		float sampleDepth = mul(input.view, float4(fragpos.Sample(samplerState, offset.xy))).z;
		float rangeCheck = smoothstep(0.0, 1.0, radius / abs(fragPos.z - sampleDepth));
		occlusion += (sampleDepth >= m_sample.z + 0.025 ? 0.0 : 1.0) * rangeCheck;
		//occlusion += (sampleDepth >= m_sample.z + 0.025 ? 1.0 : 0.0);
	}

	occlusion = 1.0 - (occlusion / 32.0f);
	occlusion = pow(occlusion, 3);
	return float4(occlusion, occlusion, occlusion, 1.0f);
	//return float4(1.0f, 0.0f, 0.0f, 1.0f);
}