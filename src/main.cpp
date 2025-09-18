#include <pbre/base.hpp>
#include <pbre/render/camera.hpp>
#include <pbre/render/material.hpp>
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
    buffers.uploadData(std::span(sphereVertices), std::span(sphereIndices));
    buffers.setAttribute(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);                   // position
    buffers.setAttribute(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float))); // normal

    PBRE::Wrapper::Shader shader;
    shader.loadFromFiles("shaders/vert.glsl", "shaders/frag.glsl");
    shader.use();

    // shader for the light indicator (simple unlit/emissive)
    PBRE::Wrapper::Shader lightShader;
    lightShader.loadFromFiles("shaders/light_vert.glsl", "shaders/light_frag.glsl");

    PBRE::Transform cubeTransform;

    PBRE::vec3 lightPosition = PBRE::vec3(5.0f, 0.0f, 0.0f);
    PBRE::vec3 lightColor = PBRE::vec3(300.0f, 300.0f, 300.0f);
    float lightIntensity = 1.0f;

    shader.set("light.position", lightPosition);
    shader.set("light.color", lightColor);
    shader.set("light.intensity", lightIntensity);

    PBRE::Render::Material material{
        .albedo = PBRE::vec3(0.5f, 0.0f, 0.0f),
        .metallic = 0.5f,
        .roughness = 0.1f};

    shader.set("material.albedo", material.albedo);
    shader.set("material.metallic", material.metallic);
    shader.set("material.roughness", material.roughness);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS); // or GL_LEQUAL

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    while (!window.shouldClose()) {
        window.beginFrame();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        {
            // Spin the cube
            cubeTransform.rotation = glm::rotate(cubeTransform.rotation, glm::radians(90.0f * 0.016f), PBRE::vec3(1.0f, 0.0f, 0.0f));
            cubeTransform.rotation = glm::rotate(cubeTransform.rotation, glm::radians(45.0f * 0.016f), PBRE::vec3(0.0f, 1.0f, 0.0f));
            cubeTransform.rotation = glm::normalize(cubeTransform.rotation);
        }

        {
            // Camera movement
            const float cameraSpeed = 5.0f * 0.016f; // adjust accordingly

            if (glfwGetKey(window.getGLFWwindow(), GLFW_KEY_W) == GLFW_PRESS)
                camera.setPosition(camera.getPosition() + cameraSpeed * glm::normalize(camera.getRotation() * PBRE::vec3(0.0f, 0.0f, -1.0f)));
            if (glfwGetKey(window.getGLFWwindow(), GLFW_KEY_S) == GLFW_PRESS)
                camera.setPosition(camera.getPosition() - cameraSpeed * glm::normalize(camera.getRotation() * PBRE::vec3(0.0f, 0.0f, -1.0f)));
            if (glfwGetKey(window.getGLFWwindow(), GLFW_KEY_A) == GLFW_PRESS)
                camera.setPosition(camera.getPosition() - cameraSpeed * glm::normalize(camera.getRotation() * PBRE::vec3(1.0f, 0.0f, 0.0f)));
            if (glfwGetKey(window.getGLFWwindow(), GLFW_KEY_D) == GLFW_PRESS)
                camera.setPosition(camera.getPosition() + cameraSpeed * glm::normalize(camera.getRotation() * PBRE::vec3(1.0f, 0.0f, 0.0f)));

            // query mouse movement
            static double lastX = 400, lastY = 300;
            double xpos, ypos;
            glfwGetCursorPos(window.getGLFWwindow(), &xpos, &ypos);
            float xoffset = xpos - lastX;
            float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom
            lastX = xpos;
            lastY = ypos;
            const float sensitivity = 0.1f;
            xoffset *= sensitivity;
            yoffset *= sensitivity;
            static float pitch = 0.0f;
            static float yaw = -90.0f; // depends on your forward direction

            yaw += -xoffset;
            pitch += yoffset;
            pitch = glm::clamp(pitch, -89.0f, 89.0f);

            glm::quat q = glm::quat(glm::vec3(glm::radians(pitch), glm::radians(yaw), 0.0f));
            camera.setRotation(q);
            glfwSetInputMode(window.getGLFWwindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }

        shader.use();
        PBRE::mat4 model = cubeTransform.toMat4();
        PBRE::mat4 view = camera.getViewMatrix();
        PBRE::mat4 projection = camera.getProjectionMatrix();

        // Update light uniforms in case the light moved
        shader.set("light.position", lightPosition);
        shader.set("light.color", lightColor);
        shader.set("light.intensity", lightIntensity);
        shader.set("model", model);
        shader.set("view", view);
        shader.set("projection", projection);
        shader.set("viewPos", camera.getPosition());

        buffers.draw();

        // Draw light indicator: small emissive sphere at lightPosition
        lightShader.use();
        PBRE::Transform lightTransform;
        lightTransform.position = lightPosition;
        lightTransform.scale = PBRE::vec3(0.2f); // small indicator
        PBRE::mat4 lightModel = lightTransform.toMat4();
        lightShader.set("model", lightModel);
        lightShader.set("view", view);
        lightShader.set("projection", projection);
        // use a visible color (tone-mapped in main shader) — provide smaller values here
        PBRE::vec3 indicatorColor = PBRE::vec3(1.0f, 1.0f, 0.8f);
        lightShader.set("color", indicatorColor);
        // Draw with depth test and same buffers
        buffers.draw();

        window.endFrame();
    }

    return 0;
}
