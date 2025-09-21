#pragma once

#include <glad/glad.h>
#include <utility>

namespace PBRE::Wrapper {
class Framebuffer {
  public:
    Framebuffer() = default;
    Framebuffer(int width, int height, int samples = 1);
    ~Framebuffer();

    Framebuffer(const Framebuffer&) = delete;
    Framebuffer& operator=(const Framebuffer&) = delete;
    Framebuffer(Framebuffer&& other) noexcept { moveFrom(std::move(other)); }
    Framebuffer& operator=(Framebuffer&& other) noexcept {
        if (this != &other) { destroy(); moveFrom(std::move(other)); }
        return *this;
    }

    void create(int width, int height, int samples = 1);
    void destroy();
    void resize(int width, int height);

    // Bind for rendering (MSAA FBO if samples>1, otherwise single-sample FBO)
    void bind() const;
    static void unbind();

    // Resolve MSAA color to single-sample color texture if MSAA is enabled
    void resolve() const;

    GLuint colorTex() const { return colorTex_; }
    GLuint fbo() const { return fbo_; }
    int width() const { return width_; }
    int height() const { return height_; }
    int samples() const { return samples_; }

  private:
    void moveFrom(Framebuffer&& other) noexcept;

    GLuint fbo_ = 0;
    GLuint colorTex_ = 0;     // RGBA16F color
    GLuint depthRbo_ = 0;     // depth buffer
    // MSAA resources (used when samples_ > 1)
    GLuint msaaFbo_ = 0;
    GLuint msaaColorRbo_ = 0;
    GLuint msaaDepthRbo_ = 0;
    int width_ = 0;
    int height_ = 0;
    int samples_ = 1;
};
} // namespace PBRE::Wrapper
