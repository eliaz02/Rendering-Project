#pragma once


#ifndef FRAME_BUFFER_OBJECT_H 
#define FRAME_BUFFER_OBJECT_H

#include <glad/gl.h>       
#define GLFW_INCLUDE_NONE    
#include <GLFW/glfw3.h>

#include <memory>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp> 

#include "LightStruct.h"
#include "Shader.h"
#include "Debugging.h"
#include "WindowContext.h"
#include "PathConfig.h"

/**
*	All of this class serve the concrete IRender in the handeling of the Frame Buffer Object  
*	for dealing: with the shadow casting, post processing effect and anti-aliasing
*/

class ShadowMapFBO
{
public:
	ShadowMapFBO() = default;
	ShadowMapFBO& operator=(const ShadowMapFBO&) = delete;
	ShadowMapFBO(const unsigned int WIDTH, const unsigned int HEIGHT);
	~ShadowMapFBO();

	void resizeWindow(const unsigned int WIDTH, const unsigned int HEIGHT);
	void Init(std::shared_ptr<Shader> inShader);
	void BindForWriting();
	void BindForReading(GLint TextureUnit);
	void clean();

	unsigned int m_Width{ 0 }, m_Height{ 0 };
	std::shared_ptr<Shader> shader;

private:
	GLuint fbo{ 0 };
	GLuint depthbufferTexture{ 0 };
};


// this class implement an abstraction layer for the cration and handling of a frame buffer object 
// that will wirite and read sampler2DArray 
class ShadowMapArrayFBO
{
public:
	ShadowMapArrayFBO() = default;
	ShadowMapArrayFBO& operator=(const ShadowMapArrayFBO&) = delete;
	ShadowMapArrayFBO(const unsigned int WIDTH, const unsigned int HEIGHT);
	~ShadowMapArrayFBO();

	void resizeWindow(const unsigned int WIDTH, const unsigned int HEIGHT);
	void Init(size_t Size, std::shared_ptr<Shader> inShader);
	void Init(size_t Size); // to be use in combo with SetupShader to have a fully working object 
	void SetupShader(std::shared_ptr<Shader> inShader); 
	void BindLayerForWriting(int layerIndex);
	void BindForReading(GLint TextureUnit);
	void clean();

	unsigned int s_Width{ 0 }, s_Height{ 0 };

	std::shared_ptr<Shader> shader;
private:
	GLuint fbo{ 0 };
	GLuint textureArray{ 0 };
	size_t size{ 0 };
};

class ShadowMapCubeFBO
{
public:
	ShadowMapCubeFBO() = delete;
	ShadowMapCubeFBO& operator=(const ShadowMapCubeFBO&) = delete;
	ShadowMapCubeFBO(const unsigned int size);
	~ShadowMapCubeFBO();

	void resizeWindow(const unsigned int WIDTH, const unsigned int HEIGHT);
	void setupUniformShader(const PointLight* light);
	void Init(size_t MAX_LIGHTS, std::shared_ptr<Shader> inShader);
	void Init(size_t MAX_LIGHTS); // to be use in combo with SetupShader to have a fully working object 
	void SetupShader(std::shared_ptr<Shader> inShader);
	void BindForWriting(int lightIndex);
	void BindForReading(GLint TextureUnit);
	void clean();

	unsigned int s_SIZE{ 0 };
	unsigned int maxLights{ 0 };

	std::shared_ptr<Shader> shader;

private:
	GLuint fbo{ 0 };
	GLuint depthCubemap{ 0 };
	int currentLightIndex = 0;
};

class ShadowMapPointDirFBO
{
public:
	ShadowMapPointDirFBO() = default;
	ShadowMapPointDirFBO& operator=(const ShadowMapPointDirFBO&) = delete;
	ShadowMapPointDirFBO(const unsigned int SIZE, const unsigned int WITH, const unsigned int HEIGHT);
	~ShadowMapPointDirFBO();

