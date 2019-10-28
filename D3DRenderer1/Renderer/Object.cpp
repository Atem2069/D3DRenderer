#include "Object.h"

bool Object::init(std::string path)
{
	std::string dir = path.substr(0, path.find_last_of(R"(\)")) + R"(\)";
	HRESULT result;	//Results for device calls
	ID3D11Device* device = D3DContext::getCurrent()->getDevice();	//Save device ref for creating buffers 

	Assimp::Importer m_importer;
	const aiScene* m_scene = m_importer.ReadFile(path, aiProcess_Triangulate | aiProcess_ConvertToLeftHanded | aiProcess_OptimizeMeshes | aiProcess_CalcTangentSpace);	//Convert to left handed cause d3d coordinates are special

	if (!m_scene || m_scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !m_scene->mRootNode)
	{
		std::cout << "Failed to load model.. " << m_importer.GetErrorString() << std::endl;
		return false;
	}
	
	//Processing each mesh with some magic 
	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;
	int globalVertexBase = 0, globalIndexBase = 0;
	for (int i = 0; i < m_scene->mNumMeshes; i++)
	{
		Mesh tempMesh = {};	//The mesh we're storing into vector
		aiMesh* currentMesh = m_scene->mMeshes[i];	//assimp mesh being worked on

		//Process vertices
		for (unsigned int j = 0; j < currentMesh->mNumVertices; j++)
		{
			Vertex tempVertex = {};
			tempVertex.position.x = currentMesh->mVertices[j].x;
			tempVertex.position.y = currentMesh->mVertices[j].y;
			tempVertex.position.z = currentMesh->mVertices[j].z;

			tempVertex.normal.x = currentMesh->mNormals[j].x;
			tempVertex.normal.y = currentMesh->mNormals[j].y;
			tempVertex.normal.z = currentMesh->mNormals[j].z;
			tempVertex.uv.x = 0.0f;
			tempVertex.uv.y = 0.0f;

			if (currentMesh->HasTangentsAndBitangents())
			{
				tempVertex.tangent.x = currentMesh->mTangents[j].x;
				tempVertex.tangent.y = currentMesh->mTangents[j].y;
				tempVertex.tangent.z = currentMesh->mTangents[j].z;

				tempVertex.bitangent.x = currentMesh->mBitangents[j].x;
				tempVertex.bitangent.y = currentMesh->mBitangents[j].y;
				tempVertex.bitangent.z = currentMesh->mBitangents[j].z;
			}

			if (currentMesh->HasTextureCoords(0))
			{
				tempVertex.uv.x = currentMesh->mTextureCoords[0][j].x;
				tempVertex.uv.y = currentMesh->mTextureCoords[0][j].y;
			}
			vertices.push_back(tempVertex);
		}
		//Process indices
		int noIndices = 0;
		for (unsigned int j = 0; j < currentMesh->mNumFaces; j++)
		{
			aiFace face = currentMesh->mFaces[j];	//not recommended method at all
			for (unsigned int k = 0; k < face.mNumIndices; k++)
				indices.push_back(face.mIndices[k]);
			noIndices += face.mNumIndices;
		}

		tempMesh.m_baseVertex = globalVertexBase;
		globalVertexBase += currentMesh->mNumVertices;

		tempMesh.m_baseElement = globalIndexBase;
		globalIndexBase += noIndices;
		
		tempMesh.m_numElements = noIndices;

		//Sorting out textures
		if (currentMesh->mMaterialIndex > 0)
		{
			aiMaterial* material = m_scene->mMaterials[currentMesh->mMaterialIndex];
			tempMesh.m_material.m_materialIndex = currentMesh->mMaterialIndex;
			if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0)
			{
				bool loadTexture = true;
				for (int j = 0; j < i; j++)
				{
					if (m_meshes[j].m_material.m_materialIndex == currentMesh->mMaterialIndex)
					{
						loadTexture = false;
						tempMesh.m_material.m_albedoTexture = m_meshes[j].m_material.m_albedoTexture;
					}
				}
				if (loadTexture)
				{
					aiString texPath;
					material->GetTexture(aiTextureType_DIFFUSE, 0, &texPath);
					std::string texFile = texPath.C_Str();
					std::string texturePath = dir + texFile;
					tempMesh.m_material.m_albedoTexture.init(texturePath);
				}
			}

			if (material->GetTextureCount(aiTextureType_SPECULAR) > 0)
			{
				bool loadTexture = true;
				for (int j = 0; j < i; j++)
				{
					if (m_meshes[j].m_material.m_materialIndex == currentMesh->mMaterialIndex)
					{
						loadTexture = false;
						tempMesh.m_material.m_specularMap = m_meshes[j].m_material.m_specularMap;
					}
				}

				if (loadTexture)
				{
					aiString texPath;
					material->GetTexture(aiTextureType_SPECULAR, 0, &texPath);
					std::string texFile = texPath.C_Str();
					std::string texturePath = dir + texFile;
					tempMesh.m_material.m_specularMap.init(texturePath);
				}
			}

		}
		m_meshes.push_back(tempMesh);

	}


	D3D11_BUFFER_DESC vboDesc = {};
	vboDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vboDesc.ByteWidth = vertices.size() * sizeof(Vertex);
	vboDesc.CPUAccessFlags = 0;
	vboDesc.MiscFlags = 0;
	vboDesc.StructureByteStride = 0;
	vboDesc.Usage = D3D11_USAGE_IMMUTABLE;

	D3D11_SUBRESOURCE_DATA vboData = {};
	vboData.pSysMem = &vertices[0];
	result = device->CreateBuffer(&vboDesc, &vboData, &m_baseVBO);
	if (FAILED(result))
	{
		std::cout << "Failed to create a base vertex buffer for one of the objects.. HRESULT " << result << std::endl;
		return false;
	}

	D3D11_BUFFER_DESC iboDesc = {};
	iboDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	iboDesc.ByteWidth = indices.size() * sizeof(unsigned int);
	iboDesc.CPUAccessFlags = 0;
	iboDesc.MiscFlags = 0;
	iboDesc.StructureByteStride = 0;
	iboDesc.Usage = D3D11_USAGE_IMMUTABLE;

	D3D11_SUBRESOURCE_DATA iboData = {};
	iboData.pSysMem = &indices[0];
	result = device->CreateBuffer(&iboDesc, &iboData, &m_baseIBO);
	if (FAILED(result))
	{
		std::cout << "Failed to create a base index buffer for one of the objects... HRESULT " << result << std::endl;
		return false;
	}

	//Setting up object transformation
	m_transformation.model = XMMatrixIdentity();
	m_transformation.model = XMMatrixMultiply(XMMatrixTranslation(1, 1, 1), m_transformation.model);
	m_transformation.model = XMMatrixMultiply(XMMatrixRotationAxis(XMVectorSet(1, 1, 1, 1), 0), m_transformation.model);
	m_transformation.model = XMMatrixMultiply(XMMatrixScaling(1, 1, 1), m_transformation.model);
	m_transformation.inverseModel = XMMatrixTranspose(XMMatrixInverse(nullptr, m_transformation.model));

	D3D11_BUFFER_DESC cbDesc = {};
	cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbDesc.ByteWidth = sizeof(ObjectTransformation);
	cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cbDesc.MiscFlags = 0;
	cbDesc.StructureByteStride = 0;
	cbDesc.Usage = D3D11_USAGE_DYNAMIC;

	D3D11_SUBRESOURCE_DATA cbInitialData = {};
	cbInitialData.pSysMem = &m_transformation;

	result = device->CreateBuffer(&cbDesc, &cbInitialData, &m_transformationBuffer);
	if (FAILED(result))
	{
		std::cout << "Failed to create transform constant buffer. " << std::endl;
		return false;
	}

	vertices.clear();
	std::vector<Vertex>().swap(vertices);
	indices.clear();
	std::vector<unsigned int>().swap(indices);
	m_importer.FreeScene();
	return true;
}

