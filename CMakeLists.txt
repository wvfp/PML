cmake_minimum_required(VERSION 3.28)
project(pml-cpp
    VERSION 0.1.0
    DESCRIPTION "PML C++23 — Lisp-style DSL for code-to-image generation"
    LANGUAGES C CXX
)

# ═══ C++23 standard ════════════════════════════════════════════════════════
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Suppress warnings inside third-party targets
if(MSVC)
    add_compile_options(/W0)  # honour each third-party's own warning policy
endif()

# ═══ Build options ════════════════════════════════════════════════════════
option(PML_BUILD_TESTS     "Build unit tests"            ON)
option(PML_BUILD_CLI       "Build pml CLI"               ON)
option(PML_BUILD_MCP       "Build pml-mcp JSON-RPC server" ON)
option(PML_BUILD_GIF       "Build gif export backend"    ON)
option(PML_BUILD_SKIA      "Build skia render backend"   ON)
option(PML_BUILD_FREETYPE  "Vendor freetype (unused for now)" OFF)

# Skia: pre-built at $PML_SKIA_OUT, headers at $PML_SKIA_DIR/include
set(PML_SKIA_DIR  "" CACHE PATH "Skia source root (headers live at <PML_SKIA_DIR>/include)")
set(PML_SKIA_OUT  "" CACHE PATH "Skia pre-built output dir (must contain skia.lib + skcms.lib)")

# ═══ Third-party: prefer local checkout, fall back to git clone ═══════════
include(FetchContent)

