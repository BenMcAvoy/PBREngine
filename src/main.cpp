#include <pbre/base.hpp>
#include <pbre/render/camera.hpp>
#include <pbre/wrapper/buffers.hpp>
#include <pbre/wrapper/shader.hpp>
#include <pbre/wrapper/window.hpp>

#include "data.h"

static inline PBRE::Render::Camera* camera = nullptr;
int main(int argc, char** argv) {
    PBRE::Wrapper::Window window(800, 600, "PBRE Example - Transform");

    glfwSetFramebufferSizeCallback(window.getGLFWwindow(), [](GLFWwindow* window, int width, int height) {
        glViewport(0, 0, width, height);
        camera->setAspectRatio(static_cast<float>(width) / static_cast<float>(height));
    });

    PBRE::Render::Camera camera;
    camera.setPosition({10.0f, 10.0f, 3.0f});
    camera.lookAt({0.0f, 0.0f, 0.0f});
    ::camera = &camera;

    PBRE::Wrapper::Buffers buffers;
    buffers.uploadData(std::span(cubeVertices), std::span(cubeIndices));
    buffers.setAttribute(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float));

    PBRE::Wrapper::Shader shader;
    shader.loadFromFiles("shaders/simple.vert", "shaders/simple.frag");
    shader.use();

    PBRE::Transform cubeTransform;

    while (!window.shouldClose()) {
        window.beginFrame();

        { // frame by frame manipulation of transform
            float scale = (sin(glfwGetTime()) + 1.0f) / 2.0f + 0.5f;
            cubeTransform.scale = PBRE::vec3(scale, scale, scale);
            cubeTransform.rotation = glm::rotate(cubeTransform.rotation, glm::radians(90.0f * 0.016f), PBRE::vec3(1.0f, 0.0f, 0.0f));
            cubeTransform.rotation = glm::rotate(cubeTransform.rotation, glm::radians(45.0f * 0.016f), PBRE::vec3(0.0f, 1.0f, 0.0f));
            cubeTransform.rotation = glm::normalize(cubeTransform.rotation);
        }

        shader.use();
        PBRE::mat4 model = cubeTransform.toMat4();
        PBRE::mat4 view = camera.getViewMatrix();
        PBRE::mat4 projection = camera.getProjectionMatrix();
        shader.set("model", model);
        shader.set("view", view);
        shader.set("projection", projection);

        buffers.draw();

        window.endFrame();
    }

    return 0;
}
