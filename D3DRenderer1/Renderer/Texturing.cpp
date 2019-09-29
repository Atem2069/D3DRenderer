#include "Texturing.h"

bool Texture2D::init(std::string filePath)
{
	int width, height;
	unsigned char * imagedata = stbi_load(filePath.c_str(), &width, &height, &m_channels, 0);	
	if (!imagedata)
	{
		std::cout << "Failed to load image... check format or filepath" << std::endl;
		return false; 
	}

	D3D11_TEXTURE2D_DESC texDesc = {};
	//D3D11_SUBRESOURCE_DATA texInitialData = {};
	texDesc.ArraySize = 1;
	texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	texDesc.CPUAccessFlags = 0;
	texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	texDesc.Width = width;
	texDesc.Height = height;
	texDesc.MipLevels = 0;
	texDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	
	//texInitialData.pSysMem = imagedata;
	//texInitialData.SysMemPitch = width * m_channels;

	HRESULT result = D3DContext::getCurrent()->getDevice()->CreateTexture2D(&texDesc, nullptr, &m_texture);
	if (FAILED(result))
	{
		std::cout << "Failed to create d3d texture object.." << std::endl;
		return false;
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC texSrvDesc = {};
	texSrvDesc.Format = texDesc.Format;
	texSrvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	texSrvDesc.Texture2D.MipLevels = -1;
	texSrvDesc.Texture2D.MostDetailedMip = 0;

	result = D3DContext::getCurrent()->getDevice()->CreateShaderResourceView(m_texture, &texSrvDesc, &m_textureView);
	if (FAILED(result))
	{
		std::cout << "Failed to create res view for texture.." << std::endl;
		return false;
	}

	D3DContext::getCurrent()->getDeviceContext()->UpdateSubresource(m_texture, 0, NULL, imagedata, width * m_channels, 0);
	stbi_image_free(imagedata);	//Free as memleaks are bad
	D3DContext::getCurrent()->getDeviceContext()->GenerateMips(m_textureView);

	if (FAILED(result))
	{
		std::cout << "Failed to create texture sampling state.." << std::endl;
		return false;
	}
	

	return true;
}

void Texture2D::destroy()
{
	//todo
}

void Texture2D::bind(int textureBindingPoint)
{
	D3DContext::getCurrent()->getDeviceContext()->PSSetShaderResources(textureBindingPoint, 1, &m_textureView);
}

ID3D11Texture2D* Texture2D::getTexture() { return m_texture; }
ID3D11ShaderResourceView* Texture2D::getTextureView() { return m_textureView; }