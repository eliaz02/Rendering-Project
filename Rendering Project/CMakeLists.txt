# Set the minimum required version of CMake.
cmake_minimum_required(VERSION 3.16)

# Define the project name, version, and languages.
# We need C and CXX because you have gl.c and C++ files.
project(RenderingProject LANGUAGES CXX C)

# This generates PathConfig.h in the build directory.
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/PathConfig.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/PathConfig.h"
)

# Set the C++ standard to C++20, as specified in your project file.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Gather all your source files into a variable for clarity.
set(SOURCES
    frameBufferObject.cpp
    gl.c
    main.cpp
    Mesh.cpp
    Utilities.cpp
    WindowContext.cpp
)

# It's good practice to list headers too, for better IDE integration.
set(HEADERS
    Animation.h
    Component.h
    exameScene.h
    WindowContext.h
    Camera.h
    Debugging.h
    DemoScene.h
    EntityComponentSysetm.h
    frameBufferObject.h
    LightStruct.h
    Mesh.h
    Shader.h
    Skybox.h
    stb_image.h
    Texture.h
    Utilities.h
)

# Create the executable from your source and header files.
add_executable(RenderingProject ${SOURCES} ${HEADERS})

# Add the build directory to includes ---
target_include_directories(RenderingProject PRIVATE
    "${CMAKE_CURRENT_BINARY_DIR}"
)
# --- Configuration for Dependencies (3dparty) ---

# Add the include directories. CMake uses forward slashes.
# These paths are relative to this CMakeLists.txt file.
target_include_directories(RenderingProject PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/3dparty/GLEW/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/3dparty/glad/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/3dparty/glfw-3.4.bin.WIN64/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/3dparty/assimp/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/3dparty/glm"
)

# Add the library directories.
# Note: The original project links the Debug assimp lib even in Release. We will replicate this first.
target_link_directories(RenderingProject PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/3dparty/GLEW/lib/Release/x64"
    "${CMAKE_CURRENT_SOURCE_DIR}/3dparty/glfw-3.4.bin.WIN64/lib-vc2022"
    "${CMAKE_CURRENT_SOURCE_DIR}/3dparty/assimp/lib/Debug"
)

# Link the libraries. CMake is smart enough to find the .lib files in the link directories.
target_link_libraries(RenderingProject PRIVATE
    assimp-vc143-mtd
    glfw3
)

# Add preprocessor definitions.
# Note: CMake automatically adds _DEBUG for Debug builds and NDEBUG for Release builds.
# We only need to add definitions common to both, like _CONSOLE.
target_compile_definitions(RenderingProject PRIVATE _CONSOLE)

# --- Post-Build Step ---

# This command copies the required DLL to the output directory after the build is complete.
# The $<TARGET_FILE_DIR:RenderingProject> is a "generator expression" that correctly
# resolves to the output folder (e.g., build/Debug) for any configuration.
add_custom_command(
    TARGET RenderingProject POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/3dparty/assimp/bin/Debug/assimp-vc143-mtd.dll"
        "$<TARGET_FILE_DIR:RenderingProject>"
    COMMENT "Copying assimp DLL to output directory"
)