#include "texture.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <stdexcept>
#include <string>
#include <vector>
#include <cmath>
#include <array>

using namespace PBRE::Wrapper;

Texture::Texture() {
    glGenTextures(1, &id_);
}
Texture::~Texture() {
    if (id_ != 0) {
        glDeleteTextures(1, &id_);
    }
}

void Texture::loadFromFile(const char* path, bool equirectangular) {
    target_ = GL_TEXTURE_2D;
    stbi_set_flip_vertically_on_load(true);

    int width, height, channels;
    if (equirectangular) {
        float* data = stbi_loadf(path, &width, &height, &channels, 0);
        if (!data) {
            throw std::runtime_error(std::string("Failed to load texture: ") + stbi_failure_reason());
        }
        glBindTexture(GL_TEXTURE_2D, id_);
        GLenum format = (channels == 3) ? GL_RGB : GL_RGBA;
        GLenum internalFormat = (channels == 3) ? GL_RGB16F : GL_RGBA16F;
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_FLOAT, data);
        stbi_image_free(data);
        width_ = width; height_ = height;
    } else {
        unsigned char* data = stbi_load(path, &width, &height, &channels, 0);
        if (!data) {
            throw std::runtime_error(std::string("Failed to load texture: ") + stbi_failure_reason());
        }
        glBindTexture(GL_TEXTURE_2D, id_);
        GLenum format = GL_RGB;
        if (channels == 1)
            format = GL_RED;
        else if (channels == 3)
            format = GL_RGB;
        else if (channels == 4)
            format = GL_RGBA;
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        stbi_image_free(data);
        width_ = width; height_ = height;
    }

    if (equirectangular) {
        // Generate mipmaps for environment textures so shader can use textureLod for roughness-based blur
        glGenerateMipmap(GL_TEXTURE_2D);
        // Equirectangular: S should repeat (wrap longitude), T clamped (avoid pole stretching)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    } else {
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void Texture::bind(unsigned int unit) const {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(target_, id_);
}

GLuint Texture::getID() const {
    return id_;
}

float Texture::getMaxMips() const {
    // number of levels N for dimension D is floor(log2(max(Dx, Dy))) + 1
    int maxDim = (width_ > height_) ? width_ : height_;
    if (maxDim <= 1) return 0.0f;
    int levels = 1;
    while ((1 << (levels - 1)) < maxDim) {
        ++levels;
        if (levels > 30) break; // safety
    }
    // levels is count; max LOD is levels - 1
    return float(levels - 1);
}

// Helpers to convert direction vector to equirectangular UV
static inline void dirToEquirectUV(const float dir[3], float& u, float& v) {
    float phi = atan2f(dir[2], dir[0]); // z,x
    float theta = asinf(fmaxf(-1.0f, fminf(1.0f, dir[1])));
    u = phi / (2.0f * 3.14159265359f) + 0.5f;
    // top-origin v: +Y (theta=+pi/2) -> v=0, -Y (theta=-pi/2) -> v=1
    v = 0.5f - theta / 3.14159265359f;
}

static inline void faceDir(int face, float x, float y, float out[3]) {
    // x,y in [-1,1], OpenGL cubemap face dirs
    switch (face) {
    case 0: out[0] =  1.0f; out[1] =     y; out[2] =    -x; break; // +X
    case 1: out[0] = -1.0f; out[1] =     y; out[2] =     x; break; // -X
    case 2: out[0] =     x; out[1] =  1.0f; out[2] =    -y; break; // +Y (z neg)
    case 3: out[0] =     x; out[1] = -1.0f; out[2] =     y; break; // -Y (z pos)
    case 4: out[0] =     x; out[1] =     y; out[2] =  1.0f; break; // +Z
    case 5: out[0] =    -x; out[1] =     y; out[2] = -1.0f; break; // -Z
    }
    // normalize
    float len = sqrtf(out[0]*out[0] + out[1]*out[1] + out[2]*out[2]);
    out[0] /= len; out[1] /= len; out[2] /= len;
}

void Texture::loadHDRAsCubemap(const char* path, int faceSize) {
    target_ = GL_TEXTURE_CUBE_MAP;
    // Do not flip when sampling into a cubemap
    stbi_set_flip_vertically_on_load(false);
    int w, h, ch;
    float* data = stbi_loadf(path, &w, &h, &ch, 0);
    if (!data) {
        throw std::runtime_error(std::string("Failed to load HDR: ") + stbi_failure_reason());
    }

    glBindTexture(GL_TEXTURE_CUBE_MAP, id_);
    width_ = faceSize; height_ = faceSize;

    std::vector<float> face(faceSize * faceSize * 3);
    auto sampleHDR = [&](float u, float v) {
        // wrap U, clamp V
        u = u - floorf(u);
        v = fmaxf(0.0f, fminf(1.0f, v));
        float x = u * (w - 1);
        float y = v * (h - 1);
        int x0 = (int)floorf(x);
        int y0 = (int)floorf(y);
        int x1 = (x0 + 1) % w;
        int y1 = (y0 + 1 < h) ? (y0 + 1) : (h - 1);
        float tx = x - x0;
        float ty = y - y0;
        auto fetch = [&](int ix, int iy) {
            int idx = (iy * w + ix) * ch;
            float r = data[idx + 0];
            float g = (ch > 1) ? data[idx + 1] : r;
            float b = (ch > 2) ? data[idx + 2] : r;
            return std::array<float,3>{r,g,b};
        };
        auto c00 = fetch(x0, y0);
        auto c10 = fetch(x1, y0);
        auto c01 = fetch(x0, y1);
        auto c11 = fetch(x1, y1);
        std::array<float,3> c{};
        for (int k = 0; k < 3; ++k) {
            float a = c00[k] * (1.0f - tx) + c10[k] * tx;
            float b = c01[k] * (1.0f - tx) + c11[k] * tx;
            c[k] = a * (1.0f - ty) + b * ty;
        }
        return c;
    };

    for (int f = 0; f < 6; ++f) {
        for (int y = 0; y < faceSize; ++y) {
            for (int x = 0; x < faceSize; ++x) {
                float sx = (2.0f * (x + 0.5f) / faceSize) - 1.0f;
                // make +y upwards on the face
                float sy = -((2.0f * (y + 0.5f) / faceSize) - 1.0f);
                float dir[3]; faceDir(f, sx, sy, dir);
                float u, v; dirToEquirectUV(dir, u, v);
                auto c = sampleHDR(u, v);
                int idx = (y * faceSize + x) * 3;
                face[idx+0] = c[0];
                face[idx+1] = c[1];
                face[idx+2] = c[2];
            }
        }
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + f, 0, GL_RGB16F,
                     faceSize, faceSize, 0, GL_RGB, GL_FLOAT, face.data());
    }
    stbi_image_free(data);

    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
}