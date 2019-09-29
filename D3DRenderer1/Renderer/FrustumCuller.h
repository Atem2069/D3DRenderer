#pragma once

#include "BaseRenderer.h"
#include <iostream>

class FrustumCuller
{
public:
	static void createPlanes(XMMATRIX projection, XMMATRIX view);

	static bool getPointIntersection(float x, float y, float z);
private:

	static XMVECTOR planes[6];
};