# Helper: use local third_party/<name> if present, otherwise git-clone via
# FetchContent_Declare (HTTPS so it works without SSH keys / vcpkg).
function(pml_third_party NAME)
    set(options)
    set(oneValueArgs REPO TAG URL HASH)
    set(multiValueArgs)
    cmake_parse_arguments(PML_TP "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    if(PML_TP_URL)
        # Tarball download — used for libraries whose canonical git host is
        # slow; the release tarball is hosted on a CDN-cached mirror instead.
        set(_local "${CMAKE_SOURCE_DIR}/third_party/${NAME}")
        message(STATUS "pml: download ${PML_TP_URL} into third_party/${NAME}")
        if(PML_TP_HASH)
            set(_url_args URL_HASH SHA256=${PML_TP_HASH})
        else()
            set(_url_args "")
        endif()
        FetchContent_Declare(
            ${NAME}
            URL          "${PML_TP_URL}"
            ${_url_args}
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
            SOURCE_DIR   "${_local}"
        )
        FetchContent_MakeAvailable(${NAME})
    elseif(PML_TP_REPO AND PML_TP_TAG)
        set(_local "${CMAKE_SOURCE_DIR}/third_party/${NAME}")
        if(EXISTS "${_local}/CMakeLists.txt")
            message(STATUS "pml: using local third_party/${NAME} at ${_local}")
            add_subdirectory("${_local}" "${CMAKE_BINARY_DIR}/_third_party/${NAME}" EXCLUDE_FROM_ALL)
        else()
            message(STATUS "pml: git clone ${PML_TP_REPO}@${PML_TP_TAG} into third_party/${NAME}")
            FetchContent_Declare(
                ${NAME}
                GIT_REPOSITORY "${PML_TP_REPO}"
                GIT_TAG        "${PML_TP_TAG}"
                GIT_SHALLOW    TRUE
                SOURCE_DIR     "${_local}"
            )
            FetchContent_MakeAvailable(${NAME})
        endif()
    else()
        message(FATAL_ERROR "pml_third_party(${NAME}): need either URL or REPO+TAG")
    endif()
endfunction()

# nlohmann_json (header-only, in third_party/json/)
pml_third_party(json
    REPO https://github.com/nlohmann/json.git
    TAG  v3.12.0
)
set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
set(JSON_Install    OFF CACHE BOOL "" FORCE)
if(NOT TARGET nlohmann_json::nlohmann_json AND TARGET nlohmann_json)
    add_library(nlohmann_json::nlohmann_json ALIAS nlohmann_json)
endif()

# GoogleTest (in third_party/googletest/)
pml_third_party(googletest
    REPO https://github.com/google/googletest.git
    TAG  v1.17.0
)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(BUILD_GMOCK   OFF CACHE BOOL "" FORCE)
# googletest's CMakeLists exposes `gtest` / `gtest_main` (not GTest::gtest);
# add aliases so dependents can use the modern `GTest::gtest` form.
if(NOT TARGET GTest::gtest AND TARGET gtest)
    add_library(GTest::gtest      ALIAS gtest)
    add_library(GTest::gtest_main ALIAS gtest_main)
endif()

# libpng depends on zlib — vendor it too
pml_third_party(zlib
    REPO https://github.com/madler/zlib.git
    TAG  v1.3.1
)
# zlib's CMakeLists creates `zlib` + `zlibstatic` but no ZLIB::ZLIB alias and
# no installed ZLIBConfig.cmake. libpng's CMakeLists then `find_package(ZLIB
# REQUIRED)`s — make it succeed by aliasing and pre-setting the cache vars
# BEFORE the libpng subdirectory is added below.
if(NOT TARGET ZLIB::ZLIB AND TARGET zlibstatic)
    add_library(ZLIB::ZLIB ALIAS zlibstatic)
    set(_zlib_bin_dir "${CMAKE_BINARY_DIR}/_third_party/zlib")
    if(WIN32)
        set(_zlib_lib "${_zlib_bin_dir}/zlibstatic.lib")
    else()
        set(_zlib_lib "${_zlib_bin_dir}/libzlibstatic.a")
    endif()
    set(ZLIB_INCLUDE_DIR "${_zlib_bin_dir}" CACHE FILEPATH "zlib include dir" FORCE)
    set(ZLIB_LIBRARY     "${_zlib_lib}"    CACHE FILEPATH "zlib library"     FORCE)
    set(ZLIB_LIBRARY_DEBUG "${_zlib_lib}"  CACHE FILEPATH "zlib library dbg" FORCE)
    set(ZLIB_FOUND TRUE CACHE BOOL "" FORCE)
endif()

# libpng (in third_party/libpng/)
# Set libpng's options BEFORE adding its subdirectory — its install(EXPORT ...)
# at the end of the file needs these in the cache or it tries to export
# zlibstatic (which is built in-tree, not in any export set).
set(PNG_SHARED   OFF CACHE BOOL "" FORCE)
set(PNG_FRAMEWORK OFF CACHE BOOL "" FORCE)
set(PNG_STATIC   ON  CACHE BOOL "" FORCE)
set(PNG_TESTS    OFF CACHE BOOL "" FORCE)
set(SKIP_INSTALL_EXPORT      ON CACHE BOOL "" FORCE)
set(SKIP_INSTALL_CONFIG_FILE ON CACHE BOOL "" FORCE)
set(SKIP_INSTALL_ALL         ON CACHE BOOL "" FORCE)
pml_third_party(libpng
    REPO https://github.com/pnggroup/libpng.git
    TAG  v1.6.58
)
# libpng's CMakeLists exposes `png_static` (not PNG::PNG); add an alias.
if(NOT TARGET PNG::PNG AND TARGET png_static)
    add_library(PNG::PNG ALIAS png_static)
endif()

# giflib — only required when PML_BUILD_GIF=ON.
# giflib 5.2.2 ships a Makefile but no CMakeLists.txt, so we use
# FetchContent_Populate to grab the source and define the target ourselves.
if(PML_BUILD_GIF)
    set(_giflib_src "${CMAKE_SOURCE_DIR}/third_party/giflib")
    if(NOT EXISTS "${_giflib_src}/gif_lib.h")
        message(STATUS "pml: git clone giflib 5.2.2 into third_party/giflib")
        FetchContent_Declare(giflib_populate
            GIT_REPOSITORY https://git.code.sf.net/p/giflib/code
            GIT_TAG        5.2.2
            GIT_SHALLOW    TRUE
            SOURCE_DIR     "${_giflib_src}"
        )
        FetchContent_MakeAvailable(giflib_populate)
    endif()
    if(EXISTS "${_giflib_src}/gif_lib.h" AND NOT TARGET GIF::GIF)
        add_library(GIF STATIC
            "${_giflib_src}/dgif_lib.c"
            "${_giflib_src}/egif_lib.c"
            "${_giflib_src}/gifalloc.c"
            "${_giflib_src}/gif_err.c"
            "${_giflib_src}/gif_font.c"
            "${_giflib_src}/gif_hash.c"
            "${_giflib_src}/quantize.c"
            "${_giflib_src}/openbsd-reallocarray.c"
        )
        target_include_directories(GIF PUBLIC "${_giflib_src}")
        # We always use giflib's bundled openbsd_reallocarray() — undefined
        # HAVE_REALLOCARRAY so the fallback in gif_lib_private.h kicks in.
        target_compile_options(GIF PRIVATE -UHAVE_REALLOCARRAY)
        # gif_font.c uses POSIX strtok_r — MSVC/Windows has strtok_s instead.
        # strtok_r signatures are identical to strtok_s, so a textual mapping
        # is safe. Also silence MSVC's "deprecated CRT function" noise which
        # giflib triggers with strcpy/strcat.
        target_compile_definitions(GIF PRIVATE
            "strtok_r=strtok_s"
            "_CRT_SECURE_NO_WARNINGS=1"
        )
        target_sources(GIF PRIVATE
            "${_giflib_src}/openbsd-reallocarray.c"
        )
        add_library(GIF::GIF ALIAS GIF)
    endif()
endif()

# freetype — vendored on demand
if(PML_BUILD_FREETYPE)
    pml_third_party(freetype
        REPO https://github.com/freetype/freetype.git
        TAG  VER-2-13-3
    )
endif()

# ═══ Skia (pre-built, no compilation) ══════════════════════════════════════
if(PML_BUILD_SKIA)
    if(NOT PML_SKIA_DIR OR NOT PML_SKIA_OUT)
        message(FATAL_ERROR
            "PML_BUILD_SKIA=ON requires PML_SKIA_DIR (skia source root) and "
            "PML_SKIA_OUT (compiled output containing skia.lib) to be set.")
    endif()
    if(NOT EXISTS "${PML_SKIA_DIR}/include")
        message(FATAL_ERROR "PML_SKIA_DIR=${PML_SKIA_DIR} has no include/ subdir")
    endif()
    if(NOT EXISTS "${PML_SKIA_OUT}/skia.lib")
        message(FATAL_ERROR "PML_SKIA_OUT=${PML_SKIA_OUT} has no skia.lib")
    endif()

    add_library(skia_headers INTERFACE)
    target_include_directories(skia_headers INTERFACE
        ${PML_SKIA_DIR}              # skia headers use include/core/… paths
        ${PML_SKIA_OUT}/gen          # skia.h
    )
    target_compile_definitions(skia_headers INTERFACE
        SK_DESKTOP_SKIA=1
        SK_GAMMA_APPLY_TO_A8=0
        SK_SUPPORT_LEGACY_PNG_WRITEBUFFER=0
    )

    # Primary static libs (per user spec: skia + skcms + core auxiliaries)
    set(_skia_libs
        skia skcms
        sksg skshaper svg skottie skresources
        jsonreader expat bentleyottmann
    )
    foreach(_lib IN LISTS _skia_libs)
        if(EXISTS "${PML_SKIA_OUT}/${_lib}.lib")
            add_library(skia::${_lib} STATIC IMPORTED GLOBAL)
            set_target_properties(skia::${_lib} PROPERTIES
                IMPORTED_LOCATION             "${PML_SKIA_OUT}/${_lib}.lib"
                INTERFACE_INCLUDE_DIRECTORIES "${PML_SKIA_DIR}/include"
            )
        endif()
    endforeach()

    add_library(pml_skia_deps INTERFACE)
    target_link_libraries(pml_skia_deps INTERFACE
        skia_headers
        skia::skia skia::skcms
        skia::sksg skia::skshaper skia::svg skia::skottie skia::skresources
        skia::jsonreader skia::expat skia::bentleyottmann
    )
    if(WIN32)
        target_link_libraries(pml_skia_deps INTERFACE
            gdi32.lib user32.lib ole32.lib oleaut32.lib uuid.lib
            shell32.lib advapi32.lib dwrite.lib
        )
    endif()
    add_library(pml::skia_deps ALIAS pml_skia_deps)
    add_compile_definitions(PML_USE_SKIA=1)
    set(PML_USE_SKIA 1)
endif()

# ═══ Subdirectories (bottom-up dependency order) ══════════════════════════
add_subdirectory(src/pml/core)
add_subdirectory(src/pml/frontend)
add_subdirectory(src/pml/backend)        # also pulls in gif via add_subdirectory
add_subdirectory(src/pml/filter)
add_subdirectory(src/pml/evaluator)
add_subdirectory(src/pml/graphics)
add_subdirectory(src/pml/graphics3d)
add_subdirectory(src/pml/layer)
if(PML_BUILD_SKIA)
    add_subdirectory(src/pml/backend/skia)
endif()
add_subdirectory(src/pml/sprites)
add_subdirectory(src/pml/animation)
add_subdirectory(src/pml/skeleton)
add_subdirectory(src/pml/module)
add_subdirectory(src/pml/asset)
add_subdirectory(src/pml/api)
if(PML_BUILD_CLI)
    add_subdirectory(src/pml/cli)
endif()
if(PML_BUILD_MCP)
    add_subdirectory(src/pml/mcp)
endif()

# ═══ Tests & Tools ════════════════════════════════════════════════════════
if(PML_BUILD_TESTS)
    add_subdirectory(tests)
endif()
