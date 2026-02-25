cmake_minimum_required(VERSION 3.14)
project(3dtiles-renderer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# GLFW
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG 3.3.8
)
FetchContent_MakeAvailable(glfw)

# GLM
FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG 0.9.9.8
)
FetchContent_MakeAvailable(glm)

# nlohmann_json
FetchContent_Declare(
  json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.2
)
FetchContent_MakeAvailable(json)

# tinygltf
FetchContent_Declare(
  tinygltf
  GIT_REPOSITORY https://github.com/syoyo/tinygltf.git
  GIT_TAG v2.8.13
)
FetchContent_MakeAvailable(tinygltf)

# stb
FetchContent_Declare(
  stb
  GIT_REPOSITORY https://github.com/nothings/stb.git
  GIT_TAG master
)
FetchContent_MakeAvailable(stb)

# GLAD (Generated locally)
# Check if glad exists, otherwise generate or warn
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/glad/include/glad/glad.h")
    message(WARNING "glad not found at external/glad. Please run 'python3 -m glad --generator=c --out-path=external/glad' before building or ensure it is generated.")
endif()

add_library(glad STATIC
    external/glad/src/glad.c
)
target_include_directories(glad PUBLIC external/glad/include)

add_executable(3dtiles-renderer
    src/main.cpp
    src/TileLoader.cpp
    src/Renderer.cpp
)

target_link_libraries(3dtiles-renderer PRIVATE
    glfw
    glm::glm
    nlohmann_json::nlohmann_json
    glad
    dl
    pthread
)

# tinygltf is header-only, we need to include its directory
target_include_directories(3dtiles-renderer PRIVATE
    ${tinygltf_SOURCE_DIR}
    ${stb_SOURCE_DIR}
    src
)
