#include "../../common.hlsli"


SamplerState samplerState : register(s0);
Texture2D fragpos : register(t3);	//Only need a few textures.
Texture2D normal : register(t4);
Texture2D depth : register(t6);

SamplerState aoTexState : register(s2);
Texture2D noiseTex : register(t5);

cbuffer kernels : register(b2)
{
	float4 samples[256];
}

cbuffer PerFrameFlags : register(b1)
{
	FrameFlags frameFlags;
};

static const float near = 1.0; // projection matrix's near plane
static const float far = 2000.0; // projection matrix's far plane
float LinearizeDepth(float depth)
{
	float z = depth *2.0 - 1.0; // back to NDC (not in d3d11)
	return (2.0 * near * far) / (far + near - z * (far - near));
}

float4 main(VS_OUT input) : SV_TARGET
{
	int numSamples = frameFlags.kernelSize;
	float4 FragPos = fragpos.Sample(samplerState,input.texcoord);
	if (!normal.Sample(samplerState,input.texcoord).a)
		clip(-1);
	float3 fragPos = FragPos.xyz;
	float3 norm = normal.Sample(samplerState, input.texcoord);
	float2 randomSamplingCoords = float2(frameFlags.resolution.x / 4.0f, frameFlags.resolution.y / 4.0f);
	float3 randomVec = noiseTex.Sample(aoTexState, input.texcoord*randomSamplingCoords).rgb;
	float3 tangent = normalize(randomVec - norm * dot(randomVec, norm));
	float3 bitangent = cross(norm, tangent);
	float3x3 TBN = float3x3(tangent, bitangent, norm);

	float occlusion = 0.0;
	for (int i = 0; i < numSamples; ++i)
	{
		float3 m_sample = mul(samples[i].xyz,TBN);
		m_sample = fragPos + m_sample * frameFlags.ssaoRadius;
		float4 offset = float4(m_sample, 1.0);

		offset = mul(input.projection,offset);
		offset.xy /= offset.w;
		offset.x = offset.x / 2 + 0.5;
		offset.y = offset.y / -2 + 0.5;
		offset.z = offset.z;
		float4 sampleCoord = fragpos.Sample(samplerState, offset.xy);
		float sampleDepth = sampleCoord.z * sampleCoord.w;
		float rangeCheck = smoothstep(0.0, 1.0, frameFlags.ssaoRadius / abs(fragPos.z - sampleDepth));
		occlusion += (sampleDepth >= m_sample.z + 0.01 ? 0.0 : 1.0) *rangeCheck;
	}

	occlusion = 1.0 - (occlusion / (float)numSamples);
	occlusion = pow(occlusion, frameFlags.ssaoPower);
	return float4(occlusion, occlusion, occlusion, 1.0f);
}