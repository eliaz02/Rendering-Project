
#include <glad/gl.h> 
#define GLFW_INCLUDE_NONE    // Evita che GLFW includa automaticamente gl.h
#include <GLFW/glfw3.h>

#include <iostream> 
#define STB_IMAGE_IMPLEMENTATION

#include "WindowContext.h"
#include "DemoScene.h"
#include "exameScene.h"
#include "EntityComponentSysetm.h"




int main()
{
    const int WIDTH{ 1600 };
    const int HEIGHT{ 1000 };
    const char* WindowName{ "finestra" };

    WindowContext context{ WIDTH ,HEIGHT ,WindowName };
    // --- ECS Application Setup ---
    { // Scope of the renderer
        // 1. Create the renderer
        auto renderer = std::make_unique<DeferredRenderer>(context);

        // 2. Create the scene
        auto scene = std::make_unique<ExameScene>(std::move(renderer));

        // 3. Initialize the scene  
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
