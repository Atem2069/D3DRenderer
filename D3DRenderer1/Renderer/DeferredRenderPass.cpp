#include "RenderPass.h"

bool DeferredRenderPass::init(int width, int height,int noRenderTargets, DXGI_FORMAT* formats, int MSAALevels, int MSAAQuality)
{
	HRESULT result;
	m_renderBuffers.resize(noRenderTargets);
	m_renderTargetViews.resize(noRenderTargets);
	m_renderBufferViews.resize(noRenderTargets);
	m_noRenderTargets = noRenderTargets;

	for (int i = 0; i < m_noRenderTargets; i++)
	{
		D3D11_TEXTURE2D_DESC backBufferDesc = {};
		backBufferDesc.ArraySize = 1;
		backBufferDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
		backBufferDesc.CPUAccessFlags = 0;
		backBufferDesc.Format = formats[i];
		backBufferDesc.Width = width;
		backBufferDesc.Height = height;
		backBufferDesc.MipLevels = 1;
		backBufferDesc.MiscFlags = 0;
		backBufferDesc.SampleDesc.Count = MSAALevels;
		backBufferDesc.SampleDesc.Quality = MSAAQuality;
		backBufferDesc.Usage = D3D11_USAGE_DEFAULT;

		result = D3DContext::getCurrent()->getDevice()->CreateTexture2D(&backBufferDesc, nullptr, &m_renderBuffers[i]);
		if (FAILED(result))
		{
			std::cout << "Error when creating deferred RTV. Texture " << i << " failed. HRESULT " << result << std::endl;
			return false;
		}

		D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.Format = backBufferDesc.Format;
		rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		
		result = D3DContext::getCurrent()->getDevice()->CreateRenderTargetView(m_renderBuffers[i], &rtvDesc, &m_renderTargetViews[i]);
		if (FAILED(result))
		{
			std::cout << "Error when creating deferred RTV. Render target view " << i << " failed. HRESULT " << result << std::endl;
			return false;
		}

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = backBufferDesc.Format;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;

		result = D3DContext::getCurrent()->getDevice()->CreateShaderResourceView(m_renderBuffers[i], &srvDesc, &m_renderBufferViews[i]);
		if (FAILED(result))
		{
			std::cout << "Error when creating deferred RTV. Shader resource view " << i << " failed. HRESULT " << result << std::endl;
			return false;
		}
	}


	D3D11_TEXTURE2D_DESC depthBufferDesc = {};
	depthBufferDesc.ArraySize = 1;
	depthBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	depthBufferDesc.CPUAccessFlags = 0;
	depthBufferDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
	depthBufferDesc.Width = width;
	depthBufferDesc.Height = height;
	depthBufferDesc.MipLevels = 1;
	depthBufferDesc.MiscFlags = 0;
	depthBufferDesc.SampleDesc.Count = MSAALevels;
	depthBufferDesc.SampleDesc.Quality = MSAAQuality;
	depthBufferDesc.Usage = D3D11_USAGE_DEFAULT;

	result = D3DContext::getCurrent()->getDevice()->CreateTexture2D(&depthBufferDesc, nullptr, &m_depthBuffer);

	D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;

	result = D3DContext::getCurrent()->getDevice()->CreateDepthStencilView(m_depthBuffer, &dsvDesc, &m_depthStencilView);
	if (FAILED(result))
	{
		std::cout << "Error when creating deferred DSV. Depth stencil view failed. HRESULT " << result << std::endl;
		return false;
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDescDepth = {};
	srvDescDepth.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	srvDescDepth.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDescDepth.Texture2D.MipLevels = 1;

	result = D3DContext::getCurrent()->getDevice()->CreateShaderResourceView(m_depthBuffer, &srvDescDepth, &m_depthBufferView);
	if (FAILED(result))
	{
		std::cout << "Error when creating deferred DSV. Shader resource view failed. HRESULT " << result << std::endl;
		return false;
	}
	return true;
}

void DeferredRenderPass::destroy()
{
	for (int i = 0; i < m_noRenderTargets; i++)
	{
		m_renderBuffers[i]->Release();
		m_renderTargetViews[i]->Release();
		m_renderBufferViews[i]->Release();
	}
	m_depthBuffer->Release();
	m_depthStencilView->Release();
	m_depthBufferView->Release();
}

void DeferredRenderPass::begin(float r, float g, float b, float a)
{
	D3DContext::getCurrent()->getDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	float colors[4] = { r,g,b,a };
	D3DContext::getCurrent()->getDeviceContext()->OMSetRenderTargets(m_noRenderTargets, &m_renderTargetViews[0], m_depthStencilView);
	for (int i = 0; i < m_noRenderTargets; i++)
		D3DContext::getCurrent()->getDeviceContext()->ClearRenderTargetView(m_renderTargetViews[i], colors);
	D3DContext::getCurrent()->getDeviceContext()->ClearDepthStencilView(m_depthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
}

void DeferredRenderPass::bindRenderTargets(int startLocation, int samplerBinding)
{
	D3DContext::getCurrent()->getDeviceContext()->PSSetSamplers(samplerBinding, 1, &D3DContext::m_samplerStateNearestNoMips);
	D3DContext::getCurrent()->getDeviceContext()->PSSetShaderResources(startLocation, m_noRenderTargets, &m_renderBufferViews[0]);
}

void DeferredRenderPass::unbindRenderTargets(int startLocation)
{
	ID3D11ShaderResourceView* views[8] = { nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr };
	D3DContext::getCurrent()->getDeviceContext()->PSSetShaderResources(startLocation, m_noRenderTargets, views);
}

void DeferredRenderPass::bindDepthStencilTarget(int texLoc, int samplerBinding)
{
	D3DContext::getCurrent()->getDeviceContext()->PSSetSamplers(samplerBinding, 1, &D3DContext::m_samplerStateNearestNoMips);
	D3DContext::getCurrent()->getDeviceContext()->PSSetShaderResources(texLoc, 1, &m_depthBufferView);
}

void DeferredRenderPass::unbindDepthStencilTarget(int texLoc)
{
	ID3D11ShaderResourceView* blank = nullptr;
	D3DContext::getCurrent()->getDeviceContext()->PSSetShaderResources(texLoc, 1, &blank);
}

std::vector<ID3D11ShaderResourceView*> DeferredRenderPass::getRenderBufferViews()
{
	return m_renderBufferViews;
}

ID3D11ShaderResourceView* DeferredRenderPass::getDepthBufferView()
{
	return m_depthBufferView;
}