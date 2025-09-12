#include "frameBufferObject.h"


// hello the code should have been made as following Strategy of Decoretor but
// it would have been too long and complex to write and and i will obtein a flexiblity 
// that would never be used.

ShadowMapFBO::ShadowMapFBO(const unsigned int s_Width, const unsigned int s_Height) :
	m_Width(s_Width),
	m_Height(s_Height)
{

}

ShadowMapFBO::~ShadowMapFBO()
{
	clean();
}
void ShadowMapFBO::clean()
{
	if (fbo != 0)
	{
		glDeleteFramebuffers(1, &fbo);
		fbo = 0;
	}
	if (depthbufferTexture != 0)
	{
		glDeleteTextures(1, &depthbufferTexture);
		depthbufferTexture = 0;
	}
}

void ShadowMapFBO::Init(std::shared_ptr<Shader> inShader)
{
	shader = std::move(inShader);
	// create FBO
	glGenFramebuffers(1, &fbo);

	// create  Depth texture
	glGenTextures(1, &depthbufferTexture);
	glBindTexture(GL_TEXTURE_2D, depthbufferTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, m_Width, m_Height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

	// Set texture parameters for shadow mapping
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

	// attach to FBO
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthbufferTexture, 0);

	// Disable read/writes to the color buffer
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);

	GLenum Status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

	if (Status != GL_FRAMEBUFFER_COMPLETE) {
		printf("FB error, status: 0x%x\n", Status);
		throw 1;
	}
	glBindTexture(GL_TEXTURE_2D, 0);
	GL_CHECK();
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	GL_CHECK();
}

void ShadowMapFBO::resizeWindow(const unsigned int s_Width, const unsigned int s_Height)
{
	m_Width = s_Width;
	m_Height = s_Height;

	// Update the depth texture
	glBindTexture(GL_TEXTURE_2D, depthbufferTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, s_Width, s_Height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

	// Verify framebuffer is still complete
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	GLenum Status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (Status != GL_FRAMEBUFFER_COMPLETE) {
		printf("FB error after resize, status: 0x%x\n", Status);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ShadowMapFBO::BindForWriting()
{
	// Additional validation checks
	if (fbo == 0) {
		std::cerr << "Error: Invalid framebuffer object" << std::endl;
		return;
	}

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);

	glViewport(0, 0, m_Width, m_Height);
	GL_CHECK();
}

void ShadowMapFBO::BindForReading(GLint TextureUnit)
{
	// Validate texture unit to prevent invalid enum
	GLint maxUnits;
	glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maxUnits);
	if (TextureUnit < 0 || TextureUnit >= maxUnits) {
		std::cerr << "Invalid texture unit: " << TextureUnit << std::endl;
		return;
	}

	glActiveTexture(GL_TEXTURE0 + TextureUnit);

	glBindTexture(GL_TEXTURE_2D, depthbufferTexture);
	GL_CHECK(); // Check after bind
}


ShadowMapArrayFBO::ShadowMapArrayFBO(const unsigned int WIDTH, const unsigned int HEIGHT) :
	s_Width(WIDTH),
	s_Height(HEIGHT)
{

}

ShadowMapArrayFBO::~ShadowMapArrayFBO()
{
	clean();
}
void ShadowMapArrayFBO::clean()
{
	if (fbo != 0)
	{
		glDeleteFramebuffers(1, &fbo);
		fbo = 0;
	}
	if (textureArray != 0)
	{
		glDeleteTextures(1, &textureArray);
		textureArray = 0;
	}

}

void ShadowMapArrayFBO::Init(size_t Size, std::shared_ptr<Shader> inShader)
{
	//clean();
	shader = std::move(inShader);
	size = Size;
	// Validate size
	if (size == 0) {
		printf("Error: Array size cannot be zero\n");
		return;
	}

	// create FBO
	glGenFramebuffers(1, &fbo);

	// create  Depth texture
	glGenTextures(1, &textureArray);
	glBindTexture(GL_TEXTURE_2D_ARRAY, textureArray);

	glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_DEPTH_COMPONENT24, s_Width, s_Height, (GLsizei)size);


	// Set texture parameters for shadow mapping
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

	// attach to FBO
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, textureArray, 0, 0);

	// Disable read/writes to the color buffer
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);

	GLenum Status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

	if (Status != GL_FRAMEBUFFER_COMPLETE) {
		printf("FB error, status: 0x%x\n", Status);
		throw 1;
	}
	glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
	GL_CHECK();
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	GL_CHECK();
}

