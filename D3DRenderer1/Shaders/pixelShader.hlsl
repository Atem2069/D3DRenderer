struct VS_OUT
{
	float4 position : SV_POSITION;
    float3 normal : NORMAL0;
	float3 campos : NORMAL1;
	float3 fragpos : NORMAL2;
	float4 fragposlightspace : NORMAL3;
	float2 texCoord : TEXCOORD0;
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

Texture2D albedoTex : register(t0);
SamplerState samplerState : register(s0);

Texture2D shadowTex : register(t1);
SamplerComparisonState shadowSampler : register(s1);

float shadowCalculation(float4 fragPosLightSpace, float3 normal, float3 lightDir)
{
	float3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
	projCoords.x = projCoords.x / 2 + 0.5;
	projCoords.y = projCoords.y / -2 + 0.5;	//Flipped because Direct3D does UV flipping.
	//The Z is not transformed given that unlike OpenGL, the Z is already 0-1. No need unless you don't like shadows..

	float currentDepth = projCoords.z;

	float bias = max(0.005f * (1.0 - dot(normal, lightDir)), 0.0005f);

	float shadow = 0.0;
	float2 texelSize;
	shadowTex.GetDimensions(texelSize.x,texelSize.y);
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

float4 main(VS_OUT input) : SV_TARGET
{

	float4 texColor = albedoTex.Sample(samplerState,input.texCoord);
	float avgTexColor = (texColor.r + texColor.g + texColor.b + texColor.a) / 4.0;
	texColor = (avgTexColor == 0) ? 1.0f : texColor;

	float3 ambient = 0.1f * light.color.xyz;

	float3 lightDir = normalize(-light.direction.xyz);
	float3 norm = normalize(input.normal.xyz);

	float diffuseIntensity = 1.0f;
	float diff = max(dot(norm, lightDir), 0.0f) * diffuseIntensity;
	float3 diffuse = light.color.xyz * diff;

	float3 viewDir = normalize(input.campos - input.fragpos);
	float3 halfwayDir = normalize(lightDir + viewDir);

	float specularIntensity = 1.0f;
	float spec = pow(max(dot(norm, halfwayDir), 0.0f), 64.0f) * specularIntensity;
	float3 specular = light.color.xyz * spec;

	float shadow = shadowCalculation(input.fragposlightspace,norm,lightDir);
	float3 result = (ambient + (1.0 - shadow) * (diffuse + specular)) * texColor;
	return float4(result, 1.0f);
}