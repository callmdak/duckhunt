cmake_minimum_required(VERSION 3.20)

project(DuckHunt VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Use the static MSVC runtime so the Windows package does not need the
# Visual C++ redistributable installed separately.
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

include(FetchContent)

find_package(OpenGL REQUIRED)

set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
)

FetchContent_MakeAvailable(glfw)

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

FetchContent_MakeAvailable(stb_image)

add_executable(duckhunt
    src/main.cpp
    src/game.cpp
    src/renderer.cpp
    src/resources.cpp
)

target_link_libraries(duckhunt
    PRIVATE
    glfw
    glad
    glm::glm
    OpenGL::GL
)

target_include_directories(duckhunt PRIVATE
    ${stb_image_SOURCE_DIR}
)

set_target_properties(duckhunt PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>"
)

foreach(resource_dir IN ITEMS config assets shaders)
    add_custom_command(TARGET duckhunt POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/${resource_dir}"
            "$<TARGET_FILE_DIR:duckhunt>/${resource_dir}"
        VERBATIM
    )
endforeach()

install(TARGETS duckhunt RUNTIME DESTINATION .)
install(DIRECTORY config assets shaders DESTINATION .)

set(CPACK_GENERATOR ZIP)
set(CPACK_PACKAGE_NAME "DuckHunt")
set(CPACK_PACKAGE_VENDOR "DuckHunt")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_FILE_NAME
    "DuckHunt-${PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
set(CPACK_PACKAGE_DIRECTORY "${CMAKE_BINARY_DIR}/package")
include(CPack)