void ShadowMapArrayFBO::Init(size_t Size)
{
	size = Size;
	// Validate size
	if (size == 0) {
		printf("Error: Array size cannot be zero\n");
		return;
	}

	// create FBO
	glGenFramebuffers(1, &fbo);

	// create  Depth texture
	glGenTextures(1, &textureArray);
	glBindTexture(GL_TEXTURE_2D_ARRAY, textureArray);

	glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_DEPTH_COMPONENT24, s_Width, s_Height, (GLsizei)size);


	// Set texture parameters for shadow mapping
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

	// attach to FBO
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, textureArray, 0, 0);

	// Disable read/writes to the color buffer
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);

	GLenum Status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

	if (Status != GL_FRAMEBUFFER_COMPLETE) {
		printf("FB error, status: 0x%x\n", Status);
		throw 1;
	}
	glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
	GL_CHECK();
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	GL_CHECK();
}

void ShadowMapArrayFBO::SetupShader(std::shared_ptr<Shader> inShader)
{
	shader = std::move(inShader);
}

void ShadowMapArrayFBO::resizeWindow(const unsigned int WIDTH, const unsigned int HEIGHT)
{
	s_Width = WIDTH;
	s_Height = HEIGHT;

	// Update the depth texture
	glBindTexture(GL_TEXTURE_2D_ARRAY, textureArray);
	glTexImage2D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT, s_Width, s_Height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

	// Verify framebuffer is still complete
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	GLenum Status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (Status != GL_FRAMEBUFFER_COMPLETE) {
		printf("FB error after resize, status: 0x%x\n", Status);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ShadowMapArrayFBO::BindLayerForWriting(int layerIndex)
{
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
	glFramebufferTextureLayer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, textureArray, 0, layerIndex);
	glViewport(0, 0, s_Width, s_Height);
	GL_CHECK();
}

void ShadowMapArrayFBO::BindForReading(GLint TextureUnit)
{
	// Validate texture unit to prevent invalid enum
	GLint maxUnits;
	glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maxUnits);
	if (TextureUnit < 0 || TextureUnit >= maxUnits) {
		std::cerr << "Invalid texture unit: " << TextureUnit << std::endl;
		return;
	}

	glActiveTexture(GL_TEXTURE0 + TextureUnit);

	glBindTexture(GL_TEXTURE_2D_ARRAY, textureArray);
	GL_CHECK(); // Check after bind
}

// ShadowMAP FBO for point light that means to use a cube texture instead a normal texture 
// in addiction use the gemoetry shader for loat every thing in a single step

ShadowMapCubeFBO::ShadowMapCubeFBO(const unsigned int SIZE) :
	s_SIZE{ SIZE }
{
	;
}
ShadowMapCubeFBO::~ShadowMapCubeFBO()
{
	clean();
}
void ShadowMapCubeFBO::clean()
{
	if (fbo != 0)
	{
		glDeleteFramebuffers(1, &fbo);
		fbo = 0;
	}
	if (depthCubemap != 0)
	{
		glDeleteTextures(1, &depthCubemap);
		depthCubemap = 0;
	}
}

void ShadowMapCubeFBO::resizeWindow(const unsigned int WIDTH, const unsigned int HEIGHT) {}

