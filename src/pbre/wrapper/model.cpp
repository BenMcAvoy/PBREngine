#include "model.hpp"

#define TINYGLTF_NOEXCEPTION
#define TINYGLTF_IMPLEMENTATION
#include <tiny_gltf.h>

#include <iostream>

using namespace PBRE::Wrapper;

bool Model::loadFromFile(const std::string& filename) {
    tinygltf::Model gltfModel;
    tinygltf::TinyGLTF loader;
    std::string err, warn;

    bool ret = loader.LoadASCIIFromFile(&gltfModel, &err, &warn, filename);
    if (!warn.empty()) {
        std::cout << "GLTF Warning: " << warn << std::endl;
    }
    if (!err.empty()) {
        std::cerr << "GLTF Error: " << err << std::endl;
    }
    if (!ret) {
        std::cerr << "Failed to load GLTF: " << filename << std::endl;
        return false;
    }

    path = filename;

    // Load materials
    materials.resize(gltfModel.materials.size());
    for (size_t i = 0; i < gltfModel.materials.size(); ++i) {
        const auto& gltfMat = gltfModel.materials[i];
        auto& mat = materials[i];

        // Albedo
        if (gltfMat.values.find("baseColorFactor") != gltfMat.values.end()) {
            const auto& factor = gltfMat.values.at("baseColorFactor").ColorFactor();
            mat.albedo = vec3(factor[0], factor[1], factor[2]);
        }
        if (gltfMat.values.find("baseColorTexture") != gltfMat.values.end()) {
            int texIndex = gltfMat.values.at("baseColorTexture").TextureIndex();
            if (texIndex >= 0 && texIndex < gltfModel.textures.size()) {
                int imgIndex = gltfModel.textures[texIndex].source;
                if (imgIndex >= 0 && imgIndex < gltfModel.images.size()) {
                    const auto& img = gltfModel.images[imgIndex];
                    auto texture = std::make_shared<Texture>();
                    if (texture->loadFromImageData(img.width, img.height, img.component, img.image)) {
                        mat.albedo = texture;
                    }
                }
            }
        }

        // Metallic
        if (gltfMat.values.find("metallicFactor") != gltfMat.values.end()) {
            mat.metallic = static_cast<float>(gltfMat.values.at("metallicFactor").Factor());
        }
        if (gltfMat.values.find("roughnessFactor") != gltfMat.values.end()) {
            mat.roughness = static_cast<float>(gltfMat.values.at("roughnessFactor").Factor());
        }
        // glTF metallicRoughness is commonly a single texture (R=occlusion in extension, G=roughness, B=metallic in base spec)
        if (gltfMat.values.find("metallicRoughnessTexture") != gltfMat.values.end()) {
            int texIndex = gltfMat.values.at("metallicRoughnessTexture").TextureIndex();
            if (texIndex >= 0 && texIndex < (int)gltfModel.textures.size()) {
                int imgIndex = gltfModel.textures[texIndex].source;
                if (imgIndex >= 0 && imgIndex < (int)gltfModel.images.size()) {
                    const auto& img = gltfModel.images[imgIndex];
                    auto texture = std::make_shared<Texture>();
                    if (texture->loadFromImageData(img.width, img.height, img.component, img.image)) {
                        // Bind the same texture to both metallic and roughness so shader can sample channels separately
                        mat.metallic = texture;   // B channel
                        mat.roughness = texture;  // G channel
                    }
                }
            }
        }

        // Normal Map
        if (gltfMat.additionalValues.find("normalTexture") != gltfMat.additionalValues.end()) {
            int texIndex = gltfMat.additionalValues.at("normalTexture").TextureIndex();
            if (texIndex >= 0 && texIndex < gltfModel.textures.size()) {
                int imgIndex = gltfModel.textures[texIndex].source;
                if (imgIndex >= 0 && imgIndex < gltfModel.images.size()) {
                    const auto& img = gltfModel.images[imgIndex];
                    auto texture = std::make_shared<Texture>();
                    if (texture->loadFromImageData(img.width, img.height, img.component, img.image)) {
                        mat.normal = texture;
                    }
                }
            }
        }
        // AO
        if (gltfMat.additionalValues.find("occlusionTexture") != gltfMat.additionalValues.end()) {
            int texIndex = gltfMat.additionalValues.at("occlusionTexture").TextureIndex();
            if (texIndex >= 0 && texIndex < gltfModel.textures.size()) {
                int imgIndex = gltfModel.textures[texIndex].source;
                if (imgIndex >= 0 && imgIndex < gltfModel.images.size()) {
                    const auto& img = gltfModel.images[imgIndex];
                    auto texture = std::make_shared<Texture>();
                    if (texture->loadFromImageData(img.width, img.height, img.component, img.image)) {
                        mat.ao = texture;
                    }
                }
            }
        }
        // Emissive
        if (gltfMat.additionalValues.find("emissiveFactor") != gltfMat.additionalValues.end()) {
            const auto& factor = gltfMat.additionalValues.at("emissiveFactor").ColorFactor();
            mat.emissive = vec3(factor[0], factor[1], factor[2]);
        }
        if (gltfMat.additionalValues.find("emissiveTexture") != gltfMat.additionalValues.end()) {
            int texIndex = gltfMat.additionalValues.at("emissiveTexture").TextureIndex();
            if (texIndex >= 0 && texIndex < gltfModel.textures.size()) {
                int imgIndex = gltfModel.textures[texIndex].source;
                if (imgIndex >= 0 && imgIndex < gltfModel.images.size()) {
                    const auto& img = gltfModel.images[imgIndex];
                    auto texture = std::make_shared<Texture>();
                    if (texture->loadFromImageData(img.width, img.height, img.component, img.image)) {
                        mat.emissive = texture;
                    }
                }
            }
        }
        mat.doubleSided = gltfMat.doubleSided;
        if (gltfMat.alphaMode == "MASK") {
            mat.alphaCutoff = static_cast<float>(gltfMat.alphaCutoff);
        } else {
            mat.alphaCutoff = 0.5f; // default
        }
    }
    // Load meshes
    meshes.resize(gltfModel.meshes.size());
    for (size_t i = 0; i < gltfModel.meshes.size(); ++i) {
        const auto& gltfMesh = gltfModel.meshes[i];
        auto& mesh = meshes[i];

        // For simplicity, we only load the first primitive of each mesh
        if (gltfMesh.primitives.empty()) continue;
        const auto& prim = gltfMesh.primitives[0];

        // Positions
        if (prim.attributes.find("POSITION") != prim.attributes.end()) {
            int accessorIndex = prim.attributes.at("POSITION");
            const auto& accessor = gltfModel.accessors[accessorIndex];
            const auto& bufferView = gltfModel.bufferViews[accessor.bufferView];
            const auto& buffer = gltfModel.buffers[bufferView.buffer];

            size_t count = accessor.count;
            mesh.positions.resize(count);
            const float* dataPtr = reinterpret_cast<const float*>(
                buffer.data.data() + bufferView.byteOffset + accessor.byteOffset);
            for (size_t j = 0; j < count; ++j) {
                mesh.positions[j] = vec3(dataPtr[j * 3 + 0], dataPtr[j * 3 + 1], dataPtr[j * 3 + 2]);
            }
        }

        // Normals
        if (prim.attributes.find("NORMAL") != prim.attributes.end()) {
            int accessorIndex = prim.attributes.at("NORMAL");
            const auto& accessor = gltfModel.accessors[accessorIndex];
            const auto& bufferView = gltfModel.bufferViews[accessor.bufferView];
            const auto& buffer = gltfModel.buffers[bufferView.buffer];

            size_t count = accessor.count;
            mesh.normals.resize(count);
            const float* dataPtr = reinterpret_cast<const float*>(
                buffer.data.data() + bufferView.byteOffset + accessor.byteOffset);
            for (size_t j = 0; j < count; ++j) {
                mesh.normals[j] = vec3(dataPtr[j * 3 + 0], dataPtr[j * 3 + 1], dataPtr[j * 3 + 2]);
            }
        }

        // Tangents
        if (prim.attributes.find("TANGENT") != prim.attributes.end()) {
            int accessorIndex = prim.attributes.at("TANGENT");
            const auto& accessor = gltfModel.accessors[accessorIndex];
            const auto& bufferView = gltfModel.bufferViews[accessor.bufferView];
            const auto& buffer = gltfModel.buffers[bufferView.buffer];
            size_t count = accessor.count;
            mesh.tangents.resize(count);
            const float* dataPtr = reinterpret_cast<const float*>(
                buffer.data.data() + bufferView.byteOffset + accessor.byteOffset);
            for (size_t j = 0; j < count; ++j) {
                mesh.tangents[j] = vec4(dataPtr[j * 4 + 0], dataPtr[j * 4 + 1], dataPtr[j * 4 + 2], dataPtr[j * 4 + 3]);
            }
        }
        // UVs
        if (prim.attributes.find("TEXCOORD_0") != prim.attributes.end()) {
            int accessorIndex = prim.attributes.at("TEXCOORD_0");
            const auto& accessor = gltfModel.accessors[accessorIndex];
            const auto& bufferView = gltfModel.bufferViews[accessor.bufferView];
            const auto& buffer = gltfModel.buffers[bufferView.buffer];

            size_t count = accessor.count;
            mesh.uvs.resize(count);
            const float* dataPtr = reinterpret_cast<const float*>(
                buffer.data.data() + bufferView.byteOffset + accessor.byteOffset);
            for (size_t j = 0; j < count; ++j) {
                mesh.uvs[j] = vec2(dataPtr[j * 2 + 0], dataPtr[j * 2 + 1]);
            }
        }
        // Indices
        if (prim.indices >= 0) {
            int accessorIndex = prim.indices;
            const auto& accessor = gltfModel.accessors[accessorIndex];
            const auto& bufferView = gltfModel.bufferViews[accessor.bufferView];
            const auto& buffer = gltfModel.buffers[bufferView.buffer];

            size_t count = accessor.count;
            mesh.indices.resize(count);
            if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
                const uint16_t* dataPtr = reinterpret_cast<const uint16_t*>(
                    buffer.data.data() + bufferView.byteOffset + accessor.byteOffset);
                for (size_t j = 0; j < count; ++j) {
                    mesh.indices[j] = static_cast<uint32_t>(dataPtr[j]);
                }
            } else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
                const uint32_t* dataPtr = reinterpret_cast<const uint32_t*>(
                    buffer.data.data() + bufferView.byteOffset + accessor.byteOffset);
                for (size_t j = 0; j < count; ++j) {
                    mesh.indices[j] = dataPtr[j];
                }
            } else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
                const uint8_t* dataPtr = reinterpret_cast<const uint8_t*>(
                    buffer.data.data() + bufferView.byteOffset + accessor.byteOffset);
                for (size_t j = 0; j < count; ++j) {
                    mesh.indices[j] = static_cast<uint32_t>(dataPtr[j]);
                }
            } else {
                std::cerr << "Unsupported index component type in GLTF." << std::endl;
            }
        }
        // Material index
        mesh.materialIndex = prim.material;

        // Create GPU buffers for this mesh
        glGenVertexArrays(1, &mesh.vao);
        glBindVertexArray(mesh.vao);

        if (!mesh.positions.empty()) {
            glGenBuffers(1, &mesh.vboPos);
            glBindBuffer(GL_ARRAY_BUFFER, mesh.vboPos);
            glBufferData(GL_ARRAY_BUFFER, mesh.positions.size() * sizeof(vec3), mesh.positions.data(), GL_STATIC_DRAW);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), (void*)0);
        }
        if (!mesh.normals.empty()) {
            glGenBuffers(1, &mesh.vboNorm);
            glBindBuffer(GL_ARRAY_BUFFER, mesh.vboNorm);
            glBufferData(GL_ARRAY_BUFFER, mesh.normals.size() * sizeof(vec3), mesh.normals.data(), GL_STATIC_DRAW);
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), (void*)0);
        }
        if (!mesh.tangents.empty()) {
            glGenBuffers(1, &mesh.vboTan);
            glBindBuffer(GL_ARRAY_BUFFER, mesh.vboTan);
            glBufferData(GL_ARRAY_BUFFER, mesh.tangents.size() * sizeof(vec4), mesh.tangents.data(), GL_STATIC_DRAW);
            glEnableVertexAttribArray(2);
            glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(vec4), (void*)0);
        }
        if (!mesh.uvs.empty()) {
            glGenBuffers(1, &mesh.vboUV);
            glBindBuffer(GL_ARRAY_BUFFER, mesh.vboUV);
            glBufferData(GL_ARRAY_BUFFER, mesh.uvs.size() * sizeof(vec2), mesh.uvs.data(), GL_STATIC_DRAW);
            glEnableVertexAttribArray(3);
            glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(vec2), (void*)0);
        }
        if (!mesh.indices.empty()) {
            glGenBuffers(1, &mesh.ebo);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.indices.size() * sizeof(uint32_t), mesh.indices.data(), GL_STATIC_DRAW);
            mesh.indexCount = static_cast<GLsizei>(mesh.indices.size());
        }
        glBindVertexArray(0);
    }
    return true;
}

void Model::draw(Shader& shader) {
    for (const auto& mesh : meshes) {
        // Bind material
        if (mesh.materialIndex < materials.size()) {
            const auto& mat = materials[mesh.materialIndex];
            BIND_MATERIAL_PARAM(mat, albedo, Albedo, 1, shader, PBRE::vec3);
            BIND_MATERIAL_PARAM(mat, metallic, Metallic, 2, shader, float);
            BIND_MATERIAL_PARAM(mat, roughness, Roughness, 3, shader, float);
            if (mat.normal) {
                shader.set("u_HasNormalMap", 1);
                shader.set("u_NormalMap", 4);
                mat.normal->bind(4);
            } else {
                shader.set("u_HasNormalMap", 0);
            }
            BIND_MATERIAL_PARAM(mat, ao, AOMap, 5, shader, float);
            BIND_MATERIAL_PARAM(mat, emissive, Emissive, 6, shader, PBRE::vec3);
            shader.set("u_AlphaCutoff", mat.alphaCutoff);
            shader.set("u_DoubleSided", mat.doubleSided);
        }

        // Draw mesh using VAO
        glBindVertexArray(mesh.vao);
        glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
}