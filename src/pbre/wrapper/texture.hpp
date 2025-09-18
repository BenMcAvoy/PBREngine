#pragma once

#include <glad/glad.h>

namespace PBRE::Wrapper {
class Texture {
  public:
    Texture();
    ~Texture();

    void loadFromFile(const char* path, bool equirectangular = false);
    // Convert an equirectangular HDR image into a cubemap of given face size (e.g., 512)
    void loadHDRAsCubemap(const char* path, int faceSize = 512);

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