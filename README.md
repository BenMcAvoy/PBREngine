# PBREngine

A modern, physically-based rendering (PBR) engine built with OpenGL 4.6 and C++20. This engine implements industry-standard PBR workflows with image-based lighting (IBL), HDR environment maps, and glTF model loading.

## Features

### Core Rendering
- **Physically Based Rendering (PBR)** - Cook-Torrance BRDF with full metallic-roughness workflow
- **Image-Based Lighting (IBL)** - Environment map-based diffuse and specular lighting
- **HDR Pipeline** - High Dynamic Range rendering with tonemapping (ACES)
- **Normal Mapping** - Tangent-space normal mapping support
- **Texture Support** - Full PBR texture maps (albedo, metallic, roughness, normal, AO, emissive)

### Lighting
- **Direct Lighting** - Point light with configurable color and intensity
- **IBL (Image-Based Lighting)** - Environment map-based ambient lighting with roughness-based reflections
- **Horizon Fade** - Configurable horizon fading for IBL
- **Light Visualization** - Visual indicators for light sources

### Models & Assets
- **glTF 2.0 Support** - Load 3D models with materials and textures via TinyGLTF
- **Procedural Geometry** - Python scripts to generate sphere and cube meshes
- **Multiple Models** - Support for loading and rendering multiple models simultaneously

### Camera & Controls
- **Free-Look Camera** - First-person camera with mouse look
- **WASD Movement** - Standard FPS-style camera movement
- **Configurable Parameters** - Adjustable FOV, near/far planes, and aspect ratio

### Debug & Development
- **ImGui Integration** - Real-time parameter tweaking and debugging
- **Debug Visualization Modes**:
  - NdotL (Normal dot Light) visualization
  - NdotV (Normal dot View) visualization
  - Direct lighting only
  - IBL only
- **Live Material Editing** - Adjust PBR parameters in real-time
- **Exposure Control** - HDR exposure adjustment

## Prerequisites

- **C++20 compatible compiler** (GCC 10+, Clang 11+, MSVC 2019+)
- **xmake** - Build system
- **Python 3** - For geometry generation scripts
- **OpenGL 4.6** capable GPU

## Dependencies

All dependencies are managed automatically by xmake:

- **GLFW** - Window and input management
- **GLAD** - OpenGL 4.6 loader
- **GLM** - Mathematics library
- **ImGui v1.92.1-docking** - Immediate mode GUI (with FreeType support)
- **spdlog** - Logging library
- **stb** - Image loading (stb_image)
- **TinyGLTF** - glTF 2.0 model loading

## Building

### Install xmake

