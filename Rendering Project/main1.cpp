/*
#include <glad/gl.h>       // Includi prima di qualsiasi altro file OpenGL
#define GLFW_INCLUDE_NONE    // Evita che GLFW includa automaticamente gl.h
#include <GLFW/glfw3.h>

#include <iostream>
#include <random>
#include <math.h>
#include <ranges>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>  
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "Utilities.h"
#include "Shader.h"
#include "Camera.h"
#include "Mesh.h"
#include "frameBufferObject.h"
#include "LightStruct.h"
#include "renderable_object.h"
#include "Skybox.h"
#include <cstdlib> // for rand()
#include <ctime>   // for seeding rand
#include "DemoScene.h"   // The new scene class defined above

// Globals from the original code that are still needed for windowing and camera control

Camera camera;


int main() {
    // --- Boilerplate: Initialize GLFW, create window ---
    // (This part is the same as your original main function)
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    float deltaTime = 0.0f;
    float lastFrame = 0.0f;
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "ECS Deferred Renderer", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoaderLoadGL()) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // --- ECS Application Setup ---
    // 1. Create the renderer
    auto renderer = std::make_unique<DeferredRenderer>();

    // 2. Create the scene, passing ownership of the renderer
    auto scene = std::make_unique<MyDemoScene>(std::move(renderer));

    // 3. Initialize the scene (this calls renderer->initialize and loadScene)
    scene->initialize(SCR_WIDTH, SCR_HEIGHT);

    // --- Main Render Loop ---
    double lastTime = glfwGetTime();
    int frameCount = 0;

    while (!glfwWindowShouldClose(window)) {
        // -- Per-frame time logic & FPS Counter --
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        double currentTime = glfwGetTime();
        frameCount++;
        if (currentTime - lastTime >= 1.0) {
            char title[100];
            sprintf_s(title, "ECS Deferred Renderer | FPS: %d", frameCount);
            glfwSetWindowTitle(window, title);
            frameCount = 0;
            lastTime = currentTime;
        }

        // -- Input --
        processInput(window);

        // -- Render the Scene --
        // This single call now replaces GeometryPass, ShadowPass, LightingPass, etc.
        scene->render(camera);

        // -- Swap buffers and poll events --
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Resources are cleaned up automatically by unique_ptr destructors
    glfwTerminate();
    return 0;
}
*/

