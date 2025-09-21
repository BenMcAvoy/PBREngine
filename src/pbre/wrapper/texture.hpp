#pragma once

#include <glad/glad.h>

#include <vector>

namespace PBRE::Wrapper {
class Texture {
  public:
    Texture();
    ~Texture();

    void loadFromFile(const char* path, bool equirectangular = false);
    void loadHDRAsCubemap(const char* path, int faceSize = 512);
    bool loadFromImageData(int width, int height, int channels, const std::vector<unsigned char>& data);

    void bind(unsigned int unit = 0) const;
    GLuint getID() const;
    int getWidth() const { return width_; }
    int getHeight() const { return height_; }
    // Returns approximate max mip level usable (mip count - 1)
    float getMaxMips() const;

  private:
    GLuint id_ = 0;
    GLenum target_ = GL_TEXTURE_2D;
    int width_ = 0;
    int height_ = 0;
};
} // namespace PBRE::Wrapper