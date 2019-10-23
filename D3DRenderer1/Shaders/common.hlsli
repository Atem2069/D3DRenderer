//VS input/output
struct VS_INPUT
{
	float3 position : POSITION;
	float3 normal : NORMAL;
	float2 texcoord : TEXCOORD;
	float3 tangent : TANGENT;
	float3 bitangent : BITANGENT;
};

struct VS_OUT
{
	float4 position : SV_POSITION;
	float2 texcoord : TEXCOORD;
	float3 normal : NORMAL;
	float3 fragpos : FRAGPOS;
	float4 fragposviewspace : FRAGPOSVIEWSPACE;
	float3 normalviewspace : NORMALVIEWSPACE;
	float3 campos : CAMPOS;
	float4x4 projection : PROJECTION;
	float4x4 view : VIEW;
	float4x4 shadowCam : LIGHTCAMERA;
};

//Flags that can be set per-frame in one const buffer
struct FrameFlags
{
	int doFXAA;
	int doSSAO;
	int doSSR;
	int doTexturing;
	float ssaoRadius;
	int kernelSize;
	int ssaoPower;
	uint coarseStepCount;
	float coarseStepIncrease;
	uint fineStepCount;
	float tolerance;
	float ssrReflectiveness;
	float ssrMetallic;
};

