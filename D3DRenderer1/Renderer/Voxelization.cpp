#include "Voxelization.h"

bool SceneVoxelizer::init()
{
	HRESULT result;
	//WxH aligned viewport - probably important even if no rendertarget is written to 
	m_viewport.Width = m_width;
	m_viewport.Height = m_height;
	m_viewport.MinDepth = 0;
	m_viewport.MaxDepth = 1;
	m_viewport.TopLeftX = 0;
	m_viewport.TopLeftY = 0;

	//3D Voxel Texture Desc
	D3D11_TEXTURE3D_DESC voxelTextureDesc = {};
	voxelTextureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_RENDER_TARGET;
	voxelTextureDesc.CPUAccessFlags = 0;
	voxelTextureDesc.Width = m_width;
	voxelTextureDesc.Height = m_height;
	voxelTextureDesc.Depth = m_depth;
	voxelTextureDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT; //Packed as R,G,B,Voxel full?
	voxelTextureDesc.MipLevels = 0;
	voxelTextureDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;
	voxelTextureDesc.Usage = D3D11_USAGE_DEFAULT;

	result = D3DContext::getCurrent()->getDevice()->CreateTexture3D(&voxelTextureDesc, nullptr, &m_voxelTexture);
	if (FAILED(result))
	{
		std::cout << "Failed to create 3D Texture.. HRESULT " << result << std::endl;
		return false;
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC voxelTextureViewDesc = {};
	voxelTextureViewDesc.Format = voxelTextureDesc.Format;
	voxelTextureViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
	voxelTextureViewDesc.Texture3D.MipLevels = -1;
	voxelTextureViewDesc.Texture3D.MostDetailedMip = 0;

	result = D3DContext::getCurrent()->getDevice()->CreateShaderResourceView(m_voxelTexture, &voxelTextureViewDesc, &m_voxelTextureView);
	if (FAILED(result))
	{
		std::cout << "Failed to create 3D Texture Shader Resource View.. HRESULT " << result << std::endl;
		return false;
	}

	D3D11_UNORDERED_ACCESS_VIEW_DESC voxelTextureUnorderedViewDesc = {};
	voxelTextureUnorderedViewDesc.Format = voxelTextureDesc.Format;
	voxelTextureUnorderedViewDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE3D;
	voxelTextureUnorderedViewDesc.Texture3D.WSize = m_depth;	//guessing

	result = D3DContext::getCurrent()->getDevice()->CreateUnorderedAccessView(m_voxelTexture, &voxelTextureUnorderedViewDesc, &m_voxelTextureUAV);
	if (FAILED(result))
	{
		std::cout << "Failed to create 3D Texture Unordered Access View.. HRESULT " << result << std::endl;
		return false;
	}

	D3D11_SAMPLER_DESC voxelSamplerDesc = {};
	voxelSamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	voxelSamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	voxelSamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	voxelSamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	
	result = D3DContext::getCurrent()->getDevice()->CreateSamplerState(&voxelSamplerDesc, &m_voxelTexSamplerState); 
	if (FAILED(result))
	{
		std::cout << "Failed to create sampler state.. HRESULT " << result << std::endl;
		return false;
	}

	D3D11_RASTERIZER_DESC rasterStateDesc = {};
	rasterStateDesc.CullMode = D3D11_CULL_NONE;
	rasterStateDesc.FillMode = D3D11_FILL_SOLID;
	
	result = D3DContext::getCurrent()->getDevice()->CreateRasterizerState(&rasterStateDesc, &m_voxelRasterState);
	if (FAILED(result))
	{
		std::cout << "Failed to create rasterizer state.. HRESULT " << result << std::endl;
		return false;
	}

	D3D11_DEPTH_STENCIL_DESC dsDesc = {};
	dsDesc.DepthEnable = TRUE;
	dsDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
	dsDesc.StencilEnable = FALSE;
	result = D3DContext::getCurrent()->getDevice()->CreateDepthStencilState(&dsDesc, &m_noDepthTestState);
	if (FAILED(result))
	{
		std::cout << "Failed to create depth stencil state.. HRESULT " << result << std::endl;
		return false;
	}

	if (!m_vertexShader.loadCompiled(R"(CompiledShaders\Voxelization\vertexShader.cso)"))
		return false;
	if (!m_pixelShader.loadCompiled(R"(CompiledShaders\Voxelization\pixelShader.cso)"))
		return false;

	return true;
}

void SceneVoxelizer::destroy()
{
	//todo
}

void SceneVoxelizer::beginVoxelizationPass()
{
	D3DContext::getCurrent()->getDeviceContext()->RSSetViewports(1, &m_viewport);
	ID3D11RenderTargetView* blank = nullptr;
	D3DContext::getCurrent()->getDeviceContext()->OMSetRenderTargetsAndUnorderedAccessViews(0, nullptr, nullptr, 0, 1, &m_voxelTextureUAV, nullptr);
	D3DContext::getCurrent()->getDeviceContext()->RSSetState(m_voxelRasterState);
	D3DContext::getCurrent()->getDeviceContext()->OMSetDepthStencilState(m_noDepthTestState, 0xFF);
	m_vertexShader.bind();
	m_pixelShader.bind();
}

void SceneVoxelizer::endVoxelizationPass()
{
	ID3D11UnorderedAccessView* blank = nullptr;
	D3DContext::getCurrent()->getDeviceContext()->OMSetRenderTargetsAndUnorderedAccessViews(0, nullptr, nullptr, 0, 1, &blank, nullptr);
	D3DContext::getCurrent()->getDeviceContext()->OMSetDepthStencilState(nullptr, 0xFF);
	D3DContext::setDefaultRasterState();

	//Generate mips so we have a full sparse voxel octree from our 3d tex
	D3DContext::getCurrent()->getDeviceContext()->GenerateMips(m_voxelTextureView);

	//user has to reset viewport to normal resolution
}

void SceneVoxelizer::bindVoxelTexture(int bindingPoint, int samplerBindingPoint)
{
	D3DContext::getCurrent()->getDeviceContext()->PSSetSamplers(samplerBindingPoint, 1, &m_voxelTexSamplerState);
	D3DContext::getCurrent()->getDeviceContext()->PSSetShaderResources(bindingPoint, 1, &m_voxelTextureView);
}

void SceneVoxelizer::unbindVoxelTexture(int bindingPoint)
{
	ID3D11ShaderResourceView* blank = nullptr;
	D3DContext::getCurrent()->getDeviceContext()->PSSetShaderResources(bindingPoint, 1, &blank);
}

ID3D11ShaderResourceView* SceneVoxelizer::getTextureView()
{
	return m_voxelTextureView;
}