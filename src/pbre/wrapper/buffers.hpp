#pragma once

#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include <span>

namespace PBRE::Wrapper {
class Buffers {
  public:
    Buffers();
    ~Buffers();

    void setAttribute(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* pointer = nullptr) const;
    void bind() const;
    void unbind() const;

    void uploadData(std::span<const float> vertices, std::span<const GLuint> indices);
    void draw() const;

  private:
    GLuint vao_ = 0;
    GLuint vbo_ = 0;
    GLuint ebo_ = 0;

    int count_ = 0;
};
} // namespace PBRE::Wrapper
