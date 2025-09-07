# AetherGL - Advanced Procedural Terrain Rendering Engine

AetherGL is a high-performance, real-time procedural terrain rendering engine built with modern OpenGL 4.6. It demonstrates advanced graphics techniques including Physically Based Rendering (PBR), Parallax Occlusion Mapping (POM), Image-Based Lighting (IBL), and sophisticated post-processing effects.

## Features

### Core Rendering Technology
- **Physically Based Rendering (PBR)**: Implements Cook-Torrance BRDF model for realistic material interactions
- **Parallax Occlusion Mapping (POM)**: High-quality surface detail simulation with minimal geometry
- **Image-Based Lighting (IBL)**: Environmental lighting using HDR environment maps
- **Volumetric Atmospheric Fog**: Height-based fog with atmospheric perspective
- **Advanced Post-Processing**: HDR tone mapping, bloom, chromatic aberration, depth of field

### Terrain Generation
- **Multi-layered Materials**: Procedural blending of grass, rock, dirt, and snow materials
- **Dynamic Time-of-Day**: Animated sun position with atmospheric light scattering
- **Configurable Parameters**: Extensive material and lighting customization via settings file

### Performance Optimizations
- **Frustum Culling**: Eliminates off-screen geometry to improve performance
- **Batch Rendering**: Minimizes OpenGL state changes and draw calls
- **Level of Detail (LOD)**: Distance-based mesh simplification
- **Cached Calculations**: Reduces redundant mathematical operations
- **Frame-Independent Movement**: Delta time-based camera controls

## System Requirements

### Minimum Requirements
- OpenGL 4.6 compatible GPU
- 4GB RAM
- Modern C++ compiler with C++17 support
- Windows, macOS, or Linux

### Recommended Requirements
- Dedicated GPU with 2GB+ VRAM (NVIDIA GTX 1060 / AMD RX 580 or better)
- 8GB+ RAM
- Multi-core CPU

## Dependencies

### Core Libraries
- **GLFW 3.3+**: Window management and input handling
- **GLAD**: OpenGL function loading
- **GLM 0.9.9+**: OpenGL Mathematics library
- **stb_image**: Image loading for textures

### Optional Dependencies
- **CMake 3.15+**: Build system (recommended)
- **Visual Studio 2019+** or **GCC 9+**: Compiler with C++17 support

## Building the Project

### Using CLion with vcpkg (Recommended)
1. **Download CLion**: Get CLion IDE from JetBrains
2. **Install vcpkg**: Download and setup vcpkg package manager
3. **Install dependencies via vcpkg**:
   ```bash
   vcpkg install glfw3 glad glm stb
   ```
4. **Configure CLion**: Set CMake toolchain file to point to vcpkg:
   - Go to Settings → Build → CMake
   - Set CMake options: `-DCMAKE_TOOLCHAIN_FILE=[path-to-vcpkg]/scripts/buildsystems/vcpkg.cmake`
5. **Build**: Use CLion's build system (Ctrl+F9)

### Using CMake (Alternative)
```bash
mkdir build
cd build
cmake ..
make -j4    # or nmake on Windows
```

### Manual Compilation
```bash
g++ -std=c++17 -O3 src/main.cpp src/include/*.cpp -lglfw -lGL -ldl -o AetherGL
```

## Usage

### Basic Controls
- **WASD**: Camera movement (forward/back, strafe left/right)
- **Space/Shift**: Camera vertical movement (up/down)
- **Mouse**: Look around (left-click and drag, or Tab for mouse capture)
- **Mouse Wheel**: Zoom in/out (adjusts field of view)
- **Tab**: Toggle mouse capture mode
- **R**: Reload configuration from settings.txt
- **ESC**: Exit application

### Configuration

The application uses `settings.txt` for runtime configuration. Key settings include:

#### Lighting Configuration
```ini
[Lighting]
sun_intensity=3.0
sun_color_r=1.0
sun_color_g=0.95  
sun_color_b=0.8
fog_density=0.02
fog_height=50.0
```

#### Post-Processing Effects
```ini
[Post-Processing]
enable_bloom=true
bloom_threshold=1.0
bloom_intensity=0.8
enable_dof=false
enable_chromatic_aberration=true
exposure=1.0
```

