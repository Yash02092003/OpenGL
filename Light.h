#pragma once
#include <GL/glew.h>
#include <glm.hpp>
#include "ShadowMap.h"
#include <gtc/matrix_transform.hpp>
class Light
{
public:
	Light();
	Light(GLfloat shadowWidth , GLfloat shadowHeight , GLfloat red, GLfloat green, GLfloat blue, GLfloat aIntensity , GLfloat dIntensity);
	ShadowMap* getShadowMap() { return shadowMap; }
	~Light();

protected:
	glm::vec3 colour;
	GLfloat ambientIntensity;
	GLfloat diffuseIntensity;

	ShadowMap* shadowMap;
	glm::mat4 lightProj;
};

