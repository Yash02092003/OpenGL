#define STB_IMAGE_IMPLEMENTATION
#include <iostream>
#include <cmath>
#include <string.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm.hpp>
#include <gtc/type_ptr.hpp>
#include <gtc/matrix_transform.hpp>
#include <vector>
#include "Mesh.h"
#include "Shader.h"
#include "Window.h"
#include "Camera.h"
#include "Texture.h"
//#include "stb_image.h"
#include "DirectionalLight.h"
#include "Material.h"
#include "CommonValue.h"
#include "PointLight.h"

const int Width = 1920, Height = 1200;

Window mainWindow;
Camera camera;
Texture brickTexture;
Texture dirtTexture;
DirectionalLight light;
PointLight pointLights[MAX_POINT_LIGHTS];

const float toRadians = 3.14 / 180;

std::vector<Mesh*> meshList;
std::vector<Shader*> shaderList;

GLfloat deltaTime = 0.0f;
GLfloat lastTIme = 0.0f;

Material shinyMaterial;
Material dullMaterial;

static const char* vShader = "                   \n\
             #version 330                        \n\
             layout (location = 0 ) in vec3 pos;\n\
	         layout(location = 1) in vec2 tex; \n\
	         layout(location = 2) in vec3 norm; \n\
             out vec4 vColour;                  \n\
             out vec2 TexCoord;                  \n\
	         out vec3 Normal;                    \n\
             out vec3 FragPos;                    \n\
             uniform mat4 model;                  \n\
	         uniform mat4 projection;              \n\
	         uniform mat4 view;                  \n\
	         void main(){                         \n\
                   gl_Position = projection * view *  model *  vec4(pos.x , pos.y , pos.z , 1.0);\n\
                   vColour = vec4(clamp(pos , 0.0f , 1.0f ) , 1.0f);\n\
			       TexCoord = tex;                 \n\
	               Normal = mat3(transpose(inverse(model))) * norm;  \n\
                   FragPos = (model * vec4(pos.x , pos.y , pos.z , 1.0)).xyz; // swizling                                                   \n\
               }                                      \n\
";

static const char* fShader = "                        \n\
    #version 330                         \n\
    out vec4 colour;                     \n\
    in vec2 TexCoord;                    \n\
    in vec4 vColour;                      \n\
    in vec3 Normal;                       \n\
    in vec3 FragPos;                     \n\
    const int MAX_POINT_LIGHTS = 3;     \n\
    struct Light {                       \n\
        vec3 col; float ambientIntensity; float diffuseIntensity; \n\
    }; \n\
    struct DirectionalLight { \n\
        Light base; \n\
        vec3 direction; \n\
    }; \n\
    struct PointLight { \n\
        Light base; \n\
        vec3 position;\n\
        float constant;\n\
        float linear;\n\
        float exponent;\n\
    }; \n\
    struct Material{                    \n\
        float specularIntensity;        \n\
        float shininess;                \n\
    };                                 \n\
    uniform sampler2D theTexture;       \n\
    uniform int pointLightCount;        \n\
    uniform DirectionalLight dLight;    \n\
    uniform PointLight pointLights[MAX_POINT_LIGHTS]; \n\
    uniform Material material;          \n\
    uniform vec3 eyePosition;           \n\
    vec4 CalcLightByDirection(Light light, vec3 direction) {\n\
        vec4 ambientColour = vec4(light.col, 1.0f) * light.ambientIntensity; \n\
        float diffuseFactor = max(dot(normalize(Normal), normalize(direction)), 0.0f); \n\
        vec4 specularColour = vec4(0, 0, 0, 0); \n\
        if (diffuseFactor > 0.0f) { \n\
            vec3 fragToEye = normalize(eyePosition - FragPos); \n\
            vec3 reflectedVertex = normalize(reflect(direction, normalize(Normal))); \n\
            float specularFactor = dot(fragToEye, reflectedVertex); \n\
            if (specularFactor > 0.0f) {\n\
                specularFactor = pow(specularFactor, material.shininess); \n\
                specularColour = vec4(light.col * material.specularIntensity * specularFactor, 1.0f);\n\
            }\n\
        }\n\
        vec4 diffuseColour = vec4(light.col, 1.0f) * light.diffuseIntensity * diffuseFactor; \n\
        return (ambientColour + diffuseColour + specularColour);\n\
    }\n\
    vec4 CalcDirectionalLight() {\n\
        return CalcLightByDirection(dLight.base, dLight.direction);\n\
    }\n\
    vec4 CalcPointLights() {\n\
        vec4 totalColour = vec4(0, 0, 0, 0);\n\
        for (int i = 0; i < pointLightCount; i++) {\n\
            vec3 direction = FragPos - pointLights[i].position;\n\
            float distance = length(direction);\n\
            direction = normalize(direction);\n\
            vec4 colour = CalcLightByDirection(pointLights[i].base, direction);\n\
            float attenuation = 1.0 / (pointLights[i].constant + pointLights[i].linear * distance + pointLights[i].exponent * distance * distance);\n\
            totalColour += (colour * attenuation);\n\
        }\n\
        return totalColour;\n\
    }\n\
    void main() {\n\
        vec4 finalColour = CalcDirectionalLight();\n\
        finalColour += CalcPointLights();\n\
        colour = texture(theTexture, TexCoord) * finalColour;\n\
    }\n\
";


