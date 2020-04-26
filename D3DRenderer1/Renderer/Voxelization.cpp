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
	voxelTextureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; //Packed as R,G,B,Voxel full?
	voxelTextureDesc.MipLevels = 0;// 1 + floor(log2(m_width));
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
	voxelTextureViewDesc.Texture3D.MipLevels = -1;// voxelTextureDesc.MipLevels;
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
	voxelTextureUnorderedViewDesc.Texture3D.WSize = m_depth;	//guessing idk

	result = D3DContext::getCurrent()->getDevice()->CreateUnorderedAccessView(m_voxelTexture, &voxelTextureUnorderedViewDesc, &m_voxelTextureUAV);
	if (FAILED(result))
	{
		std::cout << "Failed to create 3D Texture Unordered Access View.. HRESULT " << result << std::endl;
		return false;
	}
	D3D11_SAMPLER_DESC voxelSamplerDesc = {};
	/*border filtering used due to clamp/repeat causing ghosting when rays fall outside of SVO texture*/
	voxelSamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
	voxelSamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
	voxelSamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
	voxelSamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	voxelSamplerDesc.MaxAnisotropy = 0.0f;
	voxelSamplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	//voxelSamplerDesc.BorderColor[0] = 0.564f; voxelSamplerDesc.BorderColor[1] = 0.8f; voxelSamplerDesc.BorderColor[2] = 0.976f; voxelSamplerDesc.BorderColor[3] = 1.0f;
	
	result = D3DContext::getCurrent()->getDevice()->CreateSamplerState(&voxelSamplerDesc, &m_voxelTexSamplerState); 
	if (FAILED(result))
	{
		std::cout << "Failed to create sampler state.. HRESULT " << result << std::endl;
		return false;
	}

	D3D11_RASTERIZER_DESC2 rasterStateDesc = {};
	rasterStateDesc.CullMode = D3D11_CULL_NONE;
	rasterStateDesc.FillMode = D3D11_FILL_SOLID;
	rasterStateDesc.DepthClipEnable = FALSE;
	rasterStateDesc.ConservativeRaster = D3D11_CONSERVATIVE_RASTERIZATION_MODE_OFF;	//software emulated because amd gpus don't support it fuck
	
	result = D3DContext::getCurrent()->getDevice()->CreateRasterizerState2(&rasterStateDesc, &m_voxelRasterState);
	if (FAILED(result))
	{
		std::cout << "Failed to create rasterizer state.. HRESULT " << result << std::endl;
		return false;
	}

	D3D11_DEPTH_STENCIL_DESC dsDesc = {};
	dsDesc.DepthEnable = FALSE;
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
	if (!m_geometryShader.loadCompiled(R"(CompiledShaders\Voxelization\geometryShader.cso)"))
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
	float clearCol[4] = { 0,0,0,0 };
	D3DContext::getCurrent()->getDeviceContext()->ClearUnorderedAccessViewFloat(m_voxelTextureUAV, clearCol);
	D3DContext::getCurrent()->getDeviceContext()->RSSetState(m_voxelRasterState);
	D3DContext::getCurrent()->getDeviceContext()->OMSetDepthStencilState(m_noDepthTestState, 0xFF);
	D3DContext::getCurrent()->getDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_vertexShader.bind();
	m_pixelShader.bind();
	m_geometryShader.bind();
}

void SceneVoxelizer::endVoxelizationPass()
{
	ID3D11UnorderedAccessView* blank = nullptr;
	D3DContext::getCurrent()->getDeviceContext()->OMSetRenderTargetsAndUnorderedAccessViews(0, nullptr, nullptr, 0, 1, &blank, nullptr);
	D3DContext::getCurrent()->getDeviceContext()->OMSetDepthStencilState(nullptr, 0xFF);
	D3DContext::getCurrent()->getDeviceContext()->GSSetShader(nullptr, nullptr, 0);
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