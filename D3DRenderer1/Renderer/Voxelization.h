#pragma once

#include "BaseRenderer.h"
#include "VertexShader.h"
#include "PixelShader.h"
#include "GeometryShader.h"

class SceneVoxelizer
{
public:
	bool init();
	void destroy();

	void beginVoxelizationPass();
	void endVoxelizationPass();

	void bindVoxelTexture(int bindingPoint, int samplerBindingPoint);
	void unbindVoxelTexture(int bindingPoint);

	ID3D11ShaderResourceView* getTextureView();
private:
	int m_width = 256, m_height = 256, m_depth = 256;
	D3D11_VIEWPORT m_viewport;
	ID3D11Texture3D* m_voxelTexture;
	ID3D11ShaderResourceView* m_voxelTextureView;
	ID3D11UnorderedAccessView* m_voxelTextureUAV;

	ID3D11SamplerState* m_voxelTexSamplerState;
	ID3D11RasterizerState2* m_voxelRasterState;
	ID3D11DepthStencilState* m_noDepthTestState;
	VertexShader m_vertexShader;
	PixelShader m_pixelShader;
	GeometryShader m_geometryShader;

};
