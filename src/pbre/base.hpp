#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <variant>

namespace PBRE {
using vec2 = glm::vec2;
using vec3 = glm::vec3;
using vec4 = glm::vec4;
using mat4 = glm::mat4;
using quat = glm::quat;
using ivec2 = glm::ivec2;

enum class Axis {
    X,
    Y,
    Z
};

struct Transform {
    vec3 position;
    quat rotation;
    vec3 scale;

    mat4 toMat4() const {
        mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, position);
        model = model * glm::mat4_cast(rotation);
        model = glm::scale(model, scale);
        return model;
    }

    Transform()
        : position(0.0f, 0.0f, 0.0f),
          rotation(1.0f, 0.0f, 0.0f, 0.0f),
          scale(1.0f, 1.0f, 1.0f) {}
};
} // namespace PBRE

// util
template<typename T, typename... Ts>
const T* get_if(const std::variant<Ts...>& v) {
    return std::get_if<T>(&v);
}

#define BIND_MATERIAL_PARAM(material, paramName, uniformName, texUnit, shader, fallbackType) \
do { \
    if (auto tex = std::get_if<std::shared_ptr<PBRE::Wrapper::Texture>>(&material.paramName); tex && *tex) { \
        shader.set("material.has" #uniformName "Map", 1); \
        shader.set("material." #uniformName "Map", texUnit); \
        glActiveTexture(GL_TEXTURE0 + texUnit); \
        (*tex)->bind(texUnit); \
    } else { \
        shader.set("material.has" #uniformName "Map", 0); \
        shader.set("material." #uniformName, std::get<fallbackType>(material.paramName)); \
    } \
} while(0)
