// Utilities.h
#pragma once

#ifndef UTILITIES_H
#define UTILITIES_H

// 1. Keep includes needed for the declarations
#include <glad/gl.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <string>
#include "stb_image.h"
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/io.hpp>  // pretty print
#include <assimp/matrix4x4.h>
#include <assimp/Importer.hpp>  // C++ importer interface
#include <assimp/scene.h>       // Output data structure
#include <assimp/postprocess.h> // Post processing flags


#include <map>
#include "Shader.h"
#include "Skybox.h"
#include "LightStruct.h"
#include "frameBufferObject.h"
#include "Camera.h"


#include "Debugging.h"
#include <iostream>

#define ARRAY_SIZE_IN_ELEMENTS(a) (sizeof(a)/sizeof(a[0]))



GLuint loadTexture(const char* path);
GLuint loadCubemap(const char* path, const std::vector<std::string> faces);
std::string GetFullPath(const std::string& Dir, const struct aiString& Path); // Forward declare aiString
// ... and so on for all your other utility functions
void inline printMat4(const glm::mat4& mat) {
    for (int row = 0; row < 4; ++row) {
        std::cout << "[ ";
        for (int col = 0; col < 4; ++col) {
            std::cout << mat[col][row] << " "; // glm is column-major
        }
        std::cout << "]\n";
    }
}
void printMat4(const glm::mat4& matrix, const std::string& name = "Matrix");

struct Vertex {
    // position
    glm::vec3 Position;
    // normal
    glm::vec3 Normal;
    // texCoords
    glm::vec2 TexCoords;
};

struct Verticies {
    std::vector<glm::vec3> Position;
    std::vector<glm::vec2> TexCoords;
    std::vector<glm::vec3> Normal;

};
struct Vertex0 {
    // position
    glm::vec3 Position;

};

extern std::vector<std::string> faces;
extern GLuint indicesCubeNDC[];
extern float cubenDC[] ;



#endif