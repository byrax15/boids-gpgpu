cmake_minimum_required(VERSION 3.28)
project(boids-gpgpu)

set(CMAKE_CXX_STANDARD 20)
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


find_package(glfw3      CONFIG REQUIRED)
find_package(glbinding  CONFIG REQUIRED)
find_package(glm        CONFIG REQUIRED)

add_spirv_modules(boids-shaders
        SOURCE_DIR  shaders
        BINARY_DIR  shaders
        SOURCES     shader.vert shader.frag
        OPTIONS     "--target-env=opengl"
)

add_executable(boids-gpgpu main.cpp )
target_sources(boids-gpgpu PRIVATE
        FILE_SET all_modules TYPE CXX_MODULES 
        FILES 
        contexts.ixx 
        shader_program.ixx
        vertex_buffer.ixx
 "vertex_array.ixx")
target_compile_options(boids-gpgpu PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Werror>
)
add_dependencies(boids-gpgpu boids-shaders)
target_link_libraries(boids-gpgpu PRIVATE
        glfw
        glbinding::glbinding glbinding::glbinding-aux
        glm::glm
)