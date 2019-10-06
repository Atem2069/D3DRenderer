#pragma once

#include "baserenderer.h"
#include "renderpass.h"
#include "vertexshader.h"
#include "pixelshader.h"
#include "ConstantBuffer.h"
#include "Object.h"

#include <iostream>
#include <random>
#include <vector>

class AmbientOcclusionPass
{
public:
	bool init(float width, float height);
	void destroy();

	void begin();
	void renderAO();

	void bindAOTexture(int samplerStateBinding, int textureBindingPoint);
	void unbindAOTexture(int textureBindingPoint);

	ID3D11ShaderResourceView* getAOTexture();
private:
	std::vector<XMFLOAT4> m_ssaoKernels;

	ID3D11Texture2D* noiseTexture;
	ID3D11ShaderResourceView* noiseTextureView;
	ID3D11SamplerState* noiseSamplerState;

	RenderPass m_AORenderPass;
	RenderPass m_AOFinalRenderPass;

	VertexShader m_AOVertexShader;
	PixelShader m_AOPixelShader;
	VertexShader m_AOBlurVertexShader;
	PixelShader m_AOBlurPixelShader;
	ConstantBuffer m_KernelConstBuffer;
	FullscreenQuad m_fsQuad;



	float lerp(float a, float b, float f)
	{
		return a + f * (b - a);
	}
};