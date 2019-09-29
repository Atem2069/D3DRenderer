#pragma once

#include "BaseRenderer.h"

#include <iostream>
#include <d3dcompiler.h>

class VertexShader
{
public:
	bool init(std::string shaderPath);
	void destroy();

	void bind();

	ID3D11VertexShader* getVertexShader();
	ID3D11InputLayout* getInputLayout();
private:
	ID3D11VertexShader* m_vertexShader;
	ID3D11InputLayout* m_inputLayout;
};