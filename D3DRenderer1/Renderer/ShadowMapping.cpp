#include "ShadowMapping.h"

bool DirectionalShadowMap::init(float width, float height, float orthoWidth, float orthoHeight, DirectionalLight& lightInfo)
{
	HRESULT result;
	std::string vertShaderPath = R"(Shaders\Shadow\vertexShader.hlsl)";
	std::string pixShaderPath = R"(Shaders\Shadow\pixelShader.hlsl)";

	m_width = width; m_height = height;

	D3D11_TEXTURE2D_DESC m_shadowMapDesc = {};
	m_shadowMapDesc.Width = m_width;
	m_shadowMapDesc.Height = m_height;
	m_shadowMapDesc.ArraySize = 1;
	m_shadowMapDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	m_shadowMapDesc.CPUAccessFlags = 0;
	m_shadowMapDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	m_shadowMapDesc.MipLevels = 1;
	m_shadowMapDesc.MiscFlags = 0;
	m_shadowMapDesc.SampleDesc.Count = 1;
	m_shadowMapDesc.Usage = D3D11_USAGE_DEFAULT;
	result = D3DContext::getCurrent()->getDevice()->CreateTexture2D(&m_shadowMapDesc, nullptr, &m_depthBuffer);
	if (FAILED(result))
	{
		std::cout << "Failed to create shadowmap depth buffer.. HRESULT " << result << std::endl;
		return false;
	}

	D3D11_DEPTH_STENCIL_VIEW_DESC m_dsvDesc = {};
	m_dsvDesc.Flags = 0;
	m_dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	m_dsvDesc.Texture2D.MipSlice = 0;
	m_dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;

	result = D3DContext::getCurrent()->getDevice()->CreateDepthStencilView(m_depthBuffer, &m_dsvDesc, &m_depthStencilView);
	if (FAILED(result))
	{
		std::cout << "Failed to create depth stencil view.. HRESULT " << result << std::endl;
		return false;
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC m_depthBufViewDesc = {};
	m_depthBufViewDesc.Format = DXGI_FORMAT_R32_FLOAT;
	m_depthBufViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	m_depthBufViewDesc.Texture2D.MipLevels = 1;
	m_depthBufViewDesc.Texture2D.MostDetailedMip = 0;
	

	result = D3DContext::getCurrent()->getDevice()->CreateShaderResourceView(m_depthBuffer, &m_depthBufViewDesc, &m_depthBufferView);
	if (FAILED(result))
	{
		std::cout << "Failed to create shader resource view.. HRESULT " << result << std::endl;
		return false;
	}


	if (!m_vertexShader.init(vertShaderPath))
		return false;

	m_shadowCamera = {};
	m_shadowCamera.m_projection = XMMatrixOrthographicLH(orthoWidth,orthoHeight, 1.0f, 10000.0f);
	m_shadowCamera.m_view = XMMatrixLookAtLH(XMVectorNegate(XMLoadFloat4(&lightInfo.direction)), XMLoadFloat4(&lightInfo.direction), XMVectorSet(0, 1, 0, 1.0f));


	if (!m_cameraUploadBuffer.init(&m_shadowCamera, sizeof(ShadowCamera)))
		return false;
	
	m_viewport.Width = m_width;
	m_viewport.Height = m_height;
	m_viewport.MinDepth = 0.0f;
	m_viewport.MaxDepth = 1.0f;
	m_viewport.TopLeftX = 0;
	m_viewport.TopLeftY = 0;
	

	D3D11_SAMPLER_DESC m_samplerStateDesc = {};
	m_samplerStateDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	m_samplerStateDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	m_samplerStateDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	m_samplerStateDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	m_samplerStateDesc.MinLOD = 0;
	m_samplerStateDesc.MaxLOD = 0;
	m_samplerStateDesc.MaxAnisotropy = 0;
	m_samplerStateDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	result = D3DContext::getCurrent()->getDevice()->CreateSamplerState(&m_samplerStateDesc, &m_samplerState);
	if (FAILED(result))
	{
		std::cout << "Failed to create sampler state.. HRESULT " << result << std::endl;
		return false;
	}

	D3D11_RASTERIZER_DESC rstateDesc = {};
	rstateDesc.AntialiasedLineEnable = FALSE;
	rstateDesc.CullMode = D3D11_CULL_BACK;
	rstateDesc.DepthBias = 0.006f;
	rstateDesc.DepthBiasClamp = 0.0f;
	rstateDesc.DepthClipEnable = TRUE;
	rstateDesc.FillMode = D3D11_FILL_SOLID;
	rstateDesc.FrontCounterClockwise = FALSE;
	rstateDesc.MultisampleEnable = FALSE;
	rstateDesc.ScissorEnable = FALSE;
	rstateDesc.SlopeScaledDepthBias = 0;

	result = D3DContext::getCurrent()->getDevice()->CreateRasterizerState(&rstateDesc, &m_shadowMapRasterState);
	if (FAILED(result))
	{
		std::cout << "Failed to create raster state.. HRESULT " << result << std::endl;
		return false;
	}

	std::cout << "Shadowmap initialized. " << std::endl;
	return true;
}

void DirectionalShadowMap::destroy()
{
	//todo
}

void DirectionalShadowMap::beginFrame(DirectionalLight& lightInfo)
{
	m_shadowCamera.m_view = XMMatrixLookAtLH(XMVectorNegate(XMLoadFloat4(&lightInfo.direction)), XMLoadFloat4(&lightInfo.direction), XMVectorSet(0, 1, 0, 1));


	D3DContext::getCurrent()->getDeviceContext()->OMSetRenderTargets(0, nullptr, m_depthStencilView);
	D3DContext::getCurrent()->getDeviceContext()->ClearDepthStencilView(m_depthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0.0f);

	D3DContext::getCurrent()->getDeviceContext()->PSSetShader(nullptr, nullptr, 0);
	m_vertexShader.bind();
	m_cameraUploadBuffer.update(&m_shadowCamera, sizeof(ShadowCamera));
	m_cameraUploadBuffer.uploadToVertexShader(0);


	D3DContext::getCurrent()->getDeviceContext()->RSSetViewports(1, &m_viewport);
	D3DContext::getCurrent()->getDeviceContext()->RSSetState(m_shadowMapRasterState);
	D3DContext::getCurrent()->getDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void DirectionalShadowMap::endFrame()
{
	D3DContext::setDefaultRasterState();
}

void DirectionalShadowMap::bindDepthTexturePS(int samplerStateBinding, int textureBinding)
{
	D3DContext::getCurrent()->getDeviceContext()->PSSetSamplers(samplerStateBinding, 1, &m_samplerState);
	D3DContext::getCurrent()->getDeviceContext()->PSSetShaderResources(textureBinding, 1, &m_depthBufferView);
}

void DirectionalShadowMap::unbindDepthTexturePS(int textureBinding)
{
	D3DContext::getCurrent()->getDeviceContext()->PSSetShaderResources(textureBinding, 0, nullptr);
}

void DirectionalShadowMap::bindShadowCamera(int constBufferBinding)
{
	m_cameraUploadBuffer.uploadToVertexShader(constBufferBinding);
}

ID3D11ShaderResourceView* DirectionalShadowMap::getDepthBufferView()
{
	return m_depthBufferView;
}