void Object::destroy()
{
	//TODO: implement
}

void Object::doCPUUpdate()
{
	if (requiresUpdate)
	{
		m_transformation.model = (m_transformation.model);
		m_transformation.inverseModel = XMMatrixTranspose(XMMatrixInverse(nullptr, m_transformation.model));
		D3D11_MAPPED_SUBRESOURCE resData;
		HRESULT result = D3DContext::getCurrent()->getDeviceContext()->Map(m_transformationBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &resData);
		if (FAILED(result))
			std::cout << "Couldn't map constant buffer for CPU update.. " << std::endl;
		memcpy(resData.pData, &m_transformation, sizeof(ObjectTransformation));
		D3DContext::getCurrent()->getDeviceContext()->Unmap(m_transformationBuffer, 0);
	}
	requiresUpdate = false;
}

void Object::draw()
{
	this->doCPUUpdate();
	ID3D11DeviceContext* deviceContext = D3DContext::getCurrent()->getDeviceContext();	//Get reference so every call doesn't have to be getCurrent()->getDeviceContext()
	deviceContext->IASetVertexBuffers(0, 1, &m_baseVBO, &m_stride, &m_offset);
	deviceContext->IASetIndexBuffer(m_baseIBO, DXGI_FORMAT_R32_UINT, 0);
	deviceContext->VSSetConstantBuffers(1, 1, &m_transformationBuffer);
	deviceContext->PSSetSamplers(0, 1, &D3DContext::m_samplerStateAnisoFilter);
	for (int i = 0; i < m_meshes.size(); i++)
	{
		m_meshes[i].m_material.m_albedoTexture.bind(0);
		m_meshes[i].m_material.m_specularMap.bind(1);
		deviceContext->DrawIndexed(m_meshes[i].m_numElements, m_meshes[i].m_baseElement, m_meshes[i].m_baseVertex);
	}
}

