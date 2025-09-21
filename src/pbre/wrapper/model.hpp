#pragma once

#include "model.hpp"

#include "texture.hpp"
#include "shader.hpp"
#include "buffers.hpp"
#include "pbre/render/material.hpp"

#include <vector>
#include <glad/glad.h>

namespace PBRE::Wrapper {
struct Mesh {
    std::vector<vec3> positions;
    std::vector<vec3> normals;
    std::vector<vec4> tangents;
    std::vector<vec2> uvs;
    std::vector<uint32_t> indices;

    size_t materialIndex = 0;

    // GPU objects
    GLuint vao = 0;
    GLuint vboPos = 0;
    GLuint vboNorm = 0;
    GLuint vboTan = 0;
    GLuint vboUV = 0;
    GLuint ebo = 0;
    GLsizei indexCount = 0;
};

struct Model {
    std::vector<Mesh> meshes;
    std::vector<PBRE::Render::Material> materials;
    std::string path;

    bool loadFromFile(const std::string& filename);
    void draw(Shader& shader);
};
} // namespace PBRE::Wrapper