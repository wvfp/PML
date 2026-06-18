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
option(PML_BUILD_CAIRO     "Build cairo render backend"  OFF)
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
        # slow (e.g. gitlab.freedesktop.org for cairo/pixman); cairographics.org
        # hosts a CDN-cached release tarball instead.
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

# Helper: build a meson-only project (cairo, pixman, etc.) and import the
# resulting static library as a CMake target.
#
# Args:
#   NAME         — internal target name (used for the build subdir and custom
#                  command name)
#   SOURCE       — path to the meson source tree (must contain meson.build)
#   OPTIONS      — list of meson options (e.g. -Dglib=disabled)
#   TARGET_NAME  — CMake target name to create (will be STATIC IMPORTED)
#   STATIC_LIB   — filename of the resulting static lib (e.g. libcairo.a)
function(pml_meson_build NAME)
    set(options)
    set(oneValueArgs SOURCE TARGET_NAME STATIC_LIB)
    set(multiValueArgs OPTIONS EXTRA_ENV)
    cmake_parse_arguments(PML_MB "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT PML_MB_SOURCE OR NOT PML_MB_TARGET_NAME OR NOT PML_MB_STATIC_LIB)
        message(FATAL_ERROR
            "pml_meson_build(${NAME}): SOURCE, TARGET_NAME, STATIC_LIB are required")
    endif()
    if(NOT MESON_EXECUTABLE)
        find_program(MESON_EXECUTABLE meson REQUIRED)
    endif()
    if(NOT NINJA_EXECUTABLE)
        find_program(NINJA_EXECUTABLE ninja REQUIRED)
    endif()

    set(_build_dir "${CMAKE_BINARY_DIR}/_third_party/${NAME}/build")
    set(_lib_path  "${_build_dir}/${PML_MB_STATIC_LIB}")
    set(_pdb_path  "${_build_dir}/${PML_MB_TARGET_NAME}.pdb")

    # Force meson to use the SAME compiler that CMake picked (e.g. Clang 20 on
    # Windows), so the resulting .a/.lib is ABI-compatible with the rest of
    # pml-cpp. Without this meson falls back to whatever `cc` it finds first
    # (often MinGW gcc on Windows) and we get linker errors at the end.
    #
    # Compiler paths frequently contain spaces (e.g. "C:\Program Files\LLVM\bin\
    # clang.exe") which Meson doesn't accept reliably in CC=/CXX= env vars on
    # Windows, so we write a tiny .bat wrapper to a stable path inside the
    # build dir and point meson at that.
    set(_cc_wrapper  "${CMAKE_BINARY_DIR}/_third_party/${NAME}/cc_wrapper.bat")
    set(_cxx_wrapper "${CMAKE_BINARY_DIR}/_third_party/${NAME}/cxx_wrapper.bat")
    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/_third_party/${NAME}")
    file(WRITE "${_cc_wrapper}"  "@echo off\r\n\"${CMAKE_C_COMPILER}\" %*\r\n")
    file(WRITE "${_cxx_wrapper}" "@echo off\r\n\"${CMAKE_CXX_COMPILER}\" %*\r\n")
    set(_meson_env
        CC=${_cc_wrapper}
        CXX=${_cxx_wrapper}
    )
    # Append any caller-supplied env vars (e.g. PKG_CONFIG_PATH) so meson
    # can find system dependencies our cmake target already built.
    if(PML_MB_EXTRA_ENV)
        list(APPEND _meson_env ${PML_MB_EXTRA_ENV})
    endif()

    # Build the static lib via meson+ninja. We capture meson output to a log
    # so it doesn't drown the configure output.
    add_custom_command(
        OUTPUT  "${_lib_path}"
        COMMAND ${CMAKE_COMMAND} -E env ${_meson_env}
                ${CMAKE_COMMAND} -E make_directory "${_build_dir}"
        # Delete stale build dir to avoid meson complaining about a re-setup
        COMMAND ${CMAKE_COMMAND} -E rm -rf "${_build_dir}"
        COMMAND ${CMAKE_COMMAND} -E env ${_meson_env}
                ${MESON_EXECUTABLE} setup "${_build_dir}" "${PML_MB_SOURCE}"
                --buildtype=release
                --default-library=static
                ${PML_MB_OPTIONS}
        COMMAND ${NINJA_EXECUTABLE} -C "${_build_dir}"
        COMMAND ${CMAKE_COMMAND} -E echo "pml: built ${PML_MB_STATIC_LIB} (${PML_MB_SOURCE})"
        DEPENDS "${PML_MB_SOURCE}/meson.build"
                "${_cc_wrapper}" "${_cxx_wrapper}"
        COMMENT "pml: meson+ninja build of ${PML_MB_TARGET_NAME}"
        VERBATIM
    )
    add_custom_target(${NAME} DEPENDS "${_lib_path}")

    add_library(${PML_MB_TARGET_NAME} STATIC IMPORTED GLOBAL)
    add_dependencies(${PML_MB_TARGET_NAME} ${NAME})
    set_target_properties(${PML_MB_TARGET_NAME} PROPERTIES
        IMPORTED_LOCATION             "${_lib_path}"
        INTERFACE_INCLUDE_DIRECTORIES "${PML_MB_SOURCE}/src"
    )
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

# cairo + pixman — both switched to meson upstream, so we don't try to
# add_subdirectory them. Instead we build via `meson setup` + `ninja` and
# import the resulting static libraries as CMake targets.
#
# We build pixman (no tests/gtk) and cairo (no glib/fontconfig/freetype/xlib/xcb
# /quartz/dwrite — only image + PNG + toy-text surface, which is what
# pml's CairoBackend uses).
if(PML_BUILD_CAIRO)
    find_program(MESON_EXECUTABLE meson REQUIRED)
    find_program(NINJA_EXECUTABLE ninja REQUIRED)

    pml_third_party(pixman
        URL https://cairographics.org/releases/pixman-0.46.4.tar.xz
    )
    pml_meson_build(pixman_build
        SOURCE  "${CMAKE_SOURCE_DIR}/third_party/pixman"
        OPTIONS
            -Dtests=disabled
            -Ddemos=disabled
            -Dgtk=disabled
            -Dlibpng=disabled
        TARGET_NAME pixman
        STATIC_LIB libpixman-1${CMAKE_STATIC_LIBRARY_SUFFIX}
    )
    add_library(pixman::pixman ALIAS pixman)

    # Generate a pkg-config file for our built pixman so cairo's meson can
    # find it via `dependency('pixman-1')` instead of falling back to the
    # bundled (and Clang-broken) pixman-0.43.4 subproject.
    set(_pixman_pc_dir  "${CMAKE_BINARY_DIR}/_third_party/pixman_pc")
    file(MAKE_DIRECTORY "${_pixman_pc_dir}")
    set(_pixman_lib_path "${CMAKE_BINARY_DIR}/_third_party/pixman_build/build/libpixman-1${CMAKE_STATIC_LIBRARY_SUFFIX}")
    set(_pixman_inc_path "${CMAKE_SOURCE_DIR}/third_party/pixman/pixman")
    file(WRITE "${_pixman_pc_dir}/pixman.pc"
        [=[
prefix=__PREFIX__
exec_prefix=__PREFIX__
libdir=__LIBDIR__
includedir=__INCDIR__

Name: pixman-1
Description: pixman library (pml vendored build)
Version: 0.46.4
Libs: -L__LIBDIR__ -lpixman-1
Cflags: -I__INCDIR__
]=]
    )
    # CMake's configure_file()-style substitution is unavailable in file(WRITE),
    # so we use string(REPLACE) to expand our sentinels in place.
    file(READ   "${_pixman_pc_dir}/pixman.pc"   _pc_body)
    string(REPLACE "__PREFIX__" "${_pixman_pc_dir}" _pc_body "${_pc_body}")
    string(REPLACE "__LIBDIR__" "${CMAKE_BINARY_DIR}/_third_party/pixman_build/build" _pc_body "${_pc_body}")
    string(REPLACE "__INCDIR__" "${_pixman_inc_path}" _pc_body "${_pc_body}")
    file(WRITE  "${_pixman_pc_dir}/pixman.pc"   "${_pc_body}")
    message(STATUS "pml: generated ${_pixman_pc_dir}/pixman.pc")
    set(PML_PIXMAN_PC_PATH "${_pixman_pc_dir}" CACHE INTERNAL "pkgconfig dir with our pixman.pc" FORCE)

    pml_third_party(cairo
        URL https://cairographics.org/releases/cairo-1.18.4.tar.xz
    )
    # cairo's meson exposes the static lib as `libcairo.a`. We disable every
    # optional back-end: pml's CairoBackend only uses image surface, PNG I/O,
    # and the toy-text API (which is built-in and needs no fontconfig/freetype).
    pml_meson_build(cairo_build
        SOURCE  "${CMAKE_SOURCE_DIR}/third_party/cairo"
        OPTIONS
            -Dglib=disabled
            -Dfreetype=disabled
            -Dfontconfig=disabled
            # Cairo's PNG support needs a meson-found libpng, which we can't
            # satisfy from CMake-built libpng without pkg-config plumbing.
            # pml's CairoBackend does not need cairo's PNG I/O — the image
            # surface pixels are written to PNG by pml's own libpng pipeline.
            -Dpng=disabled
            -Dzlib=disabled
            -Dlzo=disabled
            -Dxlib=disabled
            -Dxcb=disabled
            -Dxlib-xcb=disabled
            -Dquartz=disabled
            -Ddwrite=disabled
            -Dtee=disabled
            -Dsymbol-lookup=disabled
            -Dtests=disabled
            -Dspectre=disabled
            # Don't download any subproject (e.g. broken pixman-0.43.4 wrap);
            # we provide our own pre-built pixman-0.46.4 via PKG_CONFIG_PATH.
            --wrap-mode=nodownload
        TARGET_NAME cairo
        STATIC_LIB libcairo${CMAKE_STATIC_LIBRARY_SUFFIX}
        # Point meson at the pixman.pc we generated above so cairo picks
        # up our pre-built libpixman-1 instead of its bundled wrap.
        EXTRA_ENV
            PKG_CONFIG_PATH=${_pixman_pc_dir}
    )
    add_library(Cairo::Cairo ALIAS cairo)
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

if(PML_BUILD_CAIRO)
    add_compile_definitions(PML_USE_CAIRO=1)
    set(PML_USE_CAIRO 1)
endif()

# ═══ Subdirectories (bottom-up dependency order) ══════════════════════════
add_subdirectory(src/pml/core)
add_subdirectory(src/pml/frontend)
add_subdirectory(src/pml/backend)        # also pulls in cairo/gif via add_subdirectory
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
