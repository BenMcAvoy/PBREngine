#pragma once

#include <pbre/base.hpp>

namespace PBRE::Render {
struct Material {
    vec3 albedo;
    float metallic;
    float roughness;
};
}; // namespace PBRE::Render
