#pragma once
#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>
#include <GL/glew.h>
#include "DirectionalLight.h"
#include "PointLight.h"
#include "CommonValue.h"
#include "SpotLight.h"


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

	void setDirectionalLight(DirectionalLight* dlight);
	void SetPointLights(PointLight* pLight, unsigned int lightCount);

	void SetSpotLights(SpotLight* sLight, unsigned int lightCount);



private:
	int pointLightCount;
	int spotLightCount;
	
	GLuint uniformPointLightCount;

	GLuint shaderID, uniformProjection, uniformModel , uniformView , uniformEyePosition , uniformSpecularIntensity , uniformShininess; 

	struct {
		GLuint uniformColour;
		GLuint uniformAmbientIntensity;
		GLuint uniformDiffuseIntensity;
		GLuint uniformDirection;
	} uniformDirectionalLight;

	struct {
		GLuint uniformColour;
		GLuint uniformAmbientIntensity;
		GLuint uniformDiffuseIntensity;
		
		GLuint uniformPosition;
		GLuint uniformConstant;
		GLuint uniformLinear;
		GLuint uniformExponent;
	} uniformPointLight[MAX_POINT_LIGHTS];

	GLuint uniformSpotLightCount;

	struct {
		GLuint uniformColour;
		GLuint uniformAmbientIntensity;
		GLuint uniformDiffuseIntensity;

		GLuint uniformPosition;
		GLuint uniformConstant;
		GLuint uniformLinear;
		GLuint uniformExponent;

		GLuint uniformDirection;
		GLuint uniformEdge;
	} uniformSpotLight[MAX_SPOT_LIGHTS];


	void compileShader(const char* vertexCode, const char* fragmnentCode );
	void addShader(GLuint theProgram, const char* shaderCode, GLenum shaderType);
};

