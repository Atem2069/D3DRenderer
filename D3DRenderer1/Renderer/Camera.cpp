#include "Camera.h"

bool PerspectiveCamera::init(float width, float height, float fovDeg, float minDepth, float maxDepth)
{
	HRESULT result;

	cameraChangeInfo.position = XMVectorSet(0.0f, 0.0f, -5.0f,1.0f);
	cameraChangeInfo.lookAt = XMVectorSet(0.0f, 0.0f, 1.0f, 1.0f);
	cameraChangeInfo.up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	m_cameraBuffer.projection = XMMatrixPerspectiveFovLH(XMConvertToRadians(fovDeg), width / height, minDepth, maxDepth);
	m_cameraBuffer.view = XMMatrixLookAtLH(cameraChangeInfo.position, cameraChangeInfo.lookAt, cameraChangeInfo.up);

	//Const buffer descriptor etc
	D3D11_BUFFER_DESC constBufferDesc = {};
	constBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	constBufferDesc.ByteWidth = sizeof(cb_perspectiveCamera);
	constBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	constBufferDesc.MiscFlags = 0;
	constBufferDesc.StructureByteStride = 0;
	constBufferDesc.Usage = D3D11_USAGE_DYNAMIC;

	D3D11_SUBRESOURCE_DATA constBufDataInitial = {};
	constBufDataInitial.pSysMem = &m_cameraBuffer;

	result = D3DContext::getCurrent()->getDevice()->CreateBuffer(&constBufferDesc, &constBufDataInitial, &m_constBuffer);
	if (FAILED(result))
	{
		std::cout << "Failed to create constant buffer.." << std::endl;
		return false;
	}

	return true;
}

void PerspectiveCamera::update()
{
	//Update CPU side
	m_cameraBuffer.view = XMMatrixLookAtLH(cameraChangeInfo.position, cameraChangeInfo.position + cameraChangeInfo.lookAt, cameraChangeInfo.up);
	m_cameraBuffer.position = cameraChangeInfo.position;
	//Upload GPU
	D3D11_MAPPED_SUBRESOURCE bufferPtr;
	HRESULT result = D3DContext::getCurrent()->getDeviceContext()->Map(m_constBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &bufferPtr);
	if (FAILED(result))
		std::cout << "Failed to map constant buffer for new write.." << std::endl;	//segfault
	memcpy(bufferPtr.pData, &m_cameraBuffer, sizeof(cb_perspectiveCamera));
	D3DContext::getCurrent()->getDeviceContext()->Unmap(m_constBuffer, 0);


	//Bind
	D3DContext::getCurrent()->getDeviceContext()->VSSetConstantBuffers(0, 1, &m_constBuffer);	//Hardcoded to register 0
}

XMMATRIX PerspectiveCamera::getProjection() { return m_projection; }
XMMATRIX PerspectiveCamera::getView() { return m_view; }