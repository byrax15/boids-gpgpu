cmake_minimum_required(VERSION 3.28)
project(boids-gpgpu)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
# Automatically download cmake-spirv with FetchContent
FetchContent_Declare(cmake-spirv
        GIT_REPOSITORY https://github.com/liliolett/cmake-spirv.git
        GIT_TAG        origin/v1
)
FetchContent_MakeAvailable(cmake-spirv)
# Add cmake-spirv to module path
list(APPEND CMAKE_MODULE_PATH ${cmake-spirv_SOURCE_DIR}/include)
# Include required modules
include(AddSpirvModules)


find_package(glbinding  CONFIG REQUIRED)
find_package(glm        CONFIG REQUIRED)
find_package(glfw3      CONFIG REQUIRED)
find_package(imgui      CONFIG REQUIRED)
find_package(imguizmo   CONFIG REQUIRED)
find_package(implot     CONFIG REQUIRED)

add_spirv_modules(boids-shaders
        SOURCE_DIR  shaders
        BINARY_DIR  shaders
        SOURCES     shader.vert
                    debug_wall_corners.vert
                    debug_velocities.vert
        
                    shader.frag 
                    debug_color.frag
        
                    move.comp 
                    interactions.comp
        OPTIONS     "--target-env=opengl"
)

add_executable(boids-gpgpu main.cpp)
target_sources(boids-gpgpu PRIVATE
        FILE_SET all_modules TYPE CXX_MODULES 
        FILES 
        "contexts.ixx"
        "shader_program.ixx"
        "vertex_buffer.ixx"
        "vertex_array.ixx"
   "physics_buffer.ixx")
set_target_properties(boids-gpgpu PROPERTIES COMPILE_WARNING_AS_ERROR ON)
add_dependencies(boids-gpgpu boids-shaders)
target_link_libraries(boids-gpgpu PRIVATE
        glbinding::glbinding glbinding::glbinding-aux
        glm::glm
        glfw
        imgui::imgui
        imguizmo::imguizmo
        implot::implot
)