#pragma once

#include "BaseRenderer.h"

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <d3dcompiler.h>

class GeometryShader
{
public:
	bool init(std::string shaderPath);
	bool loadCompiled(std::string shaderPath);
	void destroy();

	void bind();

	ID3D11GeometryShader* getGeometryShader();
private:
	ID3D11GeometryShader* m_geometryShader;

};