cmake_minimum_required(VERSION 3.24)

project(Tavoos
    VERSION 0.1.0
    DESCRIPTION "Tavoos UI framework"
    LANGUAGES C CXX
)

option(TAVOOS_BUILD_EXAMPLES "Build the Tavoos example app" ON)
option(TAVOOS_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(TavoosCompilerGuard)
include(GNUInstallDirs)
include(TavoosCompilerWarnings)
include(TavoosResources)

find_package(glfw3 CONFIG REQUIRED)
find_package(OpenGL REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(lunasvg REQUIRED)
find_package(Freetype REQUIRED)
find_package(glm CONFIG REQUIRED)

if (UNIX AND NOT APPLE)
    find_package(Fontconfig REQUIRED)
endif()

add_subdirectory(src)

if (TAVOOS_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

include(CMakePackageConfigHelpers)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/TavoosConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/TavoosConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/TavoosConfig.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/Tavoos"
)

install(
    FILES
    "${CMAKE_CURRENT_BINARY_DIR}/TavoosConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/TavoosConfigVersion.cmake"
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/TavoosResources.cmake"
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/GenerateResource.cmake"
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/TavoosCompilerGuard.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/Tavoos"
)

install(
    EXPORT TavoosTargets
    FILE TavoosTargets.cmake
    NAMESPACE Tavoos::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/Tavoos"
)

include(TavoosPackaging)
