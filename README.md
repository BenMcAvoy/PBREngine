# PBREngine

PBREngine is a C++20 OpenGL rendering experiment built around physically based materials. The demo renders a sphere grid and exposes material, light, exposure and image-based-lighting controls through Dear ImGui.

## Build and run

Install [xmake](https://xmake.io), then run:

```sh
xmake
xmake run PBREngine
```

xmake resolves GLFW, GLAD, GLM, Dear ImGui, spdlog, stb and tinygltf from `xmake.lua`.

## Status

This is a rendering study focused on material and lighting behavior, not a complete 3D engine.
