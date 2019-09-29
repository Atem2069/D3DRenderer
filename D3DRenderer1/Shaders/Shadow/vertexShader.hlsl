struct VS_INPUT
{
	float3 position : POSITION;
};

struct VS_OUT
{
	float4 position : SV_POSITION;
};

cbuffer shadowCamera : register(b0)
{
	matrix shadowProj;
	matrix shadowView;
}

cbuffer object : register(b1)
{
	matrix model;
	matrix inversemodel;
}



VS_OUT main(VS_INPUT input)
{
	VS_OUT output;
	//matrix translationMatrix = mul(projectionView, model);
	matrix transMatrix = mul(shadowProj, shadowView);
	transMatrix = mul(transMatrix, model);
	output.position = mul(transMatrix, float4(input.position, 1.0f));
	//output.position = mul(float4(input.position, 1.0f), projectionView);
	return output;
}