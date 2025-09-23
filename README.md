# OpenGL Rendering Engine

A modern C++ rendering engine built with OpenGL, featuring deferred rendering, shadow mapping, and a flexible Entity-Component-System architecture.

## Features

### Rendering Pipeline
- **Deferred Rendering**: Multi-pass rendering pipeline with G-buffer for efficient lighting
- **Shadow Mapping**: Support for directional, spot, and point light shadows
- **Post-Processing**: FXAA anti-aliasing
- **Skybox Rendering**: Cubemap-based environment rendering
- **Instanced Rendering**: Efficient rendering of multiple objects

### Lighting System
- **Point Lights**: Omnidirectional lighting with attenuation
- **Spot Lights**: Cone-shaped lighting with cutoff angles
- **Directional Light**: Sun-like lighting for outdoor scenes
- **Real-time Shadows**: Dynamic shadow casting for all light types

### Animation System
- **B-Spline Animation**: Smooth curve-based object animation
- **Flexible Animation Interface**: Extensible animation system for custom implementations
- **Time-based Animation**: Frame-rate independent animation updates

### Asset Support
- **3D Model Loading**: Support for OBJ and glTF formats via Assimp
- **Texture Loading**: Diffuse, specular, normal, and alpha maps
- **Primitive Generation**: Built-in shapes (cube, sphere, plane, B-spline curves)

### Architecture
- **Entity-Component-System**: Flexible, data-oriented design
- **Component-based**: Modular system for transforms, meshes, lights, and animations
- **Scene Management**: Easy scene creation and management

## Dependencies

### Required Libraries
- **OpenGL 4.3+**: Core graphics API
- **GLFW 3.4**: Window management and input handling
- **GLM**: Mathematics library for graphics
- **Assimp**: 3D model loading
- **GLAD**: OpenGL loader
- **STB Image**: Texture loading

### Build System
- **CMake 3.16+**: Cross-platform build system
- **C++20**: Modern C++ standard

## Building

### Prerequisites
1. Install CMake (3.16 or higher)
2. Install a C++20 compatible compiler (Visual Studio 2022, GCC 10+, or Clang 11+)

### Windows (Visual Studio)
```bash
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

### Linux/macOS
```bash
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

## Project Structure

```
Rendering Project/
├── 3dparty/                 # Third-party libraries
│   ├── assimp/             # 3D model loading
│   ├── glad/               # OpenGL loader
│   ├── glfw-3.4.bin.WIN64/ # Window management
│   ├── GLEW/               # OpenGL extension loading
│   └── glm/                # Mathematics library
├── Assets/                  # Game assets
│   ├── skybox/             # Skybox textures
│   ├── stylized_crystal_cluster/
│   └── ...                 # Other 3D models and textures
├── Shaders/                # GLSL shader files
├── Animation.h             # Animation system
├── Camera.h                # First-person camera
├── Component.h             # ECS component base
├── EntityComponentSystem.h # ECS implementation
├── Mesh.h                  # 3D mesh handling
├── Shader.h                # Shader management
├── Texture.h               # Texture loading and management
├── WindowContext.h         # Window and input management
├── frameBufferObject.h     # FBO abstractions
├── exameScene.h           # Example scene implementation
├── main.cpp               # Application entry point
└── CMakeLists.txt         # Build configuration
```

## Usage

### Basic Scene Creation