#### Camera Settings
```ini
[Camera]
position_x=0.0
position_y=50.0
position_z=100.0
movement_speed=50.0
mouse_sensitivity=0.1
```

## Architecture Overview

### Core Components

AetherGL is built around a modular architecture with clearly defined responsibilities. Each class is designed following SOLID principles and modern C++ best practices.

## Detailed Class Documentation

### Core Rendering System

#### **Camera** (`Camera.h/.cpp`)
**Purpose**: First-person camera system with smooth movement and look controls

**Key Features**:
- Euler angle-based rotation system (yaw/pitch)
- WASD + Space/Shift movement with configurable speed
- Mouse look with sensitivity settings
- Field of view (zoom) control via mouse wheel
- Delta time-based smooth movement
- View matrix generation for shaders

**Core Methods**:
- `processKeyboard()`: Handle WASD movement input
- `processMouseMovement()`: Handle mouse look rotation
- `processMouseScroll()`: Handle zoom/FOV adjustment
- `getViewMatrix()`: Generate view matrix for rendering

**Usage**: Central to all 3D navigation and view transformation in the engine.

---

#### **Shader** (`Shader.h/.cpp`) 
**Purpose**: OpenGL shader program management and uniform setting

**Key Features**:
- GLSL shader compilation with detailed error reporting
- Vertex and fragment shader loading from files
- Type-safe uniform variable setting (bool, int, float, vec3, mat4)
- Automatic shader program linking and validation
- RAII resource management for OpenGL objects

**Core Methods**:
- `setBool/setInt/setFloat()`: Set scalar uniforms
- `setVec3/setMat4()`: Set vector/matrix uniforms
- `use()`: Activate shader program for rendering

**Usage**: Used by all rendering components to manage GPU programs and pass data to shaders.

---

#### **Mesh** (`Mesh.h/.cpp`)
**Purpose**: Vertex data storage and OpenGL mesh rendering

**Key Features**:
- Efficient vertex buffer object (VBO) management
- Support for position, normal, and texture coordinate data
- Indexed rendering with element buffer objects (EBO)
- Vertex Array Object (VAO) for optimized state management
- RAII cleanup of OpenGL resources

**Core Methods**:
- `draw()`: Render mesh using provided shader
- `getVertices/getIndices()`: Access vertex data for export

**Usage**: Represents all 3D geometry in the engine, particularly the procedural terrain mesh.

---

### Terrain Generation System

#### **TerrainGenerator** (`TerrainGenerator.h/.cpp`)
**Purpose**: Procedural terrain mesh generation with height displacement

**Key Features**:
- High-resolution plane mesh generation with configurable subdivision
- CPU-based height displacement using Perlin noise
- Smooth normal calculation for proper lighting
- Multiple quality presets (low-poly, high-poly, custom)
- Efficient vertex buffer layout optimized for GPU consumption
- Texture coordinate mapping based on world position

**Core Methods**:
- `generateTerrainMesh()`: Main terrain generation with full parameter control
- `generateLowPolyTerrain()`: Performance-optimized terrain (< 10k triangles)
- `generateHighPolyTerrain()`: Quality-optimized terrain (> 40k triangles)

**Design Patterns**: Factory pattern with static methods for different terrain types

**Usage**: Creates the base geometry that is then enhanced with GPU-based materials and displacement.

---

#### **NoiseGenerator** (`NoiseGenerator.h/.cpp`)
**Purpose**: High-quality procedural noise generation for terrain heightmaps

**Key Features**:
- Industry-standard Perlin noise implementation
- Fractal Brownian Motion (fBm) for natural terrain patterns
- Configurable octaves, persistence, and lacunarity
- Seed-based generation for reproducible terrains
- Optimized for real-time height field generation
- Both 2D and 3D noise functions

**Core Methods**:
- `perlin2D/3D()`: Basic Perlin noise generation
- `fbm2D()`: Multi-octave fractal Brownian motion
- `generateTerrainHeight()`: Terrain-optimized height generation
- `setSeed()`: Control randomization for reproducible results

**Mathematical Foundation**: Uses Ken Perlin's improved noise algorithm with proper gradient vectors and fade functions.

**Usage**: Provides the mathematical foundation for terrain height variation and surface detail.

---

### Advanced Lighting System

