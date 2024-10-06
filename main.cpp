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
#include "SpotLight.h"

#include "Model.h"
#include <assimp/Importer.hpp>

const int Width = 1920, Height = 1200;

Window mainWindow;
Camera camera;
Texture brickTexture;
Texture dirtTexture;
DirectionalLight mainlight;
PointLight pointLights[MAX_POINT_LIGHTS];
SpotLight spotLights[MAX_SPOT_LIGHTS];

const float toRadians = 3.14 / 180;

std::vector<Mesh*> meshList;
std::vector<Shader*> shaderList;
Shader directionalShadowShader;

GLfloat deltaTime = 0.0f;
GLfloat lastTIme = 0.0f;

Material shinyMaterial;
Material dullMaterial;

Model xwing;
Model blackhawk;

GLuint uniformProjection = 0;
GLuint uniformModel = 0;
GLuint uniformView = 0;
GLuint uniformEyePosition = 0;
GLuint uniformSpecularIntensity = 0;
GLuint uniformShininess = 0;

unsigned int pointLightCount = 0;
unsigned int spotLightCount = 0;

static const char* vShader = "                   \n\
             #version 330                        \n\
             layout (location = 0 ) in vec3 pos;\n\
	         layout(location = 1) in vec2 tex; \n\
	         layout(location = 2) in vec3 norm; \n\
             out vec4 vColour;                  \n\
             out vec2 TexCoord;                  \n\
	         out vec3 Normal;                    \n\
             out vec3 FragPos;                    \n\
             out vec4 DirectionalLightSpacePos;   \n\
             uniform mat4 model;                  \n\
	         uniform mat4 projection;              \n\
	         uniform mat4 view;                     \n\
	         uniform mat4 directionalLightSpaceTransform; \n\
	         void main(){                         \n\
                   gl_Position = projection * view *  model *  vec4(pos.x , pos.y , pos.z , 1.0);\n\
                   DirectionalLightSpacePos = directionalLightSpaceTransform * model * vec4(pos , 1.0); \n\
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
    in vec4 DirectionalLightSpacePos;    \n\
    const int MAX_POINT_LIGHTS = 3;   \n\
    const int MAX_SPOT_LIGHTS = 3; \n\
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
    };\n\
	struct SpotLight{\n\
		PointLight base;\n\
		vec3 direction;\n\
		float edge;\n\
	}; \n\
    struct Material{                    \n\
        float specularIntensity;        \n\
        float shininess;                \n\
    };                                 \n\
    uniform sampler2D theTexture;       \n\
    uniform sampler2D directionalShadowMap;\n\
    uniform int pointLightCount; \n\
    uniform int spotLightCount; \n\
    uniform DirectionalLight dLight;    \n\
    uniform PointLight pointLights[MAX_POINT_LIGHTS]; \n\
	uniform SpotLight spotLights[MAX_SPOT_LIGHTS]; \n\
    uniform Material material;          \n\
    uniform vec3 eyePosition;\n\
    float calcDirectionalShadowFactor(DirectionalLight light){\n\
		vec3 projCoords = DirectionalLightSpacePos.xyz / DirectionalLightSpacePos.w;\n\
		projCoords = (projCoords * 0.5) + 0.5;\n\
		//float closest = texture(directionalShadowMap , projCoords.xy).r;\n\
		float current = projCoords.z;\n\
		vec3 normal = normalize(Normal);\n\
		vec3 lightDir = normalize(light.direction);\n\
		float bias = max(0.05 * (1 - dot(normal , lightDir)) , 0.005);\n\
		float shadow = 0.0f;\n\
		vec2 texelSize = 1.0 / textureSize(directionalShadowMap , 0);\n\
		for(int x = -1 ; x <= 1 ; ++x){\n\
			for (int y = -1; y <= 1; ++y) {\n\
				float pcfDepth = texture(directionalShadowMap, projCoords.xy + vec2(x, y) * texelSize).r;\n\
				shadow += current - bias > pcfDepth ? 1.0 : 0.0;\n\
			}\n\
		}\n\
		shadow /= 9.0f;\n\
		if(projCoords.z > 1.0) shadow = 0.0; \n\
		return shadow;\n\
	}\n\
    vec4 CalcLightByDirection(Light light, vec3 direction , float shadowFactor) {\n\
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
        return (ambientColour + (1.0 - shadowFactor) * (diffuseColour + specularColour));\n\
    }\n\
	vec4 CalcPointLight(PointLight pLight) {\n\
			vec3 direction = FragPos - pLight.position; \n\
			float distance = length(direction); \n\
			direction = normalize(direction); \n\
			vec4 colour = CalcLightByDirection(pLight.base, direction , 0.0f); \n\
			float attenuation = 1.0 / (pLight.constant + pLight.linear * distance + pLight.exponent * distance * distance); \n\
			return (colour / attenuation); \n\
	}\n\
    vec4 CalcDirectionalLight() {\n\
        float shadowFactor = calcDirectionalShadowFactor(dLight);\n\
        return CalcLightByDirection(dLight.base, dLight.direction , shadowFactor);\n\
    }\n\
    vec4 CalcPointLights() {\n\
        vec4 totalColour = vec4(0, 0, 0, 0);\n\
        for (int i = 0; i < pointLightCount; i++) {\n\
			totalColour += CalcPointLight(pointLights[i]);\n\
        }\n\
        return totalColour;\n\
    }\n\
	vec4 CalcSpotLight(SpotLight sLight){\n\
		vec3 rayDirection = normalize(FragPos - sLight.base.position);\n\
		float slFactor = dot(rayDirection, sLight.direction);\n\
		if (slFactor > sLight.edge) {\n\
			vec4 colour = CalcPointLight(sLight.base);\n\
			return colour *(1.0f - (1.0f - slFactor) * (1.0f / (1.0f - sLight.edge)));\n\
		}\n\
		else {\n\
			return vec4(0, 0, 0, 0);\n\
		}\n\
	}\n\
	vec4 CalcSpotLights(){\n\
		vec4 totalColour = vec4(0, 0, 0, 0);\n\
		for (int i = 0; i < spotLightCount; i++) {\n\
			totalColour += CalcSpotLight(spotLights[i]);\n\
		}\n\
		return totalColour;\n\
	}\n\
	void main() {\n\
			vec4 finalColour = CalcDirectionalLight(); \n\
			finalColour += CalcPointLights(); \n\
			finalColour += CalcSpotLights();\n\
        colour = texture(theTexture, TexCoord) * finalColour;\n\
    }\n\
