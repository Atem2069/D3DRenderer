#include "VertexShader.h"

bool VertexShader::init(std::string path)
{
	HRESULT result;
	ID3D10Blob* byteCode = nullptr;
	ID3D10Blob* error = nullptr;
	std::wstring temp = std::wstring(path.begin(), path.end());
	LPCWSTR filePath = temp.c_str();

	result = D3DCompileFromFile(filePath, nullptr, nullptr, "main", "vs_5_0", D3DCOMPILE_ENABLE_STRICTNESS, 0, &byteCode, &error);
	if (FAILED(result))
	{
		std::cout << "Vertex shader compilation failed. Error: ";
		std::cout << (char*)error->GetBufferPointer();
		return false;
	}

	result = D3DContext::getCurrent()->getDevice()->CreateVertexShader(byteCode->GetBufferPointer(), byteCode->GetBufferSize(), nullptr, &m_vertexShader);
	if (FAILED(result))
	{
		std::cout << "Failed to create vertex shader. HRESULT " << result << std::endl;
		return false;
	}

	//...why
	D3D11_INPUT_ELEMENT_DESC inputLayouts[5];
	
	inputLayouts[0].AlignedByteOffset = 0;
	inputLayouts[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	inputLayouts[0].InputSlot = 0;
	inputLayouts[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	inputLayouts[0].InstanceDataStepRate = 0;
	inputLayouts[0].SemanticIndex = 0;
	inputLayouts[0].SemanticName = "POSITION";
	
	inputLayouts[1].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	inputLayouts[1].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	inputLayouts[1].InputSlot = 0;
	inputLayouts[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	inputLayouts[1].InstanceDataStepRate = 0;
	inputLayouts[1].SemanticIndex = 0;
	inputLayouts[1].SemanticName = "NORMAL";

	inputLayouts[2].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	inputLayouts[2].Format = DXGI_FORMAT_R32G32_FLOAT;
	inputLayouts[2].InputSlot = 0;
	inputLayouts[2].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	inputLayouts[2].InstanceDataStepRate = 0;
	inputLayouts[2].SemanticIndex = 0;
	inputLayouts[2].SemanticName = "TEXCOORD";

	inputLayouts[3].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	inputLayouts[3].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	inputLayouts[3].InputSlot = 0;
	inputLayouts[3].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	inputLayouts[3].InstanceDataStepRate = 0;
	inputLayouts[3].SemanticIndex = 0;
	inputLayouts[3].SemanticName = "TANGENT";

	inputLayouts[4].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	inputLayouts[4].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	inputLayouts[4].InputSlot = 0;
	inputLayouts[4].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	inputLayouts[4].InstanceDataStepRate = 0;
	inputLayouts[4].SemanticIndex = 0;
	inputLayouts[4].SemanticName = "BITANGENT";

	result = D3DContext::getCurrent()->getDevice()->CreateInputLayout(inputLayouts, 5, byteCode->GetBufferPointer(), byteCode->GetBufferSize(), &m_inputLayout);
	if (FAILED(result))
	{
		std::cout << "Failed to create input layout. HRESULT " << result << std::endl;
		return false;
	}

	return true;
}

void VertexShader::destroy()
{
	m_vertexShader->Release();
	m_inputLayout->Release();
}

void VertexShader::bind()
{
	D3DContext::getCurrent()->getDeviceContext()->VSSetShader(m_vertexShader, nullptr, 0);
	D3DContext::getCurrent()->getDeviceContext()->IASetInputLayout(m_inputLayout);
}

ID3D11VertexShader* VertexShader::getVertexShader()
{
	return m_vertexShader;
}

ID3D11InputLayout* VertexShader::getInputLayout()
{
	return m_inputLayout;
}