void ShadowMapCubeFBO::Init(size_t MAX_LIGHTS, std::shared_ptr<Shader> inShader)
{
	//clean();
	shader = std::move(inShader);

	// Validate maxLights
	if (maxLights == 0) {
		printf("Error: Array size cannot be zero\n");
		throw 1;
	}
	maxLights = MAX_LIGHTS;
	// Generate FBO
	glGenFramebuffers(1, &fbo);

	// Generate the cubemap
	glGenTextures(1, &depthCubemap);
	glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, depthCubemap);

	// Allocate storage for all cube maps
	glTexImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 0, GL_DEPTH_COMPONENT,
		s_SIZE, s_SIZE, maxLights * 6, 0,
		GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

	// Set texture parameters
	// Set texture parameters for shadow mapping 
	glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	// Enable shadow comparison
	glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

	// Attach cubemap to FBO
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthCubemap, 0);

	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);


	// Check completeness
	GLenum Status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (Status != GL_FRAMEBUFFER_COMPLETE) {
		printf("FB error, status: 0x%x\n", Status);
		throw 1;
	}

	// unbind buffer for avoid malicious use 
	glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, 0);
	GL_CHECK();
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	GL_CHECK();

}

void ShadowMapCubeFBO::SetupShader(std::shared_ptr<Shader> inShader)
{
	shader = std::move(inShader);
}

void ShadowMapCubeFBO::Init( size_t MAX_LIGHTS)
{
	//clean();
	// Validate maxLights
	if (MAX_LIGHTS == 0) {
		printf("Error: maxLights size cannot be zero\n");
		throw 1;
	}
	maxLights = MAX_LIGHTS;
	// Generate FBO
	glGenFramebuffers(1, &fbo);

	// Generate the cubemap
	glGenTextures(1, &depthCubemap);
	glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, depthCubemap);

	// Allocate storage for all cube maps
	glTexImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 0, GL_DEPTH_COMPONENT,
		s_SIZE, s_SIZE, maxLights * 6, 0,
		GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

	// Set texture parameters
	// Set texture parameters for shadow mapping 
	glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	// Enable shadow comparison
	glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

	// Attach cubemap to FBO
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthCubemap, 0);

	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);


	// Check completeness
	GLenum Status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (Status != GL_FRAMEBUFFER_COMPLETE) {
		printf("FB error, status: 0x%x\n", Status);
		throw 1;
	}

	// unbind buffer for avoid malicious use 
	glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, 0);
	GL_CHECK();
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	GL_CHECK();
}
void ShadowMapCubeFBO::BindForWriting(int lightIndex)
{
	// Additional validation checks
	if (fbo == 0) {
		std::cerr << "Error: Invalid framebuffer object in Writing" << std::endl;
		return;
	}

	// This is the key change - we'll handle array indexing in geometry shader
	currentLightIndex = lightIndex;

	// bind framebuffer for drawing 
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
	GL_CHECK();

	// setup the size of the window
	glViewport(0, 0, s_SIZE, s_SIZE);
	GL_CHECK();
}
void ShadowMapCubeFBO::BindForReading(GLint TextureUnit)
{
	// Check if texture is valid
	if (depthCubemap == 0) {
		std::cerr << "depthCubemap is 0 (uninitialized)" << std::endl;
		return;
	}

	if (!glIsTexture(depthCubemap)) {
		std::cerr << "depthCubemap is not a valid texture object" << std::endl;
		return;
	}

	// Your existing validation code...
	GLint maxUnits;
	glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maxUnits);
	if (TextureUnit < 0 || TextureUnit >= maxUnits) {
		std::cerr << "Invalid texture unit: " << TextureUnit << std::endl;
		return;
	}

	glActiveTexture(GL_TEXTURE0 + TextureUnit);
	glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, depthCubemap);

	// Check for errors after binding
	GLenum error = glGetError();
	if (error != GL_NO_ERROR) {
		std::cerr << "glBindTexture error: " << error << std::endl;
	}
}