";

static const char* directionalMapV = "\n\
                             #version 330 \n\
                             layout (location = 0) in vec3 pos; \n\
                             uniform mat4 model; \n\
                             uniform mat4 directionalLightSpaceTransform; \n\
                             void main() { \n\
                                          gl_Position = directionalLightSpaceTransform * model * vec4(pos , 1.0) ; \n\
                             } \n\
";

static const char* directionalMapF = "\n\
                          #version 330 \n\
                          void main() {}";

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

	directionalShadowShader = Shader();
	directionalShadowShader.createFromString(directionalMapV, directionalMapF);

}

void RenderScene() {
	glm::mat4 model(1.0f);

	model = glm::translate(model, glm::vec3(0.0f, 0.0f, -1.5f));
	//model = glm::scale(model, glm::vec3(0.1f, 0.1f, 0.1f));
	//model = glm::rotate(model, currAngle * toRadians, glm::vec3(0.0f, 1.0f, 0.0f));

	glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));

	brickTexture.UseTexture();
	shinyMaterial.useMaterial(uniformSpecularIntensity, uniformShininess);

	meshList[0]->renderMesh();

	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(2.0f, 0.0f, 1.5f));
	//model = glm::scale(model, glm::vec3(0.4f, 0.4f, 0.4f));
	//model = glm::rotate(model, currAngle * toRadians, glm::vec3(0.0f, 1.0f, 0.0f));

	glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));

	dirtTexture.UseTexture();
	dullMaterial.useMaterial(uniformSpecularIntensity, uniformShininess);

	meshList[1]->renderMesh();

	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, -2.0f, 0.0f));
	glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
	dullMaterial.useMaterial(uniformSpecularIntensity, uniformShininess);
	meshList[2]->renderMesh();

	model = glm::mat4(1.0f);
	model = glm::scale(model, glm::vec3(0.5f, 0.5f, 0.5f));
	glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
	xwing.RenderModel();
}

