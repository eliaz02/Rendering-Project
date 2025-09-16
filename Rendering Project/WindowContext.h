#pragma once
#ifndef WINDOW_CONTEXT_H
#define WINDOW_CONTEXT_H

#include <glad/gl.h>      
#define GLFW_INCLUDE_NONE  
#include <GLFW/glfw3.h>
#include "Camera.h"

class Scene;  // avoid circular include #include "EntityComponentSysetm.h"


// this class has the of abstracting the handling of the 
class WindowContext
{
public:
    WindowContext(const int width, const int height, const char* title);
    ~WindowContext();

    // disable copy clone
    WindowContext(const WindowContext&) = delete;
    WindowContext& operator=(const WindowContext&) = delete;

    // Metodi per il ciclo principale
    bool shouldClose() const;
    void processInput();
    void swapBuffersAndPollEvents();
    void updateTitle();
    // Accesso ai dati
    Camera& getCamera();
    float getDeltaTime();
    float getTotalTime() const ;
    int getHeight() const ;
    int getWidth() const ;

    // Metodo per collegare la scena per le callback
    void linkScene(Scene* scene);

    void close();

private:
    // Metodi di gestione interna degli eventi
    void onResize(int width, int height);
    void onMouseMove(double xpos, double ypos);

    // Dispatcher statici per le callback di GLFW
    static void DispatchResizeCallback(GLFWwindow* window, int width, int height);
    static void DispatchMouseCallback(GLFWwindow* window, double xpos, double ypos);

private:
    GLFWwindow* m_window = nullptr;
    Scene* m_linkedScene = nullptr; // Puntatore alla scena per il resize
    Camera m_camera;

    // Stato
    int m_width;
    int m_height;
    double m_deltaTime = 0.0;
    double m_lastFrame = 0.0;
    bool m_firstMouse = true;
    double m_lastX;
    double m_lastY;
    int m_frameCount{ 0 };
    double m_fps{ 0.0 };
    double m_fps_lastFrame{ 0.0 };
    char m_title[64];
};
#endif