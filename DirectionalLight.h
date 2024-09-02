#pragma once
#include "Light.h"
class DirectionalLight : public Light
{
public:
	DirectionalLight();
	~DirectionalLight();

	DirectionalLight(GLfloat red, GLfloat green, GLfloat blue, GLfloat aIntensity, GLfloat dIntensity , GLfloat xDir, GLfloat yDir, GLfloat zDir);
	
	void useLight(GLuint ambientIntensityLocation, GLuint ambientColourLocation, GLfloat diffuseIntensityLocation, GLfloat directionLocation);


private:
	glm::vec3 direction;
};

