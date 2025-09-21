#include <pbre/base.hpp>
#include <pbre/render/camera.hpp>
#include <pbre/render/material.hpp>
#include <pbre/wrapper/buffers.hpp>
#include <pbre/wrapper/framebuffer.hpp>
#include <pbre/wrapper/model.hpp>
#include <pbre/wrapper/shader.hpp>
#include <pbre/wrapper/texture.hpp>
#include <pbre/wrapper/window.hpp>

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

    // Tonemapping post-process shader
    PBRE::Wrapper::Shader tonemap;
    tonemap.loadFromFiles("shaders/tonemap_vert.glsl", "shaders/tonemap_frag.glsl");
    tonemap.use();
    tonemap.set("uColor", 0);
    tonemap.set("uExposure", 1.0f);

    // Fullscreen triangle needs a VAO in core profile
    GLuint screenVAO = 0;
    glGenVertexArrays(1, &screenVAO);

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
    // AO fallback value if material has no AO map
    shader.set("material.AO", 1.0f);
    shader.set("horizonFadePower", 2.0f);
    shader.set("debugMode", 0);
    shader.set("enableIBL", 1);
    shader.set("enableDirect", 1);

    // HDR framebuffer (RGBA16F)
    PBRE::Wrapper::Framebuffer hdrFbo(window.getWidth(), window.getHeight(), 4);
    float exposure = 1.0f;

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

    // Test model
    PBRE::Wrapper::Model model;
    if (!model.loadFromFile("resources/lion_head/lion_head_4k.gltf")) {
        std::cerr << "Failed to load model\n";
        return -1;
    }
    PBRE::Wrapper::Model tableModel;
    if (!tableModel.loadFromFile("resources/table/round_wooden_table_02_4k.gltf")) {
        std::cerr << "Failed to load table model\n";
        return -1;
    }
    PBRE::Wrapper::Model cameraModel;
    if (!cameraModel.loadFromFile("resources/vintage_camera/vintage_video_camera_4k.gltf")) {
        std::cerr << "Failed to load camera model\n";
        return -1;
    }

    BIND_MATERIAL_PARAM(material, albedo, Albedo, 1, shader, PBRE::vec3);
    BIND_MATERIAL_PARAM(material, metallic, Metallic, 2, shader, float);
    BIND_MATERIAL_PARAM(material, roughness, Roughness, 3, shader, float);

    if (auto tex = material.normal; tex) {
        shader.set("u_HasNormalMap", 1);
        shader.set("u_NormalMap", 4);
        glActiveTexture(GL_TEXTURE0 + 4);
        tex->bind(4);
    } else {
        shader.set("u_HasNormalMap", 0);
    }

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS); // or GL_LEQUAL
    // Reduce seams between cubemap faces
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    // Grid controls
    bool mouseLocked = false;
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
            static float horizonFadePower = 2.0f;
            ImGui::Checkbox("Enable IBL", &ibl);
            ImGui::SameLine();
            ImGui::Checkbox("Enable Direct", &direct);
            ImGui::SliderFloat("IBL Intensity", &iblIntensity, 0.0f, 2.0f);
            ImGui::SliderFloat("Horizon Fade Power", &horizonFadePower, 0.0f, 8.0f);
            ImGui::SliderInt("Debug Mode", &debugMode, 0, 4);
            ImGui::Separator();
            ImGui::SliderFloat("Exposure", &exposure, 0.0f, 5.0f);

            shader.use();
            shader.set("debugMode", debugMode);
            shader.set("enableIBL", ibl ? 1 : 0);
            shader.set("enableDirect", direct ? 1 : 0);
            shader.set("iblIntensity", iblIntensity);
            shader.set("horizonFadePower", horizonFadePower);
            tonemap.use();
            tonemap.set("uExposure", exposure);

            if (auto metallicPtr = std::get_if<float>(&material.metallic); metallicPtr) {
                if (auto roughnessPtr = std::get_if<float>(&material.roughness); roughnessPtr) {
                    ImGui::SliderFloat("Metallic", metallicPtr, 0.0f, 1.0f);
                    ImGui::SliderFloat("Roughness", roughnessPtr, 0.0f, 1.0f);
                    shader.use();
                    shader.set("material.Metallic", *metallicPtr);
                    shader.set("material.Roughness", *roughnessPtr);
                }
            }

            // AO
            if (auto aoPtr = std::get_if<float>(&material.ao); aoPtr) {
                ImGui::SliderFloat("AO", aoPtr, 0.0f, 1.0f);
                shader.use();
                shader.set("material.AO", *aoPtr);
            }

            shader.use();
            // shader.set("material.albedo", material.albedo);
            // BIND_MATERIAL_PARAM(material, albedo, Albedo, 1, shader, PBRE::vec3);

            if (auto albVec3 = std::get_if<PBRE::vec3>(&material.albedo); albVec3) {
                ImGui::ColorEdit3("Albedo Color", &albVec3->x);
                shader.set("material.Albedo", *albVec3);
            } else if (auto albTex = std::get_if<std::shared_ptr<PBRE::Wrapper::Texture>>(&material.albedo); albTex && *albTex) {
                ImGui::Text("Albedo Texture: %dx%d", (*albTex)->getWidth(), (*albTex)->getHeight());
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

        // Ensure HDR framebuffer matches window size
        hdrFbo.resize(window.getWidth(), window.getHeight());

        // Render scene into HDR (MSAA) FBO
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        hdrFbo.bind();
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glDepthMask(GL_TRUE);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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

        PBRE::Transform t;
        PBRE::mat4 modelMat = t.toMat4();
        shader.set("model", modelMat);
        // Draw loaded model
        model.draw(shader);

        // Draw table
        PBRE::Transform tableTransform;
        tableTransform.position = PBRE::vec3(0.0f, -0.75f, 0.0f);
        shader.set("model", tableTransform.toMat4());
        tableModel.draw(shader);

        // Draw camera model on table
        PBRE::Transform cameraTransform;
        cameraTransform.position = PBRE::vec3(0.2f, 0.0f, 0.0f);
        cameraTransform.rotation = glm::angleAxis(glm::radians(12.0f), PBRE::vec3(0.0f, 1.0f, 0.0f));
        shader.set("model", cameraTransform.toMat4());
        cameraModel.draw(shader);

        ImGui::Begin("Table");

        // DragFloat3 for position
        ImGui::DragFloat3("Position", &cameraTransform.position.x, 0.1f);

        ImGui::End();

        lightShader.use();
        PBRE::Transform lightTransform;
        lightTransform.position = lightPosition;
        lightTransform.scale = PBRE::vec3(0.2f);
        PBRE::mat4 lightModel = lightTransform.toMat4();
        lightShader.set("model", lightModel);
        lightShader.set("view", view);
        lightShader.set("projection", projection);
        PBRE::vec3 indicatorColor = PBRE::vec3(1.0f, 1.0f, 0.8f);
        lightShader.set("color", indicatorColor);
        buffers.draw();

        // Resolve MSAA to single-sample color
        hdrFbo.resolve();

        // Tonemap to default framebuffer
        PBRE::Wrapper::Framebuffer::unbind();
        glDisable(GL_DEPTH_TEST);
        // Tonemap shader outputs gamma-corrected (sRGB) LDR. Ensure the default framebuffer doesn't apply another sRGB conversion.
        glDisable(GL_FRAMEBUFFER_SRGB);
        glBindVertexArray(screenVAO);
        tonemap.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, hdrFbo.colorTex());
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindVertexArray(0);

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