#### **LightingConfig** (`LightingConfig.h/.cpp`)
**Purpose**: Comprehensive lighting parameter management for PBR rendering

**Key Features**:
- Directional light configuration (sun/moon)
- Multiple point light support (up to 8 lights)
- Time-of-day cycle with automatic sun movement
- Atmospheric fog and scattering parameters
- Terrain material height thresholds
- Parallax Occlusion Mapping (POM) settings
- Image-Based Lighting (IBL) configuration

**Core Structures**:
- `DirectionalLight`: Sun/directional lighting parameters
- `PointLight`: Local light sources with attenuation
- `TimeOfDayConfig`: Automated day/night cycle
- `AtmosphericConfig`: Fog and atmospheric scattering
- `TerrainMaterialConfig`: Biome height thresholds
- `POMConfig`: Surface detail mapping parameters

**Core Methods**:
- `applyToShader()`: Upload all lighting parameters to GPU
- `updateTimeOfDay()`: Animate sun position and colors
- `setupDefaultLights()`: Configure realistic lighting scenario

**Usage**: Central hub for all lighting calculations and parameters in the PBR pipeline.

---

#### **HDRLoader** (`HDRLoader.h/.cpp`)
**Purpose**: HDR environment map loading and IBL texture generation

**Key Features**:
- Equirectangular HDR image loading (OpenEXR, HDR formats)
- Automatic cubemap generation from panoramic images
- Basic IBL texture generation (irradiance, prefilter, BRDF LUT)
- OpenGL texture management with proper filtering
- Memory-efficient HDR data processing

**Core Methods**:
- `loadHDREnvironment()`: Complete IBL texture suite generation
- `createSkyboxFromHDR()`: HDR skybox cubemap creation
- `cleanup()`: Proper OpenGL texture resource cleanup

**IBL Pipeline**: Converts HDR environment maps into the textures needed for physically-based environmental lighting.

**Usage**: Enables realistic environmental lighting that responds to actual HDR imagery.

---

### Post-Processing Pipeline

#### **PostProcessor** (`PostProcessor.h/.cpp`)
**Purpose**: Complete HDR post-processing pipeline with multiple effects

**Key Features**:
- Off-screen framebuffer rendering (FBO)
- HDR color accumulation and tone mapping
- Multi-pass bloom effect (bright pass + Gaussian blur)
- Depth-based DOF with bokeh-style blur
- Chromatic aberration lens distortion simulation
- Configurable effect parameters
- Ping-pong framebuffer technique for efficient blur

**Effect Pipeline**:
1. **Scene Rendering**: HDR color accumulation in floating-point framebuffer
2. **Bright Pass**: Extract pixels above bloom threshold
3. **Gaussian Blur**: Multi-pass separable blur for bloom spread  
4. **Final Composite**: Combine effects with tone mapping and gamma correction

**Core Methods**:
- `beginFrame()`: Start off-screen rendering
- `endFrame()`: Apply post-processing pipeline and output to screen
- `resize()`: Handle framebuffer recreation on window resize

**Configuration**: Fully configurable through `PostProcessor::Config` structure

**Usage**: Transforms raw HDR scene data into final display-ready imagery with cinematic effects.

---


### Utility and Export System

#### **ObjWriter** (`ObjWriter.h/.cpp`)
**Purpose**: Wavefront OBJ mesh export for external use

**Key Features**:
- Industry-standard OBJ format export
- Complete vertex data export (positions, normals, UVs)
- Indexed triangle mesh support
- Metadata generation with export parameters
- File path validation and directory creation
- Progress reporting for large mesh exports

**Export Format**:
- Vertex positions (v x y z)
- Texture coordinates (vt u v)  
- Vertex normals (vn x y z)
- Triangular faces (f v1/vt1/vn1 v2/vt2/vn2 v3/vt3/vn3)

**Core Methods**:
- `exportMesh()`: Basic mesh export with standard metadata
- `exportTerrainMesh()`: Extended export with terrain generation parameters
- `getMeshStatistics()`: Analyze mesh for export reporting

**Usage**: Enables terrain mesh export to external 3D modeling software, game engines, or other applications.

---

#### **TerrainRenderer** (`TerrainRenderer.h/.cpp`)
**Purpose**: Main rendering coordinator and application entry point