	void resizeWindow(const unsigned int WIDTH, const unsigned int HEIGHT);
	void setupUniformShader(Shader* shader, PointLight* light);
	void Init(std::shared_ptr<Shader> inShader);
	void BindForWriting();
	void BindForReading(GLint TextureUnit);
	void clean();

	unsigned int P_SIZE{ 0 };
	unsigned int D_WIDTH{ 0 };
	unsigned int D_HEIGHT{ 0 };

	std::shared_ptr<Shader> shader;

private:
	GLuint fbo{ 0 };
	GLuint depthPointMap{ 0 };
	GLuint depthDirMap{ 0 };
};

class MultisampleFramebuffer {
public:
	MultisampleFramebuffer(int s_Width, int s_Height, int sampleCount);
	~MultisampleFramebuffer();

	void init();
	void blit();
	void resize(int s_Width, int s_Height);
	void bind();

private:
	GLuint framebufferMSSA{ 0 };
	GLuint textureColorBufferMultiSampled{ 0 };
	GLuint rbo{ 0 };
	int samples;
	int m_Width, m_Height;
};

class FXAA {
public:
	FXAA() = default;
	~FXAA();

	void init(int s_Width,int s_Height);
	void resize(int s_Width, int s_Height);
	void bind();
	void unbind();
	void render();
	void clean();

	// Getters for the framebuffer texture
	GLuint getColorTexture() const { return colorTexture; }

	// FXAA quality settings
	void setQualitySubpix(float value) { qualitySubpix = value; }
	void setQualityEdgeThreshold(float value) { qualityEdgeThreshold = value; }
	void setQualityEdgeThresholdMin(float value) { qualityEdgeThresholdMin = value; }


	GLuint framebuffer{ 0 };

private:

	GLuint colorTexture{ 0 };
	GLuint depthTexture{ 0 };
	int m_Width{ 0 }, m_Height{ 0 };

	// Screen quad for post-processing
	GLuint VAO{ 0 };
	GLuint VBO{ 0 };

	// FXAA shader
	Shader fxaaShader;


	// FXAA quality parameters
	float qualitySubpix{ 0.75f };
	float qualityEdgeThreshold{ 0.166f };
	float qualityEdgeThresholdMin{ 0.0833f };

	void createFramebuffer();
	void deleteFramebuffer();
	void createScreenQuad();
	void deleteScreenQuad();
	void loadShaders();
};

class GBufferFBO
{
public:
	GBufferFBO() = default;
	GBufferFBO& operator=(const GBufferFBO&) = delete;
	~GBufferFBO();

	void Init(int s_Width, int s_Height);
	void BindForWriting();
	void UnBind();
	void BindForReading(GLint TextureUnit);
	void Resize(int s_Width, int s_Height);
	void Render();
	void clean();

	GLuint fbo{ 0 };
	GLuint depthBuffer{ 0 };
	std::shared_ptr<Shader> shaderGeom;
	std::shared_ptr<Shader> shaderLighting;
	std::shared_ptr<Shader> shaderInstanced;

private:
	GLuint gPosition{ 0 }, gNormalShiness{ 0 }, gColorSpec{ 0 };
	GLuint VAO{ 0 }, VBO{ 0 };
	int m_Height{ 0 }, m_Width{ 0 };
	inline const static float quadVertices[20] = {
		// Positions   // Texture coords
		-1.0f, -1.0f,  0.0f,  0.0f, 0.0f,  // Bottom-left
		 1.0f, -1.0f,  0.0f,  1.0f, 0.0f,  // Bottom-right
		-1.0f,  1.0f,  0.0f,  0.0f, 1.0f,  // Top-left
		 1.0f,  1.0f,  0.0f,  1.0f, 1.0f   // Top-right
	};

	void createTextures();
	void createDepthBuffer();
	void setupVAOVBO();

};
#endif // !FRAME_BUFFER_OBJECT_H