struct VS_INPUT
{
	float3 position : POSITION;
	float3 normal : NORMAL;
	float2 uv : TEXCOORD;
};

struct VS_OUT
{
	float4 position : SV_POSITION;
	float3 normal : NORMAL0;
	float3 campos : NORMAL1;
	float3 fragpos : NORMAL2;
	float4 fragposlightspace : NORMAL3;
	float2 texCoord : TEXCOORD0;
};

cbuffer camera : register(b0)
{
	matrix projection;
	matrix view;
	vector campos;
}

cbuffer object : register(b1)
{
	matrix model;
	matrix inverseModel;
}

cbuffer shadow : register(b2)
{
	matrix shadowProj;
	matrix shadowView;
}

VS_OUT main(VS_INPUT input)
{
	matrix translationMatrix = mul(projection, view);
	translationMatrix = mul(translationMatrix, model);
	matrix shadowTransMatrix = mul(shadowProj, shadowView);
	VS_OUT output;
	output.position = mul(translationMatrix, float4(input.position, 1.0f));
	output.normal = mul((float3x3)inverseModel,input.normal);
	output.campos = (float3)campos;
	output.fragpos = (float3)mul(model, float4(input.position,1.0f));
	output.fragposlightspace = mul(shadowTransMatrix,float4(output.fragpos,1.0f));
	output.texCoord = input.uv;
	return output;
}