void ShadowMapCubeFBO::setupUniformShader(const PointLight* light)
{
	glm::vec3 lightPos = light->Pos;
	glm::mat4 shadowProj = light->Projection;
	shader->setVec3("lightPos", lightPos);
	shader->setFloat("far_plane", light->far_plane);


	std::vector<glm::mat4> shadowTransforms;
	shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, (lightPos + glm::vec3(1.0f, 0.0f, 0.0f)), glm::vec3(0.0f, -1.0f, 0.0f)));
	shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, (lightPos + glm::vec3(-1.0f, 0.0f, 0.0f)), glm::vec3(0.0f, -1.0f, 0.0f)));
	shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, (lightPos + glm::vec3(0.0f, 1.0f, 0.0f)), glm::vec3(0.0f, 0.0f, 1.0f)));
	shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, (lightPos + glm::vec3(0.0f, -1.0f, 0.0f)), glm::vec3(0.0f, 0.0f, -1.0f)));
	shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, (lightPos + glm::vec3(0.0f, 0.0f, 1.0f)), glm::vec3(0.0f, -1.0f, 0.0f)));
	shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, (lightPos + glm::vec3(0.0f, 0.0f, -1.0f)), glm::vec3(0.0f, -1.0f, 0.0f)));

	for (unsigned int i = 0; i < 6; ++i)
		shader->setMat4("shadowMatrices[" + std::to_string(i) + "]", shadowTransforms[i]);
}


ShadowMapPointDirFBO::ShadowMapPointDirFBO(const unsigned int SIZE, const unsigned int WIDTH, const unsigned int HEIGHT) :
	P_SIZE{ SIZE },
	D_WIDTH{ WIDTH },
	D_HEIGHT{ HEIGHT }
{
	;
}
ShadowMapPointDirFBO::~ShadowMapPointDirFBO()
{
	clean();
}
void ShadowMapPointDirFBO::clean()
{
	if (fbo != 0)
	{
		glDeleteFramebuffers(1, &fbo);
		fbo = 0;
	}
	if (depthPointMap != 0)
	{
		glDeleteTextures(1, &depthPointMap);
		depthPointMap = 0;
	}
	if (depthDirMap != 0)
	{
		glDeleteTextures(1, &depthDirMap);
		depthDirMap = 0;
	}
}
void ShadowMapPointDirFBO::resizeWindow(const unsigned int WIDTH, const unsigned int HEIGHT) {}

void ShadowMapPointDirFBO::Init(std::shared_ptr<Shader> inShader)
{
	//clean();
	shader = std::move(inShader);
	// Generate FBO
	glGenFramebuffers(1, &fbo);

	// Generate the cubemap
	glGenTextures(1, &depthPointMap);
	glBindTexture(GL_TEXTURE_CUBE_MAP, depthPointMap);

	for (unsigned int i = 0; i < 6; ++i)
	{
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
			0,
			GL_DEPTH_COMPONENT,
			P_SIZE, P_SIZE,
			0,
			GL_DEPTH_COMPONENT,
			GL_FLOAT,
			NULL);
	}

	// Set texture parameters
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	// Attach cubemap to FBO
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthPointMap, 0);


	// create  Depth texture
	glGenTextures(1, &depthDirMap);
	glBindTexture(GL_TEXTURE_2D, depthDirMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, D_WIDTH, D_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

	// Set texture parameters for shadow mapping
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

	// attach to FBO
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthDirMap, 0);

	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);


	// Check completeness
	GLenum Status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (Status != GL_FRAMEBUFFER_COMPLETE) {
		printf("FB error, status: 0x%x\n", Status);
		throw 1;
	}

	// unbind buffer for avoid malicious use 
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
	GL_CHECK();


	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	GL_CHECK();

}
void ShadowMapPointDirFBO::BindForWriting()
{
	// Additional validation checks
	if (fbo == 0) {
		std::cerr << "Error: Invalid framebuffer object" << std::endl;
		return;
	}
	// bind framebuffer for drawing 
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
	GL_CHECK();

	// setup the size of the window
	glViewport(0, 0, P_SIZE, P_SIZE);
	GL_CHECK();
}
void ShadowMapPointDirFBO::BindForReading(GLint TextureUnit)
{
	// Validate texture unit to prevent invalid enum
	GLint maxUnits;
	glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maxUnits);
	if (TextureUnit < 0 || TextureUnit >= maxUnits) {
		std::cerr << "Invalid texture unit: " << TextureUnit << std::endl;
		return;
	}

	glActiveTexture(GL_TEXTURE0 + TextureUnit);

	glBindTexture(GL_TEXTURE_CUBE_MAP, depthPointMap);
	GL_CHECK(); // Check after bind
}
void ShadowMapPointDirFBO::setupUniformShader(Shader* shader, PointLight* light)
{
	glm::vec3 lightPos = light->Pos;
	glm::mat4 shadowProj = light->Projection;
	shader->setVec3("lightPos", lightPos);
	shader->setFloat("far_plane", light->far_plane);

	std::vector<glm::mat4> shadowTransforms;
	shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, (lightPos + glm::vec3(1.0f, 0.0f, 0.0f)), glm::vec3(0.0f, -1.0f, 0.0f)));
	shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, (lightPos + glm::vec3(-1.0f, 0.0f, 0.0f)), glm::vec3(0.0f, -1.0f, 0.0f)));
	shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, (lightPos + glm::vec3(0.0f, 1.0f, 0.0f)), glm::vec3(0.0f, 0.0f, 1.0f)));
	shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, (lightPos + glm::vec3(0.0f, -1.0f, 0.0f)), glm::vec3(0.0f, 0.0f, -1.0f)));
	shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, (lightPos + glm::vec3(0.0f, 0.0f, 1.0f)), glm::vec3(0.0f, -1.0f, 0.0f)));
	shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, (lightPos + glm::vec3(0.0f, 0.0f, -1.0f)), glm::vec3(0.0f, -1.0f, 0.0f)));

	for (unsigned int i = 0; i < 6; ++i)
		shader->setMat4("shadowMatrices[" + std::to_string(i) + "]", shadowTransforms[i]);
}

