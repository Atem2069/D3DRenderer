#pragma once
#include <iostream>
#include <vector>
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

	void begin(float r, float g, float b);

	void bindRenderTargetSRV(int bindingPoint, int samplerBindingPoint);
	void unbindRenderTargetSRV(int bindingPoint);

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

class DeferredRenderPass
{
public:
	bool init(int width, int height, int noRenderTargets, int MSAALevels, int MSAAQuality);
	void destroy();

	void begin(float r, float g, float b);

	void bindRenderTargets(int startLocation, int samplerBinding);
	void unbindRenderTargets(int startLocation);

	std::vector<ID3D11ShaderResourceView*> getRenderBufferViews();
	ID3D11ShaderResourceView* getDepthBufferView();
private:
	std::vector<ID3D11Texture2D*> m_renderBuffers;
	std::vector<ID3D11RenderTargetView*> m_renderTargetViews;
	std::vector<ID3D11ShaderResourceView*> m_renderBufferViews;

	ID3D11Texture2D* m_depthBuffer;
	ID3D11DepthStencilView* m_depthStencilView;
	ID3D11ShaderResourceView* m_depthBufferView;

	int m_noRenderTargets;
};