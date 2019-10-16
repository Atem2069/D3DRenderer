SamplerState samplerState : register(s0);
Texture2D gFinalImage : register(t5);
Texture2D gPosition : register(t3);
Texture2D gNormal: register(t4);
Texture2D ColorBuffer : register(t0);
Texture2D linearDepth : register(t6);

//uniform mat4 invView;
//uniform mat4 projection;
//uniform mat4 invprojection;
//uniform mat4 view;

struct VS_OUT
{
	float4 position : SV_POSITION;
	float2 TexCoords : TEXCOORD0;
	float4x4 projection : TEXCOORD1;
	float4x4 view : TEXCOORD5;
};

cbuffer PerFrameFlags : register(b1)
{
	int doFXAA;
	int doSSAO;
	int doSSR;
	float ssaoRadius;
	int kernelSize;
	int ssaoPower;
	uint coarseStepCount;
	float coarseStepIncrease;
	uint fineStepCount;
	float tolerance;
};



//static const uint	coarseStepCount = 64; // primary ray march step count (higher will find more in distance, but slower)
//static const float	coarseStepIncrease = 20.0f; // primary ray march step increase (higher will travel more distance, but can miss details)
//static const uint	fineStepCount = 64;	// binary step count (higher is nicer but slower)
//static const float  tolerance = 0.0009f; // early exit factor for binary search (smaller is nicer but slower)

static const float near = 1.0f; // projection matrix's near plane
static const float far = 20000.0f; // projection matrix's far plane (2x near plane)
float LinearizeDepth(float depth)
{
	float z = depth * 2.0 - 1.0; // back to NDC (not in d3d11)
	return (2.0 * near * far) / (far + near - z * (far - near));
}

float4 SSRBinarySearch(in float3 origin, in float3 direction, float4x4 projection)
{
	float fDepth;

	for (int i = 0; i < fineStepCount; i++)
	{
		float4 vProjectedCoord = mul(projection,float4(origin, 1.0f));
		vProjectedCoord.xy /= vProjectedCoord.w;
		vProjectedCoord.xy = vProjectedCoord.xy * float2(0.5f, -0.5f) + float2(0.5f, 0.5f);
		//fDepth = LinearizeDepth(linearDepth.SampleLevel(samplerState, vProjectedCoord.xy, 0),projection) * 10000;
		fDepth = LinearizeDepth(linearDepth.SampleLevel(samplerState, vProjectedCoord.xy, 0));
		float fDepthDiff = origin.z - fDepth;

		if (fDepthDiff <= 0.0f)
			origin += direction;

		direction *= 0.5f;
		origin -= direction;
	}

	float4 vProjectedCoord = mul(projection,float4(origin, 1.0f));
	vProjectedCoord.xy /= vProjectedCoord.w;
	vProjectedCoord.xy = vProjectedCoord.xy * float2(0.5f, -0.5f) + float2(0.5f, 0.5f);
	//fDepth = texture_lineardepth.SampleLevel(sampler_point_clamp, vProjectedCoord.xy, 0);
	fDepth = LinearizeDepth(linearDepth.SampleLevel(samplerState, vProjectedCoord.xy, 0));
	float fDepthDiff = origin.z - fDepth;

	return float4(vProjectedCoord.xy, fDepth, abs(fDepthDiff) < tolerance ? 1.0f : 0.0f);
}

float4 SSRRayMarch(inout float3 origin, in float3 direction, float4x4 projection)
{
	float fDepth;

	for (int i = 0; i < coarseStepCount; i++)
	{
		origin += direction;

		float4 vProjectedCoord = mul(projection,float4(origin, 1.0f));
		vProjectedCoord.xy /= vProjectedCoord.w;
		vProjectedCoord.xy = vProjectedCoord.xy * float2(0.5f, -0.5f) + float2(0.5f, 0.5f);
		//fDepth = texture_lineardepth.SampleLevel(sampler_point_clamp, vProjectedCoord.xy, 0);
		fDepth = LinearizeDepth(linearDepth.SampleLevel(samplerState, vProjectedCoord.xy, 0));
		float fDepthDiff = origin.z - fDepth;

		[branch]
		if (fDepthDiff > 0.0f)
			return SSRBinarySearch(origin,direction,projection);

		direction *= coarseStepIncrease;

	}

	return float4(0.0f, 0.0f, 0.0f, 0.0f);
}

float3 ComputeF0(in float4 baseColor, in float reflectance, in float metalness)
{
	return lerp(lerp(float3(0, 0, 0), float3(1, 1, 1), reflectance), baseColor.rgb, metalness);
}

float4 main(VS_OUT input) : SV_TARGET
{
	//const float2 uv = (DTid.xy + 0.5f) * xPPResolution_rcp;
	const float2 uv = input.TexCoords;

	//float3 P = gPosition.Sample(samplerState, uv).xyz;;
	//float3 N = gNormal.Sample(samplerState, uv).xyz;
	float3 P = gPosition.Load(int3(input.position.xy, 0)).xyz;
	float3 N = gNormal.Load(int3(input.position.xy, 0)).xyz;
	//const float3 R = normalize(reflect(P.xyz, N.xyz));

	float3 f0 = ComputeF0(ColorBuffer.Load(int3(input.position.xy,0)), 1.0f, 1.0f);

	//float3 vViewPos = mul(float4(P.xyz, 1), input.view).xyz;
	float3 vViewPos = P;
	//float3 vViewNor = mul(float4(N.xyz, 0), input.view).xyz;
	float3 vViewNor = N;
	float3 R = normalize(reflect(vViewPos.xyz, vViewNor.xyz));
	float3 vHitPos = vViewPos;

	float4 vCoords = SSRRayMarch(vHitPos,R,input.projection);

	float2 vCoordsEdgeFact = float2(1, 1) - pow(saturate(abs(vCoords.xy - float2(0.5f, 0.5f)) * 2), 8);
	float fScreenEdgeFactor = saturate(min(vCoordsEdgeFact.x, vCoordsEdgeFact.y));

	//if (!bInsideScreen(vCoords.xy))
	//	fScreenEdgeFactor = 0;


	//Color
	float reflectionIntensity =
		saturate(
			fScreenEdgeFactor *																	// screen fade
			saturate(R.z)																// camera facing fade
			* vCoords.w																			// rayhit binary fade
		);


	float3 reflectionColor = gFinalImage.SampleLevel(samplerState, vCoords.xy, 0).rgb;
	//float3 sceneColor = xTexture.Load(int3(input.pos.xy, 0)).rgb;
	//float3 sceneColor = gFinalImage.Sample(samplerState, uv).rgb;
	float3 sceneColor = gFinalImage.Load(int3(input.position.xy, 0)).rgb;
	return float4(sceneColor.rgb + reflectionColor.rgb * f0 * reflectionIntensity, 1);

}