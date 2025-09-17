#include "shader.hpp"

#include <fstream>

using namespace PBRE::Wrapper;

Shader::Shader() {}
Shader::~Shader() {
    if (program_ != 0) {
        glDeleteProgram(program_);
    }
}

void Shader::loadFromFiles(const std::filesystem::path& vertexPath, const std::filesystem::path& fragmentPath) {
    std::ifstream vertexFile(vertexPath);
    std::ifstream fragmentFile(fragmentPath);

    if (!vertexFile.is_open()) {
        throw std::runtime_error("Failed to open vertex shader file: " + vertexPath.string());
    }
    if (!fragmentFile.is_open()) {
        throw std::runtime_error("Failed to open fragment shader file: " + fragmentPath.string());
    }

    std::string vertexCode((std::istreambuf_iterator<char>(vertexFile)), std::istreambuf_iterator<char>());
    std::string fragmentCode((std::istreambuf_iterator<char>(fragmentFile)), std::istreambuf_iterator<char>());

    const char* vShaderCode = vertexCode.c_str();
    const char* fShaderCode = fragmentCode.c_str();

    GLuint vertex, fragment;
    GLint success;
    GLchar infoLog[512];

    // Vertex Shader
    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, nullptr);
    glCompileShader(vertex);
    glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertex, 512, nullptr, infoLog);
        throw std::runtime_error("Vertex shader compilation failed: " + std::string(infoLog));
    }

    // Fragment Shader
    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, nullptr);
    glCompileShader(fragment);
    glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragment, 512, nullptr, infoLog);
        throw std::runtime_error("Fragment shader compilation failed: " + std::string(infoLog));
    }

    // Shader Program
    program_ = glCreateProgram();
    glAttachShader(program_, vertex);
    glAttachShader(program_, fragment);
    glLinkProgram(program_);
    glGetProgramiv(program_, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program_, 512, nullptr, infoLog);
        throw std::runtime_error("Shader program linking failed: " + std::string(infoLog));
    }

    // Delete the shaders as they're linked into our program now and no longer necessary
    glDeleteShader(vertex);
    glDeleteShader(fragment);
}

void Shader::use() const {
    glUseProgram(program_);
}

void Shader::set(std::string_view name, int value) const {
    glUniform1i(glGetUniformLocation(program_, name.data()), value);
}
void Shader::set(std::string_view name, float value) const {
    glUniform1f(glGetUniformLocation(program_, name.data()), value);
}
void Shader::set(std::string_view name, const vec3& value) const {
    glUniform3fv(glGetUniformLocation(program_, name.data()), 1, &value[0]);
}
void Shader::set(std::string_view name, const vec4& value) const {
    glUniform4fv(glGetUniformLocation(program_, name.data()), 1, &value[0]);
}
void Shader::set(std::string_view name, const mat4& value) const {
    glUniformMatrix4fv(glGetUniformLocation(program_, name.data()), 1, GL_FALSE, &value[0][0]);
}