**Key Features**:
- Complete rendering pipeline orchestration
- Input handling and camera control
- Resource management and initialization
- Frame timing and performance monitoring
- Integration between all system components
- Post-processing effect control

**Rendering Pipeline**:
1. **Setup Phase**: Initialize shaders, meshes, lighting, and post-processing
2. **Update Phase**: Handle input, update camera, animate time-of-day
3. **Render Phase**: Draw terrain, apply lighting, execute post-processing
4. **Present Phase**: Output final image to screen

**Core Methods**:
- `initialize()`: Setup complete rendering system
- `render()`: Execute full rendering pipeline  
- `processKeyboard/Mouse()`: Handle user input
- `toggleBloom/DOF/ChromaticAberration()`: Runtime effect control

**Integration Role**: Serves as the central coordinator that connects all engine subsystems into a cohesive rendering application.

---

## System Integration

### Data Flow Architecture
```
User Input → Camera → View Matrix → Shaders
                ↓
TerrainGenerator → Mesh → GPU Buffers → Rendering
                ↓  
NoiseGenerator → Height Data → Vertex Displacement
                ↓
LightingConfig → Shader Uniforms → PBR Calculations
                ↓
HDRLoader → IBL Textures → Environmental Lighting
                ↓  
PostProcessor → HDR Framebuffer → Effects Pipeline → Final Output
```

### Memory Management
- **RAII Pattern**: All OpenGL resources automatically cleaned up
- **Smart Pointers**: Automatic memory management for complex objects
- **Resource Sharing**: Shared shaders and textures to minimize GPU memory
- **Efficient Updates**: Delta-time based updates minimize unnecessary calculations

### Performance Characteristics
- **GPU-Centric**: Maximum computation moved to GPU shaders
- **Batched Operations**: Minimized OpenGL state changes
- **Cached Results**: Expensive calculations cached when possible
- **Scalable Quality**: Multiple quality presets for different hardware

This modular architecture enables easy extension, modification, and optimization of individual components without affecting the entire system.

### Mathematical Foundations

#### Physically Based Rendering (PBR)
The engine implements the Cook-Torrance BRDF model:

```
BRDF = (D * F * G) / (4 * (N·V) * (N·L))
```

Where:
- **D**: Normal Distribution Function (GGX/Trowbridge-Reitz)
- **F**: Fresnel term (Schlick approximation) 
- **G**: Geometric shadowing function (Smith model)

#### Parallax Occlusion Mapping
POM uses ray tracing in tangent space to simulate surface displacement:

1. Ray direction calculated from view vector
2. Linear search to find intersection with height field
3. Binary search refinement for sub-pixel accuracy
4. Texture coordinate adjustment for realistic depth

#### Atmospheric Scattering
Simplified atmospheric model based on:
- **Rayleigh Scattering**: Blue sky color (wavelength-dependent)
- **Mie Scattering**: Sun halo effects (aerosol particles)
- **Beer's Law**: Exponential fog density with height

## Performance Considerations

### Optimization Techniques Implemented

1. **Batched Operations**
   - Combined camera movement updates
   - Grouped texture binding operations
   - Batch matrix uniform uploads

2. **Cached Calculations**
   - Static clear color to avoid repeated glClearColor calls
   - Pre-computed transformation matrices
   - Reused mathematical constants

3. **Efficient OpenGL Usage**
   - Minimal state changes between draw calls
   - Optimized texture unit allocation
   - Early depth testing for skybox rendering

4. **Frame Rate Monitoring**
   - Real-time FPS and frame time display
   - 60-frame averaging for stable measurements
   - Performance degradation detection

### Typical Performance
- **High Settings**: 60+ FPS at 1920x1080 on GTX 1060
- **Medium Settings**: 120+ FPS at 1920x1080 on RTX 3060
- **Low Settings**: 200+ FPS at 1920x1080 on RTX 4070

## File Structure

