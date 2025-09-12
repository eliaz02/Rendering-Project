
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
#include "Skybox.h"
#include "WindowContext.h"
#include <cstdlib> // for rand()
#include <ctime>   // for seeding rand

#include "DemoScene.h"
#include "EntityComponentSysetm.h"

const int WIDTH{ 1600};
const int HEIGHT{ 1000 };
const char* WindowName{ "finestra" };


int main()
{
    WindowContext context{ WIDTH ,HEIGHT ,WindowName };
    // --- ECS Application Setup ---
    { // Scope of the renderer
        // 1. Create the renderer
        auto renderer = std::make_unique<DeferredRenderer>(context);

        // 2. Create the scene, passing ownership of the renderer
        auto scene = std::make_unique<MyDemoScene>(std::move(renderer));

        // 3. Initialize the scene (this calls render
        scene->initialize();

        double lastTime{ 0.0 };
        int frameCount{ 0 };
        double fps{ 0.0 };


        // render loop
        // -----------
        while (!context.shouldClose())
        {
            // input
            // -----
            context.processInput();

            context.updateTitle();
            // render
            // ------

            glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glEnable(GL_DEPTH_TEST);

            scene->render();

            // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
            // -------------------------------------------------------------------------------
            context.swapBuffersAndPollEvents();
        }
    } 

    return 0;
}
