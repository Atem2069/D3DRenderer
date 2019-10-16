#pragma once

#include "BaseRenderer.h"

//Simply holds structs to a few basic light objects.

__declspec(align(16))
struct DirectionalLight
{
	XMFLOAT4 color;
	XMFLOAT4 direction;
	float specularPower;
	float m_unusedAlignment[3];
};