#include "WindowContext.h"
#include <stdexcept>
#include <iostream>

#include "EntityComponentSysetm.h"

// to do delete the camera logic from all the code outside this one, and substitute it with this class  


WindowContext::WindowContext(const int width, const  int height, const char* title):
    m_width(width),
    m_height(height),
    m_camera(glm::vec3(0.0f, 0.0f, 3.0f)),
    m_lastX(static_cast<float>(width) / 2.0f),
    m_lastY(static_cast<float>(height) / 2.0f)
{
    strcpy_s(m_title, title); 

    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW");
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif


    m_window = glfwCreateWindow(m_width, m_height, m_title, NULL, NULL);
    if (!m_window) {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window");
    }

    glfwMakeContextCurrent(m_window);

    if (!gladLoaderLoadGL()) {
        throw std::runtime_error("Failed to initialize GLAD");
    }

    // Collega questa istanza alla finestra GLFW per le callback
    glfwSetWindowUserPointer(m_window, this);

    // Imposta i dispatcher statici
    glfwSetFramebufferSizeCallback(m_window, DispatchResizeCallback);
    glfwSetCursorPosCallback(m_window, DispatchMouseCallback);

    glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glViewport(0, 0, m_width, m_height);

    // Setup openGL variable 
    glEnable(GL_BLEND);

    // enable face culling
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    // set up GL_DEPTH_TEST
    glEnable(GL_DEPTH_TEST);
}

WindowContext::~WindowContext()
{
    close();
}

void WindowContext::close()
{
    if (m_window)
    {
        glfwDestroyWindow(m_window);
        glfwTerminate();
    }
}
void WindowContext::updateTitle()
{
    float currentFrame = static_cast<float>(glfwGetTime());
    m_deltaTime = currentFrame - m_lastFrame;
    m_lastFrame = currentFrame;

    float fps_deltatime = currentFrame - m_fps_lastFrame;
    m_frameCount++;
    if (fps_deltatime >= 1.0) {
        m_fps = m_frameCount / (fps_deltatime);
        m_frameCount = 0; 
        m_fps_lastFrame = currentFrame;
       

        // Update window title with FPS
        char title[64];
        sprintf_s(title, 64, "%s | FPS: %.1f", m_title, m_fps);
        glfwSetWindowTitle(m_window, title);
    }
}


bool WindowContext::shouldClose() const
{
    return glfwWindowShouldClose(m_window);
}

void WindowContext::processInput()
{
    if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(m_window, true);

    if (glfwGetKey(m_window, GLFW_KEY_W) == GLFW_PRESS)                 m_camera.ProcessKeyboard(FORWARD, m_deltaTime);
    if (glfwGetKey(m_window, GLFW_KEY_S) == GLFW_PRESS)                 m_camera.ProcessKeyboard(BACKWARD, m_deltaTime);
    if (glfwGetKey(m_window, GLFW_KEY_A) == GLFW_PRESS)                 m_camera.ProcessKeyboard(LEFT, m_deltaTime);
    if (glfwGetKey(m_window, GLFW_KEY_D) == GLFW_PRESS)                 m_camera.ProcessKeyboard(RIGHT, m_deltaTime);
    if (glfwGetKey(m_window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)      m_camera.ProcessKeyboard(CTRL_P, m_deltaTime);
    if (glfwGetKey(m_window, GLFW_KEY_LEFT_CONTROL) == GLFW_RELEASE)    m_camera.ProcessKeyboard(CTRL_R, m_deltaTime);
}

void WindowContext::swapBuffersAndPollEvents()
{
    glfwSwapBuffers(m_window);
    glfwPollEvents();
}

Camera& WindowContext::getCamera()
{
    return m_camera;
}

float WindowContext::getDeltaTime()
{
    float currentFrame = static_cast<float>(glfwGetTime());
    m_deltaTime = currentFrame - m_lastFrame;
    m_lastFrame = currentFrame;
    return m_deltaTime;
}

void WindowContext::linkScene(Scene* scene)
{
    m_linkedScene = scene;
}

// --- Gestori di eventi e dispatcher ---

void WindowContext::onResize(int width, int height)
{
    if (width > 0 && height > 0)
    {
        m_width = width;
        m_height = height;
        glViewport(0, 0, width, height);

        // Se una scena è collegata, inoltra la chiamata di resize
        if (m_linkedScene)
        {
            m_linkedScene->resize();
        }
    }
}

void WindowContext::onMouseMove(double xpos, double ypos)
{
    if (m_firstMouse)
    {
        m_lastX = xpos;
        m_lastY = ypos;
        m_firstMouse = false;
    }

    float xoffset = static_cast<float>(xpos - m_lastX);
    float yoffset = static_cast<float>(m_lastY - ypos);
    m_lastX = static_cast<float>(xpos);
    m_lastY = static_cast<float>(ypos);

    m_camera.ProcessMouseMovement(xoffset, yoffset);
}

void WindowContext::DispatchResizeCallback(GLFWwindow* window, int width, int height)
{
    auto* context = static_cast<WindowContext*>(glfwGetWindowUserPointer(window));
    if (context) {
        context->onResize(width, height);
    }
}

void WindowContext::DispatchMouseCallback(GLFWwindow* window, double xpos, double ypos)
{
    auto* context = static_cast<WindowContext*>(glfwGetWindowUserPointer(window));
    if (context) {
        context->onMouseMove(xpos, ypos);
    }
}

int WindowContext::getHeight() const
{
    return m_height;
}

int WindowContext::getWidth() const
{
    return m_width;
}
