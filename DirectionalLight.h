#pragma once
#include "Light.h"
class DirectionalLight : public Light
{
public:
	DirectionalLight();
	~DirectionalLight();

	DirectionalLight(GLfloat shadowWidth, GLfloat shadowHeight , GLfloat red, GLfloat green, GLfloat blue, GLfloat aIntensity, GLfloat dIntensity , GLfloat xDir, GLfloat yDir, GLfloat zDir);
	
	void useLight(GLuint ambientIntensityLocation, GLuint ambientColourLocation, GLfloat diffuseIntensityLocation, GLfloat directionLocation);

	glm::mat4 calculateLightTransform();

private:
	glm::vec3 direction;
};

