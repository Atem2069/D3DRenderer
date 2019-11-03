#include "GeometryShader.h"

bool GeometryShader::init(std::string shaderPath)
{
	HRESULT result;

	ID3D10Blob* compiledShader, *error;
	
	std::wstring temp = std::wstring(shaderPath.begin(), shaderPath.end());
	LPCWSTR filePath = temp.c_str();

	result = D3DCompileFromFile(filePath, nullptr, nullptr, "main", "gs_5_0", D3DCOMPILE_ENABLE_STRICTNESS, 0, &compiledShader, &error);
	if (FAILED(result))
	{
		std::cout << "Geometry Shader compilation failed.. Error: ";
		std::cout << (char*)error->GetBufferPointer() << std::endl;
		return false;
	}

	result = D3DContext::getCurrent()->getDevice()->CreateGeometryShader(compiledShader->GetBufferPointer(), compiledShader->GetBufferSize(), nullptr, &m_geometryShader);
	if (FAILED(result))
	{
		std::cout << "Failed to init Geometry Shader.. HRESULT " << result << std::endl;
		return false;
	}

	return true;
}

bool GeometryShader::loadCompiled(std::string shaderPath)
{
	HRESULT result;
	ID3D10Blob* compiledShader;

	std::wstring temp = std::wstring(shaderPath.begin(), shaderPath.end());
	LPCWSTR filePath = temp.c_str();

	result = D3DReadFileToBlob(filePath, &compiledShader);
	if (FAILED(result))
	{
		std::cout << "Failed to read Geometry Shader.. HRESULT " << result << std::endl;
		return false;
	}

	result = D3DContext::getCurrent()->getDevice()->CreateGeometryShader(compiledShader->GetBufferPointer(), compiledShader->GetBufferSize(), nullptr, &m_geometryShader);
	if (FAILED(result))
	{
		std::cout << "Failed to init Geometry Shader.. HRESULT " << result << std::endl;
		return false;
	}

	return true;
}

void GeometryShader::destroy()
{
	if (m_geometryShader)
		m_geometryShader->Release();
}

void GeometryShader::bind()
{
	D3DContext::getCurrent()->getDeviceContext()->GSSetShader(m_geometryShader, nullptr, 0);
}

ID3D11GeometryShader* GeometryShader::getGeometryShader()
{
	return m_geometryShader;
}