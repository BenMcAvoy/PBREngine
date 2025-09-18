#include <pbre/base.hpp>
#include <pbre/render/camera.hpp>
#include <pbre/render/material.hpp>
#include <pbre/wrapper/buffers.hpp>
#include <pbre/wrapper/shader.hpp>
#include <pbre/wrapper/window.hpp>
#include <pbre/wrapper/texture.hpp>

#include <imgui.h>

#include "data.h"

#include <iostream>

static inline PBRE::Render::Camera* camera = nullptr;
int run() {
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

    PBRE::Wrapper::Texture envIBL;
    // Convert the equirect HDR into a cubemap (face size 512 by default)
    envIBL.loadHDRAsCubemap("resources/kloppenheim_06_puresky_4k.hdr", 512);
    shader.set("environmentMap", 0);
    glActiveTexture(GL_TEXTURE0);
    envIBL.bind(0);
    // Inform shader of environment map mip count for roughness-based LOD
    shader.set("envMaxMips", envIBL.getMaxMips());
    shader.set("iblIntensity", 1.0f);
    shader.set("ao", 1.0f);
    shader.set("horizonFadePower", 2.0f);
    shader.set("debugMode", 0);
    shader.set("enableIBL", 1);
    shader.set("enableDirect", 1);

    // Light
    PBRE::vec3 lightPosition = PBRE::vec3(5.0f, 5.0f, 5.0f);
    PBRE::vec3 lightColor = PBRE::vec3(1.0f, 1.0f, 1.0f);
    float lightIntensity = 1.0f;

    shader.set("light.position", lightPosition);
    shader.set("light.color", lightColor);
    shader.set("light.intensity", lightIntensity);

    // Base material (used for single-sphere mode and as base albedo for grid)
    PBRE::Render::Material material{
        .albedo = PBRE::vec3(0.8f, 0.0f, 0.0f),
        .metallic = 0.0f,
        .roughness = 0.5f};

    shader.set("material.albedo", material.albedo);
    shader.set("material.metallic", material.metallic);
    shader.set("material.roughness", material.roughness);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS); // or GL_LEQUAL
    // Reduce seams between cubemap faces
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    // Grid controls
    bool mouseLocked = false;
    static bool showGrid = true;
    static int gridRows = 6;
    static int gridCols = 6;
    static float gridSpacing = 2.5f;

    while (!window.shouldClose()) {
        window.beginFrame();

        auto fps = ImGui::GetIO().Framerate;
        ImGui::Begin("FPS");
        ImGui::Text("FPS: %.1f", fps);
        ImGui::End();

        if (!mouseLocked) {
            ImGui::Begin("Settings");

            ImGui::Text("Light Settings");
            ImGui::SliderFloat3("Position", &lightPosition.x, -20.0f, 20.0f);
            ImGui::ColorEdit3("Color", &lightColor.x);
            ImGui::SliderFloat("Intensity", &lightIntensity, 0.0f, 100.0f);

            shader.use();
            shader.set("light.position", lightPosition);
            shader.set("light.color", lightColor);
            shader.set("light.intensity", lightIntensity);

            ImGui::Separator();
            ImGui::Text("Debug");
            static int debugMode = 0;
            static bool ibl = true, direct = true;
            static float iblIntensity = 1.0f;
            static float ao = 1.0f;
            static float horizonFadePower = 2.0f;
            ImGui::Checkbox("Enable IBL", &ibl);
            ImGui::SameLine();
            ImGui::Checkbox("Enable Direct", &direct);
            ImGui::SliderFloat("IBL Intensity", &iblIntensity, 0.0f, 2.0f);
            ImGui::SliderFloat("Ambient Occlusion (AO)", &ao, 0.0f, 1.0f);
            ImGui::SliderFloat("Horizon Fade Power", &horizonFadePower, 0.0f, 8.0f);
            ImGui::SliderInt("Debug Mode", &debugMode, 0, 4);

            shader.use();
            shader.set("debugMode", debugMode);
            shader.set("enableIBL", ibl ? 1 : 0);
            shader.set("enableDirect", direct ? 1 : 0);
            shader.set("iblIntensity", iblIntensity);
            shader.set("ao", ao);
            shader.set("horizonFadePower", horizonFadePower);

            ImGui::Separator();
            ImGui::Text("PBR Test Grid");
            ImGui::Checkbox("Show Grid (NxN Spheres)", &showGrid);
            ImGui::SliderInt("Rows", &gridRows, 2, 10);
            ImGui::SliderInt("Cols", &gridCols, 2, 10);
            ImGui::SliderFloat("Spacing", &gridSpacing, 1.0f, 6.0f);
            ImGui::ColorEdit3("Base Albedo", &material.albedo.x);
            if (!showGrid) {
                ImGui::SliderFloat("Metallic", &material.metallic, 0.0f, 1.0f);
                ImGui::SliderFloat("Roughness", &material.roughness, 0.0f, 1.0f);
            }

            shader.use();
            shader.set("material.albedo", material.albedo);
            if (!showGrid) {
                shader.set("material.metallic", material.metallic);
                shader.set("material.roughness", material.roughness);
            }

            ImGui::Separator();
            ImGui::Text("Camera: ESC toggle mouse look");
            ImGui::End();
        }

        if (mouseLocked) {
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
            double xpos, ypos;
            glfwGetCursorPos(window.getGLFWwindow(), &xpos, &ypos);
            static double lastX = xpos, lastY = ypos;
            float xoffset = xpos - lastX;
            float yoffset = lastY - ypos;
            lastX = xpos;
            lastY = ypos;
            const float sensitivity = 0.1f;
            xoffset *= sensitivity;
            yoffset *= sensitivity;
            static float pitch = 0.0f;
            static float yaw = -90.0f;

            yaw += -xoffset;
            pitch += yoffset;
            pitch = glm::clamp(pitch, -89.0f, 89.0f);

            glm::quat q = glm::quat(glm::vec3(glm::radians(pitch), glm::radians(yaw), 0.0f));
            camera.setRotation(q);
        }

        static auto timeSincePress = std::chrono::high_resolution_clock::now();
        glfwSetInputMode(window.getGLFWwindow(), GLFW_CURSOR, mouseLocked ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
        if (glfwGetKey(window.getGLFWwindow(), GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            auto now = std::chrono::high_resolution_clock::now();
            if (now - timeSincePress > std::chrono::milliseconds(200)) {
                mouseLocked = !mouseLocked;
                timeSincePress = now;
            }
        }

        shader.use();
        PBRE::mat4 view = camera.getViewMatrix();
        PBRE::mat4 projection = camera.getProjectionMatrix();

        // Update light uniforms in case the light moved
        shader.set("light.position", lightPosition);
        shader.set("light.color", lightColor);
        shader.set("light.intensity", lightIntensity);
        shader.set("view", view);
        shader.set("projection", projection);
        shader.set("viewPos", camera.getPosition());

        if (showGrid) {
            // Draw a grid of spheres: metallic varies across columns, roughness across rows
            float halfCols = (gridCols - 1) * 0.5f;
            float halfRows = (gridRows - 1) * 0.5f;
            for (int r = 0; r < gridRows; ++r) {
                float rough = (gridRows > 1) ? float(r) / float(gridRows - 1) : 0.0f;
                // clamp to avoid 0.0 roughness fireflies if desired
                rough = glm::clamp(rough, 0.02f, 1.0f);
                for (int c = 0; c < gridCols; ++c) {
                    float metal = (gridCols > 1) ? float(c) / float(gridCols - 1) : 0.0f;

                    PBRE::Transform t;
                    t.position = PBRE::vec3((c - halfCols) * gridSpacing, 0.0f, (r - halfRows) * gridSpacing);
                    t.scale = PBRE::vec3(1.0f);
                    PBRE::mat4 model = t.toMat4();

                    shader.set("model", model);
                    shader.set("material.albedo", material.albedo);
                    shader.set("material.metallic", metal);
                    shader.set("material.roughness", rough);

                    buffers.draw();
                }
            }
        } else {
            // Single sphere mode
            PBRE::Transform t; // identity at origin
            PBRE::mat4 model = t.toMat4();
            shader.set("model", model);
            shader.set("material.albedo", material.albedo);
            shader.set("material.metallic", material.metallic);
            shader.set("material.roughness", material.roughness);
            buffers.draw();
        }

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


int main(int argc, char** argv) {
    try {
        return run();
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return -1;
    }
}
