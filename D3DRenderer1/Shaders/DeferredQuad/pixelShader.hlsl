#include "..\common.hlsli"
#include "..\voxelConeTracing.hlsli"
SamplerState samplerState : register(s0);
Texture2D albedoTex : register(t0);
Texture2D fragposTex : register(t1);
Texture2D normalTex : register(t2);
Texture2D AOTexture : register(t5);

Texture2D shadowTex : register(t4);
SamplerComparisonState shadowSampler : register(s1);

SamplerState voxelSampler : register(s2);
Texture3D voxelTex : register(t6);

struct DirectionalLight
{
	float4 color;
	float4 direction;
	float specularPower;
};

cbuffer LightInformation : register(b0)
{
	DirectionalLight light;
}

cbuffer PerFrameFlags : register(b1)
{
	FrameFlags frameFlags;
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

	float bias = max((BIAS*10) * (1.0 - dot(normal, lightDir)), BIAS);

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


///Voxel tracing
float4 voxelRayMarch(float3 o, float3 d, float4x4 voxelProj)
{
	float4 res = float4(0, 0, 0, 0);
	static int MAX_STEPS = 500;
	[loop]
	for (int i = 0; i < MAX_STEPS; i++)
	{
		float3 currentCoords = mul(voxelProj, float4(o.xyz, 1.f)).xyz;	//Multiply out reflection point to voxel space from world space
		currentCoords = scaleAndBias(currentCoords);					//Scale from -n -> n to 0 -> n
		res = voxelTex.Sample(voxelSampler, currentCoords.xyz);
		if (res.a > 0)
			return res;
		float camScale = voxelProj[0][0];	//[0][0] = 2/(right-left)
		camScale /= 2.f;					//Divide by 2, now = 1/(right-left)
		camScale = 1 / camScale;			//Flip to get (right-left)/1 - which is real scale
		o += d * (camScale / 512.f);		//Get the ratio of camera scale to voxel scale in order to stop seams and banding from happening.
	}
	return res;
	//return float4(0,0,0,0);	//Zero contribution if raymarch doesn't return in 2000 iters
}

inline bool is_saturated(float a) { return a == saturate(a); }
inline bool is_saturated(float2 a) { return is_saturated(a.x) && is_saturated(a.y); }
inline bool is_saturated(float3 a) { return is_saturated(a.x) && is_saturated(a.y) && is_saturated(a.z); }
inline bool is_saturated(float4 a) { return is_saturated(a.x) && is_saturated(a.y) && is_saturated(a.z) && is_saturated(a.w); }

float4 voxelConeTrace(float3 o, float3 d, float coneAperture, int iterations, float4x4 voxelProj)
{
	float distanceTravelled = 0.0f;
	float4 res = float4(0, 0, 0, 0);
	int MAX_STEPS = iterations;
	float voxSize = VOXELSIZE;
	float maximumMIPLevel = log2(voxSize);

	float camScale = voxelProj[0][0];	//[0][0] = 2/(right-left)
	camScale /= 2.f;					//Divide by 2, now = 1/(right-left)
	camScale = 1 / camScale;			//Flip to get (right-left)/1 - which is real scale

	[loop]
	for (int i = 0; i < MAX_STEPS; i++)
	{
		float diameter = max(1, coneAperture*distanceTravelled);
		//float mipLevel = min(maximumMIPLevel, log2(distanceTravelled * coneAperture));
		float mipLevel = log2(diameter);
		float3 currentCoords = mul(voxelProj, float4(o.xyz, 1.f)).xyz;	//Multiply out reflection point to voxel space from world space
		currentCoords = scaleAndBias(currentCoords);					//Scale from -n -> n to 0 -> n
		float4 result = voxelTex.SampleLevel(voxelSampler, currentCoords.xyz, mipLevel);
		float alpha = 1 - res.a;
		res.xyz += result.xyz * alpha;
		res.a += alpha * result.a;
		if (mipLevel > maximumMIPLevel || !is_saturated(currentCoords))
			break;
		o += d * (camScale / voxSize);		//Get the ratio of camera scale to voxel scale in order to stop seams and banding from happening.
		distanceTravelled += (camScale / voxSize);
	}
	return res;
	//return float4(0,0,0,0);	//Zero contribution if raymarch doesn't return in 2000 iters
}

static const float3 CONES[] =
{
	float3(0.57735, 0.57735, 0.57735),
	float3(0.57735, -0.57735, -0.57735),
	float3(-0.57735, 0.57735, -0.57735),
	float3(-0.57735, -0.57735, 0.57735),
	float3(-0.903007, -0.182696, -0.388844),
	float3(-0.903007, 0.182696, 0.388844),
	float3(0.903007, -0.182696, 0.388844),
	float3(0.903007, 0.182696, -0.388844),
	float3(-0.388844, -0.903007, -0.182696),
	float3(0.388844, -0.903007, 0.182696),
	float3(0.388844, 0.903007, -0.182696),
	float3(-0.388844, 0.903007, 0.182696),
	float3(-0.182696, -0.388844, -0.903007),
	float3(0.182696, 0.388844, -0.903007),
	float3(-0.182696, 0.388844, 0.903007),
	float3(0.182696, -0.388844, 0.903007)
};



float4 voxelConeTraceRadiance(float3 P, float3 N, float4x4 voxelProj)
{
	float4 radiance = 0.0f;
	for (int i = 0; i < frameFlags.coneCount; i++)
	{
		float3 coneDirection = normalize(CONES[i] + N);
		coneDirection *= (dot(coneDirection, N) < 0 ? -1 : 1);
		P += N * 1.5f;
		radiance += voxelConeTrace(P, coneDirection, tan(3.1415926535*0.33f), 500, voxelProj);
	}

	radiance /= frameFlags.coneCount;
	//radiance.a = saturate(radiance.a);
	return max(0, radiance);
}

float3 rayDirection(float fieldOfView, float2 size, float2 fragCoord)
{
	float2 xy = fragCoord - size / 2.0;
	float z = size.y / tan(radians(fieldOfView) / 2.0);
	return normalize(float3(xy, -z));
}

float3 ComputeF0(in float4 baseColor, in float reflectance, in float metalness)
{
	return lerp(lerp(float3(0, 0, 0), float3(1, 1, 1), reflectance), baseColor.rgb, metalness);
}

float DistributionGGX(float3 N, float3 H, float roughness);
float GeometrySchlickGGX(float NdotV, float roughness);
float GeometrySmith(float3 N, float3 V, float3 L, float roughness);
float3 fresnelSchlick(float cosTheta, float3 F0);


float4 main(VS_OUT input) : SV_TARGET0
{

	float4 albedo = albedoTex.Sample(samplerState, input.texcoord);
	float4 fragpos = fragposTex.Sample(samplerState, input.texcoord);
	//float4 fragposlightspace = fragposlightspaceTex.Sample(samplerState, input.texcoord);
	float4 normal = normalTex.Sample(samplerState, input.texcoord);
	float3 AOFactor = AOTexture.Sample(samplerState, input.texcoord).rgb;
	float4 fragposlightspace = mul(input.shadowCam, float4(fragpos.xyz, 1.0f));
	if (!frameFlags.doSSAO)
		AOFactor = 1.0f;
	
	/*float3 ambient = 0.2f * light.color.xyz * AOFactor;

	float3 lightDir = normalize(-light.direction.xyz);
	float3 norm = normal.xyz;

	float diffuseIntensity = 1.0f * AOFactor;
	float diff = max(dot(norm, lightDir), 0.0f) * diffuseIntensity;
	float3 diffuse = light.color.xyz * diff;

	float3 viewDir = normalize(input.campos - fragpos.xyz);
	float3 halfwayDir = normalize(lightDir + viewDir);

	float specularIntensity = 1.5f * albedo.w;
	float spec = pow(max(dot(norm, halfwayDir), 0.0f), light.specularPower) * specularIntensity;
	float3 specular = light.color.xyz * spec;

	float shadowFactor = shadowCalculation(fragposlightspace, norm, lightDir);
	float3 result = (ambient + (1.0 - shadowFactor) * (diffuse + specular)) * albedo.xyz;
	float4 fragColor = float4(result, 1.0f);*/

	//pbr test
	float roughness = 0.2f;
	float3 F0 = 0.04;
	F0 = lerp(F0, albedo.xyz, albedo.w);
	float3 Lo = 0.0f;
	// calculate per-light radiance
	float3 V = normalize(input.campos.xyz - fragpos.xyz);
	float3 L = normalize(-light.direction.xyz);
	float3 H = normalize(V + L);
	float3 radiance = light.color.xyz * 20.f;

	// cook-torrance brdf
	float NDF = DistributionGGX(normal.xyz, H, roughness);
	float G = GeometrySmith(normal.xyz, V, L, roughness);
	float3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

	float3 kS = F;
	float3 kD = 1.0 - kS;
	kD *= 1.0 - albedo.w;

	float3 numerator = NDF * G * F;
	float denominator = 4.0 * max(dot(normal.xyz, V), 0.0) * max(dot(normal.xyz, L), 0.0);
	float3 specular = numerator / max(denominator, 0.001);

	// add to outgoing radiance Lo
	float NdotL = max(dot(normal.xyz, L), 0.0);
	Lo += (kD * albedo.xyz / 3.1415926 + specular) * radiance * NdotL;

	float3 ambient = 0.1f * albedo.xyz * AOFactor;

	float shadowFactor = shadowCalculation(fragposlightspace, normal.xyz, L);
	float3 color = (ambient + (1.0-shadowFactor) * Lo);

	float4 fragColor = float4(color, 1.0f);
	//Voxel cone tracing
	if (frameFlags.doVoxelReflections==1)
	{
		//redundant vars should be cleaned up for further optimization
		float4 voxelPos = float4(fragpos.xyz, 1.0f);
		float3 voxelViewDir = (input.campos.xyz - voxelPos.xyz);

		float3 reflectDir = normalize(reflect(-voxelViewDir, normal.xyz));
		//voxelPos.xyz += reflectDir * (normal+10.f);	//Offset voxel position to avoid self reflection
		voxelPos.xyz += normal * 1.75f;
		float coneAperture = tan((1-albedo.a)*3.1415926535*0.5*0.1);
		float4 voxelRes = voxelConeTrace(voxelPos.xyz, reflectDir, coneAperture,1000,input.voxelProj);
		fragColor.xyz += voxelRes.xyz * ComputeF0(voxelRes, albedo.a, frameFlags.ssrMetallic);
		if (frameFlags.voxelDebug == 1)
		{
			return voxelRes;
			float3 rayDir = rayDirection(120.0f, float2(1600, -900), input.texcoord * float2(1600, -900));	//Preserve x though y is reversed due to d3d uvmapping
			float3 viewDir = mul(float4(rayDir, 1.f), input.view).xyz;	//converting to viewspace
			fragColor = voxelConeTrace(input.campos.xyz, viewDir, tan(3.1415926535*0.01*0.5), 1000, input.voxelProj);
			//voxelPos = mul(input.voxelProj, voxelPos);
			//voxelPos.xyz = scaleAndBias(voxelPos.xyz);
			//fragColor = voxelTex.Sample(voxelSampler, voxelPos);
		}
	}
	if (frameFlags.doVoxelGI == 1)
	{
		float4 radiance = voxelConeTraceRadiance(fragpos.xyz, normalize(normal.xyz),input.voxelProj);
		if (frameFlags.voxelDebug == 1)
			return radiance;
		//return radiance;
		fragColor.xyz += (radiance.xyz) * (albedo.xyz + 0.01f);
	}

	float gamma = 2.2;
	fragColor.rgb = pow(fragColor.rgb, 1.0 / gamma);

	return fragColor;
}

float3 fresnelSchlick(float cosTheta, float3 F0)
{
	return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

float DistributionGGX(float3 N, float3 H, float roughness)
{
	float a = roughness * roughness;
	float a2 = a * a;
	float NdotH = max(dot(N, H), 0.0);
	float NdotH2 = NdotH * NdotH;

	float num = a2;
	float denom = (NdotH2 * (a2 - 1.0) + 1.0);
	denom = 3.1415926535 * denom * denom;

	return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
	float r = (roughness + 1.0);
	float k = (r*r) / 8.0;

	float num = NdotV;
	float denom = NdotV * (1.0 - k) + k;

	return num / denom;
}
float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
	float NdotV = max(dot(N, V), 0.0);
	float NdotL = max(dot(N, L), 0.0);
	float ggx2 = GeometrySchlickGGX(NdotV, roughness);
	float ggx1 = GeometrySchlickGGX(NdotL, roughness);

	return ggx1 * ggx2;
}