MultisampleFramebuffer::MultisampleFramebuffer(int s_Width, int s_Height, int sampleCount = 4) :
	samples{ sampleCount },
	m_Width { s_Width },
	m_Height { s_Height }
{
}

MultisampleFramebuffer::~MultisampleFramebuffer()
{
	glDeleteTextures(1, &textureColorBufferMultiSampled);
	glDeleteRenderbuffers(1, &rbo);
}
void MultisampleFramebuffer::init()
{
	
	// Generate framebuffer
	glGenFramebuffers(1, &framebufferMSSA);
	glBindFramebuffer(GL_FRAMEBUFFER, framebufferMSSA);

	// Create multisampled color texture
	glGenTextures(1, &textureColorBufferMultiSampled);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, textureColorBufferMultiSampled);
	glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, GL_RGB, m_Width, m_Height, GL_TRUE);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, textureColorBufferMultiSampled, 0);

	// Create renderbuffer
	glGenRenderbuffers(1, &rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, rbo);
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_DEPTH24_STENCIL8, m_Width, m_Height);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

	// Unbind framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


void MultisampleFramebuffer::blit()
{
	glBindFramebuffer(GL_READ_FRAMEBUFFER, framebufferMSSA);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBlitFramebuffer(0, 0, m_Width, m_Height, 0, 0, m_Width, m_Height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
}
void MultisampleFramebuffer::bind()
{
	glBindFramebuffer(GL_FRAMEBUFFER, framebufferMSSA);
	glViewport(0, 0, m_Width, m_Height);
}
void MultisampleFramebuffer::resize(int s_Width, int s_Height)
{
	m_Width = s_Width;
	m_Height = s_Height;
	// Delete old resources
	glDeleteTextures(1, &textureColorBufferMultiSampled);
	glDeleteRenderbuffers(1, &rbo);

	// Recreate with new dimensions
	glBindFramebuffer(GL_FRAMEBUFFER, framebufferMSSA);

	// Recreate multisampled color texture
	glGenTextures(1, &textureColorBufferMultiSampled);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, textureColorBufferMultiSampled);
	glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, GL_RGB, m_Width, m_Height, GL_TRUE);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, textureColorBufferMultiSampled, 0);

	// Recreate renderbuffer
	glGenRenderbuffers(1, &rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, rbo);
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_DEPTH24_STENCIL8, m_Width, m_Height);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

	// Verify framebuffer is still complete
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		std::cout << "ERROR::FRAMEBUFFER:: MSAA Framebuffer resize failed!" << std::endl;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}




// for the Instanced and  not Instanced	use the same g-buffer 
// issue i have to think about the shader the data struct for rendering and some thing elsemabe
// radius to the light 
// understand how to handle the shadow
// screen space ambient occlusion 

