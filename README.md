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

### Using CMake (Recommended)
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

#### Rendering Pipeline
1. **TerrainRenderer**: Main rendering coordinator
2. **Camera**: First-person camera with Euler angle rotation
3. **Shader**: OpenGL shader program management
4. **Mesh**: Vertex data and rendering commands
5. **PostProcessor**: HDR and effect processing pipeline

#### Lighting System  
1. **LightingConfig**: Dynamic lighting parameter management
2. **HDRLoader**: IBL texture loading and cube map generation
3. **Atmospheric Effects**: Volumetric fog and light scattering

#### Terrain Generation
1. **TerrainGenerator**: Procedural height field generation
2. **NoiseGenerator**: Multi-octave Perlin noise implementation
3. **Material Blending**: Height and slope-based material distribution

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
│       ├── GUIManager.h/.cpp       # Debug UI (if enabled)
│       └── ControlPanel.h/.cpp     # Runtime controls
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