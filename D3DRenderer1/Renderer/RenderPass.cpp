#include "RenderPass.h"

bool RenderPass::init(int width, int height, int renderPassType, DXGI_FORMAT format, int MSAALevels, int MSAAQuality)
{
	HRESULT result;

	//Texture descriptors for renderbuffer and depthbuffe
	D3D11_TEXTURE2D_DESC renderBufferDesc = {};
	renderBufferDesc.ArraySize = 1;
	renderBufferDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	renderBufferDesc.CPUAccessFlags = 0;
	renderBufferDesc.Format = format;
	renderBufferDesc.Width = width;
	renderBufferDesc.Height = height;
	renderBufferDesc.MipLevels = 1;
	renderBufferDesc.SampleDesc.Count = MSAALevels;
	renderBufferDesc.SampleDesc.Quality = MSAAQuality;
	renderBufferDesc.Usage = D3D11_USAGE_DEFAULT;

	D3D11_TEXTURE2D_DESC depthBufferDesc = {};
	depthBufferDesc.ArraySize = 1;
	depthBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	depthBufferDesc.CPUAccessFlags = 0;
	depthBufferDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
	depthBufferDesc.Width = width;
	depthBufferDesc.Height = height;
	depthBufferDesc.MipLevels = 1;
	depthBufferDesc.SampleDesc.Count = MSAALevels;
	depthBufferDesc.SampleDesc.Quality = MSAAQuality;
	depthBufferDesc.Usage = D3D11_USAGE_DEFAULT;

	//RTV/DSV descriptors
	D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = renderBufferDesc.Format;
	rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	rtvDesc.Texture2D.MipSlice = 0;

	D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Texture2D.MipSlice = 0;

	//SRV descriptors for renderbuffer and depthbuffer
	D3D11_SHADER_RESOURCE_VIEW_DESC renderBufferViewDesc = {};
	renderBufferViewDesc.Format = renderBufferDesc.Format;
	renderBufferViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	renderBufferViewDesc.Texture2D.MipLevels = 1;
	renderBufferViewDesc.Texture2D.MostDetailedMip = 0;

	D3D11_SHADER_RESOURCE_VIEW_DESC depthBufferViewDesc = {};
	depthBufferViewDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	depthBufferViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	depthBufferViewDesc.Texture2D.MipLevels = 1;
	depthBufferViewDesc.Texture2D.MostDetailedMip = 0;



	if (renderPassType == RENDERPASS_TEXTUREBUF)	//Only create views/texture *if* a texture buffer and view are going to be used
	{
		result = D3DContext::getCurrent()->getDevice()->CreateTexture2D(&renderBufferDesc, nullptr, &m_renderBuffer);
		if (FAILED(result))
		{
			std::cout << "Failed to create renderpass render buffer." << std::endl;
			return false;
		}
		result = D3DContext::getCurrent()->getDevice()->CreateRenderTargetView(m_renderBuffer, &rtvDesc, &m_renderTargetView);
		if (FAILED(result))
		{
			std::cout << "Failed to create render target view." << std::endl;
			return false;
		}
		result = D3DContext::getCurrent()->getDevice()->CreateShaderResourceView(m_renderBuffer, &renderBufferViewDesc, &m_renderBufferView);
		if (FAILED(result))
		{
			std::cout << "Failed to create a shader resource view for the render buffer." << std::endl;
			return false;
		}
	}

	result = D3DContext::getCurrent()->getDevice()->CreateTexture2D(&depthBufferDesc, nullptr, &m_depthBuffer);
	if (FAILED(result))
	{
		std::cout << "Failed to create renderpass depth buffer." << std::endl;
		return false;
	}
	result = D3DContext::getCurrent()->getDevice()->CreateDepthStencilView(m_depthBuffer, &dsvDesc, &m_depthStencilView);
	if (FAILED(result))
	{
		std::cout << "Failed to create depth stencil view." << std::endl;
		return false;
	}
	result = D3DContext::getCurrent()->getDevice()->CreateShaderResourceView(m_depthBuffer, &depthBufferViewDesc, &m_depthBufferView);
	if (FAILED(result))
	{
		std::cout << "Failed to create a shader resource view for the depth buffer. " << std::endl;
		return false;
	}

	m_renderPassType = renderPassType;
	return true;
}

void RenderPass::destroy()
{
	//todo
}

void RenderPass::specifyRenderTarget(ID3D11RenderTargetView* newRenderTargetView)
{
	if (m_renderPassType == RENDERPASS_TEXTUREBUF)	//If for some odd reason somebody creates an off screen render-target and then chooses to specify a new view, everything gets released. 
	{
		m_renderTargetView->Release();
		m_renderBufferView->Release();
		m_renderBuffer->Release();
	}
	m_renderTargetView = newRenderTargetView;

	m_renderPassType = RENDERPASS_SWAPCHAINBUF;	//Render pass type now switched from separate renderbuffer to swapchain buffer. this enum can signal to getters and warn if an item which is not meant to exist is being called upon.
}

void RenderPass::begin(float r, float g, float b, float a)
{
	D3DContext::getCurrent()->getDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	float clr[4] = { r,g,b,a };
	D3DContext::getCurrent()->getDeviceContext()->OMSetRenderTargets(1, &m_renderTargetView, m_depthStencilView);	//1 rtv for now, no deferred
	D3DContext::getCurrent()->getDeviceContext()->ClearRenderTargetView(m_renderTargetView, clr);
	D3DContext::getCurrent()->getDeviceContext()->ClearDepthStencilView(m_depthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);	//0 on stencil test not used
}

void RenderPass::bindRenderTargetSRV(int bindingPoint, int samplerBindingPoint)
{
	D3DContext::getCurrent()->getDeviceContext()->PSSetShaderResources(bindingPoint, 1, &m_renderBufferView);
	D3DContext::getCurrent()->getDeviceContext()->PSSetSamplers(samplerBindingPoint, 1, &D3DContext::m_samplerStateNearestNoMips);
}

void RenderPass::unbindRenderTargetSRV(int bindingPoint)
{
	ID3D11ShaderResourceView* m_blank = nullptr;
	D3DContext::getCurrent()->getDeviceContext()->PSSetShaderResources(bindingPoint, 1, &m_blank);	//This is somehow the way to unbind resources in direct3d 11...
}

ID3D11RenderTargetView* RenderPass::getRenderTargetView() { return m_renderTargetView; }
ID3D11DepthStencilView* RenderPass::getDepthStencilView() { return m_depthStencilView; }

ID3D11ShaderResourceView* RenderPass::getRenderBufferView()
{ 
	if (m_renderPassType = RENDERPASS_SWAPCHAINBUF)
		std::cout << "[WARN]: A swapchain render target's shader resource view is trying to be accessed. This will result in UB as no shader resource view is specifically created for this object. " << std::endl;
	return m_renderBufferView;
}
ID3D11ShaderResourceView* RenderPass::getDepthBufferView()
{ 
	return m_depthBufferView; 
}