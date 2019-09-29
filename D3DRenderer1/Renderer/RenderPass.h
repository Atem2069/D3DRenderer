#pragma once

#include "BaseRenderer.h"
#include "VertexShader.h"
#include "PixelShader.h"

#define RENDERPASS_SWAPCHAINBUF 0
#define RENDERPASS_TEXTUREBUF 1

class RenderPass
{
public:
	bool init(int width, int height, int renderPassType, int MSAALevels, int MSAAQuality);
	void destroy();

	void specifyRenderTarget(ID3D11RenderTargetView* newRenderTargetView);

	void begin(VertexShader& vs, PixelShader& ps, float r, float g, float b);

	ID3D11RenderTargetView* getRenderTargetView();
	ID3D11DepthStencilView* getDepthStencilView();

	ID3D11ShaderResourceView* getRenderBufferView();
	ID3D11ShaderResourceView* getDepthBufferView();
private:
	ID3D11Texture2D* m_renderBuffer, *m_depthBuffer;
	ID3D11RenderTargetView* m_renderTargetView;
	ID3D11ShaderResourceView* m_renderBufferView;
	ID3D11DepthStencilView* m_depthStencilView;
	ID3D11ShaderResourceView* m_depthBufferView;
	int m_renderPassType;
};