Follow the instructions at [xmake.io](https://xmake.io/#/getting_started?id=installation) to install xmake for your platform.

### Build the project

```bash
# Clone the repository
git clone https://github.com/BenMcAvoy/PBREngine.git
cd PBREngine

# Generate geometry data (sphere by default)
python3 generate_geometry.py sphere

# Configure and build
xmake config -m release
xmake build

# Run the engine
xmake run
```

### Build Modes

```bash
# Debug build
xmake config -m debug
xmake build

# Release build
xmake config -m release
xmake build
```

## Usage

### Generating Geometry

The engine uses procedurally generated geometry stored in `src/data.h`. Generate different geometries:

```bash
# Generate sphere geometry
python3 generate_geometry.py sphere

# Generate cube geometry
python3 generate_geometry.py cube
```

After generating new geometry, rebuild the project.

### Running the Engine

```bash
xmake run
```

### Controls

- **ESC** - Toggle mouse lock (camera free-look mode)
- **W/A/S/D** - Camera movement (when mouse locked)
- **Mouse** - Look around (when mouse locked)

### UI Controls

When the mouse is unlocked, the ImGui interface provides controls for:

#### Light Settings
- Position (X, Y, Z sliders)
- Color (RGB picker)
- Intensity

#### Material Properties
- Albedo color
- Metallic value (0.0 - 1.0)
- Roughness value (0.0 - 1.0)
- Ambient Occlusion (AO)

#### Debug Options
- Enable/Disable IBL
- Enable/Disable Direct Lighting
- IBL Intensity
- Horizon Fade Power
- Debug Mode selection:
  - 0: Normal rendering
  - 1: NdotL visualization
  - 2: NdotV visualization
  - 3: Direct lighting only
  - 4: IBL only

#### Post-Processing
- Exposure control (HDR tonemapping)

## Project Structure

```
PBREngine/
├── src/
│   ├── main.cpp              # Application entry point and render loop
│   ├── data.h                # Generated geometry data (sphere/cube)
│   └── pbre/
│       ├── base.hpp          # Core types and utilities
│       ├── render/
│       │   ├── camera.hpp/cpp    # Camera implementation
│       │   └── material.hpp      # PBR material definition
│       └── wrapper/
│           ├── window.hpp/cpp        # GLFW window wrapper
│           ├── shader.hpp/cpp        # Shader compilation and management
│           ├── texture.hpp/cpp       # Texture loading and cubemap utilities
│           ├── buffers.hpp/cpp       # VBO/VAO/EBO management
│           ├── framebuffer.hpp/cpp   # FBO management
│           └── model.hpp/cpp         # glTF model loading
├── shaders/
│   ├── vert.glsl             # Main vertex shader
│   ├── frag.glsl             # PBR fragment shader
│   ├── tonemap_vert.glsl     # Tonemapping vertex shader
│   ├── tonemap_frag.glsl     # Tonemapping fragment shader
│   ├── light_vert.glsl       # Light indicator vertex shader
│   └── light_frag.glsl       # Light indicator fragment shader
├── resources/
│   ├── *.hdr                 # HDR environment maps
│   └── */                    # glTF model directories (lion_head, table, vintage_camera)
├── gen_sphere.py             # Sphere geometry generator
├── gen_cube.py               # Cube geometry generator
├── generate_geometry.py      # Geometry generation wrapper
└── xmake.lua                 # Build configuration
```

## Technical Details

### PBR Implementation

The engine implements a full Cook-Torrance BRDF with:
- **Fresnel** - Schlick approximation
- **Normal Distribution Function (NDF)** - GGX/Trowbridge-Reitz
- **Geometry Function** - Smith's method with Schlick-GGX

### IBL Implementation

Image-Based Lighting uses:
- Equirectangular to cubemap conversion
- Pre-filtered environment maps with mipmap chain
- Split-sum approximation for real-time performance
- Analytical BRDF integration (envBRDFApprox)

### HDR Pipeline

1. Scene rendered to HDR framebuffer (RGBA16F)
2. Tonemapping applied as post-process (ACES)
3. Result displayed to screen

### Material System

Materials support both constant values and texture maps:
```cpp
Material {
    albedo:    vec3 or Texture
    metallic:  float or Texture
    roughness: float or Texture
    normal:    Texture (optional)
    ao:        float or Texture
    emissive:  vec3 or Texture
}
```

## Known Limitations

- Single point light support (no multiple lights yet)
- No shadow mapping
- No skeletal animation
- Limited to glTF 2.0 format for models

## Future Enhancements

- Multiple light sources
- Shadow mapping (PCF/VSM)
- Screen-space ambient occlusion (SSAO)
- Bloom post-processing
- glTF animation support
- Deferred rendering pipeline
- Area lights

## Resources

The `resources/` directory contains:
- HDR environment maps from [Poly Haven](https://polyhaven.com/)
- Sample glTF models for demonstration

## License

This project is provided as-is for educational and personal use.

## Credits

- **Author**: BenMcAvoy
- **Environment Maps**: Poly Haven (CC0)
- **Libraries**: See Dependencies section above

## Contributing

Contributions are welcome! Please feel free to submit issues or pull requests.

---

Built with ❤️ using modern C++20 and OpenGL 4.6
