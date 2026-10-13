cmake_minimum_required(VERSION 3.20)

project(ProcessamentoGrafico)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# GLFW
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
)

FetchContent_MakeAvailable(glfw)

# GLAD
add_library(glad STATIC
    glad/src/gl.c
)

target_include_directories(glad PUBLIC
    glad/include
)

FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.1
)

FetchContent_MakeAvailable(glm)

# stb_image (PNG and GIF decoding)
FetchContent_Declare(
    stb_image
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG 2c980bb59875b0d32144a71867fbdebb2f77cd20
)

FetchContent_GetProperties(stb_image)
if(NOT stb_image_POPULATED)
    FetchContent_Populate(stb_image)
endif()

# Programa
add_executable(ProcessamentoGrafico
    src/main.cpp
)

target_link_libraries(ProcessamentoGrafico
    PRIVATE
    glfw
    glad
    glm::glm
    opengl32
)

target_compile_definitions(ProcessamentoGrafico PRIVATE
    GAME_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
)

target_include_directories(ProcessamentoGrafico PRIVATE
    ${stb_image_SOURCE_DIR}
)