#pragma once
#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>
#include <GL/glew.h>


class Shader
{
public : 
	Shader();
	~Shader();

	void createFromString(const char* vertexCode , const char * fragmnentCode);
	void createFromFiles(const char* vertexLocation, const char* fragmentLocation);

	std::string ReadFile(const char* fileLocation);

	GLuint getProjectionLoaction();
	GLuint getModelLocation();
	GLuint getViewLocation();

	GLuint getAmbientIntensity();
	GLuint getAmbientColour();

	GLuint getDiffuseIntensity();
	GLuint getDirectionLocation();

	GLuint getSpecularIntensityLocation();
	GLuint getShininessLocation();

	GLuint getUniformEyePosition();

	void useShader();
	void clearShader();


private:
	GLuint shaderID, uniformProjection, uniformModel , uniformView , uniformAbientIntensity , uniformAmbientColour , uniformDiffuseIntensity , uniformDirection , uniformEyePosition , uniformSpecularIntensity , uniformShininess;

	void compileShader(const char* vertexCode, const char* fragmnentCode );
	void addShader(GLuint theProgram, const char* shaderCode, GLenum shaderType);
};

