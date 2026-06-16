cmake_minimum_required(VERSION 3.28)
project(pml-cpp
    VERSION 0.1.0
    DESCRIPTION "PML C++23 — Lisp-style DSL for code-to-image generation"
    LANGUAGES CXX
)

# ── C++23 standard ──────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# ── Dependencies (system packages, SSH for GitHub) ──────────────────────────
# GTest (via apt: libgtest-dev)
find_package(GTest CONFIG REQUIRED)

# nlohmann-json (via apt: nlohmann-json3-dev)
find_package(nlohmann_json CONFIG REQUIRED)

# System packages via pkg-config (giflib via apt: libgif-dev, cairo: libcairo2-dev)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GIFLIB REQUIRED libgif)
pkg_check_modules(CAIRO REQUIRED cairo)
pkg_check_modules(PNG REQUIRED libpng)

# Optional: Skia (downloaded via FetchContent + SSH if available)
option(PML_BUILD_SKIA "Build with Skia GPU backend" OFF)
if(PML_BUILD_SKIA)
    add_compile_definitions(PML_USE_SKIA=1)
    include(FetchContent)
    FetchContent_Declare(
        skia
        GIT_REPOSITORY git@github.com:google/skia.git
        GIT_TAG chrome/m148
        GIT_SHALLOW TRUE
    )
    FetchContent_GetProperties(skia)
    if(NOT skia_POPULATED)
        FetchContent_Populate(skia)
        add_subdirectory(${skia_SOURCE_DIR} ${skia_BINARY_DIR} EXCLUDE_FROM_ALL)
    endif()
endif()

# ── Subdirectories (bottom-up dependency order) ─────────────────────────────
add_subdirectory(src/pml/core)
add_subdirectory(src/pml/frontend)
add_subdirectory(src/pml/backend)
add_subdirectory(src/pml/evaluator)
add_subdirectory(src/pml/graphics)
if(PML_BUILD_SKIA)
    add_subdirectory(src/pml/backend/skia)
endif()
add_subdirectory(src/pml/sprites)
add_subdirectory(src/pml/animation)
add_subdirectory(src/pml/skeleton)
add_subdirectory(src/pml/module)
add_subdirectory(src/pml/api)
add_subdirectory(src/pml/cli)
add_subdirectory(src/pml/mcp)

# ── Tests & Tools ───────────────────────────────────────────────────────────
add_subdirectory(tests)
add_subdirectory(tools)
