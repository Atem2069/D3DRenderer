#pragma once

//All D3D11 Texturing. Currently Tex2D but will be expanded


#include "BaseRenderer.h"	//For device references
#include <iostream>
#include <string>

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
	void unpackRGBToRGBA(int width, int height, unsigned char * input, unsigned char * output);
	ID3D11Texture2D* m_texture;
	ID3D11ShaderResourceView* m_textureView;
};