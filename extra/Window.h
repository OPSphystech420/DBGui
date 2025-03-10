#pragma once

typedef struct GLFWwindow GLFWwindow;


class Window {
public:
    Window() = delete;
    Window(const char* WindowName, int WindowWidth, int WindowHeight);
    ~Window();

    Window(const Window&) = delete;
    Window(Window&& Other) = delete;

    bool ShouldWindowClose() const;
    void PollEvents() const;
    void SwapBuffers() const;

    GLFWwindow* GetNativeWindow() const {
        return m_Window;
    }

private:
    mutable struct GLFWwindow* m_Window;
};