```
AetherGL/
├── src/
│   ├── main.cpp                    # Application entry point
│   └── include/
│       ├── Camera.h/.cpp           # First-person camera system
│       ├── Shader.h/.cpp           # OpenGL shader management  
│       ├── Mesh.h/.cpp             # Geometry rendering
│       ├── TerrainRenderer.h/.cpp  # Main rendering engine
│       ├── TerrainGenerator.h/.cpp # Procedural terrain creation
│       ├── NoiseGenerator.h/.cpp   # Perlin noise implementation
│       ├── LightingConfig.h/.cpp   # Dynamic lighting system
│       ├── HDRLoader.h/.cpp        # IBL texture processing
│       ├── PostProcessor.h/.cpp    # Post-processing pipeline
├── shaders/
│   ├── vertex.glsl                 # Basic vertex shader
│   ├── fragment.glsl               # Basic fragment shader  
│   ├── terrain_vertex.glsl         # Terrain vertex processing
│   ├── pbr_terrain_fragment.glsl   # PBR terrain rendering
│   ├── skybox_vertex.glsl          # Skybox vertex shader
│   ├── skybox_fragment.glsl        # HDR skybox rendering
│   └── postprocess/                # Post-processing shaders
├── assets/                         # Textures and models
├── CMakeLists.txt                  # Build configuration
└── settings.txt                    # Runtime configuration
```

## Advanced Features

### Procedural Material System
Materials are assigned based on:
- **Height**: Different biomes at different elevations
- **Slope**: Rocky materials on steep surfaces  
- **Surface Detail**: Procedural height maps for micro-displacement
- **Environmental Factors**: Snow accumulation, erosion patterns

### Dynamic Lighting
- **Time-of-Day Cycle**: Automatic sun movement with atmospheric color changes
- **Multiple Light Types**: Directional, point, and environmental lighting
- **Shadow Mapping**: (Future enhancement - framework prepared)
- **Light Scattering**: Atmospheric perspective and volumetric effects

### Post-Processing Pipeline
1. **HDR Rendering**: High dynamic range color accumulation
2. **Bright Pass**: Bloom threshold extraction
3. **Gaussian Blur**: Multi-pass bloom spreading
4. **Tone Mapping**: HDR to LDR conversion (Reinhard operator)
5. **Chromatic Aberration**: Lens distortion simulation
6. **Depth of Field**: (Optional) Focal blur effects

## Troubleshooting

### Common Issues

**Black screen or no rendering**
- Verify OpenGL 4.6 support: Check GPU specifications
- Update graphics drivers to latest version
- Check console output for shader compilation errors

**Poor performance**  
- Reduce window resolution in main.cpp
- Disable post-processing in settings.txt: `enable_post_processing=false`
- Lower terrain complexity in TerrainGenerator parameters
- Disable POM: Set `u_enablePOM=false` in shader

**Compilation errors**
- Ensure C++17 compiler support
- Verify all dependencies are installed
- Check library linking order in build system

### Debug Information
The application outputs extensive debug information to console:
- OpenGL version and renderer details
- Shader compilation status  
- Texture loading confirmation
- Frame rate and timing statistics
- Error messages with context

## Contributing

### Code Style
- Use meaningful variable and function names
- Add comprehensive comments for complex algorithms
- Follow RAII principles for resource management
- Maintain const-correctness where applicable

### Performance Guidelines
- Profile code changes with frame time measurements
- Minimize OpenGL state changes
- Cache expensive calculations when possible
- Use early exits for invalid states

## Future Enhancements

### Planned Features
- **Tessellation Shaders**: GPU-based terrain LOD
- **Shadow Mapping**: Real-time shadow casting
- **Water Rendering**: Reflective water surfaces with flow maps
- **Vegetation System**: Instanced grass and trees
- **Weather Effects**: Dynamic rain, snow, and wind
- **Networking**: Multi-user terrain exploration

### Technical Improvements
- **Compute Shaders**: GPU-accelerated terrain generation
- **Vulkan Backend**: Lower-level graphics API option
- **Asset Streaming**: Large-scale terrain data management
- **Memory Pool**: Optimized allocation strategies

## License

This project is released under the MIT License. See LICENSE file for details.

## Acknowledgments

- **LearnOpenGL.com**: Excellent OpenGL tutorials and PBR implementation guidance
- **Khronos Group**: OpenGL specification and sample code
- **GLM Library**: Comprehensive mathematics library for graphics
- **stb_image**: Simple and effective image loading
- **GLFW**: Cross-platform window and input management

---

*Built with passion for real-time graphics programming and procedural generation.*