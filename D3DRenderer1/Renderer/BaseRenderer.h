#pragma once

#include <d3d11.h>
#include <iostream>

#include <DirectXMath.h>
using namespace DirectX;

struct Vertex
{
	XMFLOAT3 position;
	XMFLOAT3 normal;
	XMFLOAT2 uv;
};

class D3D
{
public:
	bool init(int width, int height, bool fullscreen, HWND hwnd, int MSAALevels, int& MSAAQuality);
	
	ID3D11Device* getDevice();
	ID3D11DeviceContext* getDeviceContext();
	ID3D11RenderTargetView* getBackBuffer();
	ID3D11DepthStencilView* getDepthBuffer();
	IDXGISwapChain* getSwapChain();
	ID3D11RasterizerState* getRasterizerState();
	D3D11_VIEWPORT m_viewport;
private:
	ID3D11BlendState* m_blendState;
	ID3D11RasterizerState* m_rasterizerState;
	ID3D11Device* m_device;
	ID3D11DeviceContext* m_deviceContext;
	ID3D11RenderTargetView* m_backBuffer;
	ID3D11DepthStencilView* m_depthBuffer;
	ID3D11Texture2D* m_depthStencilTex;
	IDXGISwapChain* m_swapChain;
};

class D3DContext
{
public:
	static void Register(D3D& d3d);
	static D3D* getCurrent();

	//Basic manipulation
	static void setViewport(int width, int height);
	static void setDefaultRasterState();
	static void clearDefaultView(float r, float g, float b, float a);
	static bool present(bool doVsync);

	static ID3D11SamplerState* m_samplerStateAnisoFilter;
	static ID3D11SamplerState* m_samplerStateNearestNoMips;

private:
	static D3D* m_current;
};