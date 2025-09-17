#pragma once

#include "pbre/base.hpp"

#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include <filesystem>

namespace PBRE::Wrapper {
class Shader {
  public:
    Shader();
    ~Shader();

    void loadFromFiles(const std::filesystem::path& vertexPath, const std::filesystem::path& fragmentPath);

    void use() const;
    GLuint getProgram() const { return program_; }

    void set(std::string_view name, int value) const;
    void set(std::string_view name, float value) const;
    void set(std::string_view name, const vec3& value) const;
    void set(std::string_view name, const vec4& value) const;
    void set(std::string_view name, const mat4& value) const;

  private:
    GLuint program_ = 0;
};
} // namespace PBRE::Wrapper
