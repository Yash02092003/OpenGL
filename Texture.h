#pragma once
#include <GL/glew.h>
#include"stb_image.h"

class Texture
{
public:
	Texture();
	~Texture();

	Texture(const char* fileLoc);

	bool LoadTexture();
	bool LoadTextureA();
	void UseTexture();
	void ClearTexture();


private:
	GLuint textureID;
	int width, height, bitDepth;

	const char* fileLocation;
};

