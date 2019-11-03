#pragma once

#include "BaseRenderer.h"
#include "Light.h"
#include "VertexShader.h"
#include "ConstantBuffer.h"

struct ShadowCamera
{
	XMMATRIX m_projection;
	XMMATRIX m_view;
};

class DirectionalShadowMap
{
public:
	bool init(float width, float height, float orthoWidth, float orthoHeight, DirectionalLight& lightInfo);
	void destroy();

	void beginFrame(DirectionalLight& lightInfo, XMFLOAT2 dimensions);
	void endFrame();

	void bindDepthTexturePS(int samplerStateBinding, int textureBinding);
	void unbindDepthTexturePS(int textureBinding);
	void bindShadowCamera(int constBufferBinding);

	ID3D11ShaderResourceView* getDepthBufferView();
private:
	float m_width, m_height;
	ShadowCamera m_shadowCamera;

	ID3D11Texture2D* m_depthBuffer;
	ID3D11ShaderResourceView* m_depthBufferView;
	ID3D11DepthStencilView* m_depthStencilView;
	D3D11_VIEWPORT m_viewport;

	ID3D11RasterizerState* m_shadowMapRasterState;
	ID3D11SamplerState* m_samplerState;

	ConstantBuffer m_cameraUploadBuffer;
	VertexShader m_vertexShader;
};