#pragma once

#include "BaseRenderer.h"

#include <iostream>
#include <d3dcompiler.h>

class PixelShader
{
public:
	bool init(std::string path);
	void destroy();

	void bind();

	ID3D11PixelShader* getPixelShader();
private:
	ID3D11PixelShader* m_pixelShader;
};