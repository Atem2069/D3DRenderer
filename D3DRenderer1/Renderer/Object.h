#pragma once

#include "BaseRenderer.h"
#include "Texturing.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <iostream>
#include <vector>

struct ObjectTransformation
{
	XMMATRIX model;
	XMMATRIX inverseModel;
};

struct Material
{
	Texture2D m_albedoTexture;
	int m_materialIndex = 0;
};

struct Mesh
{
	int m_numElements;
	int m_baseElement;
	int m_baseVertex;
	Material m_material = {};
};

class Object
{
public:
	bool init(std::string path);
	void destroy();

	void doCPUUpdate();
	void draw();

	//Transformations
	void rotate(FXMVECTOR axis, float angleDegrees);
	void scale(FXMVECTOR scaleAxis);
	void translate(FXMVECTOR transAxis);

	XMFLOAT4 getCentre();
private:
	ID3D11Buffer* m_baseVBO, *m_baseIBO;
	ObjectTransformation m_transformation = {};
	ID3D11Buffer* m_transformationBuffer;
	std::vector<Mesh> m_meshes;
	UINT m_stride = sizeof(Vertex);
	UINT m_offset = 0;
	bool requiresUpdate = false;
};

class FullscreenQuad	//I'm lazy so implementing this myself.
{
public:
	bool init();
	void destroy();

	void draw();
private:
	ID3D11Buffer* m_baseVBO, *m_baseIBO;
	UINT m_stride = sizeof(Vertex);
	UINT m_offset = 0;
};