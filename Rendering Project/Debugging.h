#ifndef _OPENGL_DEBUG_
#define _OPENGL_DEBUG_
#include <glad/gl.h>       // Includi prima di qualsiasi altro file OpenGL
#define GLFW_INCLUDE_NONE    // Evita che GLFW includa automaticamente gl.h
#include <GLFW/glfw3.h>
#include <string>
#include <vector>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>  

// Debug function to check OpenGL errors
inline void CheckGLError(const char* function, const char* file, int line) {
    GLenum error;
    while ((error = glGetError()) != GL_NO_ERROR) {
        std::string errorMsg;
        switch (error) {
        case GL_INVALID_ENUM:       errorMsg = "GL_INVALID_ENUM"; break;
        case GL_INVALID_VALUE:      errorMsg = "GL_INVALID_VALUE"; break;
        case GL_INVALID_OPERATION:  errorMsg = "GL_INVALID_OPERATION"; break;
        case GL_OUT_OF_MEMORY:      errorMsg = "GL_OUT_OF_MEMORY"; break;
        default:                    errorMsg = "UNKNOWN"; break;
        }
        std::cerr << "OpenGL Error (" << errorMsg << ") in " << function
            << " at " << file << ":" << line << std::endl;
    }
}


#define GL_CHECK() CheckGLError(__FUNCTION__, __FILE__, __LINE__)


#define GLCheckError() (glGetError() == GL_NO_ERROR)

static void printout_opengl_glsl_info() {
    const GLubyte* renderer = glGetString(GL_RENDERER);
    const GLubyte* vendor = glGetString(GL_VENDOR);
    const GLubyte* version = glGetString(GL_VERSION);
    const GLubyte* glslVersion = glGetString(GL_SHADING_LANGUAGE_VERSION);

    std::cout << "GL Vendor            : " << vendor << std::endl;
    std::cout << "GL Renderer          : " << renderer << std::endl;
    std::cout << "GL Version (string)  : " << version << std::endl;
    std::cout << "GL Version           : ";

    GLint major, minor;
    glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);
    std::cout << major << "." << minor << std::endl;

    std::cout << "GLSL Version         : " << glslVersion << std::endl;
}

static bool check_gl_errors(int line, const char* file, bool exit_on_error = true) {
    GLenum err;
    bool has_error = false;

    while ((err = glGetError()) != GL_NO_ERROR) {
        has_error = true;
        std::string error_msg;

        switch (err) {
        case GL_INVALID_ENUM:
            error_msg = "GL_INVALID_ENUM: An unacceptable value is specified for an enumerated argument.";
            break;
        case GL_INVALID_VALUE:
            error_msg = "GL_INVALID_VALUE: A numeric argument is out of range.";
            break;
        case GL_INVALID_OPERATION:
            error_msg = "GL_INVALID_OPERATION: The specified operation is not allowed in the current state.";
            break;
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            error_msg = "GL_INVALID_FRAMEBUFFER_OPERATION: The framebuffer object is not complete.";
            break;
        case GL_OUT_OF_MEMORY:
            error_msg = "GL_OUT_OF_MEMORY: There is not enough memory left to execute the command.";
            break;
        case GL_STACK_UNDERFLOW:
            error_msg = "GL_STACK_UNDERFLOW: An attempt has been made to perform an operation that would cause an internal stack to underflow.";
            break;
        case GL_STACK_OVERFLOW:
            error_msg = "GL_STACK_OVERFLOW: An attempt has been made to perform an operation that would cause an internal stack to overflow.";
            break;
        default:
            error_msg = "Unknown OpenGL error.";
        }

        std::cout << error_msg << "\nLocation: " << file << ":" << line << std::endl;
    }

    if (has_error && exit_on_error) {
        exit(1);  // Changed from exit(0) to indicate error
    }

    return !has_error;
}

static bool check_gl_errors(bool exit_on_error = true) {
    return check_gl_errors(-1, "unknown", exit_on_error);
}


static bool check_shader(GLuint shader, bool exit_on_error = true) {
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if (success == GL_FALSE) {
        GLint max_length = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &max_length);

        std::vector<GLchar> error_log(max_length);
        glGetShaderInfoLog(shader, max_length, &max_length, error_log.data());

        std::cout << "Shader compilation failed:\n" << error_log.data() << std::endl;

        if (exit_on_error) {
            exit(1);  // Changed from exit(0) to indicate error
        }
        return false;
    }
    return true;
}

static bool validate_shader_program(GLuint program) {
    GLint success;

    // First check if program linked successfully
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (success == GL_FALSE) {
        GLint max_length = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &max_length);

        std::vector<GLchar> error_log(max_length);
        glGetProgramInfoLog(program, max_length, &max_length, error_log.data());

        std::cout << "Program linking failed:\n" << error_log.data() << std::endl;
        return false;
    }

    // Then validate the program
    glValidateProgram(program);
    glGetProgramiv(program, GL_VALIDATE_STATUS, &success);
    if (success == GL_FALSE) {
        GLint max_length = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &max_length);

        std::vector<GLchar> error_log(max_length);
        glGetProgramInfoLog(program, max_length, &max_length, error_log.data());

        std::cout << "Program validation failed:\n" << error_log.data() << std::endl;
        return false;
    }

    // Print program information
    GLint num_attributes, num_uniforms, max_uniform_length;
    glGetProgramiv(program, GL_ACTIVE_ATTRIBUTES, &num_attributes);
    glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &num_uniforms);
    glGetProgramiv(program, GL_ACTIVE_UNIFORM_MAX_LENGTH, &max_uniform_length);

    std::cout << "Program " << program << " information:" << std::endl
        << "- Active attributes: " << num_attributes << std::endl
        << "- Active uniforms: " << num_uniforms << std::endl
        << "- Max uniform name length: " << max_uniform_length << std::endl;

    return true;
}

#define SAFE_DELETE(p) if (p) { delete p; p = NULL; }
#endif