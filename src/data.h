#pragma once

#include <array>

constexpr std::array<float, 8 * 3> cubeVertices = {
    -1.0f, -1.0f, -1.0f, // 0: left-bottom-back
    1.0f, -1.0f, -1.0f,  // 1: right-bottom-back
    1.0f, 1.0f, -1.0f,   // 2: right-top-back
    -1.0f, 1.0f, -1.0f,  // 3: left-top-back
    -1.0f, -1.0f, 1.0f,  // 4: left-bottom-front
    1.0f, -1.0f, 1.0f,   // 5: right-bottom-front
    1.0f, 1.0f, 1.0f,    // 6: right-top-front
    -1.0f, 1.0f, 1.0f    // 7: left-top-front
};

constexpr std::array<unsigned int, 36> cubeIndices = {
    // back face
    0, 1, 2,
    2, 3, 0,
    // front face
    4, 5, 6,
    6, 7, 4,
    // left face
    0, 3, 7,
    7, 4, 0,
    // right face
    1, 5, 6,
    6, 2, 1,
    // bottom face
    0, 1, 5,
    5, 4, 0,
    // top face
    3, 2, 6,
    6, 7, 3};
