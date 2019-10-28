#include "..\common.hlsli"


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
	VS_OUT output = (VS_OUT)0;
	output.position = mul(translationMatrix, float4(input.position, 1.0f));
	output.normal = mul((float3x3)inverseModel, input.normal);
	output.campos = (float3)campos;
	output.fragpos = (float3)mul(model, float4(input.position, 1.0f));
	output.fragposviewspace = mul(mul(view,model), float4(input.position, 1.0f));
	output.normalviewspace = mul(mul((float3x3)view, (float3x3)inverseModel), input.normal);
	//float3x3 normalmatrix = (float3x3)transpose(inverse(mul(model,view)));
	//output.normalviewspace = mul(normalmatrix,input.normal);
	output.texcoord = input.texcoord;
	return output;
}