void DirectionalShadowMapPass(DirectionalLight* light) {
	directionalShadowShader.useShader();
	glViewport(0, 0, light->getShadowMap()->GetShadowWidth(), light->getShadowMap()->GetShadowWidth());
	light->getShadowMap()->write();
	glClear(GL_DEPTH_BUFFER_BIT);
	uniformModel = directionalShadowShader.getModelLocation();
	glm::mat4 lTransform = light->calculateLightTransform();
	glm::mat4* lTransformPtr = &lTransform;
	directionalShadowShader.SetDirectionalLightTransform(lTransformPtr);

	RenderScene();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RenderPass(glm::mat4 projectionMatrix, glm::mat4 viewMatrix) {
	shaderList[0]->useShader();

	uniformModel = shaderList[0]->getModelLocation();
	uniformProjection = shaderList[0]->getProjectionLoaction();
	uniformView = shaderList[0]->getViewLocation();
	uniformEyePosition = shaderList[0]->getUniformEyePosition();
	uniformSpecularIntensity = shaderList[0]->getSpecularIntensityLocation();
	uniformShininess = shaderList[0]->getShininessLocation();

	glViewport(0, 0, 1920, 1200);

	//Claer the Window
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//mainlight.useLight(uniformAbientIntensity, uniformAmbientColour , uniformDiffusedLight , uniformDirection);

	glUniformMatrix4fv(uniformProjection, 1, GL_FALSE, glm::value_ptr(projectionMatrix));
	glUniformMatrix4fv(uniformView, 1, GL_FALSE, glm::value_ptr(viewMatrix));
	glUniform3f(uniformEyePosition, camera.getCameraPosition().x, camera.getCameraPosition().y, camera.getCameraPosition().z);
	
	shaderList[0]->setDirectionalLight(&mainlight);
	shaderList[0]->SetPointLights(pointLights, pointLightCount);
	shaderList[0]->SetSpotLights(spotLights, spotLightCount);
	glm::mat4 lTransform = mainlight.calculateLightTransform();
	glm::mat4* lTransformPtr = &lTransform;
	shaderList[0]->SetDirectionalLightTransform(lTransformPtr);


	mainlight.getShadowMap()->Read(GL_TEXTURE1);
	shaderList[0]->SetTexture(0);
	shaderList[0]->SetDirectionalShadowMap(1);

	glm::vec3 lowerLight = camera.getCameraPosition();
	lowerLight.y -= 0.3f;
	//spotLights[0].SetFlash(lowerLight, camera.getCameraDirection());

	RenderScene();

}

int main()
{
	mainWindow = Window(Width , Height);

	mainWindow.initialise();

	createTriangle();
	createShaders();

	camera = Camera(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, 0.0f, 5.0f, 0.5f);

	brickTexture = Texture("Textures/brick.png");
	brickTexture.LoadTextureA();

	dirtTexture = Texture("Textures/dirt.png");
	dirtTexture.LoadTextureA();

	shinyMaterial = Material(1.0f, 32);
	dullMaterial = Material(0.3f, 4);

	xwing = Model();
	xwing.LoadModel("Models/star wars x-wing.obj");


	mainlight = DirectionalLight(2048 , 2048 , 1.0f , 1.0f , 1.0f , 0.1f, 0.6f , 0.0f , -15.0f , -10.0f );

	
	//pointLights[0] = PointLight(0.0f, 1.0f, 0.0f, 0.6f, 0.1f, -4.0f, 0.0f, 0.0f, 0.3f, 0.2f, 0.1f);
	//pointLightCount++;

	//pointLights[1] = PointLight(0.0f, 0.0f, 1.0f, 0.2f, 0.5f, 4.0f, 0.0f, 0.0f, 0.3f, 0.2f, 0.1f);
	//pointLightCount++;

	//pointLights[2] = PointLight(1.0f, 0.0f, 0.0f, 0.8f, 0.9f, 0.0f, 0.0f, 4.0f, 0.3f, 0.2f, 0.1f);
	//pointLightCount++;

	
	spotLights[0] = SpotLight(1.0f, 1.0f, 1.0f, 0.6f, 0.1f, -4.0f, 0.0f, 0.0f, 0.0f , -1.0f , 0.0f ,  1.0f, 0.0f, 0.0f , 20.0f);
	spotLightCount++;

	spotLights[1] = SpotLight(1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.5f, 0.0f, -5.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 20.0f);
	spotLightCount++;

	Assimp::Importer* imp = new Assimp::Importer();


	glm::mat4 projection = glm::perspective(45.0f , (GLfloat)mainWindow.getBufferWidth() / (GLfloat) mainWindow.getBufferHeight(), 0.1f, 100.0f);
	
	while (!mainWindow.getShouldWindowClose()) {

		GLfloat now = glfwGetTime(); // in SDL SDL_GetPerformanceCounter();
		deltaTime = now - lastTIme; // in SDl now - lasTime / SDL_GetPerformanceFrequence
		lastTIme = now;
 

		glfwPollEvents();

		camera.keyControl(mainWindow.getsKeys() , deltaTime);
		camera.mouseControl(mainWindow.getXChange(), mainWindow.getYChange());

		DirectionalShadowMapPass(&mainlight);

		RenderPass(projection, camera.calculateViewMatrix());

		mainWindow.swapBuffers();

	}

	return 0;

}

