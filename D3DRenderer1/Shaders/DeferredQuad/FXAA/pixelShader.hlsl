#include "../../common.hlsli"

SamplerState samplerState : register(s0);
Texture2D inputTex : register(t0);
Texture2D ssr : register(t1);

cbuffer PerFrameFlags : register(b1)
{
	FrameFlags frameFlags;
}

float4 FXAA(VS_OUT input)
{
	float FXAA_SPAN_MAX = 8.0;
	float FXAA_REDUCE_MUL = 1.0 / 8.0;
	float FXAA_REDUCE_MIN = 1.0 / 128.0;

	float3 rgbNW = inputTex.Sample(samplerState, input.texcoord + (float2(-1.0, -1.0) / frameFlags.resolution)).xyz;
	float3 rgbNE = inputTex.Sample(samplerState, input.texcoord + (float2(1.0, -1.0) / frameFlags.resolution)).xyz;
	float3 rgbSW = inputTex.Sample(samplerState, input.texcoord + (float2(-1.0, 1.0) / frameFlags.resolution)).xyz;
	float3 rgbSE = inputTex.Sample(samplerState, input.texcoord + (float2(1.0, 1.0) / frameFlags.resolution)).xyz;
	float3 rgbM = inputTex.Sample(samplerState, input.texcoord).xyz;

	float3 luma = float3(0.299, 0.587, 0.114);
	float lumaNW = dot(rgbNW, luma);
	float lumaNE = dot(rgbNE, luma);
	float lumaSW = dot(rgbSW, luma);
	float lumaSE = dot(rgbSE, luma);
	float lumaM = dot(rgbM, luma);

	float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
	float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));

	float2 dir;
	dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
	dir.y = ((lumaNW + lumaSW) - (lumaNE + lumaSE));

	float dirReduce = max(
		(lumaNW + lumaNE + lumaSW + lumaSE) * (0.25 * FXAA_REDUCE_MUL),
		FXAA_REDUCE_MIN);

	float rcpDirMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + dirReduce);

	dir = min(float2(FXAA_SPAN_MAX, FXAA_SPAN_MAX),
		max(float2(-FXAA_SPAN_MAX, -FXAA_SPAN_MAX),
			dir * rcpDirMin)) / frameFlags.resolution;

	float3 rgbA = (1.0 / 2.0) * (inputTex.Sample(samplerState, input.texcoord + dir * (1.0 / 3.0 - 0.5)).xyz +
		inputTex.Sample(samplerState, input.texcoord + dir * (2.0 / 3.0 - 0.5)).xyz);
	float3 rgbB = rgbA * (1.0 / 2.0) + (1.0 / 4.0) * (inputTex.Sample(samplerState, input.texcoord + dir * (0.0 / 3.0 - 0.5)).xyz +
		inputTex.Sample(samplerState, input.texcoord + dir * (3.0 / 3.0 - 0.5)).xyz);

	float lumaB = dot(rgbB, luma);

	float4 outputColor = 1.0f;

	if ((lumaB < lumaMin) || (lumaB > lumaMax)) {
		outputColor.xyz = rgbA;
	}
	else {
		outputColor.xyz = rgbB;
	}

	return outputColor;
}

float4 main(VS_OUT input) : SV_TARGET
{
	float4 result = 1.0f;
	if (frameFlags.doFXAA == 1)
		result = FXAA(input);
	else
		result.rgb = inputTex.Sample(samplerState, input.texcoord).rgb;

	return result;
}