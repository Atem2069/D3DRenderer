#include "FrustumCuller.h"

void FrustumCuller::createPlanes(XMMATRIX projection, XMMATRIX view)
{
	XMMATRIX resultingMatrix;
	XMFLOAT4X4  matrix;
	XMFLOAT4 currentPlane;

	XMStoreFloat4x4(&matrix, projection);
	float zMinimum = -matrix._43 / matrix._33;
	float r = 1000.0f / (1000.0f - zMinimum);
	matrix._33 = r;
	matrix._43 = -r * zMinimum;

	XMMATRIX tempProjection = XMLoadFloat4x4(&matrix);

	resultingMatrix = XMMatrixMultiply(view, tempProjection);
	XMStoreFloat4x4(&matrix, resultingMatrix);

	//Near plane
	currentPlane.x = matrix._14 + matrix._13;
	currentPlane.y = matrix._24 + matrix._23;
	currentPlane.z = matrix._34 + matrix._33;
	currentPlane.w = matrix._44 + matrix._43;
	planes[0] = XMLoadFloat4(&currentPlane);
	planes[0] = XMPlaneNormalize(planes[0]);

	//Far plane
	currentPlane.x = matrix._14 - matrix._13;
	currentPlane.y = matrix._24 - matrix._23;
	currentPlane.z = matrix._34 - matrix._33;
	currentPlane.w = matrix._44 - matrix._43;
	planes[1] = XMLoadFloat4(&currentPlane);
	planes[1] = XMPlaneNormalize(planes[1]);

	//Left plane
	currentPlane.x = matrix._14 + matrix._11;
	currentPlane.y = matrix._24 + matrix._21;
	currentPlane.z = matrix._34 + matrix._31;
	currentPlane.w = matrix._44 + matrix._41;
	planes[2] = XMLoadFloat4(&currentPlane);
	planes[2] = XMPlaneNormalize(planes[2]);

	//Right plane
	currentPlane.x = matrix._14 - matrix._11;
	currentPlane.y = matrix._24 - matrix._21;
	currentPlane.z = matrix._34 - matrix._31;
	currentPlane.w = matrix._44 - matrix._41;
	planes[3] = XMLoadFloat4(&currentPlane);
	planes[3] = XMPlaneNormalize(planes[3]);

	//Top plane
	currentPlane.x = matrix._14 - matrix._12;
	currentPlane.y = matrix._24 = matrix._22;
	currentPlane.z = matrix._34 - matrix._32;
	currentPlane.w = matrix._44 - matrix._42;
	planes[4] = XMLoadFloat4(&currentPlane);
	planes[4] = XMPlaneNormalize(planes[4]);

	//Bottom plane
	currentPlane.x = matrix._14 + matrix._12;
	currentPlane.y = matrix._24 + matrix._22;
	currentPlane.z = matrix._34 + matrix._32;
	currentPlane.w = matrix._44 + matrix._42;
	planes[5] = XMLoadFloat4(&currentPlane);
	planes[5] = XMPlaneNormalize(planes[5]);
}

bool FrustumCuller::getPointIntersection(float x, float y, float z)
{
	for (int i = 0; i < 6; i++)
	{
		//Long way cause directxmath fucking sucks.....
		XMVECTOR temp = XMPlaneDotCoord(planes[i], XMVectorSet(x, y, z, 1.0f));
		XMFLOAT4 temp2;
		XMStoreFloat4(&temp2, temp);

		if (temp2.x < 0.0f)
			return false;
	}

	return true;
}

XMVECTOR FrustumCuller::planes[6] = {};