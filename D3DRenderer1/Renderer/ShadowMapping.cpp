#include "ShadowMapping.h"

bool DirectionalShadowMap::init(float width, float height, float orthoWidth, float orthoHeight, DirectionalLight& lightInfo)
{
	HRESULT result;
	std::string vertShaderPath = R"(CompiledShaders\Shadow\vertexShader.cso)";
	std::string pixShaderPath = R"(CompiledShaders\Shadow\pixelShader.cso)";

	m_width = width; m_height = height;
	if (!m_depthRenderPass.init(width, height))
		return false;


	if (!m_vertexShader.loadCompiled(vertShaderPath))
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
	m_samplerStateDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
	m_samplerStateDesc.ComparisonFunc = D3D11_COMPARISON_GREATER;
	m_samplerStateDesc.MinLOD = 0;
	m_samplerStateDesc.MaxLOD = 0;
	m_samplerStateDesc.MaxAnisotropy = 0;
	result = D3DContext::getCurrent()->getDevice()->CreateSamplerState(&m_samplerStateDesc, &m_samplerState);
	if (FAILED(result))
	{
		std::cout << "Failed to create sampler state.. HRESULT " << result << std::endl;
		return false;
	}

	D3D11_RASTERIZER_DESC rstateDesc = {};
	rstateDesc.AntialiasedLineEnable = FALSE;
	rstateDesc.CullMode = D3D11_CULL_BACK;
	rstateDesc.DepthBias = 0.0f;
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

void DirectionalShadowMap::beginFrame(DirectionalLight& lightInfo, XMFLOAT2 dimensions)
{
	m_shadowCamera.m_view = XMMatrixLookAtLH(XMVectorNegate(XMLoadFloat4(&lightInfo.direction)), XMLoadFloat4(&lightInfo.direction), XMVectorSet(0, 1, 0, 1));
	m_shadowCamera.m_projection = XMMatrixOrthographicLH(dimensions.x, dimensions.y, 1.0f, 10000.0f);

	m_depthRenderPass.begin();
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
	m_depthRenderPass.bindDepthTarget(textureBinding);
	D3DContext::getCurrent()->getDeviceContext()->PSSetSamplers(samplerStateBinding, 1, &m_samplerState);
}

void DirectionalShadowMap::unbindDepthTexturePS(int textureBinding)
{
	m_depthRenderPass.unbindDepthTarget(textureBinding);
}

void DirectionalShadowMap::bindShadowCamera(int constBufferBinding)
{
	m_cameraUploadBuffer.uploadToVertexShader(constBufferBinding);
}

ID3D11ShaderResourceView* DirectionalShadowMap::getDepthBufferView()
{
	return m_depthRenderPass.getDepthBufferView();
}