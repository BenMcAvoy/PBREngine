#include "window.hpp"

#include <glad/glad.h>
#include <stdexcept>

using namespace PBRE::Wrapper;

Window::Window(int width, int height, const char* title) {
    if (!glfwInit()) {
        throw std::runtime_error("Failed to init GLFW3");
    }

    window_ = glfwCreateWindow(width, height, title, nullptr, nullptr);

    if (!window_) {
        glfwTerminate();
        throw std::runtime_error("Failed to create window");
    }

    glfwMakeContextCurrent(window_);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
}
Window::~Window() {
    if (window_) {
        glfwDestroyWindow(window_);
    }
    glfwTerminate();
}
bool Window::shouldClose() const {
    return glfwWindowShouldClose(window_);
}
void Window::beginFrame() const {
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}
void Window::endFrame() const {
    glfwSwapBuffers(window_);
    glfwPollEvents();
}
int Window::getWidth() const {
    int width, height;
    glfwGetFramebufferSize(window_, &width, &height);
    return width;
}
int Window::getHeight() const {
    int width, height;
    glfwGetFramebufferSize(window_, &width, &height);
    return height;
}