void calcAverageNormals(unsigned int* indices, unsigned int indiceCount, GLfloat*  vertices, unsigned int verticeCount, unsigned int vLength, unsigned int normalOffset) {
	for (size_t i = 0; i < indiceCount; i += 3) {
		unsigned int in0 = indices[i] * vLength;
		unsigned int in1 = indices[i + 1] * vLength;
		unsigned int in2 = indices[i + 2] * vLength;
		glm::vec3 v1(vertices[in1] - vertices[in0], vertices[in1 + 1] - vertices[in0 + 1], vertices[in1 + 2] - vertices[in0 + 2]);
		glm::vec3 v2(vertices[in2] - vertices[in0], vertices[in2 + 1] - vertices[in0 + 1], vertices[in2 + 2] - vertices[in0 + 2]);
		glm::vec3 normal = glm::cross(v1, v2);
		normal = glm::normalize(normal);

		in0 += normalOffset;
		in1 += normalOffset;
		in2 += normalOffset;

		vertices[in0] += normal.x;
		vertices[in0 + 1] += normal.y;
		vertices[in0 + 2] += normal.z;

		vertices[in1] += normal.x;
		vertices[in1 + 1] += normal.y;
		vertices[in1 + 2] += normal.z;

		vertices[in1] += normal.x;
		vertices[in1 + 1] += normal.y;
		vertices[in1 + 2] += normal.z;
	}

	for (size_t i = 0; i < verticeCount / vLength; i++) {
		unsigned int nOffset = i * vLength + normalOffset;
		glm::vec3 vec(vertices[nOffset], vertices[nOffset + 1], vertices[nOffset + 2]);
		vec = glm::normalize(vec);
		vertices[nOffset] = vec.x; vertices[nOffset + 1] = vec.y; vertices[nOffset + 2] = vec.z;
	}
}

static void createTriangle() {
	
	unsigned int indices[] = {
		0 , 3 , 1 , 
		1 , 3 , 2 ,
		2 , 3 , 0 ,
		0 , 1 , 2 
	};

	GLfloat vertices[] = {
	//    x     y      z       u      v
		0.0f , 1.1f , 0.0f ,     0.0f ,  0.0f ,    0.0f , 0.0f , 0.0f , 
		0.0f , -1.0f , 1.0f ,    0.5f , 0.0f ,     0.0f , 0.0f , 0.0f ,
		1.1f , -1.1f , 0.0f ,    1.0f , 0.0f ,     0.0f , 0.0f , 0.0f ,
		-1.1f , -1.1f , 0.0f ,   0.5f , 1.0f ,     0.0f , 0.0f , 0.0f 
	};

	unsigned int floorIndices[] = {
		0 , 2 , 1 ,
		1 , 2 , 3 
	};

	GLfloat floorVertices[] = {
	//   x         y        z
		-10.0f , 0.0f , -10.0f , 0.0f , 0.0f ,  0.0f , -1.0f , 0.0f ,
		10.0f , 0.0f , -10.0f ,  10.0f , 0.0f , 0.0f , -1.0f , 0.0f ,
		-10.0f , 0.0f , 10.0f ,  0.0f , 10.0f , 0.0f , -1.0f , 0.0f ,
		10.0f , 0.0f , 10.0f  ,  10.0f , 10.0f , 0.0f , -1.0f , 0.0f 
	};


	calcAverageNormals(indices, 12, vertices, 32, 8, 5);

	Mesh* obj1 = new Mesh();
	obj1->createMesh(vertices, indices, 32, 12);
	meshList.push_back(obj1);

	Mesh* obj2 = new Mesh();
	obj2->createMesh(vertices, indices, 32, 12);
	meshList.push_back(obj2);

	Mesh* obj3 = new Mesh();
	obj3->createMesh(floorVertices, floorIndices, 32, 6);
	meshList.push_back(obj3);

}

void createShaders() {
	Shader* shader1 = new Shader();
	shader1->createFromString(vShader, fShader);
	shaderList.push_back(shader1);
}

