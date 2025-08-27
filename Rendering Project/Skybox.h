#pragma once

#ifndef SKYBOX_H
#define SKYBOX_H

#include <glad/gl.h>       
#include "Texture.h"
#include "Shader.h"


extern glm::mat4 projection;
extern glm::mat4 view;


class Skybox
{
public:
	Skybox();
	Skybox(const char* const path, std::vector<std::string> faces, const char* const  vert = "shader/skyboxShader.vert", const char* const frag = "shader/skyboxShader.frag");
	~Skybox();

	void load(const char* const path, std::vector<std::string> faces, const char* const vert = "shader/skyboxShader.vert", const char* const frag = "shader/skyboxShader.frag");
	void clear();
	void Draw();
	void Render(glm::mat4  projection, glm::mat4 view);
public:
	Texture_cube textureCube;
	Shader shader;

};

extern Skybox skybox;

inline Skybox::Skybox(const char* const path, std::vector<std::string> faces, const char* const vert, const char* const frag) :
	textureCube(path, faces),
	shader(vert, frag)
{
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
}
inline void Skybox::load(const char* const path, std::vector<std::string> faces, const char* const vert, const char* const frag)
{
	textureCube.load(path, faces);
	shader.load(vert, frag);
}

inline void Skybox::Render(glm::mat4  projection, glm::mat4 view)
{
	glDepthFunc(GL_LEQUAL);     // Allow depth values equal to the far plane
	glDepthMask(GL_FALSE);      // Don't write to depth buffer
	skybox.shader.use();
	skybox.shader.setMat4("projection", projection);
	skybox.shader.setMat4("view", view);
	skybox.Draw();
	glDepthFunc(GL_LESS);       // Restore default
	glDepthMask(GL_TRUE);
}

inline void Skybox::Draw()
{
	textureCube.Draw(&shader);
}
#endif // !SKYBOX_H
