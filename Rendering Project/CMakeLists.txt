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

# Add the build directory to includes
target_include_directories(RenderingProject PRIVATE
    "${CMAKE_CURRENT_BINARY_DIR}"
)

# --- Configuration for Dependencies ---

if(WIN32)
    # Windows-specific configuration
    target_include_directories(RenderingProject PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/3dparty/GLEW/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/3dparty/glad/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/3dparty/glfw-3.4.bin.WIN64/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/3dparty/assimp/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/3dparty/glm"
    )
    
    target_link_directories(RenderingProject PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/3dparty/GLEW/lib/Release/x64"
        "${CMAKE_CURRENT_SOURCE_DIR}/3dparty/glfw-3.4.bin.WIN64/lib-vc2022"
        "${CMAKE_CURRENT_SOURCE_DIR}/3dparty/assimp/lib/Debug"
    )
    
    target_link_libraries(RenderingProject PRIVATE
        assimp-vc143-mtd
        glfw3
    )
    
    target_compile_definitions(RenderingProject PRIVATE _CONSOLE)
    
    # Post-build DLL copy
    add_custom_command(
        TARGET RenderingProject POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/3dparty/assimp/bin/Debug/assimp-vc143-mtd.dll"
            "$<TARGET_FILE_DIR:RenderingProject>"
        COMMENT "Copying assimp DLL to output directory"
    )
    
else()
    # Linux-specific configuration
    
    # Find required packages using system package manager or local builds
    find_package(PkgConfig REQUIRED)
    
    # Try to find GLFW3
    find_package(glfw3 QUIET)
    if(NOT glfw3_FOUND)
        pkg_check_modules(GLFW3 REQUIRED glfw3)
        target_include_directories(RenderingProject PRIVATE ${GLFW3_INCLUDE_DIRS})
        target_link_libraries(RenderingProject PRIVATE ${GLFW3_LIBRARIES})
    else()
        target_link_libraries(RenderingProject PRIVATE glfw)
    endif()
    
    # Find OpenGL
    find_package(OpenGL REQUIRED)
    target_link_libraries(RenderingProject PRIVATE OpenGL::GL)
    
    # Find GLEW
    find_package(GLEW QUIET)
    if(GLEW_FOUND)
        target_link_libraries(RenderingProject PRIVATE GLEW::GLEW)
    else()
        pkg_check_modules(GLEW REQUIRED glew)
        target_include_directories(RenderingProject PRIVATE ${GLEW_INCLUDE_DIRS})
        target_link_libraries(RenderingProject PRIVATE ${GLEW_LIBRARIES})
    endif()
    
    # Find Assimp
    find_package(assimp REQUIRED)
    target_link_libraries(RenderingProject PRIVATE assimp)
    
    # GLM (header-only, might be in system or 3dparty)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/3dparty/glm")
        target_include_directories(RenderingProject PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/3dparty/glm"
        )
    else()
        find_package(glm REQUIRED)
    endif()
    
    # GLAD (if using local copy)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/3dparty/glad/include")
        target_include_directories(RenderingProject PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/3dparty/glad/include"
        )
    endif()
    
    # Link additional Linux libraries
    target_link_libraries(RenderingProject PRIVATE
        ${CMAKE_DL_LIBS}  # For dynamic linking
        pthread           # For threading
    )
    
endif()