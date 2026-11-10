cmake_minimum_required(VERSION 3.16)
project(YanbianVillage LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
)
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.1
)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw glm)

add_library(glad STATIC third_party/glad/src/glad.c)
target_include_directories(glad PUBLIC third_party/glad/include)

file(GLOB_RECURSE PROJECT_SOURCES
    "src/*.cpp"
)

add_executable(YanbianVillage ${PROJECT_SOURCES})
target_include_directories(YanbianVillage PRIVATE
    src
    third_party/glad/include
    third_party/stb
)
target_link_libraries(YanbianVillage PRIVATE glfw glm::glm glad)

if(WIN32)
    target_link_libraries(YanbianVillage PRIVATE opengl32 gdi32)
endif()

target_compile_definitions(YanbianVillage PRIVATE
    PROJECT_ROOT="${CMAKE_SOURCE_DIR}"
    GLM_ENABLE_EXPERIMENTAL
)