void Object::rotate(FXMVECTOR axis, float angleDegrees)
{
	m_transformation.model = XMMatrixMultiply(XMMatrixRotationAxis(axis, XMConvertToRadians(angleDegrees)),m_transformation.model);
	requiresUpdate = true;
	
}
void Object::scale(FXMVECTOR scaleAxis)
{
	m_transformation.model = XMMatrixMultiply(XMMatrixScalingFromVector(scaleAxis),m_transformation.model);
	requiresUpdate = true;
}
void Object::translate(FXMVECTOR transAxis)
{
	m_transformation.model = XMMatrixMultiply(XMMatrixTranslationFromVector(transAxis),m_transformation.model);
	requiresUpdate = true;
}

XMFLOAT4 Object::getCentre()
{
	XMFLOAT4 centre;	//Initialize for XMStore.. call
	XMVECTOR tmpCentre = XMVector4Transform(XMVectorSet(0, 0, 0, 1), m_transformation.model);
	XMStoreFloat4(&centre, tmpCentre);
	return centre;
}

/*Fullscreen quad*/

bool FullscreenQuad::init()
{
	HRESULT result;
	Vertex vertices[4];
	unsigned int indices[6] = { 0,1,2,2,3,0 };	//All stack allocated.

	vertices[0].position = XMFLOAT3(-1.0f, -1.0f, 0.0f);
	vertices[0].normal = XMFLOAT3(0.0f, 0.0f, 0.0f);
	vertices[0].uv = XMFLOAT2(0.0f, 1.0f);

	vertices[1].position = XMFLOAT3(-1.0f, 1.0f, 0.0f);
	vertices[1].normal = XMFLOAT3(0.0f, 0.0f, 0.0f);
	vertices[1].uv = XMFLOAT2(0.0f, 0.0f);

	vertices[2].position = XMFLOAT3(1.0f, 1.0f, 0.0f);
	vertices[2].normal = XMFLOAT3(0.0f, 0.0f, 0.0f);
	vertices[2].uv = XMFLOAT2(1.0f, 0.0f);

	vertices[3].position = XMFLOAT3(1.0f, -1.0f, 0.0f);
	vertices[3].normal = XMFLOAT3(0.0f, 0.0f, 0.0f);
	vertices[3].uv = XMFLOAT2(1.0f, 1.0f);

	D3D11_BUFFER_DESC vboDesc = {};
	D3D11_BUFFER_DESC iboDesc = {};
	D3D11_SUBRESOURCE_DATA vboData = {};
	D3D11_SUBRESOURCE_DATA iboData = {};

	vboDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vboDesc.ByteWidth = 4 * sizeof(Vertex);
	vboDesc.CPUAccessFlags = 0;
	vboDesc.MiscFlags = 0;
	vboDesc.StructureByteStride = 0;
	vboDesc.Usage = D3D11_USAGE_DEFAULT;

	vboData.pSysMem = vertices;

	iboDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	iboDesc.ByteWidth = 6 * sizeof(unsigned int);
	iboDesc.CPUAccessFlags = 0;
	iboDesc.MiscFlags = 0;
	iboDesc.StructureByteStride = 0;
	iboDesc.Usage = D3D11_USAGE_DEFAULT;

	iboData.pSysMem = indices;

	result = D3DContext::getCurrent()->getDevice()->CreateBuffer(&vboDesc, &vboData, &m_baseVBO);
	if (FAILED(result))
	{
		std::cout << "Failed to create vertex buffer.. HRESULT " << result << std::endl;
		return false;
	}

	result = D3DContext::getCurrent()->getDevice()->CreateBuffer(&iboDesc, &iboData, &m_baseIBO);
	if (FAILED(result))
	{
		std::cout << "Failed to create index buffer.. HRESULT " << result << std::endl;
		return false;
	}
	return true;
}

void FullscreenQuad::destroy()
{
	m_baseVBO->Release();
	m_baseIBO->Release();
}

void FullscreenQuad::draw()
{
	D3DContext::getCurrent()->getDeviceContext()->IASetVertexBuffers(0, 1, &m_baseVBO, &m_stride, &m_offset);
	D3DContext::getCurrent()->getDeviceContext()->IASetIndexBuffer(m_baseIBO, DXGI_FORMAT_R32_UINT, 0);	//32 bit unsigned int
	D3DContext::getCurrent()->getDeviceContext()->DrawIndexed(6, 0, 0);
}