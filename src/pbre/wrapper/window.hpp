#pragma once

#include <GLFW/glfw3.h>

namespace PBRE::Wrapper {
class Window {
  public:
    Window(int width, int height, const char* title);
    ~Window();

    bool shouldClose() const;
    void beginFrame() const;
    void endFrame() const;

    GLFWwindow* getGLFWwindow() const { return window_; }

    int getWidth() const;
    int getHeight() const;

  private:
    GLFWwindow* window_ = nullptr;
};
} // namespace PBRE::Wrapper
