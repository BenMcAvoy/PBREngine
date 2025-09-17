#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace PBRE {
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
