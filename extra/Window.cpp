#include <stdexcept>

#include <GLFW/glfw3.h>

#include "Window.h"

Window::Window(const char* WindowName, int WindowWidth, int WindowHeight) {
    if (glfwInit() == GLFW_FALSE) {
        const char* Description;
        glfwGetError(&Description);
        throw std::runtime_error(Description);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#if defined(__APPLE__)
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    m_Window = glfwCreateWindow(WindowWidth, WindowHeight, WindowName, nullptr, nullptr);
    if (!m_Window) {
        const char* Description;
        glfwGetError(&Description);
        throw std::runtime_error(Description);
    }

    glfwFocusWindow(m_Window);

    glfwMakeContextCurrent(m_Window);
    glfwSwapInterval(1);
}

Window::~Window() {
    glfwDestroyWindow(m_Window);
    glfwTerminate();
}

bool Window::ShouldWindowClose() const {
    return glfwWindowShouldClose(m_Window);
}

void Window::PollEvents() const {
    glfwPollEvents();
}

void Window::SwapBuffers() const {
    glfwSwapBuffers(m_Window);
}
