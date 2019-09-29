#pragma once

//All D3D11 Texturing. Currently Tex2D but will be expanded


#include "BaseRenderer.h"	//For device references
#include <iostream>

//stb image for img loading
#include <stb_image.h>


class Texture2D
{
public:
	bool init(std::string filePath);
	void destroy();

	void bind(int textureBindingPoint);

	ID3D11Texture2D* getTexture();
	ID3D11ShaderResourceView* getTextureView();
	int m_channels;
private:
	void fast_unpack(unsigned char* rgba, const unsigned char* rgb, const int count) {
		if (count == 0)
			return;
		for (int i = count; --i; rgba += 4, rgb += 3) {
			*(uint32_t*)(void*)rgba = *(const uint32_t*)(const void*)rgb;
		}
		for (int j = 0; j < 3; ++j) {
			rgba[j] = rgb[j];
		}
	}
	ID3D11Texture2D* m_texture;
	ID3D11ShaderResourceView* m_textureView;
};