void GBufferFBO::Init(int s_Width, int s_Height) 
{
//	clean();
	m_Width = s_Width;
	m_Height = s_Height;
	// Generate FBO
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	// Create all textures
	createTextures();

	// Create depth buffer
	createDepthBuffer();

	// Create VAO and VBO of a simple square
	setupVAOVBO();

	// Tell OpenGL which color attachments we'll use for rendering 
	unsigned int attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
	glDrawBuffers(3, attachments);


	shaderGeom = std::make_shared<Shader>();
	shaderLighting = std::make_shared<Shader>();
	// create shader object 
	shaderGeom->load("shader/Geometry_pass.vert", "shader/Geometry_pass.frag");
	shaderLighting->load("shader/Lighting_pass_test.vert", "shader/Lighting_pass_test.frag");

}

void GBufferFBO::createTextures() {
	// Position + linear depth color buffer 
	glGenTextures(1, &gPosition);
	glBindTexture(GL_TEXTURE_2D, gPosition);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, m_Width, m_Height, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPosition, 0);

	// Normal + shininess color buffer
	glGenTextures(1, &gNormalShiness);
	glBindTexture(GL_TEXTURE_2D, gNormalShiness);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, m_Width, m_Height, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormalShiness, 0);

	// Color + specular color buffer
	glGenTextures(1, &gColorSpec);
	glBindTexture(GL_TEXTURE_2D, gColorSpec);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_Width, m_Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gColorSpec, 0);
}

