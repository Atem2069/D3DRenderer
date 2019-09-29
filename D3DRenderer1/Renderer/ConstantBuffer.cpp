#include "ConstantBuffer.h"

bool ConstantBuffer::init(void* data, size_t dataSize)
{
	HRESULT result;

	D3D11_BUFFER_DESC cbDesc = {};
	cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbDesc.ByteWidth = dataSize;
	cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cbDesc.MiscFlags = 0;
	cbDesc.StructureByteStride = 0;
	cbDesc.Usage = D3D11_USAGE_DYNAMIC;

	D3D11_SUBRESOURCE_DATA cbInitialData = {};
	cbInitialData.pSysMem = data;

	result = D3DContext::getCurrent()->getDevice()->CreateBuffer(&cbDesc, &cbInitialData, &m_constBuffer);
	if (FAILED(result))
	{
		std::cout << "Failed to create constant buffer.. HRESULT " << result << std::endl;
		return false;
	}

	return true;
}

void ConstantBuffer::destroy()
{
	m_constBuffer->Release();
}

bool ConstantBuffer::update(void* data, size_t dataSize)
{
	HRESULT result;
	D3D11_MAPPED_SUBRESOURCE bufferPtr;
	result = D3DContext::getCurrent()->getDeviceContext()->Map(m_constBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &bufferPtr);
	if (FAILED(result))
	{
		std::cout << "Failed to map constant buffer for data write.. HRESULT " << result << std::endl;
		return false;
	}

	if(data)
		memcpy(bufferPtr.pData, data, dataSize);	//hopefully no segfault?

	D3DContext::getCurrent()->getDeviceContext()->Unmap(m_constBuffer, 0);


	return true;
}

void ConstantBuffer::uploadToVertexShader(int bindingPoint)
{
	D3DContext::getCurrent()->getDeviceContext()->VSSetConstantBuffers(bindingPoint, 1, &m_constBuffer);
}
void ConstantBuffer::uploadToPixelShader(int bindingPoint)
{
	D3DContext::getCurrent()->getDeviceContext()->PSSetConstantBuffers(bindingPoint, 1, &m_constBuffer);
}
