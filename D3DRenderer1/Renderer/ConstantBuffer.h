#pragma once

#include "BaseRenderer.h"

class ConstantBuffer
{
public:
	bool init(void* initialData, size_t dataSize);
	void destroy();

	bool update(void* newData, size_t dataSize);

	void uploadToVertexShader(int bindingPoint);
	void uploadToPixelShader(int bindingPoint);
private:
	ID3D11Buffer* m_constBuffer;
};