void GBufferFBO::createDepthBuffer() {
	// Create depth renderbuffer
	glGenTextures(1, &depthBuffer);
	glBindTexture(GL_TEXTURE_2D, depthBuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, m_Width, m_Height,
		0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// Attach as depth attachment
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthBuffer, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
}


void GBufferFBO::BindForWriting()
{
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glViewport(0, 0, m_Width, m_Height);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Keep it black so it doesn't leak into g-buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void GBufferFBO::UnBind()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
void GBufferFBO::BindForReading(GLint TextureUnit) {
	glActiveTexture(GL_TEXTURE0 + TextureUnit);
	glBindTexture(GL_TEXTURE_2D, gPosition);

	glActiveTexture(GL_TEXTURE0 + TextureUnit + 1);
	glBindTexture(GL_TEXTURE_2D, gNormalShiness);

	glActiveTexture(GL_TEXTURE0 + TextureUnit + 2);
	glBindTexture(GL_TEXTURE_2D, gColorSpec);

	glActiveTexture(GL_TEXTURE0 + TextureUnit + 3);
	glBindTexture(GL_TEXTURE_2D, depthBuffer);
}

void GBufferFBO::Resize(int s_Width, int s_Height) 
{
	m_Height = s_Height;
	m_Width = m_Width;
	// Delete old textures and renderbuffer
	glDeleteTextures(1, &gPosition);
	glDeleteTextures(1, &gNormalShiness);
	glDeleteTextures(1, &gColorSpec);
	glDeleteTextures(1, &depthBuffer);

	// Bind framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	// Recreate textures with new dimensions
	createTextures();

	// Recreate depth buffer with new dimensions
	createDepthBuffer();

	// Reset draw buffers
	unsigned int attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
	glDrawBuffers(3, attachments);

	// Check framebuffer completeness
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		std::cout << "ERROR::FRAMEBUFFER:: GBuffer framebuffer resize failed!" << std::endl;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
void GBufferFBO::setupVAOVBO()
{
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);

	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

	// Position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	// Texture coordinate attribute
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	glBindVertexArray(0);
}

void GBufferFBO::Render()
{
	glDisable(GL_DEPTH_TEST);
	glBindVertexArray(VAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
	glEnable(GL_DEPTH_TEST);
}

GBufferFBO::~GBufferFBO()
{
	clean();
}


void GBufferFBO::clean()
{

	if (gPosition != 0 || gNormalShiness != 0 || gColorSpec != 0)
	{
		GLuint textures[] = { gPosition, gNormalShiness, gColorSpec };
		glDeleteTextures(3, textures);
		gPosition = 0;
		gNormalShiness = 0;
		gColorSpec = 0;
	}

	if (depthBuffer != 0)
	{
		glDeleteRenderbuffers(1, &depthBuffer);
		depthBuffer = 0;
	}
	if (fbo != 0)
	{
		glDeleteFramebuffers(1, &fbo);
		fbo = 0;
	}

	if (VBO != 0)
	{
		glDeleteBuffers(1, &VBO);
		VBO = 0;
	}
	if (VAO != 0)
	{
		glDeleteVertexArrays(1, &VAO);
		VAO = 0;
	}

	shaderGeom->clean();
	shaderLighting->clean();
}

void FXAA::clean() {
	fxaaShader.clean();
	deleteFramebuffer();
	deleteScreenQuad();
}

FXAA::~FXAA() {
	clean();
}

void FXAA::init(int s_Width, int s_Height) 
{
	//clean();
	m_Width = s_Width;
	m_Height = s_Height;
	loadShaders();
	createFramebuffer();
	createScreenQuad();
}

void FXAA::resize(int s_Width, int s_Height) {
	m_Width = s_Width;
	m_Height = s_Height;
	// Recreate framebuffer with new dimensions
	deleteFramebuffer();
	createFramebuffer();
}

void FXAA::bind() {
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
	glViewport(0, 0, m_Width, m_Height);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void FXAA::unbind() {
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void FXAA::render() {
	// Bind default framebuffer for final output
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, m_Width, m_Height);
	glClear(GL_COLOR_BUFFER_BIT);

	// Disable depth testing for post-processing
	glDisable(GL_DEPTH_TEST);

	// Use FXAA shader
	fxaaShader.use();

	// Set uniforms
	fxaaShader.setVec2("rcpFrame", 1.0f / m_Width, 1.0f / m_Height);

	// Bind the color texture from our framebuffer
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, colorTexture);
	fxaaShader.setInt("screenTexture", 0);

	// Render the screen quad
	glBindVertexArray(VAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);

	// Re-enable depth testing
	glEnable(GL_DEPTH_TEST);
}

void FXAA::createFramebuffer() {
	// Generate framebuffer
	glGenFramebuffers(1, &framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
	
	// Create color texture
	glGenTextures(1, &colorTexture);
	glBindTexture(GL_TEXTURE_2D, colorTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_Width, m_Height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture, 0);

	// Create depth texture
	glGenTextures(1, &depthTexture);
	glBindTexture(GL_TEXTURE_2D, depthTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, m_Width, m_Height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture, 0);

	// Check framebuffer completeness
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		std::cout << "ERROR::FXAA::FRAMEBUFFER_NOT_COMPLETE" << std::endl;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void FXAA::deleteFramebuffer() {
	if (colorTexture != 0) {
		glDeleteTextures(1, &colorTexture);
		colorTexture = 0;
	}
	if (depthTexture != 0) {
		glDeleteTextures(1, &depthTexture);
		depthTexture = 0;
	}
	if (framebuffer != 0) {
		glDeleteFramebuffers(1, &framebuffer);
		framebuffer = 0;
	}
}

void FXAA::createScreenQuad() {
	// Screen quad vertices (position and texture coordinates)
	float quadVertices[] = {
		// positions   // texCoords
		-1.0f,  1.0f,  0.0f, 1.0f,
		-1.0f, -1.0f,  0.0f, 0.0f,
		 1.0f,  1.0f,  1.0f, 1.0f,
		 1.0f, -1.0f,  1.0f, 0.0f,
	};

	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);

	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

	// Position attribute
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

	// Texture coordinate attribute
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

	glBindVertexArray(0);
}

void FXAA::deleteScreenQuad() {
	if (VAO != 0) {
		glDeleteVertexArrays(1, &VAO);
		VAO = 0;
	}
	if (VBO != 0) {
		glDeleteBuffers(1, &VBO);
		VBO = 0;
	}
}

void FXAA::loadShaders() {
	// You'll need to create these shader files
	fxaaShader.load("shader/fxaa.vert", "shader/fxaa.frag");
}
