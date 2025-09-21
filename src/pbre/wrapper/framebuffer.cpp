#include "framebuffer.hpp"

using namespace PBRE::Wrapper;

Framebuffer::Framebuffer(int w, int h, int samples) { create(w, h, samples); }
Framebuffer::~Framebuffer() { destroy(); }

void Framebuffer::moveFrom(Framebuffer&& other) noexcept {
    fbo_ = other.fbo_; other.fbo_ = 0;
    colorTex_ = other.colorTex_; other.colorTex_ = 0;
    depthRbo_ = other.depthRbo_; other.depthRbo_ = 0;
    width_ = other.width_; other.width_ = 0;
    height_ = other.height_; other.height_ = 0;
}

void Framebuffer::create(int w, int h, int samples) {
    destroy();
    width_ = w; height_ = h; samples_ = samples > 1 ? samples : 1;

    // Single-sample target (always present for tonemapping)
    glGenFramebuffers(1, &fbo_);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    glGenTextures(1, &colorTex_);
    glBindTexture(GL_TEXTURE_2D, colorTex_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width_, height_, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTex_, 0);
    // Provide a depth for single-sample if we render directly to it (samples_==1)
    glGenRenderbuffers(1, &depthRbo_);
    glBindRenderbuffer(GL_RENDERBUFFER, depthRbo_);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width_, height_);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthRbo_);
    GLenum drawBuf = GL_COLOR_ATTACHMENT0;
    glDrawBuffers(1, &drawBuf);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    if (samples_ > 1) {
        // Multisample rendering target
        glGenFramebuffers(1, &msaaFbo_);
        glBindFramebuffer(GL_FRAMEBUFFER, msaaFbo_);

        glGenRenderbuffers(1, &msaaColorRbo_);
        glBindRenderbuffer(GL_RENDERBUFFER, msaaColorRbo_);
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples_, GL_RGBA16F, width_, height_);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, msaaColorRbo_);

        glGenRenderbuffers(1, &msaaDepthRbo_);
        glBindRenderbuffer(GL_RENDERBUFFER, msaaDepthRbo_);
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples_, GL_DEPTH24_STENCIL8, width_, height_);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, msaaDepthRbo_);

        GLenum msaaDraw = GL_COLOR_ATTACHMENT0;
        glDrawBuffers(1, &msaaDraw);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
}

void Framebuffer::destroy() {
    if (depthRbo_) glDeleteRenderbuffers(1, &depthRbo_), depthRbo_ = 0;
    if (colorTex_) glDeleteTextures(1, &colorTex_), colorTex_ = 0;
    if (fbo_) glDeleteFramebuffers(1, &fbo_), fbo_ = 0;
    if (msaaDepthRbo_) glDeleteRenderbuffers(1, &msaaDepthRbo_), msaaDepthRbo_ = 0;
    if (msaaColorRbo_) glDeleteRenderbuffers(1, &msaaColorRbo_), msaaColorRbo_ = 0;
    if (msaaFbo_) glDeleteFramebuffers(1, &msaaFbo_), msaaFbo_ = 0;
}

void Framebuffer::resize(int w, int h) {
    if (w == width_ && h == height_) return;
    create(w, h, samples_);
}

void Framebuffer::bind() const {
    if (samples_ > 1 && msaaFbo_) {
        glBindFramebuffer(GL_FRAMEBUFFER, msaaFbo_);
    } else {
        glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    }
    glViewport(0, 0, width_, height_);
}

void Framebuffer::unbind() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::resolve() const {
    if (samples_ <= 1 || !msaaFbo_) return; // nothing to resolve
    glBindFramebuffer(GL_READ_FRAMEBUFFER, msaaFbo_);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo_);
    glBlitFramebuffer(0, 0, width_, height_, 0, 0, width_, height_, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}
