#pragma once

#include "BaseRenderer.h"

struct cb_perspectiveCamera
{
	XMMATRIX projection;
	XMMATRIX view;
	XMVECTOR position;
	XMMATRIX orthoVoxel;
};

struct perspectiveCameraChangeInfo
{
	XMVECTOR position;
	XMVECTOR lookAt;
	XMVECTOR up;
};

class PerspectiveCamera
{
public:
	bool init(float width, float height, float fovDeg, float minDepth, float maxDepth);

	void update();
	void bind(int bindingPoint);

	perspectiveCameraChangeInfo cameraChangeInfo = {};

	XMMATRIX getProjection();
	XMMATRIX getView();
private:
	XMMATRIX m_projection, m_view;
	cb_perspectiveCamera m_cameraBuffer = {};

	ID3D11Buffer* m_constBuffer;
};