```cpp
#include "EntityComponentSystem.h"
#include "WindowContext.h"

class MyScene : public Scene {
public:
    MyScene(std::unique_ptr<IRenderer> renderer) 
        : Scene(std::move(renderer)) {}

protected:
    void loadScene() override {
        // Create a simple object
        EntityID entity = createEntity();
        
        // Add transform component
        Transform transform;
        transform.position = glm::vec3(0.0f, 0.0f, -5.0f);
        addComponent(entity, transform);
        
        // Add mesh renderer
        MeshRenderer renderer;
        auto mesh = std::make_shared<BasicMesh>();
        mesh->LoadMesh("path/to/model.obj");
        renderer.mesh = mesh;
        addComponent(entity, renderer);
        
        // Add lighting
        EntityID light = createEntity();
        PointLight pointLight(
            glm::vec3(2.0f, 2.0f, 2.0f), // position
            glm::vec3(0.1f),              // ambient
            glm::vec3(0.8f),              // diffuse  
            glm::vec3(1.0f),              // specular
            1.0f, 25.0f                   // near, far
        );
        addComponent(light, pointLight);
    }
};

int main() {
    WindowContext window(1920, 1080, "My Rendering Engine");
    
    auto renderer = std::make_unique<DeferredRenderer>(window);
    auto scene = std::make_unique<MyScene>(std::move(renderer));
    
    scene->initialize();
    
    while (!window.shouldClose()) {
        window.processInput();
        scene->render();
        window.swapBuffersAndPollEvents();
    }
    
    return 0;
}
```

### Animation Example

```cpp
// Create animated object
EntityID animatedEntity = createEntity();

// Set up B-spline animation path
std::vector<glm::vec3> path = {
    glm::vec3(0.0f, 0.0f, 0.0f),
    glm::vec3(5.0f, 2.0f, 0.0f),
    glm::vec3(10.0f, 0.0f, 5.0f),
    glm::vec3(0.0f, 0.0f, 10.0f)
};

std::vector<float> timing = {2.0f, 2.0f, 2.0f, 2.0f}; // Duration for each segment

Animation animComponent;
animComponent.animation = std::make_unique<BSplineAnimation>(path, timing, true);
addComponent(animatedEntity, std::move(animComponent));
```

## Controls

- **WASD**: Camera movement
- **Mouse**: Look around
- **Ctrl+P**: Increase movement speed
- **Ctrl+R**: Reset movement speed
- **Mouse Wheel**: Zoom in/out

## Technical Details

### Deferred Rendering Pipeline
1. **Geometry Pass**: Renders scene geometry to G-buffer (position, normal, color, depth)
2. **Shadow Pass**: Generates shadow maps for all light types
3. **Lighting Pass**: Combines G-buffer data with lighting calculations
4. **Forward Pass**: Renders transparent objects and skybox
5. **Post-Processing**: Applies FXAA anti-aliasing

### Shadow Mapping
- **Directional Lights**: Standard shadow mapping with orthographic projection
- **Spot Lights**: Shadow mapping with perspective projection stored in texture arrays
- **Point Lights**: Cube shadow mapping for omnidirectional shadows

### Memory Layout
The ECS uses cache-friendly packed arrays for optimal performance:
- Components are stored contiguously in memory
- Fast iteration over component types
- Efficient cache usage during rendering

## License

This project uses assets from Sketchfab with appropriate licensing:
- Some models under CC-BY-4.0 license (attribution required)
- Some models under Sketchfab Standard license
- See individual license.txt files in Assets/ subdirectories

## Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## Troubleshooting

### Common Issues

**Build fails with missing dependencies:**
- Ensure all submodules are initialized: `git submodule update --init --recursive`
- Check that CMake can find all required libraries

**Runtime crashes:**
- Verify OpenGL 4.3+ support on your graphics card
- Check that asset paths are correct in PathConfig.h
- Ensure all required DLLs are in the output directory

**Poor performance:**
- Enable GPU hardware acceleration
- Check that you're using the discrete GPU (not integrated graphics)
- Verify shaders compile successfully (check console output)

## Roadmap

- [ ] PBR (Physically Based Rendering) materials
- [ ] Screen Space Ambient Occlusion (SSAO)
- [ ] Volumetric lighting
- [ ] Particle systems
- [ ] Audio integration
- [ ] ImGui integration for debugging
- [ ] Vulkan backend

---

*This project was created as part of a computer graphics course, demonstrating modern rendering techniques and software architecture principles.*
