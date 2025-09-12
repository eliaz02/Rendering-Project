#pragma once

#ifndef SKYBOX_H
#define SKYBOX_H

#include <glad/gl.h>       
#include "Texture.h"
#include "Shader.h"


class Skybox
{
public:
	Skybox();
	Skybox(const char* const path, std::vector<std::string> faces, const char* const  vert = "shader/skyboxShader.vert", const char* const frag = "shader/skyboxShader.frag");
	~Skybox();

	void load(const char* const path, std::vector<std::string> faces, const char* const vert = "shader/skyboxShader.vert", const char* const frag = "shader/skyboxShader.frag");
	void clear();
	void Render(glm::mat4  projection, glm::mat4 view);
	void initializeCubeData();
public:
	Texture_cube textureCube;
	Shader shader;
private:
	GLuint VAO{ 0 };
	GLuint VBO{ 0 };
};


inline Skybox::Skybox(const char* const path, std::vector<std::string> faces, const char* const vert, const char* const frag) :
	textureCube(),
	shader(vert, frag)
{
	textureCube.load(path, faces);
}

inline Skybox::Skybox() : textureCube(), shader() {}
inline Skybox::~Skybox()
{
	clear();
}
inline void  Skybox::clear()
{
	textureCube.Delete();
	shader.clean();
	if (VBO != 0) glDeleteBuffers(1, &VBO);
	if (VAO != 0) glDeleteVertexArrays(1, &VAO);
}
inline void Skybox::load(const char* const path, std::vector<std::string> faces, const char* const vert, const char* const frag)
{
	textureCube.load(path, faces);
	shader.load(vert, frag);
	initializeCubeData();
}

inline void Skybox::Render(glm::mat4  projection, glm::mat4 view)
{
	// --- STATE SETUP ---
	   // 1. Change the depth function to LEQUAL so the shader's z=w trick works.
	glDepthFunc(GL_LEQUAL);

	// 2. Change face culling. We are inside the cube, so we want to render
	//    the back faces and cull the front faces.
	glCullFace(GL_BACK);

	glDepthMask(GL_FALSE);

	// --- RENDERING ---
	shader.use();
	shader.setInt("skybox", 0);
	shader.setMat4("projection", projection);
	glm::mat4 View = glm::mat4(glm::mat3(view));
	shader.setMat4("view", View); // The shader handles the translation removal

	glBindVertexArray(VAO);
	textureCube.Bind();

	glDrawArrays(GL_TRIANGLES, 0, 36);


	// --- STATE RESTORATION ---
	// IMPORTANT: Reset the state back to the defaults for the rest of your scene.
	glBindVertexArray(0);
	glDepthMask(GL_TRUE);
	glCullFace(GL_BACK); // Set culling back to the default
	glDepthFunc(GL_LESS); // Set depth function back to the default

}

inline void Skybox::initializeCubeData() 
{
	// Use the same vertex data you had in Texture_cube
	const float skyboxVertices[] = {
	-1.0f,  1.0f, -1.0f,
	-1.0f, -1.0f, -1.0f,
	 1.0f, -1.0f, -1.0f,
	 1.0f, -1.0f, -1.0f,
	 1.0f,  1.0f, -1.0f,
	-1.0f,  1.0f, -1.0f,

	-1.0f, -1.0f,  1.0f,
	-1.0f, -1.0f, -1.0f,
	-1.0f,  1.0f, -1.0f,
	-1.0f,  1.0f, -1.0f,
	-1.0f,  1.0f,  1.0f,
	-1.0f, -1.0f,  1.0f,

	 1.0f, -1.0f, -1.0f,
	 1.0f, -1.0f,  1.0f,
	 1.0f,  1.0f,  1.0f,
	 1.0f,  1.0f,  1.0f,
	 1.0f,  1.0f, -1.0f,
	 1.0f, -1.0f, -1.0f,
	 
	-1.0f, -1.0f,  1.0f,
	-1.0f,  1.0f,  1.0f,
	 1.0f,  1.0f,  1.0f,
	 1.0f,  1.0f,  1.0f,
	 1.0f, -1.0f,  1.0f,
	-1.0f, -1.0f,  1.0f,

	-1.0f,  1.0f, -1.0f,
	 1.0f,  1.0f, -1.0f,
	 1.0f,  1.0f,  1.0f,
	 1.0f,  1.0f,  1.0f,
	-1.0f,  1.0f,  1.0f,
	-1.0f,  1.0f, -1.0f,

	-1.0f, -1.0f, -1.0f,
	-1.0f, -1.0f,  1.0f,
	 1.0f, -1.0f, -1.0f,
	 1.0f, -1.0f, -1.0f,
	-1.0f, -1.0f,  1.0f,
	 1.0f, -1.0f,  1.0f 
};

	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);

	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

	glBindVertexArray(0);
}



#endif // !SKYBOX_H