int main()
{
	mainWindow = Window();

	mainWindow.initialise();

	createTriangle();
	createShaders();

	camera = Camera(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, 0.0f, 5.0f, 0.5f);

	brickTexture = Texture("Textures/brick.png");
	brickTexture.LoadTexture();

	dirtTexture = Texture("Textures/dirt.png");
	dirtTexture.LoadTexture();

	shinyMaterial = Material(1.0f, 32);
	dullMaterial = Material(0.3f, 4);

	light = DirectionalLight(1.0f , 1.0f , 1.0f , 0.5f, 0.3f , 1.0f , 1.0f , 2.0f );

	unsigned int pointLightCount = 0;
	pointLights[0] = PointLight(0.0f, 1.0f, 0.0f, 0.6f, 0.1f, -4.0f, 0.0f, 0.0f, 0.3f, 0.2f, 0.1f);
	pointLightCount++;

	pointLights[1] = PointLight(0.0f, 0.0f, 1.0f, 0.2f, 0.5f, 4.0f, 0.0f, 0.0f, 0.3f, 0.2f, 0.1f);
	pointLightCount++;

	pointLights[2] = PointLight(1.0f, 0.0f, 0.0f, 0.8f, 0.9f, 0.0f, 0.0f, 4.0f, 0.3f, 0.2f, 0.1f);
	pointLightCount++;



	GLuint uniformProjection = 0;
	GLuint uniformModel = 0;
	GLuint uniformView = 0;
	GLuint uniformEyePosition = 0;
	GLuint uniformSpecularIntensity = 0;
	GLuint uniformShininess = 0;
	glm::mat4 projection = glm::perspective(45.0f , (GLfloat)mainWindow.getBufferWidth() / (GLfloat) mainWindow.getBufferHeight(), 0.1f, 100.0f);
	
	while (!mainWindow.getShouldWindowClose()) {

		GLfloat now = glfwGetTime(); // in SDL SDL_GetPerformanceCounter();
		deltaTime = now - lastTIme; // in SDl now - lasTime / SDL_GetPerformanceFrequence
		lastTIme = now;
 

		glfwPollEvents();

		camera.keyControl(mainWindow.getsKeys() , deltaTime);
		camera.mouseControl(mainWindow.getXChange(), mainWindow.getYChange());

		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

		shaderList[0]->useShader();


		uniformModel = shaderList[0]->getModelLocation();
		uniformProjection = shaderList[0]->getProjectionLoaction();
		uniformView = shaderList[0]->getViewLocation();
		uniformEyePosition = shaderList[0]->getUniformEyePosition();
		uniformSpecularIntensity = shaderList[0]->getSpecularIntensityLocation();
		uniformShininess = shaderList[0]->getShininessLocation();

		//light.useLight(uniformAbientIntensity, uniformAmbientColour , uniformDiffusedLight , uniformDirection);
		shaderList[0]->setDirectionalLight(&light);
		shaderList[0]->SetPointLights(pointLights, pointLightCount);

		glm::mat4 model(1.0f);

		model = glm::translate(model, glm::vec3(0.0f, 0.0f , -1.5f ));
		//model = glm::scale(model, glm::vec3(0.1f, 0.1f, 0.1f));
		//model = glm::rotate(model, currAngle * toRadians, glm::vec3(0.0f, 1.0f, 0.0f));

		glUniformMatrix4fv(uniformProjection, 1, GL_FALSE, glm::value_ptr(projection));
		glUniformMatrix4fv(uniformView, 1, GL_FALSE, glm::value_ptr(camera.calculateViewMatrix()));
		glUniform3f(uniformEyePosition , camera.getCameraPosition().x , camera.getCameraPosition().y , camera.getCameraPosition().z);
		

		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		
		brickTexture.UseTexture();
		shinyMaterial.useMaterial(uniformSpecularIntensity, uniformShininess);

		meshList[0]->renderMesh();

		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(2.0f , 0.0f, 1.5f));
		//model = glm::scale(model, glm::vec3(0.4f, 0.4f, 0.4f));
		//model = glm::rotate(model, currAngle * toRadians, glm::vec3(0.0f, 1.0f, 0.0f));

		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		
		dirtTexture.UseTexture();
		dullMaterial.useMaterial(uniformSpecularIntensity, uniformShininess);

		meshList[1]->renderMesh();

		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f , -2.0f , 0.0f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		dirtTexture.UseTexture();
		dullMaterial.useMaterial(uniformSpecularIntensity, uniformShininess);
		meshList[2]->renderMesh();

		mainWindow.swapBuffers();

	}

	return 0;

}

