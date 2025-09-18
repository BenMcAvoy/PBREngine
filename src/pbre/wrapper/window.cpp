#include "window.hpp"

#include <glad/glad.h>

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <imgui_internal.h>
#include <imgui_stdlib.h>

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

    // enable vsync
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;     // Enable Docking

    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window_, true);
    ImGui_ImplOpenGL3_Init("#version 460");

    glViewport(0, 0, width, height);
}
Window::~Window() {
    if (window_) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

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

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}
void Window::endFrame() const {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

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
