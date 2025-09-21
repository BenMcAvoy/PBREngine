#pragma once

#include <pbre/base.hpp>

#include <pbre/wrapper/texture.hpp>

#include <variant>
#include <memory>

namespace PBRE::Render {

using AlbedoParam = std::variant<vec3, std::shared_ptr<PBRE::Wrapper::Texture>>;
using MetallicParam = std::variant<float, std::shared_ptr<PBRE::Wrapper::Texture>>;
using RoughnessParam = std::variant<float, std::shared_ptr<PBRE::Wrapper::Texture>>;
using NormalParam = std::shared_ptr<PBRE::Wrapper::Texture>; // only texture
using AOParam = std::variant<float, std::shared_ptr<PBRE::Wrapper::Texture>>;
using EmissiveParam = std::variant<vec3, std::shared_ptr<PBRE::Wrapper::Texture>>;

struct Material {
    AlbedoParam albedo = vec3(1.0f);
    MetallicParam metallic = 0.0f;
    RoughnessParam roughness = 1.0f;
    NormalParam normal = nullptr;
    AOParam ao = 1.0f;
    EmissiveParam emissive = vec3(0.0f);

    bool doubleSided = false; // not used yet (glTF extension, default false)
    float alphaCutoff = 0.5f; // not used yet (glTF extension, default 0.5)
};
}; // namespace PBRE::Render
