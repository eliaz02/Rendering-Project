#pragma once
#ifndef TEXTURE_H
#define TEXTURE_H

#include <glad/gl.h>
#include <glm/glm.hpp>
#include "stb_image.h"
//#include "utilities.h"
#include "Debugging.h"
#include <string>
#include "Shader.h"



#define COLOR_TEXTURE_UNIT 0
#define SPECULAR_EXPONENT_UNIT 1
#define NORMAL_TEXTURE_UNIT 2
#define ALPHA_TEXTURE_UNIT 3
#define SHADOW_MAP_DIR_UNIT 4
#define SHADOW_MAP_CUBE_UNIT 5
#define SHADOW_MAP_SPOT_UNIT 6

class Texture {
private:
    GLuint ID{ 0 };
    GLenum type;
    GLint slot;
    int width, height, nrComponents;


public:
    Texture() :
        type{ GL_TEXTURE_2D },
        slot{ 0 },
        width{ 0 },
        height{ 0 },
        nrComponents{ 0 }
    {
        ;
    }


    ~Texture()
    {
        Texture::Delete();
    }

    // Disable copying to prevent double-deletion
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    Texture(const char* image, GLint texSlot = 0, GLenum texType = GL_TEXTURE_2D, GLenum pixelType = GL_UNSIGNED_BYTE) :
        type{ texType },
        slot{ texSlot },
        width{ 0 },
        height{ 0 },
        nrComponents{ 0 }
    {
        glGenTextures(1, &ID);
        GL_CHECK();
        // STB loading code
        int w, h, c;
        if (!stbi_info(image, &w, &h, &c))
        {
            std::cout << "Invalid image file: " << image << std::endl;
            return;
        }

        unsigned char* data = stbi_load(image, &width, &height, &nrComponents, 0);

        if (data)
        {
            if (width <= 0 || height <= 0)
            {
                std::cout << "Invalid texture dimensions in " << image << std::endl;
                stbi_image_free(data);
                return;
            }
            GLenum format = GL_RED;
            if (nrComponents == 1)
                format = GL_RED;
            else if (nrComponents == 3)
                format = GL_RGB;
            else if (nrComponents == 4)
                format = GL_RGBA;
            this->Bind();

            glTexImage2D(type, 0, format, width, height, 0, format, pixelType, data);
            glGenerateMipmap(type);

            glTexParameteri(type, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(type, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(type, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(type, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            stbi_image_free(data);
            this->Unbind();
        }
        else {
            std::cout << "Texture failed to load at path: " << image << std::endl;
            stbi_image_free(data);
        }

        GL_CHECK();
    }

    // Assigns a texture unit to a texture
    void texUnit(Shader* shader, const char* uniformName, GLuint unit)
    {
        // Shader needs to be activated before changing the value of a uniform
        shader->use();
        // Sets the value of the uniform
        shader->setInt(uniformName, unit);
        GL_CHECK();
    }
    // Binds a texture
    void Bind()
    {
        glActiveTexture(GL_TEXTURE0 + slot);
        glBindTexture(type, ID);
        GL_CHECK();
    }
    // Unbinds a texture
    void Unbind()
    {
        glActiveTexture(GL_TEXTURE0 + slot);
        glBindTexture(type, 0);
        GL_CHECK();
    }
    // Deletes a texture
    void Delete()
    {
        glDeleteTextures(1, &ID);
    }
};


// use for skybox
class Texture_cube {
public:
    GLuint ID;
    GLuint slot;
    const static GLenum type{ GL_TEXTURE_CUBE_MAP }; // Fixed: should be cube map type
    std::string typeStr;

    // Empty constructor - just initializes default values
    Texture_cube()
        : ID(0), slot(0), vbo(0), vao(0)
    {
    }

    // Original constructor with full initialization
    Texture_cube(char const* path, std::vector<std::string> faces, GLint texSlot = 0)
        : ID(0), slot(0), vbo(0), vao(0)
    {
        load(path, faces, texSlot);
    }

    // Disable copy constructor and assignment to prevent double deletion
    Texture_cube(const Texture_cube&) = delete;
    Texture_cube& operator=(const Texture_cube&) = delete;

    // Load function to initialize everything
    void load(char const* path, std::vector<std::string> faces, GLint texSlot = 0)
    {
        // Validate input
        if (faces.size() != 6) {
            std::cout << "Error: Cubemap requires exactly 6 faces, got " << faces.size() << std::endl;
            return;
        }

        // Define skybox vertices (cube coordinates)
        static const std::vector<glm::vec3> skyboxVertices = {
            // positions          
            {-1.0f,  1.0f, -1.0f},
            {-1.0f, -1.0f, -1.0f},
            { 1.0f, -1.0f, -1.0f},
            { 1.0f, -1.0f, -1.0f},
            { 1.0f,  1.0f, -1.0f},
            {-1.0f,  1.0f, -1.0f},

            {-1.0f, -1.0f,  1.0f},
            {-1.0f, -1.0f, -1.0f},
            {-1.0f,  1.0f, -1.0f},
            {-1.0f,  1.0f, -1.0f},
            {-1.0f,  1.0f,  1.0f},
            {-1.0f, -1.0f,  1.0f},

            { 1.0f, -1.0f, -1.0f},
            { 1.0f, -1.0f,  1.0f},
            { 1.0f,  1.0f,  1.0f},
            { 1.0f,  1.0f,  1.0f},
            { 1.0f,  1.0f, -1.0f},
            { 1.0f, -1.0f, -1.0f},

            {-1.0f, -1.0f,  1.0f},
            {-1.0f,  1.0f,  1.0f},
            { 1.0f,  1.0f,  1.0f},
            { 1.0f,  1.0f,  1.0f},
            { 1.0f, -1.0f,  1.0f},
            {-1.0f, -1.0f,  1.0f},

            {-1.0f,  1.0f, -1.0f},
            { 1.0f,  1.0f, -1.0f},
            { 1.0f,  1.0f,  1.0f},
            { 1.0f,  1.0f,  1.0f},
            {-1.0f,  1.0f,  1.0f},
            {-1.0f,  1.0f, -1.0f},

            {-1.0f, -1.0f, -1.0f},
            {-1.0f, -1.0f,  1.0f},
            { 1.0f, -1.0f, -1.0f},
            { 1.0f, -1.0f, -1.0f},
            {-1.0f, -1.0f,  1.0f},
            { 1.0f, -1.0f,  1.0f}
        };

        // VBO for the cube 
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, skyboxVertices.size() * sizeof(glm::vec3), skyboxVertices.data(), GL_STATIC_DRAW);

        // VAO for the cube 
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

        // Fixed: removed unnecessary scope resolution
        slot = texSlot;
        glGenTextures(1, &ID);
        glBindTexture(GL_TEXTURE_CUBE_MAP, ID);

        int width, height, nrChannels;
        std::string strpath = std::string(path);

        for (unsigned int i = 0; i < faces.size(); i++)
        {
            unsigned char* data = stbi_load((strpath + std::string(faces[i].c_str())).c_str(), &width, &height, &nrChannels, 0);
            if (!data)
            {
                std::cerr << "Error loading image: " << (strpath + std::string(faces[i].c_str())).c_str() << std::endl;
                std::cerr << "Reason: " << stbi_failure_reason() << std::endl;
                continue; // in this way i can see if all the file are missing (or something else) or just someone 
            }
            GLenum format;
            switch (nrChannels) {
            case 1: format = GL_RED; break;
            case 3: format = GL_RGB; break;
            case 4: format = GL_RGBA; break;
            default:
                std::cout << "Unsupported number of channels: " << nrChannels << " for face: " << faces[i] << std::endl;
                stbi_image_free(data);
                continue;
            }
            if (data)
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            else
                std::cout << "Cubemap tex failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        // Unbind
        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    }

    ~Texture_cube()
    {
        Delete(); // Fixed: removed unnecessary scope resolution
    }

    void Draw(Shader* shader) {
        // set control of the z-buffer for skybox
        glDepthFunc(GL_LEQUAL);
        // bind shaders 
        shader->use();
        // bind VAO
        glBindVertexArray(vao);
        // bind texture
        Bind();
        // draw the skybox
        glDrawArrays(GL_TRIANGLES, 0, 36);

        //unbind everything 
        glBindVertexArray(0);
        glDepthFunc(GL_LESS);
        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    }

    // Assigns a texture unit to a texture
    void texUnit(Shader* shader, const char* uniformName, GLuint unit)
    {
        // Shader needs to be activated before changing the value of a uniform
        shader->use();
        // Sets the value of the uniform
        shader->setInt(uniformName, unit);
        GL_CHECK();
    }

    // Binds a texture
    void Bind()
    {
        glActiveTexture(GL_TEXTURE0 + slot);
        glBindTexture(GL_TEXTURE_CUBE_MAP, ID); // Fixed: use correct type
        GL_CHECK();
    }

    // Unbinds a texture
    void Unbind()
    {
        glActiveTexture(GL_TEXTURE0 + slot);
        glBindTexture(GL_TEXTURE_CUBE_MAP, 0); // Fixed: use correct type
        GL_CHECK();
    }

    // Deletes a texture
    void Delete()
    {
        if (ID != 0) {
            glDeleteTextures(1, &ID);
            ID = 0;
        }
        if (vbo != 0) {
            glDeleteBuffers(1, &vbo);
            vbo = 0;
        }
        if (vao != 0) {
            glDeleteVertexArrays(1, &vao);
            vao = 0;
        }
    }

private:
    GLuint vbo;
    GLuint vao;
};
/// <summary>
/// PBRMaterial is the strusct for physical base rendering but i don't think i'll implement this in my project 
/// </summary>
struct PBRMaterial
{
    float Roughness = 0.0f;
    bool IsMetal = false;
    glm::vec3 Color = glm::vec3(0.0f, 0.0f, 0.0f);
    Texture* pAlbedo = NULL;
    Texture* pRoughness = NULL;
    Texture* pMetallic = NULL;
    Texture* pNormalMap = NULL;
};



/// <summary>
/// material for phong model rendering used in Mesh class
/// </summary>
class Material {

public:

    std::string m_name;

    glm::vec4 AmbientColor = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
    glm::vec4 DiffuseColor = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
    glm::vec4 SpecularColor = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
    float Shininess = 32.0f;

    Texture* pDiffuse = NULL; // base color of the material
    Texture* pSpecularExponent = NULL;
    Texture* pNormal = NULL;
    Texture* pAlpha = NULL;


    ~Material()
    {
        if (pDiffuse) {
            delete pDiffuse;
        }

        if (pSpecularExponent) {
            delete pSpecularExponent;
        }

        if (pNormal) {
            delete pNormal;
        }
        if (pAlpha) {
            delete pAlpha;
        }
    }
};

#endif 