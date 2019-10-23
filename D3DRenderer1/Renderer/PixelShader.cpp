#include "PixelShader.h"

bool PixelShader::init(std::string path)
{
	HRESULT result;
	ID3D10Blob* byteCode = nullptr;
	ID3D10Blob* error = nullptr;

	std::wstring temp = std::wstring(path.begin(), path.end());
	LPCWSTR filePath = temp.c_str();
	result = D3DCompileFromFile(filePath, nullptr, nullptr, "main", "ps_5_0", D3DCOMPILE_ENABLE_STRICTNESS, 0, &byteCode, &error);
	if (FAILED(result))
	{
		std::cout << "Pixel shader compilation failed. Error: ";
		std::cout << (char*)error->GetBufferPointer();
		return false;
	}

	result = D3DContext::getCurrent()->getDevice()->CreatePixelShader(byteCode->GetBufferPointer(), byteCode->GetBufferSize(), nullptr, &m_pixelShader);
	if (FAILED(result))
	{
		std::cout << "Failed to create pixel shader. HRESULT " << result << std::endl;
		return false;
	}


	return true;
}

bool PixelShader::loadCompiled(std::string path)
{
	HRESULT result;
	std::wstring temp = std::wstring(path.begin(), path.end());
	LPCWSTR filePath = temp.c_str();
	ID3D10Blob* fileContents;

	result = D3DReadFileToBlob(filePath, &fileContents);
	if (FAILED(result))
	{
		std::cout << "File : " << path << std::endl;
		std::cout << "Failed to load compiled shader. HRESULT " << result << std::endl;
		return false;
	}

	result = D3DContext::getCurrent()->getDevice()->CreatePixelShader(fileContents->GetBufferPointer(), fileContents->GetBufferSize(), nullptr, &m_pixelShader);
	if (FAILED(result))
	{
		std::cout << "File : " << path << std::endl;
		std::cout << "Failed to create pixel shader. HRESULT " << result << std::endl;
		return false;
	}

	return true;
}

void PixelShader::destroy()
{
	m_pixelShader->Release();
}

void PixelShader::bind()
{
	D3DContext::getCurrent()->getDeviceContext()->PSSetShader(m_pixelShader, nullptr, 0);
}

ID3D11PixelShader* PixelShader::getPixelShader()
{
	return m_pixelShader;
}