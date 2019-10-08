
SamplerState samplerState : register(s0);
Texture2D albedoTex : register(t0);
Texture2D fragposTex : register(t1);
Texture2D normalTex : register(t2);
Texture2D AOTexture : register(t5);

Texture2D shadowTex : register(t4);
SamplerComparisonState shadowSampler : register(s1);

struct VS_OUT
{
	float4 position : SV_POSITION;
	float2 texcoord : TEXCOORD0;
	float3 campos : TEXCOORD1;
	matrix shadowCam : TEXCOORD2;
};

struct DirectionalLight
{
	float4 color;
	float4 direction;
};

cbuffer LightInformation : register(b0)
{
	DirectionalLight light;
}

cbuffer PerFrameFlags : register(b1)
{
	int doFXAA;
	int doSSAO;
};

float4 MSAAResolve(Texture2DMS<float4> inputTexture, int numSamples, uint2 pixelTexCoords)
{
	float4 result = 0.0f;
	for (int i = 0; i < numSamples; i++)
		result += inputTexture.sample[i][pixelTexCoords];	//Combine all the samples
	result /= numSamples;	//And calculate average. MSAA!
	return result;
}


#define BIAS 0.0006

float shadowCalculation(float4 fragPosLightSpace, float3 normal, float3 lightDir)
{
	float3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
	projCoords.x = projCoords.x / 2 + 0.5;
	projCoords.y = projCoords.y / -2 + 0.5;	//Flipped because Direct3D does UV flipping.
	//The Z is not transformed given that unlike OpenGL, the Z is already 0-1. No need unless you don't like shadows..

	float currentDepth = projCoords.z;

	float bias = max((BIAS * 10) * (1.0 - dot(normal, lightDir)), BIAS);

	float shadow = 0.0;
	float2 texelSize;
	shadowTex.GetDimensions(texelSize.x, texelSize.y);
	texelSize = 1.0f / texelSize;
	for (int x = -2; x <= 2; ++x)
	{
		for (int y = -2; y <= 2; ++y)
		{
			//float pcfDepth = shadowTex.Sample(shadowSampler, projCoords.xy + float2(x, y) * texelSize).r;
			//shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
			shadow += shadowTex.SampleCmpLevelZero(shadowSampler, projCoords.xy + float2(x, y)*texelSize, currentDepth - bias).r;
		}
	}
	shadow /= 25.0;
	return shadow;
}

float4 main(VS_OUT input) : SV_TARGET0
{

	float4 albedo = albedoTex.Sample(samplerState, input.texcoord);
	float4 fragpos = fragposTex.Sample(samplerState, input.texcoord);
	//float4 fragposlightspace = fragposlightspaceTex.Sample(samplerState, input.texcoord);
	float4 normal = normalTex.Sample(samplerState, input.texcoord);
	float3 AOFactor = AOTexture.Sample(samplerState, input.texcoord).rgb;
	float4 fragposlightspace = mul(input.shadowCam, float4(fragpos.xyz, 1.0f));
	if (!doSSAO)
		AOFactor = 1.0f;

	float3 ambient = 0.3f * light.color.xyz * AOFactor;

	float3 lightDir = normalize(-light.direction.xyz);
	float3 norm = normal.xyz;

	float diffuseIntensity = 2.5f;
	float diff = max(dot(norm, lightDir), 0.0f) * diffuseIntensity;
	float3 diffuse = light.color.xyz * diff;

	float3 viewDir = normalize(input.campos - fragpos.xyz);
	float3 halfwayDir = normalize(lightDir + viewDir);

	float specularIntensity = 2.5f;
	float spec = pow(max(dot(norm, halfwayDir), 0.0f), 128.0f) * specularIntensity;
	float3 specular = light.color.xyz * spec;

	float shadowFactor = shadowCalculation(fragposlightspace, norm, lightDir);

	float3 result = (ambient + (1.0 - shadowFactor) * (diffuse + specular)) * albedo.xyz;
	float4 fragColor = float4(result, 1.0f);
	float gamma = 2.2;
	fragColor.rgb = pow(fragColor.rgb, 1.0 / gamma);
	return fragColor;
}