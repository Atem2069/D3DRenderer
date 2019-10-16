struct VS_OUT
{
	float4 position : SV_POSITION;
	float2 texcoord : TEXCOORD0;
	matrix projection : TEXCOORD1;
	matrix view : TEXCOORD5;
};

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
	int doFXAA;
	int doSSAO;
	int doSSR;
	float ssaoRadius;
	int kernelSize;
	int ssaoPower;
};

float hash(float n)
{
	return frac(sin(n)*43758.5453);
}

float noise(float3 x)
{
	// The noise function returns a value in the range -1.0f -> 1.0f

	float3 p = floor(x);
	float3 f = frac(x);

	f = f * f*(3.0 - 2.0*f);
	float n = p.x + p.y*57.0 + 113.0*p.z;

	return lerp(lerp(lerp(hash(n + 0.0), hash(n + 1.0), f.x),
		lerp(hash(n + 57.0), hash(n + 58.0), f.x), f.y),
		lerp(lerp(hash(n + 113.0), hash(n + 114.0), f.x),
			lerp(hash(n + 170.0), hash(n + 171.0), f.x), f.y), f.z);
}

static const float near = 1.0; // projection matrix's near plane
static const float far = 10000.0; // projection matrix's far plane
float LinearizeDepth(float depth)
{
	//float z = depth * 2.0 - 1.0; // back to NDC (not in d3d11)
	float z = depth;
	return (2.0 * near * far) / (far + near - z * (far - near));
}

float4 main(VS_OUT input) : SV_TARGET
{
	int numSamples = kernelSize;
	float4 FragPos = fragpos.Sample(samplerState,input.texcoord);
	if (!FragPos.a)
		clip(-1);
	float3 fragPos = FragPos.xyz;
	float3 norm = normal.Sample(samplerState, input.texcoord);
	float2 randomSamplingCoords = float2(1600.0f / 4.0f, 900.0f / 4.0f);
	float3 randomVec = noiseTex.Sample(aoTexState, input.texcoord*randomSamplingCoords).rgb;
	//float3 randomVec = float3(noise(float3(input.texcoord,0.0f)), noise(float3(input.texcoord*randomSamplingCoords,1.0f)), 0.0f);
	//return float4(randomVec, 1.0f);
	float3 tangent = normalize(randomVec - norm * dot(randomVec, norm));
	float3 bitangent = cross(norm, tangent);
	float3x3 TBN = float3x3(tangent, bitangent, norm);

	float occlusion = 0.0;
	for (int i = 0; i < numSamples; ++i)
	{
		float3 m_sample = mul(samples[i].xyz,TBN);
		m_sample = fragPos + m_sample * ssaoRadius;
		float4 offset = float4(m_sample, 1.0);

		offset = mul(input.projection,offset);
		offset.xy /= offset.w;
		offset.x = offset.x / 2 + 0.5;
		offset.y = offset.y / -2 + 0.5;
		//offset.z = offset.z / 2 + 0.5;// - big no - D3D11 does 0-1 coords.

		float sampleDepth = fragpos.Sample(samplerState, offset.xy).z;
		//float sampleDepth = depth.Sample(samplerState, offset.xy);
		//sampleDepth = LinearizeDepth(sampleDepth);
		//float sampleDepth = mul(input.view, float4(fragpos.Sample(samplerState, offset.xy))).z;
		float rangeCheck = smoothstep(0.0, 1.0, ssaoRadius / abs(fragPos.z - sampleDepth));
		occlusion += (sampleDepth >= m_sample.z + 0.025 ? 0.0 : 1.0) * rangeCheck;
		//occlusion += (sampleDepth >= m_sample.z + 0.025 ? 1.0 : 0.0);
	}

	occlusion = 1.0 - (occlusion / (float)numSamples);
	occlusion = pow(occlusion, ssaoPower);
	return float4(occlusion, occlusion, occlusion, 1.0f);
}